/**
 * Created by intwi on 2026/05/28.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_AUDIO_ENGINE_H
#define WIT_AUDIO_ENGINE_H

#include <cstdint>
#include <memory>

#include <wit_types.h>

namespace wit {

class ProjectData;
class CueCollection;

/**
 * @class AudioEngine
 * @brief Runtime core
 *
 * @note The implementation is hidden.
 */
class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

#pragma region
    AudioEngine(const AudioEngine&)            = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;
    AudioEngine(AudioEngine&&)                 = delete;
    AudioEngine& operator=(AudioEngine&&)      = delete;
#pragma endregion

    /**
     * @brief Load .wpb, start the miniaudio playback device, and construct internal registers.
     */
    WitResult Init(const char* wpb_path, const WitInitParams* params);

    /**
     * @brief Stop the device, release all Wccbs and CuePlayers, drop the project.
     * @note Safe to call on an uninitialized engine.
     */
    void Shutdown();

    /**
     * @brief Main-thread frame update. Drains events posted by the audio thread since the last call.
     * @return INIT_FAILED If the engine has not been initialized.
     */
    WitResult Update();

    /**
     * @brief Miniaudio device pause.
     */
    void PauseOutput();

    /**
     * @brief Miniaudio device resume.
     */
    void ResumeOutput();

    /**
     * @brief initialized check.
     */
    [[nodiscard]] bool IsInitialized() const;

    /**
     * @brief Accessors used by higher-level API implementations.
     */
    [[nodiscard]] const ProjectData* GetProjectData() const;

    /**
     * @brief Loads WCCB and WWB files, parses their contents, and registers them with the internal audio system.
     *
     * @param out_status Receive detailed information about WitResult.
     * @retval WitWccbHn If the operation is successful,
     * @retval WIT_INVALID_WCCB_HN In case of an error.
     */
    WitWccbHn LoadCueCollection(const char* wccb_path, const char* wwb_path, WitResult& out_status);

    /**
     * @brief Unloads a WCCB (Waveform Cue Control Block) associated with the given handle.
     */
    void      UnloadCueCollection(WitWccbHn handle);

    /**
     * @brief Finds a CueCollection associated with the provided handle.
     * @return A pointer to the CueCollection if found; otherwise, nullptr.
     */
    const CueCollection* FindCueCollection(WitWccbHn handle) const;

    // ---- CuePlayer management ----
    WitCuePlayerHn CreateCuePlayer();
    void           DestroyCuePlayer(WitCuePlayerHn handle);


	/* =====================================================================
	 * Cue Attach
	 * ===================================================================== */
	WitResult CuePlayerAttachCue(WitCuePlayerHn player_handle, WitWccbHn wccb_handle, const char* cue_name);

	/* =====================================================================
	 * Start playback.
	 * ===================================================================== */
    WitResult CuePlayerPlay(WitCuePlayerHn player_handle, WitVoiceHn* out_voice);

	/* =====================================================================
	 * Broadcast operations across every voice CuePlayer spawned
	 * ===================================================================== */
	WitResult CuePlayerStopAll(WitCuePlayerHn player_handle);
    WitResult CuePlayerPauseAll(WitCuePlayerHn player_handle);
    WitResult CuePlayerResumeAll(WitCuePlayerHn player_handle);

	/* =====================================================================
	 * For SEQUENTIAL CueType
	 * ===================================================================== */
    WitResult CuePlayerSetSequentialIndex(WitCuePlayerHn player_handle, uint32_t  index);
    WitResult CuePlayerGetSequentialIndex(WitCuePlayerHn player_handle, uint32_t* out_index);

	/* =====================================================================
	 * Voice controller
	 * ===================================================================== */
    WitResult VoiceGetStatus(WitVoiceHn voice_handle, WitPlaybackStatus* out_status);
    WitResult VoicePause (WitVoiceHn voice_handle);
    WitResult VoiceResume(WitVoiceHn voice_handle);
    WitResult VoiceStop  (WitVoiceHn voice_handle);

public:
    struct Impl;

private:
    std::unique_ptr<Impl> impl_;
};

}

#endif // WIT_AUDIO_ENGINE_H
