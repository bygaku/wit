/**
 * Created by intwi on 2026/06/28.
 * Copyright (c) 2026 All rights reserved.
 */
#include "binary_reader.h"

#include <cstring>

#include "uuid.h"

namespace wit::binary {

BinaryReader::BinaryReader(const uint8_t* data, std::size_t size) noexcept
    : data_(data), size_(size), position_(0) {}

bool BinaryReader::ReadU8(uint8_t& out) noexcept {
    if (Remaining() < 1) return false;
    out = data_[position_];
    position_ += 1;
    return true;
}

bool BinaryReader::ReadU16(uint16_t& out) noexcept {
    if (Remaining() < 2) return false;
    const uint8_t* p = data_ + position_;
    out = static_cast<uint16_t>(
              static_cast<uint16_t>(p[0])
            | (static_cast<uint16_t>(p[1]) << 8));
    position_ += 2;
    return true;
}

bool BinaryReader::ReadU32(uint32_t& out) noexcept {
    if (Remaining() < 4) return false;
    const uint8_t* p = data_ + position_;
    out =  static_cast<uint32_t>(p[0])
        | (static_cast<uint32_t>(p[1]) << 8)
        | (static_cast<uint32_t>(p[2]) << 16)
        | (static_cast<uint32_t>(p[3]) << 24);
    position_ += 4;
    return true;
}

bool BinaryReader::ReadU64(uint64_t& out) noexcept {
    if (Remaining() < 8) return false;
    const uint8_t* p = data_ + position_;
    out =  static_cast<uint64_t>(p[0])
        | (static_cast<uint64_t>(p[1]) << 8)
        | (static_cast<uint64_t>(p[2]) << 16)
        | (static_cast<uint64_t>(p[3]) << 24)
        | (static_cast<uint64_t>(p[4]) << 32)
        | (static_cast<uint64_t>(p[5]) << 40)
        | (static_cast<uint64_t>(p[6]) << 48)
        | (static_cast<uint64_t>(p[7]) << 56);
    position_ += 8;
    return true;
}

bool BinaryReader::ReadS8(int8_t& out) noexcept {
    uint8_t u;
    if (!ReadU8(u)) return false;
    out = static_cast<int8_t>(u);
    return true;
}

bool BinaryReader::ReadS16(int16_t& out) noexcept {
    uint16_t u;
    if (!ReadU16(u)) return false;
    out = static_cast<int16_t>(u);
    return true;
}

bool BinaryReader::ReadS32(int32_t& out) noexcept {
    uint32_t u;
    if (!ReadU32(u)) return false;
    out = static_cast<int32_t>(u);
    return true;
}

bool BinaryReader::ReadS64(int64_t& out) noexcept {
    uint64_t u;
    if (!ReadU64(u)) return false;
    out = static_cast<int64_t>(u);
    return true;
}

bool BinaryReader::ReadF32(float& out) noexcept {
    uint32_t bits;
    if (!ReadU32(bits)) return false;
    std::memcpy(&out, &bits, sizeof(out));
    return true;
}

bool BinaryReader::ReadF64(double& out) noexcept {
    uint64_t bits;
    if (!ReadU64(bits)) return false;
    std::memcpy(&out, &bits, sizeof(out));
    return true;
}

bool BinaryReader::ReadBytes(void* dst, std::size_t size) noexcept {
    if (Remaining() < size) return false;
    std::memcpy(dst, data_ + position_, size);
    position_ += size;
    return true;
}

bool BinaryReader::ReadFourCc(uint32_t& out) noexcept {
    return ReadU32(out);
}

bool BinaryReader::ReadUuid(Uuid& out) noexcept {
    if (Remaining() < 16) return false;
    out = Uuid::FromBytes(data_ + position_);
    position_ += 16;
    return true;
}

bool BinaryReader::ReadString(std::string& out) {
    uint16_t length;
    if (!ReadU16(length)) return false;
    if (Remaining() < length) return false;
    out.assign(reinterpret_cast<const char*>(data_ + position_), length);
    position_ += length;
    return true;
}

bool BinaryReader::Skip(std::size_t n) noexcept {
    if (Remaining() < n) return false;
    position_ += n;
    return true;
}

bool BinaryReader::SeekAbsolute(std::size_t position) noexcept {
    if (position > size_) return false;
    position_ = position;
    return true;
}

}