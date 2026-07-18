#ifndef WIT_MAIN_WINDOW_H
#define WIT_MAIN_WINDOW_H

#include <QMainWindow>
#include <filesystem>

#include "build_artifacts.h"

namespace wit::studio {

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
    void AddCue();
    void DeleteSelectedItem();
	void Reset() {};
    void OpenCategoryEditor();
	void OpenWaveformViewer() {};	///< This method not be implemented in v1.0.0.

    /* =====================================================================
     * Build
     * ===================================================================== */
    void RunBuild();

	/* =====================================================================
	 * Other actions
	 * ===================================================================== */
	bool Undo() { return false; };	///< This method not be implemented in v1.0.0
	bool Redo() { return false; };	///< This method not be implemented in v1.0.0
};

}

#endif // WIT_MAIN_WINDOW_H
