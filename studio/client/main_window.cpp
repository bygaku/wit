#include "main_window.h"

#include <QWidget>
#include <QSplitter>
#include <QVBoxLayout>
#include <QLabel>
#include <QPalette>

namespace wit::studio {
constexpr auto	WINDOW_WIDTH  		= 1080;
constexpr auto	WINDOW_HEIGHT 		=  720;
constexpr auto	LEFT_WIDTH_RATIO	= 2;
constexpr auto	RIGHT_WIDTH_RATIO 	= 5;
constexpr auto	TOP_HEIGHT_RATIO	= 1;
constexpr auto	BOTTOM_HEIGHT_RATIO = 3;

MainWindow::MainWindow() {
    resize(WINDOW_WIDTH, WINDOW_HEIGHT);

	BuildBodyLayout();
}

/* =====================================================================
 * UI construction
 * ===================================================================== */
void MainWindow::BuildMenus() {

}

void MainWindow::BuildBodyLayout() {
	auto* vertical_split	= new QSplitter(Qt::Vertical, this);
	auto* horizontal_split	= new QSplitter(Qt::Horizontal, vertical_split);

	auto palette = QPalette();
	// Upper left side
	auto tree_widget = new QWidget(this);
	{

		tree_widget->setAutoFillBackground(true);
		palette = tree_widget->palette();
		palette.setColor(QPalette::Window, QColor("#ff0000"));
		tree_widget->setPalette(palette);

		horizontal_split->addWidget(tree_widget);
	}

	// Upper right side
	auto inspector_widget = new QWidget(this);
	{

		inspector_widget->setAutoFillBackground(true);
		palette = inspector_widget->palette();
		palette.setColor(QPalette::Window, QColor("#00ff00"));
		inspector_widget->setPalette(palette);

		horizontal_split->addWidget(inspector_widget);

		horizontal_split->setStretchFactor(0, 2);
		horizontal_split->setStretchFactor(1, 5);
	}

	// Body bottom
	auto bottom_widget = new QWidget(this);
	{

		bottom_widget->setAutoFillBackground(true);
		palette = bottom_widget->palette();
		palette.setColor(QPalette::Window, QColor("#0000ff"));
		bottom_widget->setPalette(palette);
	}

	// Make skeleton
	{
		vertical_split->addWidget(horizontal_split);
		vertical_split->addWidget(bottom_widget);
		vertical_split->setStretchFactor(0, 3);
		vertical_split->setStretchFactor(1, 1);
	}

	auto width  = WINDOW_WIDTH  / (LEFT_WIDTH_RATIO + RIGHT_WIDTH_RATIO);
	auto height = WINDOW_HEIGHT / (TOP_HEIGHT_RATIO + BOTTOM_HEIGHT_RATIO);

	vertical_split->setSizes({height * TOP_HEIGHT_RATIO, height * BOTTOM_HEIGHT_RATIO});
	horizontal_split->setSizes({width * LEFT_WIDTH_RATIO, width * RIGHT_WIDTH_RATIO});

	// Register as the central widget in the main window
	setCentralWidget(vertical_split);
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
