/**
 * Created by intwi on 2026/05/28.
 * Copyright (c) 2026 All rights reserved.
 */
#include "audio_engine.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <numeric>
#include <random>
#include <system_error>
#include <vector>

#include "data/cue_data.h"
#include "data/project_data.h"
#include "data/streaming_reader.h"
#include "data/streaming_waveform_provider.h"
#include "data/waveform_provider.h"
#include "data/cue_collection.h"
#include "data/cue_collection_store.h"
#include "runtime/command.h"
#include "runtime/cue_player.h"
#include "runtime/cue_player_register.h"
#include "runtime/event.h"
#include "runtime/mixer.h"
#include "runtime/voice_handle.h"
#include "runtime/voice_pool.h"
#include "util/ring_buffer.h"

#include "miniaudio.h"

namespace wit {

namespace {

constexpr size_t SPSC_QUEUE_CAPACITY = 512; ///< Runtime capacities for SPSC queue

WitResult ReadFileBytes(const char* path, std::vector<uint8_t>& out) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) return WIT_RESULT_FILE_NOT_FOUND;
    const auto file_size = std::filesystem::file_size(path, ec);
    if (ec) return WIT_RESULT_INVALID_FORMAT;

    std::ifstream file(path, std::ios::binary);
    if (!file) return WIT_RESULT_FILE_NOT_FOUND;

    out.resize(static_cast<size_t>(file_size));
    if (file_size > 0) {
        file.read(reinterpret_cast<char*>(out.data()),
                  static_cast<std::streamsize>(file_size));
        if (!file) return WIT_RESULT_INVALID_FORMAT;
    }

    return WIT_RESULT_SUCCESS;
}

/*
 * Shared PRNG for cue randomization (volume / pitch scaling per Play).
 * Fixed seed so behavior is deterministic; can be re-seeded from a time source later if desired. Main thread only.
 */
static std::mt19937 g_random_engine(0xDEADBEEF);

float SampleRandomInRange(float min_v, float max_v) {
    if (min_v == max_v) return min_v;
    if (min_v > max_v)  return min_v;
    std::uniform_real_distribution<float> dist(min_v, max_v);
    return dist(g_random_engine);
}

}  // namespace

/*
 * Forward declaration of the C-linkage callback.
 */
extern "C" {
static void WitDataCallback(ma_device* device, void* output,
                            const void* input, ma_uint32 frame_count);
}

/* =====================================================================
 * pImpl
 * ===================================================================== */
struct AudioEngine::Impl {
    ma_device            device_{};
    bool                 device_initialized_ = false;
    bool                 device_started_     = false;

    std::unique_ptr<ProjectData>       project_data_;			///<
    std::unique_ptr<CueCollectionStore>         cue_collection_store_;			///<
    std::unique_ptr<CuePlayerRegister> cue_player_register_;	///<

    VoicePool									voice_pool_;	///<
    Mixer										mixer_;			///<
    uint32_t									fade_samples_ = 1;	///< Fade length in samples
    std::unique_ptr<StreamingReader>			streaming_reader_;	///< Owns the streaming read thread
    RingBuffer<Command, SPSC_QUEUE_CAPACITY>	command_queue_;	///<
    RingBuffer<Event,   SPSC_QUEUE_CAPACITY>	event_queue_;	///<

    /**
     * @brief Audio-thread entry
     */
    void OnAudioCallback(float* output, ma_uint32 frame_count) noexcept {
        // Apply any commands from the main thread posted since the last callback.
        Command cmd;
        while (command_queue_.Dequeue(cmd)) ApplyCommand(cmd);

        // Keep streaming buffers filled and promote voices whose data arrived.
        ServiceStreamingVoices();

        // Mix
        if (project_data_) {
            mixer_.Process(voice_pool_, project_data_->Categories(), frame_count, output);
        } else {
            std::memset(output, 0,static_cast<size_t>(frame_count) * 2 * sizeof(float));
        }

        // Settle voices whose pause / stop fade finished during this Process().
        ConfirmFadeTransitions();

        // Detect voices that reached the end of their waveforms naturally and mark them FINISHED.
        DetectAndReportFinishedVoices();
    }

    /**
     * @brief Start ramping a Voice up toward normal playback speed.
     * @param from_standstill When true the ramp restarts from a full stop,
     *        otherwise it continues from the current speed.
     */
    static void BeginTapeRise(Voice& v, bool from_standstill) noexcept {
        if (from_standstill) v.tape.position = 0.0f;

        // The tape taper already brings the level up from silence, so the amplitude fade must stay out of the way.
        v.fade_gain = 1.0f;
        v.fade_step = 0.0f;

        // A non-positive duration means the ramp is skipped entirely.
        if (v.tape.rise_step <= 0.0f || v.tape.position >= 1.0f) {
            v.tape.position = 1.0f;
            v.tape.step     = 0.0f;
            return;
        }

        // Scale to what actually remains and the configured duration holds wherever the ramp starts.
        v.tape.step = v.tape.rise_step * (1.0f - v.tape.position);
    }

