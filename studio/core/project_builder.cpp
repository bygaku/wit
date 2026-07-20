#include "project_builder.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <set>

#include "binary/binary_writer.h"
#include "binary/fourcc.h"
#include "project_validator.h"
#include "audio_converter.h"

namespace wit::studio {

namespace {

constexpr uint16_t FORMAT_VERSION_MAJOR = 1;
constexpr uint16_t FORMAT_VERSION_MINOR = 0;

/* =====================================================================
 * Chunk helpers
 * ===================================================================== */
/**
 * @brief Write a chunk header with a placeholder size; returns the size offset.
 */
std::size_t BeginChunk(binary::BinaryWriter& w, uint32_t fourcc) {
    w.WriteFourCc(fourcc);
    return w.ReserveU32();
}

/**
 * @brief Patch the chunk size and add the RIFF odd-size padding byte if needed.
 */
void EndChunk(binary::BinaryWriter& w, std::size_t size_offset) {
    const uint32_t chunk_size =
        static_cast<uint32_t>(w.Size() - (size_offset + 4));
    w.PatchU32(size_offset, chunk_size);
    if ((chunk_size & 1u) != 0u) {
        w.WriteU8(0);
    }
}

/**
 * @brief Begin the RIFF container; returns the total_size offset.
 */
std::size_t BeginRiff(binary::BinaryWriter& w, uint32_t form_type) {
    w.WriteFourCc(binary::FOURCC_RIFF);
    const std::size_t total_offset = w.ReserveU32();
    w.WriteFourCc(form_type);
    return total_offset;
}

/**
 * @brief Patch the RIFF total_size (all bytes following the total_size field).
 */
void EndRiff(binary::BinaryWriter& w, std::size_t total_offset) {
    w.PatchU32(total_offset, static_cast<uint32_t>(w.Size() - (total_offset + 4)));
}

/* =====================================================================
 * Sample processing
 * ===================================================================== */
/**
 * @brief Converts a decibel value to its linear scale equivalent.
 */
float DbToLinear(float db) {
    return std::pow(10.0f, db / 20.0f);
}

/**
 * @brief Apply gain in the float domain and return the resulting absolute peak.
 */
float ApplyGainAndMeasurePeak(std::vector<float>& samples, float linear_gain) {
    float peak = 0.0f;
    for (float& s : samples) {
        s *= linear_gain;
        peak = std::max(peak, std::abs(s));
    }
    return peak;
}

/**
 * @brief Append samples to the .wwb writer in the unified format.
 */
void WriteSamples(binary::BinaryWriter& w, const std::vector<float>& samples, const AudioFormat& fmt) {
    if (fmt.sample_format == SampleFormat::FLOAT) {
        for (float s : samples) {
            w.WriteF32(s);
        }
    } else {
        for (float s : samples) {
            const float clamped = std::clamp(s, -1.0f, 1.0f);
            w.WriteS16(static_cast<int16_t>(std::lround(clamped * 32767.0f)));
        }
    }
}

void BuildWpb(const ProjectModel& project, binary::BinaryWriter& w) {
    const std::size_t riff = BeginRiff(w, binary::FOURCC_FORM_WPB);

    // "HEAD"
    std::size_t chunk = BeginChunk(w, binary::FOURCC_HEAD);
    w.WriteU16(FORMAT_VERSION_MAJOR);
    w.WriteU16(FORMAT_VERSION_MINOR);
    w.WriteUuid(project.project_uuid);
    w.WriteString(project.project_name);
    EndChunk(w, chunk);

    // "FMT "
    chunk = BeginChunk(w, binary::FOURCC_FMT);
    w.WriteU32(project.audio_format.sample_rate);
    w.WriteU16(project.audio_format.bit_depth);
    w.WriteU8(static_cast<uint8_t>(project.audio_format.sample_format));
    w.WriteU8(project.audio_format.channels);
    EndChunk(w, chunk);

    // "DUCK"
    chunk = BeginChunk(w, binary::FOURCC_DUCK);
    w.WriteU32(project.ducking.fade_in_ms);
    w.WriteU32(project.ducking.fade_out_ms);
    EndChunk(w, chunk);

    // "CATE"
    chunk = BeginChunk(w, binary::FOURCC_CATE);
    w.WriteU16(static_cast<uint16_t>(project.categories.size()));
    for (const auto& cate : project.categories) {
        w.WriteU16(cate.id);
        w.WriteString(cate.name);
        w.WriteU8(cate.is_preset ? 1 : 0);
        w.WriteF32(cate.static_volume_db);
        w.WriteF32(cate.ducking_attenuation_db);
        w.WriteU8(cate.is_ducker ? 1 : 0);
    }
    EndChunk(w, chunk);

    EndRiff(w, riff);
}

/**
 * @brief Offsets of one converted waveform inside the .wwb being assembled.
 */
struct WrittenWaveform {
    uint64_t wwb_offset   = 0;
    uint64_t wwb_size     = 0;
    uint64_t sample_count = 0;
};

void BuildWccb(const ProjectModel& project,
               const CueCollectionModel& collection,
               const std::vector<std::vector<WrittenWaveform>>& written,
               binary::BinaryWriter& w) {
    const std::size_t riff = BeginRiff(w, binary::FOURCC_FORM_WCCB);

    // "HEAD"
    std::size_t chunk = BeginChunk(w, binary::FOURCC_HEAD);
    w.WriteU16(FORMAT_VERSION_MAJOR);
    w.WriteU16(FORMAT_VERSION_MINOR);
    w.WriteUuid(project.project_uuid);
    w.WriteUuid(collection.wccb_uuid);
    w.WriteString(collection.name);
    EndChunk(w, chunk);

    // "CUSE": unique category ids referenced by this collection's cues.
    std::set<uint16_t> used_categories;
    for (const auto& cue : collection.cues) {
        used_categories.insert(cue.category_id);
    }
    chunk = BeginChunk(w, binary::FOURCC_USEC);
    w.WriteU16(static_cast<uint16_t>(used_categories.size()));
	for (uint16_t id : used_categories) {
        w.WriteU16(id);
    }
    EndChunk(w, chunk);

    // "CUES"
    chunk = BeginChunk(w, binary::FOURCC_CUES);
    w.WriteU32(static_cast<uint32_t>(collection.cues.size()));
    for (std::size_t cue_index = 0; cue_index < collection.cues.size(); ++cue_index) {
        const CueModel& cue = collection.cues[cue_index];
        const std::vector<WrittenWaveform>& cue_waveforms = written[cue_index];

        w.WriteU32(cue.cue_id);
        w.WriteString(cue.cue_name);
        w.WriteU16(cue.category_id);
        w.WriteU8(static_cast<uint8_t>(cue.cue_type));
        w.WriteU8(cue.loop_enabled ? 1 : 0);
    	w.WriteU64(0);	///< loop_start_point:	0 = cue head
    	w.WriteU64(0);	///< loop_end_point:	0 = end of longest waveform
        w.WriteU8(static_cast<uint8_t>(cue.streaming_mode));
        w.WriteF32(cue.base_volume);
        w.WriteF32(cue.base_pitch);
        w.WriteU8(cue.volume_random.enabled ? 1 : 0);
        w.WriteF32(cue.volume_random.min);
        w.WriteF32(cue.volume_random.max);
        w.WriteU8(cue.pitch_random.enabled ? 1 : 0);
        w.WriteF32(cue.pitch_random.min);
        w.WriteF32(cue.pitch_random.max);
        w.WriteU8(0);   // is_3d: always 0 in v1.0

        const uint8_t reserved_3d[36] = {};
        w.WriteBytes(reserved_3d, sizeof(reserved_3d));

        w.WriteU16(static_cast<uint16_t>(cue.waveforms.size()));
        for (std::size_t wf_index = 0; wf_index < cue.waveforms.size(); ++wf_index) {
            const WrittenWaveform& info = cue_waveforms[wf_index];
            w.WriteU64(info.wwb_offset);
            w.WriteU64(info.wwb_size);
            w.WriteU64(info.sample_count);
            w.WriteString(cue.waveforms[wf_index].display_name);
        }
    }
    EndChunk(w, chunk);

    EndRiff(w, riff);
}

bool WriteFile(const std::filesystem::path& path,
               const binary::BinaryWriter& w,
               std::vector<Issue>& issues) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        issues.push_back({ Severity::ERROR, "FILE_WRITE_FAILED", path.filename().string(),
                           "書き込み用に開くことができませんでした: " + path.string() });
        return false;
    }

