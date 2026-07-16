/**
 * Created by intwi on 2026/07/03.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_PROJECT_DATA_H
#define WIT_PROJECT_DATA_H

#include <cstddef>
#include <cstdint>
#include <string>

#include <wit.h>

#include "audio_format.h"
#include "binary/uuid.h"
#include "category_register.h"
#include "ducking_settings.h"

namespace wit {

/**
 * @class ProjectData
 * @brief Parses and stores wpb files.
 */
class ProjectData {
public:
    ProjectData() = default;

    /**
     * @brief Parse the .wpb file from the buffer.
	 *
	 * @retval WIT_RESULT_SUCCESS           on success.
	 * @retval WIT_RESULT_INVALID_FORMAT    malformed binary or required chunk missing.
	 * @retval WIT_RESULT_VERSION_MISMATCH  format_version_major does not match runtime.
     */
    WitResult Load(const uint8_t* data, size_t size);

	/* =====================================================================
	 * Read-onlyyyy accessors
	 * ===================================================================== */
    [[nodiscard]] uint16_t                VersionMajor() const noexcept { return version_major_; }
    [[nodiscard]] uint16_t                VersionMinor() const noexcept { return version_minor_; }
    [[nodiscard]] const binary::Uuid&     ProjectUuid()  const noexcept { return project_uuid_; }
    [[nodiscard]] const std::string&      ProjectName()  const noexcept { return project_name_; }
    [[nodiscard]] const AudioFormat&      Format()       const noexcept { return format_; }
    [[nodiscard]] const DuckingSettings&  Ducking()      const noexcept { return ducking_; }
    [[nodiscard]] const CategoryRegister& Categories()   const noexcept { return categories_; }

private:
    WitResult ParseHeadChunk(const uint8_t* data, uint32_t size);
    WitResult ParseFmtChunk (const uint8_t* data, uint32_t size);
    WitResult ParseDuckChunk(const uint8_t* data, uint32_t size);
    WitResult ParseCateChunk(const uint8_t* data, uint32_t size);

	/* =====================================================================
	 * Identify
	 * ===================================================================== */
    uint16_t          version_major_ = 0;
    uint16_t          version_minor_ = 0;
    binary::Uuid      project_uuid_;
    std::string       project_name_;

	/* =====================================================================
	 * Parameters
	 * ===================================================================== */
    AudioFormat       format_;
    DuckingSettings   ducking_;
    CategoryRegister  categories_;
};

}

#endif // WIT_PROJECT_DATA_H