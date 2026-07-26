/**
 * Created by intwi on 2026/07/02.
 * Copyright (c) 2026 All rights reserved.
 */

#ifndef WIT_H
#define WIT_H

#include "wit_types.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =====================================================================
 * System control
 * ===================================================================== */

/**
 * @brief Witas を起動し、.wpb を読み込む。
 * @param wpb_path .wpb ファイルへのパス (UTF-8)。
 * @param params   初期化パラメータ。NULL 可。
 * @retval WIT_RESULT_SUCCESS 初期化成功時。
 * @retval その他 エラー。
 */
WitResult WitInit(const char* wpb_path, const WitInitParams* params);

/**
 * @brief Witas を終了し、全てのリソースを解放する。
 */
void WitUninit(void);

/**
 * @brief メインスレッド更新。ゲームループから毎フレーム呼ぶ。
 * @retval WIT_RESULT_SUCCESS 成功時
 * @retval その他 エラー。
 */
WitResult WitSystem_Update(void);

/* =====================================================================
 * Output control
 * ===================================================================== */

/**
 * @brief デバイス出力を一時停止。
 * @note WitSystem_Update は動き続ける。
 */
void WitOutput_Pause(void);

/**
 * @brief デバイス出力を再開。
 */
void WitOutput_Resume(void);

/* =====================================================================
 * Wccb handle
 * ===================================================================== */

/**
 * @brief .wccb + .wwb を読み込み、Wccb ハンドルを返す。
 * @retval WitWccbHn 使用可能なハンドル。
 * @retval WIT_INVALID_WCCB_HN(0) 作成失敗時: e.g., パスが正しくない, wccb と wwb が一致していない。
 */
WitWccbHn WitWccbHandle_Create(const char* wccb_path, const char* wwb_path);

/**
 * @brief Wccb ハンドルを破棄する。
 * @note 関連する CuePlayer を先に破棄してください。
 */
void WitWccbHandle_Destroy(WitWccbHn wccb);

/* =====================================================================
 * CuePlayer
 * ===================================================================== */

/**
 * @brief 空の CuePlayer を作成します。
 * @retval WitCuePlayerHn 使用可能なハンドル。
 * @retval WIT_INVALID_CUE_PLAYER_HN(0)
 */
WitCuePlayerHn WitCuePlayer_Create(void);

/**
 * @brief CuePlayer ハンドルを破棄する。
 * @note 再生中の CuePlayer の解放はおすすめしません。
 */
void WitCuePlayer_Destroy(WitCuePlayerHn player);

/**
 * @brief WitCuePlayerHn を使用して、対応する CuePlayer に Cue を装備させます。
 * @param player Cue をアタッチしたい CuePlayer のハンドル
 * @param wccb アタッチしたい Cue が含まれる WCCB のハンドル
 * @param cue_name WitStudio 上で登録された Cue の名前
 * @retval WIT_RESULT_SUCCESS アタッチが完了した。
 * @retval その他 エラー。
 *
 * @note CuePlayer が空である必要はありません。ただし、上書きアタッチ扱いになるので、前使った Cue を使うには再度アタッチしてください。
 */
WitResult WitCuePlayer_AttachCue(WitCuePlayerHn player, WitWccbHn wccb, const char* cue_name);

/**
 * @brief Cue を再生開始。
 * @param player 有効な CuePlayerHn を渡す。
 * @param out_voice 有効な Voice ハンドルが欲しい場合使用する引数です。不要なら out_voice に NULL を渡す。
 * @retval WIT_RESULT_SUCCESS 再生指示が発行された。
 * @retval その他 エラー。
 */
WitResult WitCuePlayer_Play(WitCuePlayerHn player, WitVoiceHn* out_voice);

/**
 * @brief Cue が生成したすべての再生中 Voice を終了する。Voice ハンドルが不要なら out_voice に NULL を渡す。
 * @param player 有効な CuePlayerHn を渡す。
 * @retval WIT_RESULT_SUCCESS すべてのボイスを停止する指示が発行された。
 * @retval その他 エラー。
 *
 * @note 一時停止ではなく停止です。とめたところから再開する場合は Pause/Resume を使用してください。
 */
// WitResult WitCuePlayer_StopAll(WitCuePlayerHn   player);

/**
 * @brief Cue が生成したすべての再生中 Voice を一時停止する。
 * @param player 有効な CuePlayerHn を渡す。
 * @retval WIT_RESULT_SUCCESS すべてのボイスを一時停止する指示が発行された。
 * @retval その他 エラー。
 */
// WitResult WitCuePlayer_PauseAll(WitCuePlayerHn  player);

/**
 * @brief Cue が生成したすべての停止中の Voice を再開する。
 * @param player 有効な CuePlayerHn を渡す。
 * @retval WIT_RESULT_SUCCESS すべての一時停止中のボイスを再開する指示が発行された。
 * @retval その他 エラー。
 */
