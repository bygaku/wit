#ifndef WIT_PROJECT_VALIDATOR_H
#define WIT_PROJECT_VALIDATOR_H

#include <filesystem>
#include <vector>

#include "build_artifacts.h"

namespace wit::studio {
/**
 * @class ProjectValidator
 * @brief Pre-build integrity checks on a ProjectModel.
 */
class ProjectValidator {
public:
	/**
	 * @brief Run all checks.
	 * @param project     The project to validate.
	 * @param project_dir Base directory used to resolve relative audio source paths
	 *                    (the directory containing the .wproj file).
	 * @return All issues found. Empty when the project is fully valid.
	 */
	static std::vector<Issue> Validate(const ProjectModel& project, const std::filesystem::path& project_dir);
};

}

#endif // WIT_PROJECT_VALIDATOR_H