    /**
     * @brief Start ramping a Voice down toward a full stop.
     * @note Called from the audio thread only. The amplitude fade is left
     *       alone; the speed ramp alone carries the transition.
     */
    static void BeginTapeFall(Voice& v) noexcept {
        // The tape taper handles the fade to silence at the bottom of the ramp.
        v.fade_gain = 1.0f;
        v.fade_step = 0.0f;

        if (v.tape.fall_step <= 0.0f || v.tape.position <= 0.0f) {
            v.tape.position = 0.0f;
            v.tape.step     = 0.0f;
            return;
        }

        // Scale the step to the remaining distance.
        v.tape.step = -v.tape.fall_step * v.tape.position;
    }

    /**
     * @brief Apply a single command to the addressed slot, ignoring stale handles.
     * @note Called from the audio thread only.
     */
    void ApplyCommand(const Command& cmd) noexcept {
        const auto hn = voice_handle::Encode(cmd.slot_index, cmd.generation);

        auto* v = voice_pool_.Find(hn);
        if (v == nullptr) return;

        switch (cmd.type) {
            case CommandType::START_PLAYING: {
	            if (v->state == VoiceState::CLAIMED) {
	            	bool all_ready = true;
	            	for (uint8_t wi = 0; wi < v->active_slot_count; ++wi) {
	            		Voice::Slot& slot = v->slots[wi];
	            		if (slot.streaming == nullptr) continue;

	            		const uint64_t total   = slot.streaming->TotalSampleCount();
	            		const uint64_t wrap_at = (v->loop_end > 0 && v->loop_end < total) ? v->loop_end : 0;
	            		slot.streaming->SetLoopRegion(v->loop_end > 0 ? v->loop_start : 0, wrap_at);

	            		slot.streaming->UpdateStreaming(static_cast<uint64_t>(slot.cursor));
	            		if (!slot.streaming->IsReadyAt(static_cast<uint64_t>(slot.cursor))) all_ready = false;
	            	}
	            	v->state = all_ready ? VoiceState::PLAYING : VoiceState::PREPARING;

	            	if (v->tape.active) BeginTapeRise(*v, true);
	            }
            	break;
            }
            case CommandType::PAUSE_VOICE: {
	            if (v->state == VoiceState::PLAYING) {
	            	v->state = VoiceState::PAUSING;
	            	if (v->tape.active) {
	            		BeginTapeFall(*v);
	            	} else {
	            		v->fade_step = -1.0f / static_cast<float>(fade_samples_);
	            	}
	            }
            	break;
            }
            case CommandType::RESUME_VOICE: {
	            if (v->state == VoiceState::PAUSED) {
	            	// Skip the click-suppression rewind.
	            	const double rewind = v->tape.active ? 0.0 : static_cast<double>(fade_samples_);
	            	for (uint8_t wi = 0; wi < v->active_slot_count; ++wi) {
	            		double c = v->slots[wi].cursor - rewind;
	            		if (c < 0.0) c = 0.0;
	            		v->slots[wi].cursor = c;
	            	}
	            	v->state = VoiceState::PLAYING;
	            	if (v->tape.active) {
	            		BeginTapeRise(*v, true);
	            	} else {
	            		v->fade_step = 1.0f / static_cast<float>(fade_samples_);
	            	}
	            } else if (v->state == VoiceState::PAUSING) {
	            	v->state = VoiceState::PLAYING;
	            	if (v->tape.active) {
	            		// Reverse the in-flight fall without restarting from a standstill.
	            		BeginTapeRise(*v, false);
	            	} else {
	            		v->fade_step = 1.0f / static_cast<float>(fade_samples_);
	            	}
	            }
            	break;
            }
            case CommandType::STOP_VOICE: {
	            if (v->state == VoiceState::PLAYING || v->state == VoiceState::PAUSING) {
	            	v->state = VoiceState::STOPPING;
	            	if (v->tape.active) {
	            		BeginTapeFall(*v);
	            	} else {
	            		v->fade_step = -1.0f / static_cast<float>(fade_samples_);
	            	}
	            } else if (v->state == VoiceState::PAUSED) {
	            	v->state = VoiceState::FINISHED;
	            	PostVoiceFinished(cmd.slot_index, cmd.generation);
	            }
            	break;
            }
        	case CommandType::SET_FILTER: {
	            v->filter.coeffs = cmd.filter.coeffs;
            	v->filter.mix    = cmd.filter.mix;
            	if (!v->filter.active) {
            		v->filter.channel[0].Reset();
            		v->filter.channel[1].Reset();
            	}
            	v->filter.active = true;
            	break;
            }
            case CommandType::CLEAR_FILTER: {
	            v->filter.active = false;
            	v->filter.channel[0].Reset();
            	v->filter.channel[1].Reset();
            	break;
            }
        	case CommandType::SET_TAPE: {
	            v->tape.rise_step = cmd.tape.rise_step;
            	v->tape.fall_step = cmd.tape.fall_step;
            	if (!v->tape.active) {
            		v->tape.position = 1.0f;
            		v->tape.step     = 0.0f;
            	}
            	v->tape.active = true;
            	break;
            }
            case CommandType::CLEAR_TAPE: {
	            v->tape.active   = false;
            	v->tape.position = 1.0f;
            	v->tape.step     = 0.0f;
            	break;
            }
            case CommandType::NONE:
            default:
                break;
        }
    }

