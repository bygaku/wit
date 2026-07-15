/**
 * Created by intwi on 2026/06/28.
 * Copyright (c) 2026 All rights reserved.
 */

#include "riff_chunk_iterator.h"

#include "binary_reader.h"
#include "fourcc.h"

namespace wit::binary {

namespace {
constexpr std::size_t RIFF_HEADER_SIZE  = 12;	///< Total size of the RIFF header
constexpr std::size_t CHUNK_HEADER_SIZE = 8;	///< Size of Individual Chunk Headers
} // namespace

bool RiffChunkIterator::Open(const uint8_t* data,				std::size_t			size,
                             uint32_t       expected_form_type,	RiffChunkIterator&  out) noexcept {
    if (data == nullptr || size < RIFF_HEADER_SIZE) return false;

    BinaryReader reader(data, size);
    uint32_t riff_id;
    uint32_t total_size;
    uint32_t form_type;
    if (!reader.ReadFourCc(riff_id))    	return false;
    if (!reader.ReadU32(total_size))    	return false;
    if (!reader.ReadFourCc(form_type))  	return false;

    if (riff_id != FOURCC_RIFF)				return false;	///< Its not the RIFF file format
    if (form_type != expected_form_type)	return false;

    if (static_cast<uint64_t>(total_size) + 8 > size) return false;

    out.data_      = data;
    out.size_      = size;
    out.cursor_    = RIFF_HEADER_SIZE;
    out.form_type_ = form_type;
    out.has_error_ = false;
    return true;
}

bool RiffChunkIterator::Next(Chunk& out) noexcept {
    if (has_error_)		  return false;
    if (data_ == nullptr) return false;
    if (cursor_ >= size_) return false;

    if (size_ - cursor_ < CHUNK_HEADER_SIZE) {
        has_error_ = true;
        return false;
    }

    BinaryReader reader(data_, size_);
    if (!reader.SeekAbsolute(cursor_)) {
        has_error_ = true;
        return false;
    }

    uint32_t chunk_id;
    uint32_t chunk_size;
    if (!reader.ReadFourCc(chunk_id)) {
	    has_error_ = true;
    	return false;
    }
    if (!reader.ReadU32(chunk_size)) {
	    has_error_ = true;
    	return false;
    }

    const std::size_t data_offset = cursor_ + CHUNK_HEADER_SIZE;
    if (chunk_size > size_ || data_offset + chunk_size > size_) {
        has_error_ = true;
        return false;
    }

    out.chunk_id   = chunk_id;
    out.chunk_size = chunk_size;
    out.data       = data_ + data_offset;

    const std::size_t padding = (chunk_size & 1u) ? 1u : 0u;
    cursor_ = data_offset + chunk_size + padding;

    if (cursor_ > size_) cursor_ = size_;

    return true;
}

}