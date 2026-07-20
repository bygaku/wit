/**
 * Created by intwi on 2026/07/07.
 * Copyright (c) 2026 All rights reserved.
 */
#include "cue_collection_store.h"

#include "project_data.h"

namespace wit {

WitWccbHn CueCollectionStore::Create(const uint8_t* wccb_data, size_t wccb_size, const char* wwb_path,
                                     const ProjectData& project, StreamingReader& streaming_reader, WitResult& out_status) {
    if (wccb_data == nullptr || wwb_path == nullptr) {
        out_status = WIT_RESULT_INVALID_FORMAT;
        return WIT_INVALID_WCCB_HN;
    }

    auto entry = std::make_unique<Entry>();

    out_status = entry->cc_data.Load(wccb_data, wccb_size, project);
    if (out_status != WIT_RESULT_SUCCESS) return WIT_INVALID_WCCB_HN;

    out_status = entry->providers.Load(entry->cc_data, wwb_path, project.Format(), streaming_reader);
    if (out_status != WIT_RESULT_SUCCESS) return WIT_INVALID_WCCB_HN;

    const WitWccbHn handle = allocator_.Next();
    if (handle == WIT_INVALID_WCCB_HN) {
        out_status = WIT_RESULT_INIT_FAILED;
        return WIT_INVALID_WCCB_HN;
    }

    entries_.emplace(handle, std::move(entry));
    out_status = WIT_RESULT_SUCCESS;
    return handle;
}

void CueCollectionStore::Destroy(WitWccbHn handle) noexcept {
    entries_.erase(handle);
}

const CueCollection* CueCollectionStore::Data(WitWccbHn handle) const noexcept {
    const auto itr = entries_.find(handle);
    return itr != entries_.end() ? &itr->second->cc_data : nullptr;
}

const WaveformBinding* CueCollectionStore::Binding(WitWccbHn handle, const CueData& cue,
                                                   uint16_t waveform_index) const noexcept {
    const auto itr = entries_.find(handle);
    if (itr == entries_.end()) return nullptr;

    const size_t cue_index = itr->second->cc_data.CueIndexOf(cue);
    if (cue_index == SIZE_MAX) return nullptr;

    return itr->second->providers.Find(cue_index, waveform_index);
}

}
