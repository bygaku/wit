/**
 * Created by intwi on 2026/07/06.
 * Copyright (c) 2026 All rights reserved.
 */
#include "cue_collection.h"

#include "project_data.h"

#include <cstring>

#include "binary/binary_reader.h"
#include "binary/fourcc.h"
#include "binary/riff_chunk_iterator.h"

namespace wit {

namespace {
constexpr uint16_t RUNTIME_FORMAT_VERSION_MAJOR = 1;  ///< Major version of the binary format supported by the runtime

/**
 * @brief Read a length-prefixed wire string into a fixed NUL-terminated array.
 */
template <std::size_t N>
bool ReadFixedString(binary::BinaryReader& r, std::array<char, N>& out) {
	std::string tmp;

	if (!r.ReadString(tmp))   return false;
	if (tmp.size() >= N)		 return false;	///< Needs a space for the terminating NUL

	out.fill('\0');
	std::memcpy(out.data(), tmp.data(), tmp.size());

	return true;
}

/// HACK: Expansion field v1.x
WitResult ParseCueEntry(binary::BinaryReader& r, CueData& out) {
	if (!r.ReadU32(out.cue_id))					return WIT_RESULT_INVALID_FORMAT;
	if (!ReadFixedString(r, out.cue_name))	return WIT_RESULT_INVALID_FORMAT;
	if (!r.ReadU16(out.category_id))				return WIT_RESULT_INVALID_FORMAT;

	uint8_t ct_raw = 0;
	if (!r.ReadU8(ct_raw))								return WIT_RESULT_INVALID_FORMAT;
	if (ct_raw > static_cast<uint8_t>(CueType::SEQUENTIAL))	return WIT_RESULT_INVALID_FORMAT;
	out.cue_type = static_cast<CueType>(ct_raw);

	uint8_t loop_raw = 0;
	if (!r.ReadU8(loop_raw)) return WIT_RESULT_INVALID_FORMAT;
	out.loop_enabled = (loop_raw != 0);

	if (!r.ReadU64(out.loop_start_point)) return WIT_RESULT_INVALID_FORMAT;
	if (!r.ReadU64(out.loop_end_point))   return WIT_RESULT_INVALID_FORMAT;
	if (out.loop_end_point != 0 && out.loop_start_point >= out.loop_end_point) return WIT_RESULT_INVALID_FORMAT;

	uint8_t streaming_raw = 0;
	if (!r.ReadU8(streaming_raw))									return WIT_RESULT_INVALID_FORMAT;
	if (streaming_raw > static_cast<uint8_t>(StreamingMode::STREAMING)) return WIT_RESULT_INVALID_FORMAT;
	out.streaming_mode = static_cast<StreamingMode>(streaming_raw);

	if (!r.ReadF32(out.base_volume)) return WIT_RESULT_INVALID_FORMAT;
	if (!r.ReadF32(out.base_pitch))  return WIT_RESULT_INVALID_FORMAT;

	uint8_t vol_enabled_raw = 0;
	if (!r.ReadU8(vol_enabled_raw))          return WIT_RESULT_INVALID_FORMAT;
	if (!r.ReadF32(out.volume_random.min))   return WIT_RESULT_INVALID_FORMAT;
	if (!r.ReadF32(out.volume_random.max))   return WIT_RESULT_INVALID_FORMAT;
	out.volume_random.enabled = (vol_enabled_raw != 0);

	uint8_t pitch_enabled_raw = 0;
	if (!r.ReadU8(pitch_enabled_raw))        return WIT_RESULT_INVALID_FORMAT;
	if (!r.ReadF32(out.pitch_random.min))    return WIT_RESULT_INVALID_FORMAT;
	if (!r.ReadF32(out.pitch_random.max))    return WIT_RESULT_INVALID_FORMAT;
	out.pitch_random.enabled = (pitch_enabled_raw != 0);

	// 3D flag and reserved 36-byte block.
	uint8_t is_3d_raw = 0;
	if (!r.ReadU8(is_3d_raw)) return WIT_RESULT_INVALID_FORMAT;
	out.is_3d = (is_3d_raw != 0);
	if (!r.ReadBytes(out.reserved_3d.data(), out.reserved_3d.size())) return WIT_RESULT_INVALID_FORMAT;

	uint16_t wf_count = 0;
	if (!r.ReadU16(wf_count))			return WIT_RESULT_INVALID_FORMAT;
	if (wf_count > MAX_WAVEFORMS_PER_CUE)	return WIT_RESULT_INVALID_FORMAT;

	out.waveform_count = wf_count;
	for (uint16_t i = 0; i < wf_count; ++i) {
		WaveformInfo& wf = out.waveforms[i];
		if (!r.ReadU64(wf.wwb_offset))					return WIT_RESULT_INVALID_FORMAT;
		if (!r.ReadU64(wf.wwb_size))						return WIT_RESULT_INVALID_FORMAT;
		if (!r.ReadU64(wf.sample_count))					return WIT_RESULT_INVALID_FORMAT;
		if (!ReadFixedString(r, wf.waveform_name))	return WIT_RESULT_INVALID_FORMAT;
	}

	return WIT_RESULT_SUCCESS;
}

}  // namespace

WitResult CueCollection::ParseHeadChunk(const uint8_t* data, uint32_t size) {
	binary::BinaryReader r(data, size);

	if (!r.ReadU16(version_major_)) 					return WIT_RESULT_INVALID_FORMAT;
	if (!r.ReadU16(version_minor_)) 					return WIT_RESULT_INVALID_FORMAT;
	if (version_major_ != RUNTIME_FORMAT_VERSION_MAJOR) return WIT_RESULT_VERSION_MISMATCH;
	if (!r.ReadUuid(project_uuid_))      			return WIT_RESULT_INVALID_FORMAT;
	if (!r.ReadUuid(wwb_uuid_))          			return WIT_RESULT_INVALID_FORMAT;
	if (!ReadFixedString(r, collection_name_))	return WIT_RESULT_INVALID_FORMAT;

	return WIT_RESULT_SUCCESS;
}

WitResult CueCollection::ParseUsecChunk(const uint8_t* data, uint32_t size, const ProjectData& project) {
	binary::BinaryReader r(data, size);

	uint16_t used_count = 0;
	if (!r.ReadU16(used_count)) return WIT_RESULT_INVALID_FORMAT;

	used_category_ids_.reserve(used_count);
	for (uint16_t i = 0; i < used_count; ++i) {
		uint16_t cat_id = 0;

		if (!r.ReadU16(cat_id))					return WIT_RESULT_INVALID_FORMAT;
		if (!project.Categories().Contains(cat_id)) return WIT_RESULT_UNKNOWN_CATEGORY;

		used_category_ids_.push_back(cat_id);
	}

	return WIT_RESULT_SUCCESS;
}

WitResult CueCollection::ParseCuesChunk(const uint8_t* data, uint32_t size) {
	auto res = WIT_RESULT_SUCCESS;

	binary::BinaryReader r(data, size);
	uint32_t cue_count = 0;
	if (!r.ReadU32(cue_count)) return WIT_RESULT_INVALID_FORMAT;

	cues_.reserve(cue_count);
	for (uint32_t i = 0; i < cue_count; ++i) {
		CueData cue;

		res = ParseCueEntry(r, cue);
		if (res != WIT_RESULT_SUCCESS) return res;

		cues_.push_back(cue);
	}

	return WIT_RESULT_SUCCESS;
}


WitResult CueCollection::Load(const uint8_t* data, size_t size, const ProjectData& project) {
	auto res = WIT_RESULT_SUCCESS;

	binary::RiffChunkIterator itr;
	if (!binary::RiffChunkIterator::Open(data, size, binary::FOURCC_FORM_WCCB, itr)) {
		return WIT_RESULT_INVALID_FORMAT;
	}

	// Track presence of the three required chunks. and duplicate prevention.
	bool has_head = false;
	bool has_usec = false;
	bool has_cues = false;

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
			case binary::FOURCC_USEC: {
				if (has_usec) return WIT_RESULT_INVALID_FORMAT;

				res = ParseUsecChunk(chunk.data, chunk.chunk_size, project);
				if (res != WIT_RESULT_SUCCESS) return res;

				has_usec = true;
				break;
			}
			case binary::FOURCC_CUES: {
				if (has_cues) return WIT_RESULT_INVALID_FORMAT;

				res = ParseCuesChunk(chunk.data, chunk.chunk_size);
				if (res != WIT_RESULT_SUCCESS) return res;

				has_cues = true;
				break;
			}
			default:
				// Skip unknown chunk
				break;
		}
	}
	if (itr.HasError())							return WIT_RESULT_INVALID_FORMAT;
	if (!has_head || !has_usec || !has_cues)	return WIT_RESULT_INVALID_FORMAT;
	if (project_uuid_ != project.ProjectUuid()) return WIT_RESULT_PROJECT_MISMATCH;	///< Check cross-file integrity

	return WIT_RESULT_SUCCESS;
}

const CueData* CueCollection::FindCue(const char* cue_name) const noexcept {
	if (cue_name == nullptr) return nullptr;

	for (const auto& c : cues_) {
		if (std::strncmp(c.cue_name.data(), cue_name, MAX_CUE_NAME_LENGTH) == 0) return &c;
	}
	return nullptr;
}

size_t CueCollection::CueIndexOf(const CueData& cue) const noexcept {
	if (cues_.empty()) return SIZE_MAX;

	const CueData* base = cues_.data();
	if (&cue < base || &cue >= base + cues_.size()) return SIZE_MAX;

	return static_cast<size_t>(&cue - base);
}

}

