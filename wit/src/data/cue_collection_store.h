/**
 * Created by intwi on 2026/07/06.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_CUE_COLLECTION_STORE_H
#define WIT_CUE_COLLECTION_STORE_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>

#include <wit_types.h>

#include "cue_collection.h"
#include "util/handle.h"
#include "waveform_provider_store.h"

namespace wit {

class StreamingReader;
class ProjectData;

/**
 * @class CueCollectionStore
 * @brief Owns loaded cue collections (data + their waveform providers) and issues opaque WitWccbHn handles for them.
 */
class CueCollectionStore {
public:
    CueCollectionStore()  = default;
    ~CueCollectionStore() = default;

#pragma region
    CueCollectionStore(const CueCollectionStore&)            = delete;
    CueCollectionStore& operator=(const CueCollectionStore&) = delete;
    CueCollectionStore(CueCollectionStore&&)                 = delete;
    CueCollectionStore& operator=(CueCollectionStore&&)      = delete;
#pragma endregion

    /**
     * @brief Parse a .wccb buffer, build providers from its paired .wwb file,
     * and register the pair under a fresh handle.
     * @param out_status Receives the specific WitResult (SUCCESS on success, else the failing step's code).
     */
    WitWccbHn Create(const uint8_t*		wccb_data,  size_t		wccb_size,
                     const char*		wwb_path,
                     const ProjectData& project,	StreamingReader& streaming_reader,
                     WitResult&			out_status);

    /**
     * @brief Destroy the collection bound to the given handle.
     * @note cannot signal an error
     */
    void Destroy(WitWccbHn handle) noexcept;

    /**
     * @brief Look up the collection data bound to a handle.
	 * @retval nullptr WIT_INVALID_WCCB_HN or any never-issued, already-destroyed handle.
     */
    [[nodiscard]] const CueCollection* Data(WitWccbHn handle) const noexcept;

    /**
     * @brief Resolve the provider binding for one waveform of a cue.
     * @retval nullptr Unknown handle, foreign cue, or index out of range.
     */
    [[nodiscard]] const WaveformBinding* Binding(WitWccbHn handle, const CueData& cue,
                                                 uint16_t waveform_index) const noexcept;

    /**
     * @brief Check how many collections are loaded.
     */
    [[nodiscard]] size_t Count() const noexcept { return entries_.size(); }

private:
    /**
     * @struct Entry
     * @brief One loaded .wccb/.wwb pair: parse result + its runtime providers.
     */
    struct Entry {
        CueCollection         cc_data;
        WaveformProviderStore providers;
    };

    HandleAllocator<WitWccbHn>							  allocator_;
    std::unordered_map<WitWccbHn, std::unique_ptr<Entry>> entries_;
};

}

#endif // WIT_CUE_COLLECTION_STORE_H