    /**
     * @brief Per-callback streaming maintenance.
     * PREPARING voices are promoted to PLAYING once every slot is ready.
     * PLAYING voices keep their providers' buffers topped up.
     * @note Memory-resident providers make both hooks no-ops.
     */
    void ServiceStreamingVoices() noexcept {
        for (size_t si = 0; si < VoicePool::MAX_VOICE_COUNT; ++si) {
            Voice& v = voice_pool_.At(si);

            const bool serviceable = v.state == VoiceState::PREPARING
                                  || v.state == VoiceState::PLAYING
                                  || v.state == VoiceState::PAUSING
                                  || v.state == VoiceState::STOPPING;
            if (!serviceable) continue;

            bool all_ready = true;
            for (uint8_t wi = 0; wi < v.active_slot_count; ++wi) {
                Voice::Slot& slot = v.slots[wi];
                if (slot.streaming == nullptr) continue;	///< Memory resident: nothing to process

                const auto cursor = static_cast<uint64_t>(slot.cursor);
                if (cursor >= slot.streaming->TotalSampleCount()) continue;

                slot.streaming->UpdateStreaming(cursor);
                if (!slot.streaming->IsReadyAt(cursor)) all_ready = false;
            }

            if (v.state == VoiceState::PREPARING && all_ready) {
                v.state = VoiceState::PLAYING;
            }
        }
    }

    /**
     * @brief Settle voices whose pause / stop fade has completed.
     */
    void ConfirmFadeTransitions() noexcept {
        for (uint8_t slot = 0; slot < VoicePool::MAX_VOICE_COUNT; ++slot) {
            Voice& v = voice_pool_.At(slot);
            switch (v.state) {
                case VoiceState::PAUSING: {
	                if (v.tape.active) {
	                	if (v.tape.position <= Voice::TAPE_MIN_POSITION) {
	                		v.tape.position = 0.0f;
	                		v.tape.step     = 0.0f;
	                		v.state         = VoiceState::PAUSED;
	                	}
	                } else if (v.fade_gain <= 0.0f) {
	                	v.fade_gain = 0.0f;
	                	v.fade_step = 0.0f;
	                	v.state     = VoiceState::PAUSED;
	                }
                	break;
                }
                case VoiceState::STOPPING: {
	                if (v.tape.active) {
	                	if (v.tape.position <= Voice::TAPE_MIN_POSITION) {
	                		v.tape.position = 0.0f;
	                		v.tape.step     = 0.0f;
	                		v.state         = VoiceState::FINISHED;
	                		PostVoiceFinishedForSlot(slot);
	                	}
	                } else if (v.fade_gain <= 0.0f) {
	                	v.fade_gain = 0.0f;
	                	v.fade_step = 0.0f;
	                	v.state     = VoiceState::FINISHED;
	                	PostVoiceFinishedForSlot(slot);
	                }
                	break;
                }
                case VoiceState::PLAYING: {
	                // Fade-in finished: hold at full gain.
                	if (v.fade_step > 0.0f && v.fade_gain >= 1.0f) {
                		v.fade_gain = 1.0f;
                		v.fade_step = 0.0f;
                	}

                	if (v.tape.active && v.tape.step > 0.0f && v.tape.position >= 1.0f) {
                		v.tape.position = 1.0f;
                		v.tape.step     = 0.0f;
                	}
                	break;
                }
                default: break;
            }
        }
    }

    /**
     * @brief Walk every PLAYING voice.
     */
	void DetectAndReportFinishedVoices() noexcept {
        for (uint8_t slot = 0; slot < VoicePool::MAX_VOICE_COUNT; ++slot) {
            Voice& v = voice_pool_.At(slot);
            if (v.state != VoiceState::PLAYING) continue;
            if (v.active_slot_count == 0)       continue;
            if (v.loop_end > 0)                 continue;	///< Looping voices only stop by command

            bool all_done = true;
            for (uint8_t wi = 0; wi < v.active_slot_count; ++wi) {
                const Voice::Slot& active = v.slots[wi];
                if (active.provider == nullptr) continue;

                const auto total = active.provider->TotalSampleCount();
                if (active.cursor < static_cast<double>(total)) {
                    all_done = false;
                    break;
                }
            }

            if (all_done) {
                v.state = VoiceState::FINISHED;
                PostVoiceFinishedForSlot(slot);
            }
        }
    }

    /**
     * @brief Posts a VOICE_FINISHED event for a specific slot to the event queue.
     * @param slot The index of the slot for which the VOICE_FINISHED event is posted.
     * @param generation The generation identifier to associate with this event.
     */
    void PostVoiceFinished(uint8_t slot, uint32_t generation) noexcept {
        Event e;
        e.type       = EventType::VOICE_FINISHED;
        e.slot_index = slot;
        e.generation = generation;
    	(void)event_queue_.Enqueue(e);
    }

    /**
     * @brief Posts a VOICE_FINISHED event for a specific slot to the event queue.
     * @param slot The index of the slot for which the VOICE_FINISHED event is posted.
     */
    void PostVoiceFinishedForSlot(uint8_t slot) noexcept {
        Event e;
        e.type       = EventType::VOICE_FINISHED;
        e.slot_index = slot;
        e.generation = 0;
        (void)event_queue_.Enqueue(e);
    }

