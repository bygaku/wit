/**
 * Created by intwi on 2026/07/07.
 * Copyright (c) 2026 All rights reserved.
 */
#include "waveform_provider_store.h"

#include <fstream>
#include <vector>

#include "binary/binary_reader.h"
#include "binary/fourcc.h"
#include "cue_collection.h"
#include "memory_waveform_provider.h"
#include "streaming_waveform_provider.h"

namespace wit {

namespace {
constexpr uint16_t RUNTIME_FORMAT_VERSION_MAJOR = 1;	///< Major version of the binary format supported by the runtime
}  // namespace

WitResult WaveformProviderStore::ValidateWwbFile(const char* wwb_path, const CueCollection& collection, uint64_t& out_file_size) {
	std::ifstream file(wwb_path, std::ios::binary | std::ios::ate);
	if (!file.is_open()) return WIT_RESULT_FILE_NOT_FOUND;

	const auto file_end = file.tellg();
	if (file_end < 0) return WIT_RESULT_INVALID_FORMAT;
	out_file_size = static_cast<uint64_t>(file_end);
	file.seekg(0, std::ios::beg);

	uint8_t riff_header[12] = {};		///< "RIFF" + total_size + "WWB "
	file.read(reinterpret_cast<char*>(riff_header), sizeof(riff_header));
	if (file.gcount() != static_cast<std::streamsize>(sizeof(riff_header)))	return WIT_RESULT_INVALID_FORMAT;
	{
		binary::BinaryReader r(riff_header, sizeof(riff_header));
		uint32_t riff_id    = 0;
		uint32_t total_size = 0;
		uint32_t form_type  = 0;
		if (!r.ReadU32(riff_id) || !r.ReadU32(total_size) || !r.ReadU32(form_type)) return WIT_RESULT_INVALID_FORMAT;
		if (riff_id   != binary::FOURCC_RIFF)	  return WIT_RESULT_INVALID_FORMAT;
		if (form_type != binary::FOURCC_FORM_WWB) return WIT_RESULT_INVALID_FORMAT;
	}

	// Walk the chunk list, reading only headers and the HEAD body.
	bool         has_head = false;
	bool         has_data = false;
	binary::Uuid wwb_head_uuid = {};

	uint64_t offset = 12;
	while (offset + 8 <= out_file_size) {
		uint8_t chunk_header[8] = {};
		file.seekg(static_cast<std::streamoff>(offset), std::ios::beg);

		file.read(reinterpret_cast<char*>(chunk_header), sizeof(chunk_header));
		if (file.gcount() != static_cast<std::streamsize>(sizeof(chunk_header))) return WIT_RESULT_INVALID_FORMAT;

		uint32_t chunk_id   = 0;
		uint32_t chunk_size = 0;
		{
			binary::BinaryReader r(chunk_header, sizeof(chunk_header));
			if (!r.ReadU32(chunk_id) || !r.ReadU32(chunk_size)) return WIT_RESULT_INVALID_FORMAT;
		}
		if (offset + 8 + chunk_size > out_file_size) return WIT_RESULT_INVALID_FORMAT;

		switch (chunk_id) {
			case binary::FOURCC_HEAD: {
				if (has_head)		 return WIT_RESULT_INVALID_FORMAT;
				if (chunk_size < 20) return WIT_RESULT_INVALID_FORMAT;

				uint8_t head_body[20] = {};
				file.read(reinterpret_cast<char*>(head_body), sizeof(head_body));
				if (file.gcount() != static_cast<std::streamsize>(sizeof(head_body))) return WIT_RESULT_INVALID_FORMAT;

				binary::BinaryReader r(head_body, sizeof(head_body));
				uint16_t wwb_ver_major = 0;
				uint16_t wwb_ver_minor = 0;
				if (!r.ReadU16(wwb_ver_major)) 					return WIT_RESULT_INVALID_FORMAT;
				if (!r.ReadU16(wwb_ver_minor)) 					return WIT_RESULT_INVALID_FORMAT;
				if (wwb_ver_major != RUNTIME_FORMAT_VERSION_MAJOR)	return WIT_RESULT_VERSION_MISMATCH;
				if (!r.ReadUuid(wwb_head_uuid))					return WIT_RESULT_INVALID_FORMAT;

				has_head = true;
			}
			case binary::FOURCC_DATA: {
				if (has_data) return WIT_RESULT_INVALID_FORMAT;

				has_data = true;
			}
			default: break;
		}

		uint64_t advance = chunk_size;
		if (advance & 1) ++advance;	///< Chunk boundaries are even-byte aligned

		offset += 8 + advance;
	}

	if (!has_head || !has_data)					return WIT_RESULT_INVALID_FORMAT;
	if (wwb_head_uuid != collection.WwbUuid())	return WIT_RESULT_WWB_MISMATCH;		///< Cross-file integrity

	return WIT_RESULT_SUCCESS;
}

WitResult WaveformProviderStore::CreateProviders(const CueCollection& collection,	 const char*		wwb_path,
                                                 uint64_t			  wwb_file_size, const AudioFormat& fmt,
                                                 StreamingReader&	  streaming_reader) {
	std::ifstream file(wwb_path, std::ios::binary);
	if (!file.is_open()) return WIT_RESULT_FILE_NOT_FOUND;

	base_index_.reserve(collection.Cues().size());

	for (const CueData& cue : collection.Cues()) {
		base_index_.push_back(bindings_.size());
		counts_.push_back(cue.waveform_count);

		for (uint16_t wi = 0; wi < cue.waveform_count; ++wi) {
			const WaveformInfo& wf = cue.waveforms[wi];

			if (wf.wwb_offset > wwb_file_size)					return WIT_RESULT_INVALID_FORMAT;
			if (wf.wwb_size > wwb_file_size)					return WIT_RESULT_INVALID_FORMAT;
			if (wf.wwb_offset + wf.wwb_size > wwb_file_size)	return WIT_RESULT_INVALID_FORMAT;

			if (cue.streaming_mode == StreamingMode::STREAMING) {
				auto provider = std::make_unique<StreamingWaveformProvider>(
					std::string(wwb_path),
					wf.wwb_offset,
					wf.wwb_size,
					wf.sample_count,
					fmt.sample_format,
					fmt.bit_depth,
					fmt.channels,
					&streaming_reader);

				bindings_.push_back(WaveformBinding{provider.get(), provider.get()});
				providers_.push_back(std::move(provider));
			} else {
				std::vector<uint8_t> pcm(wf.wwb_size);
				file.clear();
				file.seekg(static_cast<std::streamoff>(wf.wwb_offset), std::ios::beg);
				file.read(reinterpret_cast<char*>(pcm.data()), static_cast<std::streamsize>(pcm.size()));
				if (file.gcount() != static_cast<std::streamsize>(pcm.size())) return WIT_RESULT_INVALID_FORMAT;

				auto provider = std::make_unique<MemoryWaveformProvider>(
					std::move(pcm),
					wf.sample_count,
					fmt.sample_format,
					fmt.bit_depth,
					fmt.channels);

				bindings_.push_back(WaveformBinding{provider.get(), nullptr});
				providers_.push_back(std::move(provider));
			}
		}
	}

	return WIT_RESULT_SUCCESS;
}

WitResult WaveformProviderStore::Load(const CueCollection& collection, const char*		wwb_path,
                                      const AudioFormat&   format,	   StreamingReader& streaming_reader) {
	if (wwb_path == nullptr) return WIT_RESULT_FILE_NOT_FOUND;

	uint64_t wwb_file_size = 0;
	auto res = ValidateWwbFile(wwb_path, collection, wwb_file_size);
	if (res != WIT_RESULT_SUCCESS) return res;

	return CreateProviders(collection, wwb_path, wwb_file_size, format, streaming_reader);
}

const WaveformBinding* WaveformProviderStore::Find(size_t cue_index, uint16_t waveform_index) const noexcept {
	if (cue_index >= base_index_.size())      return nullptr;
	if (waveform_index >= counts_[cue_index]) return nullptr;
	return &bindings_[base_index_[cue_index] + waveform_index];
}

}
