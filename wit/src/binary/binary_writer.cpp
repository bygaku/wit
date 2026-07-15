/**
 * Created by intwi on 2026/06/28.
 * Copyright (c) 2026 All rights reserved.
 */
#include "binary_writer.h"

#include <cstring>

#include "uuid.h"

namespace wit::binary {

void BinaryWriter::WriteU8(uint8_t v) {
    buffer_.push_back(v);
}

void BinaryWriter::WriteU16(uint16_t v) {
    buffer_.push_back(static_cast<uint8_t>( v        & 0xFF));
    buffer_.push_back(static_cast<uint8_t>((v >>  8) & 0xFF));
}

void BinaryWriter::WriteU32(uint32_t v) {
    buffer_.push_back(static_cast<uint8_t>( v        & 0xFF));
    buffer_.push_back(static_cast<uint8_t>((v >>  8) & 0xFF));
    buffer_.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    buffer_.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}

void BinaryWriter::WriteU64(uint64_t v) {
    buffer_.push_back(static_cast<uint8_t>( v        & 0xFF));
    buffer_.push_back(static_cast<uint8_t>((v >>  8) & 0xFF));
    buffer_.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    buffer_.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    buffer_.push_back(static_cast<uint8_t>((v >> 32) & 0xFF));
    buffer_.push_back(static_cast<uint8_t>((v >> 40) & 0xFF));
    buffer_.push_back(static_cast<uint8_t>((v >> 48) & 0xFF));
    buffer_.push_back(static_cast<uint8_t>((v >> 56) & 0xFF));
}

void BinaryWriter::WriteS8(int8_t v)   { WriteU8(static_cast<uint8_t>(v)); }
void BinaryWriter::WriteS16(int16_t v) { WriteU16(static_cast<uint16_t>(v)); }
void BinaryWriter::WriteS32(int32_t v) { WriteU32(static_cast<uint32_t>(v)); }
void BinaryWriter::WriteS64(int64_t v) { WriteU64(static_cast<uint64_t>(v)); }

void BinaryWriter::WriteF32(float v) {
    uint32_t bits;
    std::memcpy(&bits, &v, sizeof(bits));

    WriteU32(bits);
}

void BinaryWriter::WriteF64(double v) {
    uint64_t bits;
    std::memcpy(&bits, &v, sizeof(bits));
    WriteU64(bits);
}

void BinaryWriter::WriteBytes(const void* src, std::size_t size) {
    const auto* p = static_cast<const uint8_t*>(src);
    buffer_.insert(buffer_.end(), p, p + size);
}

void BinaryWriter::WriteFourCc(uint32_t v) {
    WriteU32(v);
}

void BinaryWriter::WriteUuid(const Uuid& v) {
    const auto& bytes = v.Bytes();
    buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
}

bool BinaryWriter::WriteString(std::string_view s) {
    if (s.size() > 0xFFFFu) return false;

    WriteU16(static_cast<uint16_t>(s.size()));
    WriteBytes(s.data(), s.size());
    return true;
}

std::size_t BinaryWriter::ReserveU32() {
    const std::size_t offset = buffer_.size();
    buffer_.resize(offset + 4, 0);
    return offset;
}

bool BinaryWriter::PatchU32(std::size_t offset, uint32_t v) {
    if (offset + 4 > buffer_.size()) return false;
    buffer_[offset + 0] = static_cast<uint8_t>( v        & 0xFF);
    buffer_[offset + 1] = static_cast<uint8_t>((v >>  8) & 0xFF);
    buffer_[offset + 2] = static_cast<uint8_t>((v >> 16) & 0xFF);
    buffer_[offset + 3] = static_cast<uint8_t>((v >> 24) & 0xFF);
    return true;
}

}