#ifndef WIT_MAIN_WINDOW_H
#define WIT_MAIN_WINDOW_H

#include <QMainWindow>
#include <filesystem>

#include "build_artifacts.h"

class QTreeWidgetItem;
class QStackedWidget;

namespace wit::studio {

class ObjectTreeWidget;
class ProjectPropertiesEditorWidget;
class CuePropertiesEditorWidget;

/**
 * @class MainWindow
 * @brief WitStudio main window.
 *
 * Header: Menu Bar
 * Left: Project tree
 * Right: Inspector switched by the current tree selection.
 * Bottom: Waveform viewr. Non-scoped - v1.0.0
 * Footer: issue list produced by validation / build.
 */
class MainWindow : public QMainWindow {
public:
    MainWindow();

protected:
    /**
     * @brief Guards against losing unsaved change on window close.
	 */
    void closeEvent(QCloseEvent* event) override;

private:
    /* =====================================================================
     * UI construction
     * ===================================================================== */
    void BuildMenus();
    void BuildBodyLayout();

    /* =====================================================================
     * File actions
     * ===================================================================== */
    void NewProject();
    void OpenProject();
    bool SaveProject();
    bool SaveProjectAs();
    bool ConfirmDiscardChanges();

    /* =====================================================================
     * Edit actions
     * ===================================================================== */
    void AddCollection();
    void AddCue(QTreeWidgetItem* parent_collection = nullptr);
    void DeleteSelectedItem();
	void Reset() {};
    void OpenCategoryEditor();
	void OpenWaveformViewer() {};	///< This method not be implemented in v1.0.0.

	/**
	 * @brief Rename the collection bound to a tree item, keeping the model in sync.
	 */
	void RenameCollection(QTreeWidgetItem* item, const QString& new_name);

	/**
	 * @brief Produce a name not yet used among the given existing names.
	 * @param base   Prefix such as "新規Collection".
	 * @param taken  Names already in use.
	 */
	static std::string MakeUniqueName(const std::string& base, const std::vector<std::string>& taken);

	/* =====================================================================
	 * Inspector
	 * ===================================================================== */

	/**
	 * @brief Swap the right-hand inspector to match the current tree selection.
	 */
	void OnTreeSelectionChanged(int item_type, const QString& name);

    /* =====================================================================
     * Build
     * ===================================================================== */
    void RunBuild();

	/* =====================================================================
	 * Other actions
	 * ===================================================================== */
	bool Undo() { return false; };	///< This method not be implemented in v1.0.0
	bool Redo() { return false; };	///< This method not be implemented in v1.0.0

	/* =====================================================================
	 * Project lifecycle
	 * ===================================================================== */

	/**
	 * @brief Swap in a freshly loaded project and refresh every view.
	 * @param project The project that becomes the current one (moved in).
	 * @param path    Backing .wsp path, or empty for an unsaved new project.
	 */
	void ReplaceProject(ProjectModel&& project, const std::filesystem::path& path);

	/**
	 * @brief Repopulate the tree from the current project model.
	 */
	void RebuildTreeFromProject();

	/**
	 * @brief Flag the project as having unsaved changes and refresh the title.
	 */
	void MarkDirty();

	/**
	 * @brief Reflect the project name, backing path, and dirty state in the title bar.
	 */
	void UpdateWindowTitle();

private:
	ObjectTreeWidget* 				object_tree_				= nullptr;
	QStackedWidget*	  				inspector_view_				= nullptr;
	ProjectPropertiesEditorWidget*	project_properties_editor_	= nullptr;
	CuePropertiesEditorWidget*		cue_properties_editor_		= nullptr;

	ProjectModel			project_;
	std::filesystem::path	current_path_;			///< Backing .wsp path, empty if never saved
	bool					dirty_		= false;	///< Unsaved changes exist
};

}

#endif // WIT_MAIN_WINDOW_H
