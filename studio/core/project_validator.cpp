#include "project_validator.h"

#include <set>
#include <string>

namespace wit::studio {

namespace {
/**
 * @brief Resolve a possibly relative source path against the project directory.
 */
std::filesystem::path ResolveSourcePath(const std::filesystem::path& project_dir, const std::string& file_path) {
    std::filesystem::path p(file_path);
    if (p.is_absolute()) return p;
    return project_dir / p;
}

void ValidateAudioFormat(const ProjectModel& project, std::vector<Issue>& out) {
    const AudioFormat& fmt = project.audio_format;

    const bool rate_ok = (fmt.sample_rate == 22050u ||
                          fmt.sample_rate == 44100u ||
                          fmt.sample_rate == 48000u);
    if (!rate_ok) {
        out.push_back({ Severity::ERROR, "INVALID_SAMPLE_RATE", "project",
                        "sample_rate は 22050, 44100 または 48000 に対応しています" });
    }

    const bool float_with_16bit = (fmt.sample_format == SampleFormat::FLOAT && fmt.bit_depth != 32);
    if (float_with_16bit) {
        out.push_back({ Severity::ERROR, "INVALID_FORMAT_COMBINATION", "project",
                        "sample_format: float には、bit_depth: 32 が必要です" });
    }

	if (fmt.bit_depth != 16 && fmt.bit_depth != 32) {
        out.push_back({ Severity::ERROR, "INVALID_BIT_DEPTH", "project",
                        "bit_depth は 16 または 32 が必要です" });
    }

    if (fmt.channels != 1 && fmt.channels != 2) { ///< Support for Channel 1 will be added in the future. Reason: There is no demand for it.
        out.push_back({ Severity::ERROR, "INVALID_CHANNELS", "project",
                        "channels は 2 に対応しています" });
    }
}

void ValidateCategories(const ProjectModel& project, std::vector<Issue>& out) {
    if (project.categories.empty()) {
        out.push_back({ Severity::ERROR, "NO_CATEGORIES", "project",
                        "カテゴリが定義されていません" });
        return;
    }

    std::set<uint16_t> seen_ids;
    for (const auto& cate : project.categories) {
        if (!seen_ids.insert(cate.id).second) {
            out.push_back({ Severity::ERROR, "DUPLICATE_CATEGORY_ID",
                            "category id " + std::to_string(cate.id),
                            "カテゴリIDが複数回定義されています" });
        }
    }
}

void ValidateCollections(const ProjectModel& project,
                         const std::filesystem::path& project_dir,
                         std::vector<Issue>& out) {
    std::set<uint16_t> category_ids;
    for (const auto& cate : project.categories) {
        category_ids.insert(cate.id);
    }

    for (const auto& collection : project.cue_collections) {
        if (collection.name.empty()) {
            out.push_back({ Severity::ERROR, "EMPTY_COLLECTION_NAME", "project",
                            "キューコレクションの名前が空です" });
        }

        if (collection.wccb_uuid.IsNil()) {
            out.push_back({ Severity::ERROR, "NIL_COLLECTION_UUID", collection.name,
                            "キューコレクションのUUIDが無効になっています" });
        }

        std::set<std::string> seen_names;
        for (const auto& cue : collection.cues) {
            const std::string loc = collection.name + "/" + cue.cue_name;

            if (cue.cue_name.empty()) {
                out.push_back({ Severity::ERROR, "EMPTY_CUE_NAME", collection.name,
                                "キューの名前が空欄です" });
            } else if (!seen_names.insert(cue.cue_name).second) {
                out.push_back({ Severity::ERROR, "DUPLICATE_CUE_NAME", loc,
                                "コレクション内に重複するキュー名があります" });
            }

            if (category_ids.find(cue.category_id) == category_ids.end()) {
                out.push_back({ Severity::ERROR, "INVALID_CATEGORY_ID", loc,
                                "キュー参照のカテゴリID: " +
                                std::to_string(cue.category_id) +
                                " は定義されていません" });
            }

            if (cue.waveforms.empty()) {
                out.push_back({ Severity::WARNING, "CUE_HAS_NO_WAVEFORM", loc,
                                "キューに登録された波形がありません" });
            } else if (cue.waveforms.size() > MAX_WAVEFORMS_PER_CUE) {
                out.push_back({ Severity::ERROR, "TOO_MANY_WAVEFORMS", loc,
                                "キューに対する波形の最大登録数: " +
                                std::to_string(MAX_WAVEFORMS_PER_CUE) + " を超えています。" });
            }

            if (cue.base_volume < 0.0f || cue.base_volume > 1.0f) {
                out.push_back({ Severity::WARNING, "VOLUME_OUT_OF_RANGE", loc,
                                "base_volume を [0.0, 1.0] の範囲に収めてください" });
            }

            if (cue.volume_random.enabled && cue.volume_random.min > cue.volume_random.max) {
                out.push_back({ Severity::ERROR, "INVALID_RANDOM_RANGE", loc,
                                "volume_random の 最小値 が 最大値 より大きい値になっています" });
            }

        	if (cue.pitch_random.enabled && cue.pitch_random.min > cue.pitch_random.max) {
                out.push_back({ Severity::ERROR, "INVALID_RANDOM_RANGE", loc,
                                "pitch_random の 最小値 が 最大値 より大きい値になっています" });
            }

            for (const auto& wf : cue.waveforms) {
                const auto resolved = ResolveSourcePath(project_dir, wf.file_path);
                std::error_code err;
                if (!std::filesystem::exists(resolved, err)) {
                    out.push_back({ Severity::ERROR, "FILE_NOT_FOUND", loc,
                                    "参照されたファイルが見つかりません: " + resolved.string() });
                }
            }
        }
    }
}

}  // namespace

std::vector<Issue> ProjectValidator::Validate(const ProjectModel& project,
                                              const std::filesystem::path& project_dir) {
    std::vector<Issue> issues;

    if (project.project_uuid.IsNil()) {
        issues.push_back({ Severity::ERROR, "NIL_PROJECT_UUID", "project",
                           "プロジェクトのUUIDが無効になっています" });
    }
    if (project.project_name.empty()) {
        issues.push_back({ Severity::WARNING, "EMPTY_PROJECT_NAME", "project",
                           "プロジェクトの名前が設定されていません" });
    }

    ValidateAudioFormat(project, issues);
    ValidateCategories(project, issues);
    ValidateCollections(project, project_dir, issues);

    return issues;
}

}
