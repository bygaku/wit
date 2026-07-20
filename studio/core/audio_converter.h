#ifndef WIT_AUDIO_CONVERTER_H
#define WIT_AUDIO_CONVERTER_H

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "data/audio_format.h"

namespace wit::studio {

/**
 * @struct ConvertedWaveform
 * @brief Result of decoding and converting an audio file to the unified project format.
 */
struct ConvertedWaveform {
	std::vector<float> samples;         ///< Interleaved (frame-major) float samples
	uint64_t           frame_count = 0; ///< Per-channel sample count
};

/**
 * @class AudioConverter
 * @brief Offline decoding and format conversion via miniaudio.
 */
class AudioConverter {
public:
	/**
	 * @brief Decode an audio file and convert it to the unified format.
	 * @param file_path      Path to the source audio file.
	 * @param target_format Unified project format (sample_rate / channels are used here).
	 * @param out           Receives the converted samples on success.
	 * @param out_error     Receives a human-readable message on failure.
	 * @retval true  Success.
	 * @retval false Decode or conversion failed; Error details are recorded in `out_error`.
	 */
	static bool Convert(const std::filesystem::path& file_path,
						const AudioFormat& target_format,
						ConvertedWaveform& out,
						std::string& out_error);
};

}

#endif // WIT_AUDIO_CONVERTER_H
