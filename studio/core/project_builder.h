#ifndef WIT_PROJECT_BUILDER_H
#define WIT_PROJECT_BUILDER_H

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

#include "build_artifacts.h"

namespace wit::studio {

/**
 * @struct BuildProgress
 */
struct BuildProgress {
	uint32_t    total_waveforms     = 0;
	uint32_t    processed_waveforms = 0;
	std::string current_item;				///< e.g. "bgm/stage1"
};

/**
 * @class ProjectBuilder
 * @brief Builds .wpb / .wccb / .wwb binaries from a ProjectModel.
 *
 * Any validation error aborts the build before touching the output directory.
 */
class ProjectBuilder {
public:
	using ProgressCallback = std::function<void(const BuildProgress&)>;

	/**
	 * @brief Validate and build the whole project.
	 * @param project     The project to build.
	 * @param project_dir Base directory used to resolve relative audio source paths.
	 * @param output_dir  Destination directory (created if missing), e.g. "wit_assets".
	 * @param on_progress Optional progress callback, invoked per waveform.
	 * @return BuildResult::succeeded is false if any ERROR issue occurred.
	 */
	static BuildResult Build(const ProjectModel& project,
							 const std::filesystem::path& project_dir,
							 const std::filesystem::path& output_dir,
							 const ProgressCallback& on_progress = nullptr);
};

}

#endif // WIT_PROJECT_BUILDER_H
