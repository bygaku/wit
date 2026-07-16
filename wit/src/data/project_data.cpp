/**
 * Created by intwi on 2026/07/04.
 * Copyright (c) 2026 All rights reserved.
 */
#include "project_data.h"

#include "binary/binary_reader.h"
#include "binary/fourcc.h"
#include "binary/riff_chunk_iterator.h"

namespace wit {

namespace {
constexpr uint16_t RUNTIME_FORMAT_VERSION_MAJOR = 1; ///< Major version of the binary format supported by the runtime
}  // namespace

WitResult ProjectData::Load(const uint8_t* data, size_t size) {
	auto res = WIT_RESULT_SUCCESS;

    binary::RiffChunkIterator itr;
    if (!binary::RiffChunkIterator::Open(data, size, binary::FOURCC_FORM_WPB, itr)) {
        return WIT_RESULT_INVALID_FORMAT;
    }

	// Track presence of the four required chunks. and duplicate prevention.
    bool has_head = false;
    bool has_fmt  = false;
    bool has_duck = false;
    bool has_cate = false;

    binary::RiffChunkIterator::Chunk chunk;
    while (itr.Next(chunk)) {
        switch (chunk.chunk_id) {
            case binary::FOURCC_HEAD: {
                if (has_head) return WIT_RESULT_INVALID_FORMAT;

            	res = ParseHeadChunk(chunk.data, chunk.chunk_size);
                if (res != WIT_RESULT_SUCCESS) return res;

                has_head = true;
                break;
            }
            case binary::FOURCC_FMT: {
                if (has_fmt) return WIT_RESULT_INVALID_FORMAT;

            	res = ParseFmtChunk(chunk.data, chunk.chunk_size);
                if (res != WIT_RESULT_SUCCESS) return res;

                has_fmt = true;
                break;
            }
            case binary::FOURCC_DUCK: {
                if (has_duck) return WIT_RESULT_INVALID_FORMAT;

            	res = ParseDuckChunk(chunk.data, chunk.chunk_size);
                if (res != WIT_RESULT_SUCCESS) return res;

                has_duck = true;
                break;
            }
            case binary::FOURCC_CATE: {
                if (has_cate) return WIT_RESULT_INVALID_FORMAT;

            	res = ParseCateChunk(chunk.data, chunk.chunk_size);
                if (res != WIT_RESULT_SUCCESS) return res;

                has_cate = true;
                break;
            }
            default:
        		// Skip unknown chunk.
                break;
        }
    }

    if (itr.HasError())									 return WIT_RESULT_INVALID_FORMAT;
    if (!has_head || !has_fmt || !has_duck || !has_cate) return WIT_RESULT_INVALID_FORMAT;

    return WIT_RESULT_SUCCESS;
}

WitResult ProjectData::ParseHeadChunk(const uint8_t* data, uint32_t size) {
    binary::BinaryReader r(data, size);

    if (!r.ReadU16(version_major_))    				return WIT_RESULT_INVALID_FORMAT;
    if (!r.ReadU16(version_minor_))    				return WIT_RESULT_INVALID_FORMAT;
    if (version_major_ != RUNTIME_FORMAT_VERSION_MAJOR) return WIT_RESULT_VERSION_MISMATCH;
    if (!r.ReadUuid(project_uuid_))    				return WIT_RESULT_INVALID_FORMAT;
    if (!r.ReadString(project_name_))  				return WIT_RESULT_INVALID_FORMAT;

    return WIT_RESULT_SUCCESS;
}

WitResult ProjectData::ParseFmtChunk(const uint8_t* data, uint32_t size) {
    binary::BinaryReader r(data, size);

    uint8_t sf_raw = 0;
    if (!r.ReadU32(format_.sample_rate))	return WIT_RESULT_INVALID_FORMAT;
    if (!r.ReadU16(format_.bit_depth))	return WIT_RESULT_INVALID_FORMAT;
    if (!r.ReadU8(sf_raw))				return WIT_RESULT_INVALID_FORMAT;
    if (!r.ReadU8(format_.channels))		return WIT_RESULT_INVALID_FORMAT;
    format_.sample_format = static_cast<SampleFormat>(sf_raw);

    return WIT_RESULT_SUCCESS;
}

WitResult ProjectData::ParseDuckChunk(const uint8_t* data, uint32_t size) {
    binary::BinaryReader r(data, size);

    if (!r.ReadU32(ducking_.fade_in_ms))  return WIT_RESULT_INVALID_FORMAT;
    if (!r.ReadU32(ducking_.fade_out_ms)) return WIT_RESULT_INVALID_FORMAT;

	return WIT_RESULT_SUCCESS;
}

WitResult ProjectData::ParseCateChunk(const uint8_t* data, uint32_t size) {
    binary::BinaryReader r(data, size);

    uint16_t category_count = 0;
    if (!r.ReadU16(category_count)) return WIT_RESULT_INVALID_FORMAT;

	// If there are no user-defined categories, category_count = 5.
    for (uint16_t i = 0; i < category_count; ++i) {
        CategoryInfo info;
        uint8_t is_preset_raw = 0;
        uint8_t is_ducker_raw = 0;

    	// The ID is in the range of 0–4. If user-defined categories are added, it may be 5 or higher.
        if (!r.ReadU16(info.id))                     return WIT_RESULT_INVALID_FORMAT;
        if (!r.ReadString(info.name))                return WIT_RESULT_INVALID_FORMAT;
        if (!r.ReadU8(is_preset_raw))                return WIT_RESULT_INVALID_FORMAT;
        if (!r.ReadF32(info.static_volume_db))       return WIT_RESULT_INVALID_FORMAT;
        if (!r.ReadF32(info.ducking_attenuation_db)) return WIT_RESULT_INVALID_FORMAT;
        if (!r.ReadU8(is_ducker_raw))                return WIT_RESULT_INVALID_FORMAT;

        info.is_preset = (is_preset_raw != 0);
        info.is_ducker = (is_ducker_raw != 0);

        if (!categories_.Add(info)) return WIT_RESULT_INVALID_FORMAT;
    }

    return WIT_RESULT_SUCCESS;
}

}
