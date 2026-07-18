/**
 * Created by intwi on 2026/07/06.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_WAVEFORM_PROVIDER_STORE_H
#define WIT_WAVEFORM_PROVIDER_STORE_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <wit.h>

#include "audio_format.h"
#include "waveform_provider.h"

namespace wit {

class StreamingReader;
class StreamingWaveformProvider;
class CueCollection;

/**
 * @struct WaveformBinding
 * @brief Runtime binding of one waveform entry to its sample source.
 */
struct WaveformBinding {
    IWaveformProvider*         provider  = nullptr;
    StreamingWaveformProvider* streaming = nullptr;
};

/**
 * @class WaveformProviderStore
 * @brief Owns every waveform provider of one cue collection.
 */
class WaveformProviderStore {
public:
    WaveformProviderStore()  = default;
    ~WaveformProviderStore() = default;

#pragma region
    WaveformProviderStore(const WaveformProviderStore&)            = delete;
    WaveformProviderStore& operator=(const WaveformProviderStore&) = delete;
    WaveformProviderStore(WaveformProviderStore&&)                 = default;
    WaveformProviderStore& operator=(WaveformProviderStore&&)      = default;
#pragma endregion

    /**
     * @brief Build all providers for `collection` from its .wwb file.
     *
     * @retval WIT_RESULT_SUCCESS         on success.
     * @retval WIT_RESULT_FILE_NOT_FOUND  the .wwb file could not be opened.
     * @retval WIT_RESULT_INVALID_FORMAT  malformed .wwb or a waveform range out of bounds.
     * @retval WIT_RESULT_VERSION_MISMATCH .wwb format_version_major does not match runtime.
     * @retval WIT_RESULT_WWB_MISMATCH    .wwb uuid does not match collection.wwb_uuid.
     */
    WitResult Load(const CueCollection& collection, const char*		 wwb_path,
                   const AudioFormat&	format,		StreamingReader& streaming_reader);

    /**
     * @brief Retrieves the required WaveformProvider.
     * @retval nullptr If either index is out of range.
     */
    [[nodiscard]] const WaveformBinding* Find(size_t cue_index, uint16_t waveform_index) const noexcept;

    /**
     * @brief The number of providers under management.
     */
    [[nodiscard]] size_t Count() const noexcept { return providers_.size(); }

private:
    WitResult ValidateWwbFile(const char* wwb_path, const CueCollection& collection, uint64_t& out_file_size);
    WitResult CreateProviders(const CueCollection& collection,	  const char*		 wwb_path,
                              uint64_t			   wwb_file_size, const AudioFormat& format,
                              StreamingReader&	   streaming_reader);

    std::vector<std::unique_ptr<IWaveformProvider>> providers_;		///< Owned provider instances
    std::vector<WaveformBinding>                    bindings_;		///< Flat, in cue order
    std::vector<size_t>                             base_index_;	///< bindings_ offset of each cue
    std::vector<uint16_t>                           counts_;		///< Waveform count of each cue
};

}

#endif // WIT_WAVEFORM_PROVIDER_STORE_H
