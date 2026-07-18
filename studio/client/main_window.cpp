#include "main_window.h"

namespace wit::studio {

MainWindow::MainWindow() {
    resize(1080, 720);

}

/* =====================================================================
 * UI construction
 * ===================================================================== */
void MainWindow::BuildMenus() {

}

void MainWindow::BuildBodyLayout() {

}

/* =====================================================================
 * File actions
 * ===================================================================== */

void MainWindow::NewProject() {

}

void MainWindow::OpenProject() {

}

bool MainWindow::SaveProject() {

	return true;
}

bool MainWindow::SaveProjectAs() {
	return true;
}

bool MainWindow::ConfirmDiscardChanges() {

	return true;
}

void MainWindow::closeEvent(QCloseEvent* event) {

}

/* =====================================================================
 * Edit actions
 * ===================================================================== */

void MainWindow::AddCollection() {

}

void MainWindow::AddCue() {

}

void MainWindow::DeleteSelectedItem() {

}

void MainWindow::OpenCategoryEditor() {
}

/* =====================================================================
 * Build
 * ===================================================================== */

void MainWindow::RunBuild() {
}

}
