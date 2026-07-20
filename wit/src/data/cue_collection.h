/**
 * Created by intwi on 2026/07/06.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_CUE_COLLECTION_H
#define WIT_CUE_COLLECTION_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <wit.h>

#include "binary/uuid.h"
#include "cue_data.h"

namespace wit {

class ProjectData;

constexpr std::size_t MAX_COLLECTION_NAME_LENGTH = 32;	///< Maximum length of name (31 chars + NUL)

/**
 * @class CueCollection
 * @brief Parse result of one .wccb file. Immutable once loaded.
 */
class CueCollection {
public:
    CueCollection() = default;

    /**
     * @brief Parse the .wccb byte buffer with cross-checks against the project.
	 *
	 * @retval WIT_RESULT_SUCCESS           on success.
	 * @retval WIT_RESULT_INVALID_FORMAT    malformed binary or required chunk missing.
	 * @retval WIT_RESULT_VERSION_MISMATCH  format_version_major does not match runtime.
	 * @retval WIT_RESULT_PROJECT_MISMATCH  project_uuid does not match project.
	 * @retval WIT_RESULT_UNKNOWN_CATEGORY  CUSE references a category not in project.
     */
    WitResult Load(const uint8_t* wccb_data, size_t wccb_size, const ProjectData& project);

	/* =====================================================================
	 * Read-only accessors
	 * ===================================================================== */
    [[nodiscard]] uint16_t                     VersionMajor()    const noexcept { return version_major_; }
    [[nodiscard]] uint16_t                     VersionMinor()    const noexcept { return version_minor_; }
    [[nodiscard]] const binary::Uuid&          ProjectUuid()     const noexcept { return project_uuid_; }
    [[nodiscard]] const binary::Uuid&          WwbUuid()         const noexcept { return wwb_uuid_; }
    [[nodiscard]] const char*                  CollectionName()  const noexcept { return collection_name_.data(); }
    [[nodiscard]] const std::vector<uint16_t>& UsedCategoryIds() const noexcept { return used_category_ids_; }
    [[nodiscard]] const std::vector<CueData>&  Cues()            const noexcept { return cues_; }

	/* =====================================================================
	 * Data queries
	 * ===================================================================== */
	/**
	 * @brief Find a cue by name.
	 * @retval nullptr If the name is not present in this collection.
	 */
    [[nodiscard]] const CueData* FindCue(const char* cue_name) const noexcept;

	/**
	 * @brief Index of `cue` inside Cues(), or SIZE_MAX if it is not from this collection.
	 */
	[[nodiscard]] size_t CueIndexOf(const CueData& cue) const noexcept;

private:
    WitResult ParseHeadChunk(const uint8_t* data, uint32_t size);
    WitResult ParseUsecChunk(const uint8_t* data, uint32_t size, const ProjectData& project);
    WitResult ParseCuesChunk(const uint8_t* data, uint32_t size);

	/* =====================================================================
	 * Identify (HEAD chunk)
	 * ===================================================================== */
    uint16_t     version_major_ = 0;
    uint16_t     version_minor_ = 0;
    binary::Uuid project_uuid_  = {};
    binary::Uuid wwb_uuid_      = {};
    std::array<char, MAX_COLLECTION_NAME_LENGTH> collection_name_ = {0};

	/* =====================================================================
	 * Contents (USEC / CUES chunks)
	 * ===================================================================== */
    std::vector<uint16_t> used_category_ids_;
    std::vector<CueData>  cues_;
};

}

#endif // WIT_CUE_COLLECTION_H
