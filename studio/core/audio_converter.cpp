#include "audio_converter.h"

#include <miniaudio.h>

namespace wit::studio {

bool AudioConverter::Convert(const std::filesystem::path& file_path,
							 const AudioFormat& target_format,
                             ConvertedWaveform& out,
                             std::string& out_error) {
    out.samples.clear();
    out.frame_count = 0;
    out_error.clear();

    const auto config = ma_decoder_config_init(
        ma_format_f32,
        target_format.channels,
        target_format.sample_rate);

    ma_decoder decoder;
    auto res = ma_decoder_init_file(file_path.string().c_str(), &config, &decoder);
    if (res != MA_SUCCESS) {
        out_error = "ファイルを開けなかった, またはデコードに失敗した: " + file_path.string();
        return false;
    }

    constexpr ma_uint64 READ_CHUNK_FRAMES = 16384;
    const	  uint32_t	channels		  = target_format.channels;
    std::vector<float> chunk(static_cast<size_t>(READ_CHUNK_FRAMES) * channels);

    // Read until the decoder is exhausted.
    for (;;) {
        ma_uint64 frames_read = 0;
        res = ma_decoder_read_pcm_frames(&decoder, chunk.data(), READ_CHUNK_FRAMES, &frames_read);

    	if (frames_read > 0) {
            const size_t sample_count = frames_read * channels;
            out.samples.insert(out.samples.end(),
                               chunk.begin(), chunk.begin() + sample_count);
            out.frame_count += frames_read;
        }

    	if (res == MA_AT_END) break;

        if (res != MA_SUCCESS) {
            ma_decoder_uninit(&decoder);
            out_error = "読み取り中のデコードエラー： " + file_path.string();
            return false;
        }

    	if (frames_read == 0) break;
    }

    ma_decoder_uninit(&decoder);

    if (out.frame_count == 0) {
        out_error = "波形情報が見つからなかった: " + file_path.string();
        return false;
    }

    return true;
}

}
