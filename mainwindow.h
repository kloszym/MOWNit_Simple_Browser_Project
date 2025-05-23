#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCheckBox>
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>


QT_BEGIN_NAMESPACE
class QSvgWidget;
QT_END_NAMESPACE

#include "searchenginecore.h"

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

private slots:
	void onSearchButtonClicked();
	void onResultItemClicked(QListWidgetItem *item);
	void onBackButtonClicked();

private:
	QStackedWidget *stackedWidget;
	QWidget *initialSearchPage;
	QWidget *resultsPage;

	QSvgWidget *logoWidget;
	QLineEdit *initialSearchQueryEdit;
	QPushButton *initialSearchButton;

	QLineEdit *headerSearchQueryEdit;
	QPushButton *headerSearchButton;
	QSvgWidget *headerTitleWidget;
	QPushButton *backButton;
	QCheckBox *useLsiCheckBox;
	QListWidget *resultsListWidget;

	SearchEngineCore engine;

	void setupUi();
	void applyStyles();
	void loadDataOnStartup();
};

#endif // MAINWINDOW_H