    /**
     * @brief Main-thread event handler.
     */
    void HandleEvent(const Event& e) noexcept {
        if (e.type != EventType::VOICE_FINISHED) return;

        // Locate the voice living in this slot right now.
        Voice& v = voice_pool_.At(e.slot_index);
        if (v.state == VoiceState::INACTIVE) return;

        // Unlink from the owning CuePlayer, if any, before releasing.
        if (v.owner != nullptr) {
            const uint32_t gen = voice_pool_.SlotGeneration(e.slot_index);
            const WitVoiceHn live = voice_handle::Encode(e.slot_index, gen);
            v.owner->ForgetVoice(live);
        }

        // Release the slot, bumping its generation counter so any outstanding user-side handle is invalidated.
        const uint32_t gen = voice_pool_.SlotGeneration(e.slot_index);
        const WitVoiceHn live = voice_handle::Encode(e.slot_index, gen);
        voice_pool_.Release(live);
    }
};

extern "C" {
/**
 * @brief Callback function for audio data processing.
 *
 * @note This function is executed on the audio thread.
 */
static void WitDataCallback(ma_device* device, void* output,
                            const void* input, ma_uint32 frame_count) {
    (void)input; ///< unused in this function
    auto* impl = static_cast<AudioEngine::Impl*>(device->pUserData);
    if (impl == nullptr) {
        std::memset(output, 0, static_cast<size_t>(frame_count) * 2 * sizeof(float));
        return;
    }
    impl->OnAudioCallback(static_cast<float*>(output), frame_count);
}
}

/* =====================================================================
 * AudioEnigne Public methods
 * ===================================================================== */
AudioEngine::AudioEngine() : impl_(std::make_unique<Impl>()) {}
AudioEngine::~AudioEngine() { Shutdown(); }

WitResult AudioEngine::Init(const char* wpb_path, const WitInitParams* params) {
    (void)params;
	WitResult res = WIT_RESULT_SUCCESS;

    if (!impl_)                     return WIT_RESULT_INIT_FAILED;
    if (impl_->project_data_)       return WIT_RESULT_INIT_FAILED;
    if (wpb_path == nullptr)        return WIT_RESULT_INIT_FAILED;

    std::vector<uint8_t> bytes;
	res = ReadFileBytes(wpb_path, bytes);
    if (res != WIT_RESULT_SUCCESS) return res;

    auto pd = std::make_unique<ProjectData>();
	res = pd->Load(bytes.data(), bytes.size());
    if (res != WIT_RESULT_SUCCESS) return res;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate        = pd->Format().sample_rate;
    config.dataCallback      = &WitDataCallback;
    config.pUserData         = impl_.get();

    if (ma_device_init(nullptr, &config, &impl_->device_) != MA_SUCCESS) {
        std::fprintf(stderr, "[wit] miniaudio device init failed\n");
        return WIT_RESULT_INIT_FAILED;
    }
    impl_->device_initialized_ = true;

    if (ma_device_start(&impl_->device_) != MA_SUCCESS) {
        std::fprintf(stderr, "[wit] miniaudio device start failed\n");
        ma_device_uninit(&impl_->device_);
        impl_->device_initialized_ = false;
        return WIT_RESULT_INIT_FAILED;
    }
    impl_->device_started_ = true;

    impl_->project_data_        = std::move(pd);

    // Resolve the fade length in samples once, against the real sample rate.
    const uint32_t sample_rate  = impl_->project_data_->Format().sample_rate;
    const auto     fade_samples = static_cast<uint32_t>(
        (Mixer::FADE_DURATION_MS * static_cast<float>(sample_rate)) / 1000.0f);
    impl_->fade_samples_        = fade_samples > 0 ? fade_samples : 1;

    impl_->cue_collection_store_       = std::make_unique<CueCollectionStore>();
    impl_->cue_player_register_ = std::make_unique<CuePlayerRegister>();

    impl_->streaming_reader_ = std::make_unique<StreamingReader>();
    impl_->streaming_reader_->Start();

    return WIT_RESULT_SUCCESS;
}

void AudioEngine::Shutdown() {
    if (!impl_) return;

    if (impl_->device_started_) {
        ma_device_stop(&impl_->device_);
        impl_->device_started_ = false;
    }
    if (impl_->device_initialized_) {
        ma_device_uninit(&impl_->device_);
        impl_->device_initialized_ = false;
    }

    if (impl_->streaming_reader_) {
        impl_->streaming_reader_->Stop();
        impl_->streaming_reader_.reset();
    }

    impl_->cue_player_register_.reset();
    impl_->cue_collection_store_.reset();
    impl_->project_data_.reset();
}

WitResult AudioEngine::Update() {
    if (!impl_ || !impl_->project_data_) return WIT_RESULT_INIT_FAILED;

    Event event;
    while (impl_->event_queue_.Dequeue(event)) impl_->HandleEvent(event);

    return WIT_RESULT_SUCCESS;
}

void AudioEngine::PauseOutput() {
    if (!impl_ || !impl_->device_started_) return;
    ma_device_stop(&impl_->device_);
    impl_->device_started_ = false;
}

void AudioEngine::ResumeOutput() {
    if (!impl_ || !impl_->device_initialized_) return;
    if (impl_->device_started_)                return;
    if (ma_device_start(&impl_->device_) == MA_SUCCESS) impl_->device_started_ = true;
}

