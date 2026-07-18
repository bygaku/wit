#ifndef WIT_BUILD_DATAS_H
#define WIT_BUILD_DATAS_H

#include <cstdint>
#include <string>
#include <vector>

#include "binary/uuid.h"
#include "data/audio_format.h"
#include "data/category_info.h"
#include "data/cue_data.h"
#include "data/ducking_settings.h"

namespace wit::studio {

/**
 * @enum Severity
 */
enum class Severity {
	ERROR,      ///< The build cannot proceed / did not produce valid output
	WARNING,    ///< The build succeeded but the user should be notified
};

/**
 * @struct Issue
 */
struct Issue {
	Severity    severity = Severity::ERROR;
	std::string code;       ///< Stable identifier, e.g. "WAV_NOT_FOUND"
	std::string location;   ///< Location, e.g. "bgm/stage1"
	std::string message;    ///< Description
};

/**
 * @struct BuildResult
 * @brief Aggregated outcome of validation + build.
 */
struct BuildResult {
	bool               succeeded = false;
	std::vector<Issue> issues;

	[[nodiscard]] bool HasErrors() const noexcept {
		for (const auto& issue : issues) {
			if (issue.severity == Severity::ERROR) return true;
		}
		return false;
	}
};

/**
 * @struct WaveformModel
 * @brief A single audio file reference registered to a cue.
 *
 * @note wav_file_path is stored relative to the .wproj location.
 */
struct WaveformModel {
    std::string wav_file_path;
    std::string display_name;
};

/**
 * @struct CueModel
 * @brief Editing-time state of a single cue.
 */
struct CueModel {
    uint32_t                    cue_id          = 0;    ///< Assigned by WitStudio.
    std::string                 cue_name;
    uint16_t                    category_id     = 1;
    CueType                     cue_type        = CueType::POLYPHONIC;
    bool                        loop_enabled    = true;
    StreamingMode               streaming_mode  = StreamingMode::MEMORY_RESIDENT;
    float                       base_volume     = 1.0f;
    float                       base_pitch      = 1.0f;
    RandomizeRange              volume_random;
    RandomizeRange              pitch_random;
    std::vector<WaveformModel>	waveforms;
};

/**
 * @struct CueCollectionModel
 * @brief Editing-time state of a cue collection.
 * Builds into one .wccb + .wwb pair.
 */
struct CueCollectionModel {
    std::string				name;
    binary::Uuid			wccb_uuid;	///< Generated once at creation
    std::vector<CueModel>	cues;
};

/**
 * @struct ProjectModel
 * @brief Editing-time state of the whole project (the .wproj content).
 */
struct ProjectModel {
    binary::Uuid                    project_uuid;		///< Generated once at creation
    std::string                     project_name;
    AudioFormat                     audio_format;
    DuckingSettings                 ducking;
    std::vector<CategoryInfo>       categories;
    std::vector<CueCollectionModel> cue_collections;
    uint32_t                        next_cue_id = 1;	///< Counter for cue_id assignment
};

}

#endif // WIT_BUILD_DATAS_H
