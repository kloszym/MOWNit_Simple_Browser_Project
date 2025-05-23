#include "mainwindow.h"
#include <QApplication>
#include <QDebug> // For qDebug

int main(int argc, char *argv[]) {
	qDebug() << ">>> main: Application START";
	QApplication a(argc, argv);
	qDebug() << ">>> main: QApplication initialized";

	MainWindow w;
	qDebug() << ">>> main: MainWindow object 'w' constructed";

	w.show();
	qDebug() << ">>> main: w.show() called";

	int result = a.exec();
	qDebug() << ">>> main: a.exec() returned with" << result;
	qDebug() << ">>> main: Application END";
	return result;
}