bool AudioEngine::IsInitialized() const {
    return impl_ && impl_->project_data_ != nullptr;
}

const ProjectData* AudioEngine::GetProjectData() const {
    return impl_ ? impl_->project_data_.get() : nullptr;
}

WitWccbHn AudioEngine::LoadCueCollection(const char* wccb_path, const char* wwb_path,
                                WitResult& out_status) {
    if (!impl_ || !impl_->project_data_ || !impl_->cue_collection_store_) {
        out_status = WIT_RESULT_INIT_FAILED;
        return WIT_INVALID_WCCB_HN;
    }
    if (wccb_path == nullptr || wwb_path == nullptr) {
        out_status = WIT_RESULT_FILE_NOT_FOUND;
        return WIT_INVALID_WCCB_HN;
    }

	WitResult res = WIT_RESULT_SUCCESS;
    std::vector<uint8_t> wccb_bytes;

	res = ReadFileBytes(wccb_path, wccb_bytes);
    if (res != WIT_RESULT_SUCCESS) {
        out_status = res;
        return WIT_INVALID_WCCB_HN;
    }

    if (!impl_->streaming_reader_) {
        out_status = WIT_RESULT_INIT_FAILED;
        return WIT_INVALID_WCCB_HN;
    }

    // The .wwb is not loaded here: memory-resident waveforms are range-read
    // inside CueCollection::Load, streaming waveforms stay on disk until playback.
    return impl_->cue_collection_store_->Create(
        wccb_bytes.data(), wccb_bytes.size(),
        wwb_path,
        *impl_->project_data_, *impl_->streaming_reader_, out_status);
}

void AudioEngine::UnloadCueCollection(WitWccbHn handle) {
    if (!impl_ || !impl_->cue_collection_store_) return;
    impl_->cue_collection_store_->Destroy(handle);
}

const CueCollection* AudioEngine::FindCueCollection(WitWccbHn handle) const {
    if (!impl_ || !impl_->cue_collection_store_) return nullptr;
    return impl_->cue_collection_store_->Data(handle);
}

WitCuePlayerHn AudioEngine::CreateCuePlayer() {
    if (!impl_ || !impl_->cue_player_register_) return WIT_INVALID_CUE_PLAYER_HN;
    return impl_->cue_player_register_->Create();
}

void AudioEngine::DestroyCuePlayer(WitCuePlayerHn handle) {
    if (!impl_ || !impl_->cue_player_register_) return;
    impl_->cue_player_register_->Destroy(handle);
}

WitResult AudioEngine::CuePlayerAttachCue(WitCuePlayerHn player_handle, WitWccbHn wccb_handle, const char* cue_name) {
    if (!impl_ || !impl_->cue_player_register_) return WIT_RESULT_INIT_FAILED;
    if (cue_name == nullptr)                    return WIT_RESULT_NO_CUE_ATTACHED;

    CuePlayer* player = impl_->cue_player_register_->Find(player_handle);
    if (player == nullptr) return WIT_RESULT_INVALID_HANDLE;

    const CueCollection* collection = impl_->cue_collection_store_ ? impl_->cue_collection_store_->Data(wccb_handle) : nullptr;
    if (collection == nullptr) return WIT_RESULT_INVALID_HANDLE;

    // Data query happens here; the player only stores the resolved binding key.
    const CueData* cue = collection->FindCue(cue_name);
    if (cue == nullptr) return WIT_RESULT_NO_CUE_ATTACHED;

    return player->AttachCue(wccb_handle, cue);
}

