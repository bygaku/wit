#ifndef WIT_PROJECT_SERIALIZER_H
#define WIT_PROJECT_SERIALIZER_H

#include <filesystem>
#include <string>

#include "build_artifacts.h"

namespace wit::studio {
/**
 * @class ProjectSerializer
 * @brief .wsp (JSON) load / save for ProjectModel.
 */
class ProjectSerializer {
public:
	/**
	 * @brief Load a .wsp file into a ProjectModel.
	 * @param path      Path to the .wsp file.
	 * @param out       Receives the parsed project on success.
	 * @param out_error Receives a message on failure.
	 * @retval true  Success.
	 * @retval false Read or parse failed; out_error is filled.
	 */
	static bool Load(const std::filesystem::path& path, ProjectModel& out, std::string& out_error);

	/**
	 * @brief Save a ProjectModel as a .wsp file.
	 * @param path      Destination path.
	 * @param project   The project to serialize.
	 * @param out_error Receives a message on failure.
	 * @retval true  Success.
	 * @retval false Write failed; out_error is filled.
	 */
	static bool SaveToFile(const std::filesystem::path& path, const ProjectModel& project, std::string& out_error);

	/**
	 * @brief Create a new project with a fresh UUID and the five preset
	 * categories. This is the "ファイル > 新しいプロジェクト" entry point.
	 */
	static ProjectModel MakeNewProject(const std::string& project_name);

	/**
	 * @brief Create a new cue collection with a fresh UUID.
	 */
	static CueCollectionModel MakeNewCollection(const std::string& name);
};

}

#endif // WIT_PROJECT_SERIALIZER_H
