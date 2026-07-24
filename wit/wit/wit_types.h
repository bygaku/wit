/**
 * Created by intwi on 2026/07/02.
 * Copyright (c) 2026 All rights reserved.
 */

#ifndef WIT_TYPES_H
#define WIT_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =====================================================================
 * Opaque handle types
 * ===================================================================== */

typedef struct WitCuePlayerTag  WitCuePlayerTag;
typedef struct WitWccbTag       WitWccbTag;
typedef struct WitVoiceTag      WitVoiceTag;

typedef WitCuePlayerTag*  WitCuePlayerHn;
typedef WitWccbTag*       WitWccbHn;
typedef WitVoiceTag*      WitVoiceHn;

#define WIT_INVALID_CUE_PLAYER_HN  ((WitCuePlayerHn)0)
#define WIT_INVALID_WCCB_HN        ((WitWccbHn)0)
#define WIT_INVALID_VOICE_HN       ((WitVoiceHn)0)

/* =====================================================================
 * Result codes
 * ===================================================================== */

typedef enum WitResult {
    WIT_RESULT_SUCCESS = 0,			///< 成功
    WIT_RESULT_VOICE_EXHAUSTED,		///< 使用可能なボイスがありません。同時再生の最大値: 64
    WIT_RESULT_INVALID_HANDLE,		///< 無効なハンドルです。
    WIT_RESULT_NO_CUE_ATTACHED,		///< CuePlayer に Cue がアタッチされていません。
    WIT_RESULT_INIT_FAILED,			///< 初期化不良。
    WIT_RESULT_FILE_NOT_FOUND,		///< 指定されたファイルが見つかりません。
    WIT_RESULT_INVALID_FORMAT,		///< 無効なフォーマットです。
    WIT_RESULT_VERSION_MISMATCH,	///< バージョンの互換性がありません。
    WIT_RESULT_PROJECT_MISMATCH,	///< プロジェクトに含まれていない WCCB や WWB が指定されました。
    WIT_RESULT_WWB_MISMATCH,		///< WCCB に含まれていない WWB が指定されました。
    WIT_RESULT_UNKNOWN_CATEGORY,	///< 未知のカテゴリが指定されました。（0-4: Wit Defined 5-: User defined）
	WIT_RESULT_STREAMING_BUSY,		///< ストリーミング再生を設定したキューは同時に最大 1Voice のみ再生できます。
} WitResult;

typedef enum WitPlaybackStatus {
    WIT_PLAYBACK_STATUS_IDLE = 0,	///< 再生指示は出ていません。
    WIT_PLAYBACK_STATUS_PENDING,	///< 再生の準備が完了しました。まもなく PLAYING に移行します。
    WIT_PLAYBACK_STATUS_PLAYING,	///< 再生中です。
    WIT_PLAYBACK_STATUS_PAUSED,		///< 一時停止中です。
    WIT_PLAYBACK_STATUS_STOPPING,	///< 停止指示が出ました。まもなく FINISHED に移行して IDLE になります。
    WIT_PLAYBACK_STATUS_FINISHED,	///< 再生が完了しました。まもなく IDLE に移行します。
} WitPlaybackStatus;

/* =====================================================================
 * Initialization parameters
 * ===================================================================== */

typedef struct WitInitParams {
    int32_t reserved;  ///< Placeholder for future field additions
} WitInitParams;

/* =====================================================================
 * Filter parameters
 * ===================================================================== */

/**
 * @enum WitFilterType
 * @brief フィルタの種類
 */
typedef enum WitFilterType {
    WIT_FILTER_TYPE_LOWPASS = 0,	///< TIPS: こもった音になります。
} WitFilterType;

/** HACK: Q は固定（0.707）で、ユーザに認識させません。*/

/**
 * @struct WitFilterParams
 * @brief Voice に適用するフィルタのパラメータ。
 *
 * @note WIT_FILTER_PARAMS_DEFAULT で初期化すると、ローパス / 2000Hz / mix 100 が入ります。
 */
typedef struct WitFilterParams {
    WitFilterType type;			///< フィルタ種別。デフォルト: WIT_FILTER_TYPE_LOWPASS
    float         cutoff_hz;	///< カットオフ周波数[Hz]。デフォルト: 2000.0f
    float         mix_level;	///< 適応量[0-1]。1.f で完全にフィルタが適応されます。デフォルト: 1.f
} WitFilterParams;

/**
 * @brief WitFilterParams の既定値。
 * @note C++ では WitFilterParams p = WIT_FILTER_PARAMS_DEFAULT; のように使えます。
 */
#define WIT_FILTER_PARAMS_DEFAULT \
    { WIT_FILTER_TYPE_LOWPASS, 2000.0f, 1.0f }

/* =====================================================================
 * Tape parameters
 * ===================================================================== */

/**
 * @struct WitTapeParams
 * @brief Voice に適用するテープ効果のパラメータ。
 *
 * テープデッキの停止・再始動のように、再生速度が滑らかに変化します。
 * このパラメータを設定した Voice は、Play / Resume で start_ms をかけて通常速度まで加速し、
 * Stop / Pause で stop_ms をかけて停止します。
 *
 * @attention v1.0.0 beta.2 において、stop_ms に 2000ms の最大値を設けています。
 * reason: ストリーミング再生の音源の場合、多くのサンプルを消費し、ピッチが落ちきる前に補充が止まってしまうため。
 */
typedef struct WitTapeParams {
    float start_ms;	///< 停止状態から通常速度に戻るまでの時間[ms]。デフォルト: 400.f
    float stop_ms;	///< 通常速度から停止するまでの時間[ms]。デフォルト: 600.f 最大値: 2000.f
} WitTapeParams;

/**
 * @brief WitTapeParams の既定値。
 * @note C++ では WitTapeParams p = WIT_TAPE_PARAMS_DEFAULT; のように使えます。
 */
#define WIT_TAPE_PARAMS_DEFAULT \
    { 500.0f, 500.0f }

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // WIT_TYPES_H