WitResult AudioEngine::CuePlayerPlay(WitCuePlayerHn player_handle, WitVoiceHn* out_voice) {
    if (out_voice != nullptr) *out_voice = WIT_INVALID_VOICE_HN;
    if (!impl_) return WIT_RESULT_INIT_FAILED;

    CuePlayer* player = impl_->cue_player_register_ ? impl_->cue_player_register_->Find(player_handle) : nullptr;
    if (player == nullptr) return WIT_RESULT_INVALID_HANDLE;

    const CueData*  cue    = player->AttachedCue();
    const WitWccbHn handle = player->AttachedHandle();
    if (cue == nullptr || handle == WIT_INVALID_WCCB_HN) return WIT_RESULT_NO_CUE_ATTACHED;
    if (cue->waveform_count == 0)                        return WIT_RESULT_NO_CUE_ATTACHED;
    CueCollectionStore& store = *impl_->cue_collection_store_;

    // A streaming provider holds one read position, so a streaming Cue may drive only one active Voice at a time.
	// HACK: Refactor
    if (cue->streaming_mode == StreamingMode::STREAMING) {
        for (size_t si = 0; si < VoicePool::MAX_VOICE_COUNT; ++si) {
            const Voice& other = impl_->voice_pool_.At(si);
            if (other.cue != cue)                     continue;
            if (other.state == VoiceState::INACTIVE)  continue;
            if (other.state == VoiceState::FINISHED)  continue;
            return WIT_RESULT_STREAMING_BUSY;
        }
    }

    // Acquire a Voice slot.
    const WitVoiceHn voice_hn = impl_->voice_pool_.Acquire();
    if (voice_hn == WIT_INVALID_VOICE_HN) return WIT_RESULT_VOICE_EXHAUSTED;

    Voice* v = impl_->voice_pool_.Find(voice_hn);
    if (v == nullptr) {
        impl_->voice_pool_.Release(voice_hn);
        return WIT_RESULT_VOICE_EXHAUSTED;
    }

    // Compute post-randomization volume scalars.
    float vol_scale = cue->base_volume;
    if (cue->volume_random.enabled) {
        vol_scale *= SampleRandomInRange(cue->volume_random.min, cue->volume_random.max);
    }

    // Compute post-randomization pitch scalars.
    float pitch_scale = cue->base_pitch;
    if (cue->pitch_random.enabled) {
        pitch_scale *= SampleRandomInRange(cue->pitch_random.min, cue->pitch_random.max);
    }

    v->category_id  = cue->category_id;
    v->loop_enabled = cue->loop_enabled;
    v->volume       = vol_scale;
    v->pitch        = pitch_scale;
    v->cue          = cue;
    v->owner		= player;

    v->loop_start = 0;
    v->loop_end   = 0;
    if (cue->loop_enabled && cue->cue_type == CueType::POLYPHONIC) {
        uint64_t longest = 0;
        for (uint16_t wi = 0; wi < cue->waveform_count; ++wi) {
            longest = std::max(longest, cue->waveforms[wi].sample_count);
        }
        const uint64_t loop_end = cue->loop_end_point != 0
            ? std::min(cue->loop_end_point, longest)
            : longest;	///< 0 = end of the longest registered waveform
        if (cue->loop_start_point < loop_end) {
            v->loop_start = cue->loop_start_point;
            v->loop_end   = loop_end;
        }
    }

    // Fill in waveform slots based on the cue's type.
    const auto total_waves = static_cast<uint32_t>(cue->waveform_count);
    switch (cue->cue_type) {
        case CueType::POLYPHONIC: {
            v->active_slot_count = static_cast<uint8_t>(total_waves);
            for (uint16_t i = 0; i < total_waves; ++i) {
                const WaveformBinding* binding = store.Binding(handle, *cue, i);
                if (binding == nullptr) return WIT_RESULT_INVALID_HANDLE;
                v->slots[i].provider  = binding->provider;
                v->slots[i].streaming = binding->streaming;
                v->slots[i].cursor    = 0.0;
            }

        	break;
        }
        case CueType::SHUFFLE: {
            if (!player->HasShuffleOrder()) {
                std::vector<uint8_t> order(total_waves);
                std::iota(order.begin(), order.end(), static_cast<uint8_t>(0));
                std::shuffle(order.begin(), order.end(), g_random_engine);
                player->SetShuffleOrder(std::move(order));
            }
            const auto& order  = player->ShuffleOrder();
            const uint32_t pos = std::min<uint32_t>(player->ShuffleCursor(), total_waves - 1);
            const uint16_t pick = order[pos];

            const WaveformBinding* binding = store.Binding(handle, *cue, pick);
            if (binding == nullptr) return WIT_RESULT_INVALID_HANDLE;
            v->active_slot_count  = 1;
            v->slots[0].provider  = binding->provider;
            v->slots[0].streaming = binding->streaming;
            v->slots[0].cursor    = 0.0;

            player->SetShuffleCursor(cue->loop_enabled
                ? (pos + 1) % total_waves
                : std::min(pos + 1, total_waves - 1));

            break;
        }
        case CueType::SEQUENTIAL: {
            const uint32_t idx = cue->loop_enabled
                ? player->GetSequentialIndex() % total_waves
                : std::min(player->GetSequentialIndex(), total_waves - 1);

            const WaveformBinding* binding = store.Binding(handle, *cue, static_cast<uint16_t>(idx));
            if (binding == nullptr) return WIT_RESULT_INVALID_HANDLE;
            v->active_slot_count  = 1;
            v->slots[0].provider  = binding->provider;
            v->slots[0].streaming = binding->streaming;
            v->slots[0].cursor    = 0.0;

            player->SetSequentialIndex(cue->loop_enabled
                ? (idx + 1) % total_waves
                : std::min(idx + 1, total_waves - 1));	///< NEXT!

            break;
        }
    }

    // Post START_PLAYING so the audio thread flips state from CLAIMED to PLAYING.
    Command cmd;
    cmd.type       = CommandType::START_PLAYING;
    cmd.slot_index = voice_handle::DecodeSlot(voice_hn);
    cmd.generation = voice_handle::DecodeGeneration(voice_hn);
    if (!impl_->command_queue_.Enqueue(cmd)) {
        impl_->voice_pool_.Release(voice_hn);
        return WIT_RESULT_VOICE_EXHAUSTED;
    }

    player->RegisterSpawnedVoice(voice_hn);
    if (out_voice != nullptr) *out_voice = voice_hn;

	return WIT_RESULT_SUCCESS;
}