// WitResult WitCuePlayer_ResumeAll(WitCuePlayerHn player);

/**
 * @brief 次の Play 時に再生する Cue に登録された Waveform を設定します。
 * @param player 有効な CuePlayerHn を渡す。
 * @param index 次の Play 時に再生する Waveform を（ 0-15 ）指定する。
 * @retval WIT_RESULT_SUCCESS 成功時。
 * @retval その他 エラー。
 *
 * @note CueType: シーケンシャル に対応した関数です。
 */
WitResult WitCuePlayer_SetSequentialIndex(WitCuePlayerHn player, uint32_t index);

/**
 * @brief 次の Play 時に再生する Cue に登録された Waveform を取得します。
 * @param player 有効な CuePlayerHn を渡す。
 * @param out_index 値を入れ、次の Play 時に再生される Waveform を（ 0-15 ）取得する。
 * @retval WIT_RESULT_SUCCESS 成功時。
 * @retval その他 エラー。
 *
 * @note CueType: シーケンシャル に対応した関数です。
 */
WitResult WitCuePlayer_GetSequentialIndex(WitCuePlayerHn player, uint32_t* out_index);

/* =====================================================================
 * Voice
 * ===================================================================== */

/**
 * @brief Voice の現在の再生状況を返します。
 * @param voice Play 時に発行された WitVoiceHn。
 * @retval WitPlaybackStatus 現在の状態。無効ハンドルの場合は WIT_PLAYBACK_STATUS_IDLE。
 * @note 無効ハンドルは IDLE として扱われます。
 *       Pause/Resume のトグルなど、状態に応じた分岐に使用してください。
 */
WitPlaybackStatus WitVoice_GetStatus(WitVoiceHn voice);

/**
 * @brief Voice が生存中（PLAYING / PENDING / PAUSED）なら true を返します。
 * @param voice Play 時に発行された WitVoiceHn。
 * @retval true  再生セッションが継続中（一時停止含む）。
 * @retval false IDLE / STOPPING / FINISHED、または無効ハンドル。
 */
bool WitVoice_IsActive(WitVoiceHn voice);

/**
 * @brief Voice を一時停止します。
 * @param voice Play 時に発行された有効な WitVoiceHn。
 * @retval WIT_RESULT_SUCCESS 成功時。
 * @retval その他 エラー。
 */
WitResult WitVoice_Pause(WitVoiceHn voice);

/**
 * @brief 一時停止中の Voice を再開します。
 * @param voice Play 時に発行された有効な WitVoiceHn。
 * @retval WIT_RESULT_SUCCESS 成功時。
 * @retval その他 エラー。
 */
WitResult WitVoice_Resume(WitVoiceHn voice);

/**
 * @brief Voice を停止します。
 * @param voice Play 時に発行された有効な WitVoiceHn。
 * @retval WIT_RESULT_SUCCESS 成功時。
 * @retval その他 エラー。
 *
 * @note 途中から再開したい場合は Pause/Resume を使用してください。
 */
WitResult WitVoice_Stop(WitVoiceHn voice);

/**
 * @brief Voice にフィルタを設定（または上書き）します。
 * @param voice  Play 時に発行された有効な WitVoiceHn。
 * @param params フィルタのパラメータ。NULL 不可。
 * @retval WIT_RESULT_SUCCESS 成功時。
 * @retval その他 エラー。
 *
 * @note Q は固定（0.707）です。
 */
WitResult WitVoice_SetFilter(WitVoiceHn voice, const WitFilterParams* params);

/**
 * @brief Voice に設定されたフィルタを解除します。
 * @param voice Play 時に発行された有効な WitVoiceHn。
 * @retval WIT_RESULT_SUCCESS 成功時。
 * @retval その他 エラー。
 *
 * @note フィルタが設定されていない場合でも成功を返します。
 */
WitResult WitVoice_ClearFilter(WitVoiceHn voice);

/**
 * @brief Voice にテープ効果を設定（または上書き）します。
 * @param voice  Play 時に発行された有効な WitVoiceHn。
 * @param params テープ効果のパラメータ。NULL 不可。
 * @retval WIT_RESULT_SUCCESS 成功時。
 * @retval その他 エラー。
 *
 * @note 設定後は、この Voice に対する Play / Stop / Pause / Resume がすべてテープ効果つきの動作に変わります。
 */
WitResult WitVoice_SetTapeEffect(WitVoiceHn voice, const WitTapeParams* params);

/**
 * @brief Voice に設定されたテープ効果を解除します。
 * @param voice Play 時に発行された有効な WitVoiceHn。
 * @retval WIT_RESULT_SUCCESS 成功時。
 * @retval その他 エラー。
 *
 * @note 解除すると Play / Stop / Pause / Resume は通常動作に戻ります。
 */
WitResult WitVoice_ClearTapeEffect(WitVoiceHn voice);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // WIT_H