	file.write(reinterpret_cast<const char*>(w.Bytes().data()),
               static_cast<std::streamsize>(w.Size()));

    if (!file.good()) {
        issues.push_back({ Severity::ERROR, "FILE_WRITE_FAILED", path.filename().string(),
                           "書き込みエラーが発生しました: " + path.string() });
        return false;
    }

	return true;
}

std::filesystem::path ResolveSourcePath(const std::filesystem::path& project_dir, const std::string& file_path) {
    std::filesystem::path p(file_path);
    if (p.is_absolute()) return p;
    return project_dir / p;
}

float FindStaticVolumeDb(const ProjectModel& project, uint16_t category_id) {
    for (const auto& cate : project.categories) {
        if (cate.id == category_id) return cate.static_volume_db;
    }
    return 0.0f;    ///< Unreachable after validation
}

}  // namespace

BuildResult ProjectBuilder::Build(const ProjectModel& project,
                                  const std::filesystem::path& project_dir,
                                  const std::filesystem::path& output_dir,
                                  const ProgressCallback& on_progress) {
    BuildResult res;

	res.issues = ProjectValidator::Validate(project, project_dir);
    if (res.HasErrors()) {
        return res;
    }

    std::error_code err;
    std::filesystem::create_directories(output_dir, err);
    if (err) {
        res.issues.push_back({ Severity::ERROR, "DIRECTORY_CREATE_FAILED",
                                  output_dir.string(),
                                  "出力ディレクトリの作成に失敗しました" });
        return res;
    }

    BuildProgress prog;
    for (const auto& collection : project.cue_collections) {
        for (const auto& cue : collection.cues) {
            prog.total_waveforms += static_cast<uint32_t>(cue.waveforms.size());
        }
    }

    const AudioFormat& fmt = project.audio_format;
    const uint32_t bytes_per_sample = fmt.bit_depth / 8u;

    for (const auto& collection : project.cue_collections) {
        binary::BinaryWriter wwb;
        const std::size_t wwb_riff = BeginRiff(wwb, binary::FOURCC_FORM_WWB);

        std::size_t chunk = BeginChunk(wwb, binary::FOURCC_HEAD);
        wwb.WriteU16(FORMAT_VERSION_MAJOR);
        wwb.WriteU16(FORMAT_VERSION_MINOR);
        wwb.WriteUuid(collection.wccb_uuid);
        EndChunk(wwb, chunk);

        chunk = BeginChunk(wwb, binary::FOURCC_DATA);

        std::vector<std::vector<WrittenWaveform>> written(collection.cues.size());

        for (std::size_t cue_index = 0; cue_index < collection.cues.size(); ++cue_index) {
            const CueModel& cue		= collection.cues[cue_index];
            const float linear_gain = DbToLinear(FindStaticVolumeDb(project, cue.category_id));

            for (const auto& wf : cue.waveforms) {
                prog.current_item = collection.name + "/" + cue.cue_name;
                if (on_progress) on_progress(prog);

                const auto path = ResolveSourcePath(project_dir, wf.file_path);

                ConvertedWaveform converted;
                std::string       error;
                if (!AudioConverter::Convert(path, fmt, converted, error)) {
                    res.issues.push_back({ Severity::ERROR, "WAV_DECODE_FAILED",
                                              prog.current_item, error });
                    return res;
                }

                const float peak = ApplyGainAndMeasurePeak(converted.samples, linear_gain);
                if (peak > 1.0f) {
                    res.issues.push_back({ Severity::WARNING, "PEAK_CLIPPING",
                                              prog.current_item,
                                              "Peak: " + std::to_string(peak) +
                                              " ゲイン適用後に最大値を超えます"
                                              "※クリッピングが発生するため、ボリュームを下げてください" });
                }

                WrittenWaveform info;
                info.wwb_offset   = wwb.Size();
                info.sample_count = converted.frame_count;
                info.wwb_size     = converted.frame_count *
                                    static_cast<uint64_t>(fmt.channels) *
                                    bytes_per_sample;

                WriteSamples(wwb, converted.samples, fmt);
                written[cue_index].push_back(info);

                ++prog.processed_waveforms;
                if (on_progress) on_progress(prog);
            }
        }

        EndChunk(wwb, chunk);
        EndRiff(wwb, wwb_riff);

        binary::BinaryWriter wccb;
        BuildWccb(project, collection, written, wccb);

        const auto wwb_path  = output_dir / (collection.name + ".wwb");
        const auto wccb_path = output_dir / (collection.name + ".wccb");
        if (!WriteFile(wwb_path, wwb, res.issues))   return res;
        if (!WriteFile(wccb_path, wccb, res.issues)) return res;
    }

    binary::BinaryWriter wpb;
    BuildWpb(project, wpb);
    const auto wpb_path = output_dir / (project.project_name + ".wpb");
    if (!WriteFile(wpb_path, wpb, res.issues)) return res;

    res.succeeded = !res.HasErrors();
    return res;
}

}