namespace {
/**
 * @brief Broadcasts a command to a set of voice handles.
 * Silent no-op for invalid handles or if the queue is full.
 *
 * @param q       The ring buffer where commands are enqueued.
 * @param handles A vector of voice handles to which the command will be sent.
 *                Invalid handles are skipped.
 * @param type    The type of command being broadcast to the handles.
 */
void BroadcastCommand(RingBuffer<Command, SPSC_QUEUE_CAPACITY>& q, const std::vector<WitVoiceHn>& handles, CommandType type) {
    for (const WitVoiceHn hn : handles) {
        if (hn == WIT_INVALID_VOICE_HN) continue;
        Command cmd;
        cmd.type       = type;
        cmd.slot_index = voice_handle::DecodeSlot(hn);
        cmd.generation = voice_handle::DecodeGeneration(hn);
        (void)q.Enqueue(cmd);
    }
}

}  // namespace

WitResult AudioEngine::CuePlayerStopAll(WitCuePlayerHn player_handle) {
    if (!impl_) return WIT_RESULT_INIT_FAILED;

    CuePlayer* p = impl_->cue_player_register_ ? impl_->cue_player_register_->Find(player_handle) : nullptr;
    if (p == nullptr) return WIT_RESULT_INVALID_HANDLE;

	BroadcastCommand(impl_->command_queue_, p->SpawnedVoices(), CommandType::STOP_VOICE);

    return WIT_RESULT_SUCCESS;
}

WitResult AudioEngine::CuePlayerPauseAll(WitCuePlayerHn player_handle) {
    if (!impl_) return WIT_RESULT_INIT_FAILED;

    CuePlayer* p = impl_->cue_player_register_ ? impl_->cue_player_register_->Find(player_handle) : nullptr;
    if (p == nullptr) return WIT_RESULT_INVALID_HANDLE;

	BroadcastCommand(impl_->command_queue_, p->SpawnedVoices(), CommandType::PAUSE_VOICE);

	return WIT_RESULT_SUCCESS;
}

WitResult AudioEngine::CuePlayerResumeAll(WitCuePlayerHn player_handle) {
    if (!impl_) return WIT_RESULT_INIT_FAILED;

    CuePlayer* p = impl_->cue_player_register_ ? impl_->cue_player_register_->Find(player_handle) : nullptr;
    if (p == nullptr) return WIT_RESULT_INVALID_HANDLE;

	BroadcastCommand(impl_->command_queue_, p->SpawnedVoices(), CommandType::RESUME_VOICE);

    return WIT_RESULT_SUCCESS;
}

WitResult AudioEngine::CuePlayerSetSequentialIndex(WitCuePlayerHn player_handle, uint32_t index) {
    if (!impl_) return WIT_RESULT_INIT_FAILED;

    CuePlayer* p = impl_->cue_player_register_ ? impl_->cue_player_register_->Find(player_handle) : nullptr;
    if (p == nullptr) return WIT_RESULT_INVALID_HANDLE;

	p->SetSequentialIndex(index);

	return WIT_RESULT_SUCCESS;
}

WitResult AudioEngine::CuePlayerGetSequentialIndex(WitCuePlayerHn player_handle, uint32_t* out_index) {
    if (out_index != nullptr) *out_index = 0;
    if (!impl_) return WIT_RESULT_INIT_FAILED;

	const CuePlayer* p = impl_->cue_player_register_ ? impl_->cue_player_register_->Find(player_handle) : nullptr;
    if (p == nullptr) return WIT_RESULT_INVALID_HANDLE;
    if (out_index != nullptr) *out_index = p->GetSequentialIndex();

    return WIT_RESULT_SUCCESS;
}

WitPlaybackStatus AudioEngine::VoiceGetStatus(WitVoiceHn voice_handle) {
	if (!impl_) return WIT_PLAYBACK_STATUS_IDLE;

	const Voice* v = impl_->voice_pool_.Find(voice_handle);
	if (v == nullptr) return WIT_PLAYBACK_STATUS_IDLE;

	switch (v->state) {
		case VoiceState::INACTIVE:  return WIT_PLAYBACK_STATUS_IDLE;
		case VoiceState::CLAIMED:   return WIT_PLAYBACK_STATUS_PENDING;
		case VoiceState::PREPARING: return WIT_PLAYBACK_STATUS_PENDING;
		case VoiceState::PLAYING:   return WIT_PLAYBACK_STATUS_PLAYING;
		case VoiceState::PAUSING:   return WIT_PLAYBACK_STATUS_PAUSED;
		case VoiceState::PAUSED:    return WIT_PLAYBACK_STATUS_PAUSED;
		case VoiceState::STOPPING:  return WIT_PLAYBACK_STATUS_STOPPING;
		case VoiceState::FINISHED:  return WIT_PLAYBACK_STATUS_FINISHED;
	}

	return WIT_PLAYBACK_STATUS_IDLE;
}

bool AudioEngine::VoiceIsActive(WitVoiceHn voice_handle) {
	if (!impl_) return false;

	const Voice* v = impl_->voice_pool_.Find(voice_handle);
	if (v == nullptr) return false;

	switch (v->state) {
		case VoiceState::CLAIMED:
		case VoiceState::PREPARING:
		case VoiceState::PLAYING:
		case VoiceState::PAUSING:
		case VoiceState::PAUSED:
			return true;

		case VoiceState::INACTIVE:
		case VoiceState::STOPPING:
		case VoiceState::FINISHED:
			return false;
	}

	return false;
}

