#include <QApplication>

#include "main_window.h"

int main(int argc, char* argv[]) {
	QApplication app(argc, argv);
	QApplication::setApplicationName("WitStudio");

	wit::studio::MainWindow window;
	window.show();

	return QApplication::exec();
}
