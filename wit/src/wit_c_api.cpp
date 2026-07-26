/**
 * Created by intwi on 2026/07/02.
 * Copyright (c) 2026 All rights reserved.
 */

#include <wit.h>

#include <cstdio>
#include <memory>

#include "audio_engine.h"

namespace {
/**
 * @brief Process-wide engine instance.
 * @note std::unique_ptr auto-destroys at program exit, so callers who forget WitUninit still leak nothing.
 */
std::unique_ptr<wit::AudioEngine> g_engine;
}  // namespace

extern "C" {

/* =====================================================================
 * System
 * ===================================================================== */

WitResult WitInit(const char* wpb_path, const WitInitParams* params) {
    if (g_engine)             return WIT_RESULT_INIT_FAILED;
    if (wpb_path == nullptr)  return WIT_RESULT_INIT_FAILED;

    auto engine = std::make_unique<wit::AudioEngine>();
    const WitResult result = engine->Init(wpb_path, params);
    if (result != WIT_RESULT_SUCCESS) {
        return result;
    }
    g_engine = std::move(engine);
    return WIT_RESULT_SUCCESS;
}

void WitUninit(void) {
    if (!g_engine) return;
    g_engine->Shutdown();
    g_engine.reset();
}

WitResult WitSystem_Update(void) {
    if (!g_engine) return WIT_RESULT_INIT_FAILED;
    return g_engine->Update();
}

/* =====================================================================
 * Output control
 * ===================================================================== */

void WitOutput_Pause(void) {
    if (g_engine) g_engine->PauseOutput();
}

void WitOutput_Resume(void) {
    if (g_engine) g_engine->ResumeOutput();
}

/* =====================================================================
 * Wccb handle (C API name kept: the handle names the loaded .wccb file)
 * ===================================================================== */

WitWccbHn WitWccbHandle_Create(const char* wccb_path, const char* wwb_path) {
    if (!g_engine) return WIT_INVALID_WCCB_HN;

    WitResult status = WIT_RESULT_INIT_FAILED;
    const WitWccbHn h = g_engine->LoadCueCollection(wccb_path, wwb_path, status);
    if (h == WIT_INVALID_WCCB_HN) {
        std::fprintf(stderr,
                     "[wit] WitWccbHandle_Createに失敗してWitResultを返しました=%d\n",
                     static_cast<int>(status));
    }
    return h;
}

void WitWccbHandle_Destroy(WitWccbHn wccb) {
    if (!g_engine) return;
    g_engine->UnloadCueCollection(wccb);
}

/* =====================================================================
 * CuePlayer
 * ===================================================================== */

WitCuePlayerHn WitCuePlayer_Create(void) {
    if (!g_engine) return WIT_INVALID_CUE_PLAYER_HN;
    return g_engine->CreateCuePlayer();
}

void WitCuePlayer_Destroy(WitCuePlayerHn player) {
    if (!g_engine) return;
    g_engine->DestroyCuePlayer(player);
}

WitResult WitCuePlayer_AttachCue(WitCuePlayerHn player,
                                 WitWccbHn      wccb,
                                 const char*    cue_name) {
    if (!g_engine) return WIT_RESULT_INIT_FAILED;
    return g_engine->CuePlayerAttachCue(player, wccb, cue_name);
}

WitResult WitCuePlayer_Play(WitCuePlayerHn player, WitVoiceHn* out_voice) {
    if (!g_engine) return WIT_RESULT_INIT_FAILED;
    return g_engine->CuePlayerPlay(player, out_voice);
}

WitResult WitCuePlayer_StopAll(WitCuePlayerHn player) {
    if (!g_engine) return WIT_RESULT_INIT_FAILED;
    return g_engine->CuePlayerStopAll(player);
}

WitResult WitCuePlayer_PauseAll(WitCuePlayerHn player) {
    if (!g_engine) return WIT_RESULT_INIT_FAILED;
    return g_engine->CuePlayerPauseAll(player);
}

WitResult WitCuePlayer_ResumeAll(WitCuePlayerHn player) {
    if (!g_engine) return WIT_RESULT_INIT_FAILED;
    return g_engine->CuePlayerResumeAll(player);
}

WitResult WitCuePlayer_SetSequentialIndex(WitCuePlayerHn player, uint32_t index) {
    if (!g_engine) return WIT_RESULT_INIT_FAILED;
    return g_engine->CuePlayerSetSequentialIndex(player, index);
}

WitResult WitCuePlayer_GetSequentialIndex(WitCuePlayerHn player, uint32_t* out_index) {
    if (!g_engine) return WIT_RESULT_INIT_FAILED;
    return g_engine->CuePlayerGetSequentialIndex(player, out_index);
}

/* =====================================================================
 * Voice - all
 * ===================================================================== */

WitPlaybackStatus WitVoice_GetStatus(WitVoiceHn voice) {
    if (!g_engine) return WIT_PLAYBACK_STATUS_IDLE;
    return g_engine->VoiceGetStatus(voice);
}

bool WitVoice_IsActive(WitVoiceHn voice) {
	if (!g_engine) return false;
	return g_engine->VoiceIsActive(voice);
}

WitResult WitVoice_Pause(WitVoiceHn voice) {
    if (!g_engine) return WIT_RESULT_INIT_FAILED;
    return g_engine->VoicePause(voice);
}

WitResult WitVoice_Resume(WitVoiceHn voice) {
    if (!g_engine) return WIT_RESULT_INIT_FAILED;
    return g_engine->VoiceResume(voice);
}

WitResult WitVoice_Stop(WitVoiceHn voice) {
    if (!g_engine) return WIT_RESULT_INIT_FAILED;
    return g_engine->VoiceStop(voice);
}

/* =====================================================================
 * Voice - filter effect
 * ===================================================================== */

WitResult WitVoice_SetFilter(WitVoiceHn voice, const WitFilterParams* params) {
    if (!g_engine) return WIT_RESULT_INIT_FAILED;
    return g_engine->VoiceSetFilter(voice, params);
}

WitResult WitVoice_ClearFilter(WitVoiceHn voice) {
    if (!g_engine) return WIT_RESULT_INIT_FAILED;
    return g_engine->VoiceClearFilter(voice);
}

/* =====================================================================
 * Voice - Tape effect
 * ===================================================================== */

WitResult WitVoice_SetTapeEffect(WitVoiceHn voice, const WitTapeParams* params) {
    if (!g_engine) return WIT_RESULT_INIT_FAILED;	///< HACK: No clamp now,
    return g_engine->VoiceSetTapeEffect(voice, params);
}

WitResult WitVoice_ClearTapeEffect(WitVoiceHn voice) {
    if (!g_engine) return WIT_RESULT_INIT_FAILED;
    return g_engine->VoiceClearTapeEffect(voice);
}

}  // extern "C"