namespace {
/**
 * @brief Posts a voice-related command into the queue for audio processing.
 * @return WIT_RESULT_SUCCESS if the command was successfully posted to the queue.
 * @retval WIT_RESULT_INVALID_HANDLE if the provided voice handle is invalid.
 */
WitResult PostVoiceCommand(RingBuffer<Command, SPSC_QUEUE_CAPACITY>& q, VoicePool&  pool,
                           WitVoiceHn handle,							CommandType type) {
    if (pool.Find(handle) == nullptr) return WIT_RESULT_INVALID_HANDLE;

    Command cmd;
    cmd.type       = type;
    cmd.slot_index = voice_handle::DecodeSlot(handle);
    cmd.generation = voice_handle::DecodeGeneration(handle);
    (void)q.Enqueue(cmd);

	return WIT_RESULT_SUCCESS;
}
}  // namespace

WitResult AudioEngine::VoicePause(WitVoiceHn voice_handle) {
    if (!impl_) return WIT_RESULT_INIT_FAILED;
    return PostVoiceCommand(impl_->command_queue_, impl_->voice_pool_, voice_handle, CommandType::PAUSE_VOICE);
}

WitResult AudioEngine::VoiceResume(WitVoiceHn voice_handle) {
    if (!impl_) return WIT_RESULT_INIT_FAILED;
    return PostVoiceCommand(impl_->command_queue_, impl_->voice_pool_, voice_handle, CommandType::RESUME_VOICE);
}

WitResult AudioEngine::VoiceStop(WitVoiceHn voice_handle) {
    if (!impl_) return WIT_RESULT_INIT_FAILED;
    return PostVoiceCommand(impl_->command_queue_, impl_->voice_pool_, voice_handle, CommandType::STOP_VOICE);
}

WitResult AudioEngine::VoiceSetFilter(WitVoiceHn voice_handle, const WitFilterParams* params) {
    if (!impl_)                        return WIT_RESULT_INIT_FAILED;
    if (params == nullptr)             return WIT_RESULT_INVALID_HANDLE;
    if (!impl_->project_data_)         return WIT_RESULT_INIT_FAILED;
    if (impl_->voice_pool_.Find(voice_handle) == nullptr) return WIT_RESULT_INVALID_HANDLE;

    // Coefficients are computed here on the main thread.
    const auto  sample_rate = static_cast<float>(impl_->project_data_->Format().sample_rate);
    const float mix         = params->mix_level;

    Command cmd;
    cmd.type          = CommandType::SET_FILTER;
    cmd.slot_index    = voice_handle::DecodeSlot(voice_handle);
    cmd.generation    = voice_handle::DecodeGeneration(voice_handle);
    cmd.filter.coeffs = biquad::MakeLowpass(params->cutoff_hz, sample_rate);
    cmd.filter.mix    = (mix < 0.0f) ? 0.0f : (mix > 1.0f ? 1.0f : mix);
    (void)impl_->command_queue_.Enqueue(cmd);

    return WIT_RESULT_SUCCESS;
}

WitResult AudioEngine::VoiceClearFilter(WitVoiceHn voice_handle) {
    if (!impl_) return WIT_RESULT_INIT_FAILED;
    return PostVoiceCommand(impl_->command_queue_, impl_->voice_pool_, voice_handle, CommandType::CLEAR_FILTER);
}

/* =====================================================================
 * Voice - tape effect
 * ===================================================================== */
namespace {
/**
 * @brief Convert a ramp duration into a per-sample speed increment.
 * @return 0 when the duration is not positive, meaning the ramp is skipped.
 */
float TapeStepFromMs(float duration_ms, float sample_rate) noexcept {
    if (duration_ms <= 0.0f) return 0.0f;

    const float samples = (duration_ms * sample_rate) / 1000.0f;
    if (samples < 1.0f) return 0.0f;

    return 1.0f / samples;
}
}  // namespace

WitResult AudioEngine::VoiceSetTapeEffect(WitVoiceHn voice_handle, const WitTapeParams* params) {
    if (!impl_)                        return WIT_RESULT_INIT_FAILED;
    if (params == nullptr)             return WIT_RESULT_INVALID_HANDLE;
    if (!impl_->project_data_)         return WIT_RESULT_INIT_FAILED;
    if (impl_->voice_pool_.Find(voice_handle) == nullptr) return WIT_RESULT_INVALID_HANDLE;

    const auto sample_rate = static_cast<float>(impl_->project_data_->Format().sample_rate);

    Command cmd;
    cmd.type           = CommandType::SET_TAPE;
    cmd.slot_index     = voice_handle::DecodeSlot(voice_handle);
    cmd.generation     = voice_handle::DecodeGeneration(voice_handle);
    cmd.tape.rise_step = TapeStepFromMs(params->start_ms, sample_rate);
    cmd.tape.fall_step = TapeStepFromMs(params->stop_ms,  sample_rate);
    (void)impl_->command_queue_.Enqueue(cmd);

    return WIT_RESULT_SUCCESS;
}

WitResult AudioEngine::VoiceClearTapeEffect(WitVoiceHn voice_handle) {
    if (!impl_) return WIT_RESULT_INIT_FAILED;
    return PostVoiceCommand(impl_->command_queue_, impl_->voice_pool_, voice_handle, CommandType::CLEAR_TAPE);
}

}