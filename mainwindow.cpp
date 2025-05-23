#include "mainwindow.h"
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QIcon>
#include <QFile>
#include <QStyle>
#include <QSvgWidget>
#include <QSvgRenderer>
#include <QPainter>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    qDebug() << ">>> MainWindow constructor: START";
    setupUi();
    applyStyles();
    loadDataOnStartup();
    stackedWidget->setCurrentWidget(initialSearchPage);
    qDebug() << ">>> MainWindow constructor: END";
}

MainWindow::~MainWindow() {
    qDebug() << ">>> MainWindow destructor";
}

void MainWindow::setupUi() {
    qDebug() << ">>> setupUi: START";
    setWindowTitle("Leyme Search");
    this->setMinimumSize(800, 600);

    stackedWidget = new QStackedWidget(this);
    this->setCentralWidget(stackedWidget);

    initialSearchPage = new QWidget();
    initialSearchPage->setObjectName("initialSearchPage");
    QVBoxLayout *initialPageLayout = new QVBoxLayout(initialSearchPage);
    initialPageLayout->setContentsMargins(50, 50, 50, 50);
    initialPageLayout->setAlignment(Qt::AlignCenter);

    logoWidget = new QSvgWidget(":/app_logo");
    logoWidget->setObjectName("logoWidget");

    logoWidget->setFixedSize(380, 100);

    initialSearchQueryEdit = new QLineEdit();
    initialSearchQueryEdit->setPlaceholderText("Wpisz zapytanie...");
    initialSearchQueryEdit->setObjectName("initialSearchEdit");

    initialSearchButton = new QPushButton();
    initialSearchButton->setObjectName("initialSearchButton");

    QString svgIconPath = ":/icon_search";
    QSvgRenderer svgRenderer(svgIconPath);

    if (svgRenderer.isValid()) {

        QSize iconSize(28, 28);

        QPixmap pixmap(iconSize);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        svgRenderer.render(&painter);

        QIcon searchIcon(pixmap);
        initialSearchButton->setIcon(searchIcon);
        initialSearchButton->setIconSize(iconSize);
    } else {
        qWarning() << "Nie udało się załadować ikony SVG:" << svgIconPath;
        initialSearchButton->setIcon(this->style()->standardIcon(QStyle::SP_FileDialogContentsView));
    }

    initialSearchButton->setToolTip("Szukaj");
    initialSearchButton->setFixedSize(40,40);

    QHBoxLayout *initialSearchLayout = new QHBoxLayout();
    initialSearchLayout->addStretch();
    initialSearchLayout->addWidget(initialSearchQueryEdit, 3);
    initialSearchLayout->addWidget(initialSearchButton, 0);
    initialSearchLayout->addStretch();

    initialPageLayout->addStretch(1);
    initialPageLayout->addWidget(logoWidget, 0, Qt::AlignCenter);
    initialPageLayout->addSpacing(30);
    initialPageLayout->addLayout(initialSearchLayout);
    initialPageLayout->addStretch(2);
    stackedWidget->addWidget(initialSearchPage);

    resultsPage = new QWidget();
    resultsPage->setObjectName("resultsPage");
    QVBoxLayout *resultsPageLayout = new QVBoxLayout(resultsPage);
    resultsPageLayout->setContentsMargins(0,0,0,0);
    resultsPageLayout->setSpacing(0);

    QWidget *headerWidget = new QWidget();
    headerWidget->setObjectName("resultsHeader");
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(8, 8, 8, 8);

    backButton = new QPushButton();
    backButton->setObjectName("backButton");

    QString svgIconPathForBack = ":/icon_home";
    QSvgRenderer headerSvgRendererBack(svgIconPathForBack);

    if (headerSvgRendererBack.isValid()) {

        QSize headerIconSize(20, 20);

        QPixmap headerPixmap(headerIconSize);
        headerPixmap.fill(Qt::transparent);

        QPainter headerPainter(&headerPixmap);
        headerSvgRendererBack.render(&headerPainter);

        QIcon searchIconForHeader(headerPixmap);
        backButton->setIcon(searchIconForHeader);
        backButton->setIconSize(headerIconSize);
    } else {
        qWarning() << "Nie udało się załadować ikony SVG dla headerSearchButton:" << svgIconPathForBack;
        headerSearchButton->setIcon(this->style()->standardIcon(QStyle::SP_FileDialogContentsView));
    }

    backButton->setFixedSize(30,30);
    backButton->setToolTip("Wróć");

    headerSearchQueryEdit = new QLineEdit();
    headerSearchQueryEdit->setPlaceholderText("Szukaj ponownie...");
    headerSearchQueryEdit->setObjectName("headerSearchEdit");

    headerSearchButton = new QPushButton();
    headerSearchButton->setObjectName("headerSearchButton");

    QString svgIconPathForHeader = ":/icon_search";
    QSvgRenderer headerSvgRenderer(svgIconPathForHeader);

    if (headerSvgRenderer.isValid()) {

        QSize headerIconSize(20, 20);

        QPixmap headerPixmap(headerIconSize);
        headerPixmap.fill(Qt::transparent);

        QPainter headerPainter(&headerPixmap);
        headerSvgRenderer.render(&headerPainter);

        QIcon searchIconForHeader(headerPixmap);
        headerSearchButton->setIcon(searchIconForHeader);
        headerSearchButton->setIconSize(headerIconSize);
    } else {
        qWarning() << "Nie udało się załadować ikony SVG dla headerSearchButton:" << svgIconPathForHeader;
        headerSearchButton->setIcon(this->style()->standardIcon(QStyle::SP_FileDialogContentsView));
    }

    headerSearchButton->setFixedSize(30,30);
    headerSearchButton->setToolTip("Szukaj");

    useLsiCheckBox = new QCheckBox("Użyj LSI/SVD");
    useLsiCheckBox->setObjectName("useLsiCheckBox");
    useLsiCheckBox->setChecked(true);
    useLsiCheckBox->setEnabled(false);

    headerTitleWidget = new QSvgWidget(":/app_logo");
    headerTitleWidget->setObjectName("logoWidget");

    headerTitleWidget->setFixedSize(200, 50);

    headerLayout->addWidget(backButton);
    headerLayout->addSpacing(5);
    headerLayout->addWidget(headerSearchQueryEdit, 1);
    headerLayout->addWidget(headerSearchButton);
    headerLayout->addSpacing(10);
    headerLayout->addWidget(useLsiCheckBox);
    headerLayout->addSpacing(15);
    headerLayout->addWidget(headerTitleWidget);
    headerLayout->addStretch();

    resultsListWidget = new QListWidget();
    resultsListWidget->setObjectName("resultsList");

    resultsPageLayout->addWidget(headerWidget);
    resultsPageLayout->addWidget(resultsListWidget, 1);
    stackedWidget->addWidget(resultsPage);

    connect(initialSearchButton, &QPushButton::clicked, this, &MainWindow::onSearchButtonClicked);
    connect(initialSearchQueryEdit, &QLineEdit::returnPressed, this, &MainWindow::onSearchButtonClicked);
    connect(headerSearchButton, &QPushButton::clicked, this, &MainWindow::onSearchButtonClicked);
    connect(headerSearchQueryEdit, &QLineEdit::returnPressed, this, &MainWindow::onSearchButtonClicked);
    connect(resultsListWidget, &QListWidget::itemClicked, this, &MainWindow::onResultItemClicked);
    connect(backButton, &QPushButton::clicked, this, &MainWindow::onBackButtonClicked);
    qDebug() << ">>> setupUi: END";
}

void MainWindow::applyStyles() {
    qDebug() << ">>> applyStyles: START";
    QString styles = R"(
        QMainWindow {
            background-color: #2c3e50;
        }

        #initialSearchPage {
            background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #34495e, stop:1 #2c3e50);
        }

        QLineEdit {
            padding: 10px;
            border: 1px solid #7f8c8d;
            border-radius: 8px;
            font-size: 16px;
            background-color: #ecf0f1;
            color: #2c3e50;
            selection-background-color: #3498db;
        }
        QLineEdit:focus {
            border: 2px solid #3498db;
            background-color: white;
        }

        #initialSearchEdit {
            min-height: 40px;
            font-size: 18px;
            max-width: 500px;
        }
        #headerSearchEdit {
            min-height: 28px;
            font-size: 14px;
        }

        #initialSearchButton {
            background-color: #3498db;
            border: none;
            border-radius: 8px;
            color: white;
        }
        #initialSearchButton:hover { background-color: #2980b9; }
        #initialSearchButton:pressed { background-color: #1f618d; }


        #headerSearchButton, #backButton {
            background-color: transparent;
            border: none;
            border-radius: 15px;
            padding: 0px;
            min-height: 30px;
            min-width: 30px;
            color: #ecf0f1;
        }
        #headerSearchButton:hover, #backButton:hover { background-color: rgba(236, 240, 241, 0.2); }
        #headerSearchButton:pressed, #backButton:pressed { background-color: rgba(236, 240, 241, 0.4); }

        #resultsHeader {
            background-color: #34495e;
            border-bottom: 2px solid #2c3e50;
            padding: 8px;
        }
        #headerTitle {
            font-family: 'Segoe UI', Arial, sans-serif;
            font-size: 20px;
            font-weight: bold;
            color: #ecf0f1;
            padding-left: 10px;
        }

        #useLsiCheckBox {
            color: #ecf0f1;
            font-size: 13px;
            font-family: 'Segoe UI', Arial, sans-serif;
            background-color: transparent;
            padding-left: 3px;
        }
        #useLsiCheckBox::indicator {
            width: 15px;
            height: 15px;
            border: 1px solid #ecf0f1;
            border-radius: 4px;
            background-color: rgba(255, 255, 255, 0.1);
        }
        #useLsiCheckBox::indicator:hover { border: 1px solid #3498db; }
        #useLsiCheckBox::indicator:checked {
            background-color: #3498db;
            border: 1px solid #2980b9;
        }
        #useLsiCheckBox::indicator:checked:hover { background-color: #2980b9; }
        #useLsiCheckBox::indicator:disabled {
            border: 1px solid #7f8c8d;
            background-color: rgba(127, 140, 141, 0.2);
        }
        #useLsiCheckBox:disabled { color: #7f8c8d; }


        #resultsList {
            border: none;
            font-size: 15px;
            background-color: #ecf0f1;
            color: #2c3e50;
        }
        #resultsList::item {
            padding: 12px 15px;
            border-bottom: 1px solid #bdc3c7;
        }
        #resultsList::item:hover { background-color: #d1e9fc; }
        #resultsList::item:selected {
            background-color: #3498db;
            color: white;
        }
        QScrollBar:vertical {
            border: 1px solid #4a6278;
            background: #34495e;
            width: 12px;
            margin: 0px 0px 0px 0px;
        }
        QScrollBar::handle:vertical {
            background: #7f8c8d;
            min-height: 25px;
            border-radius: 6px;
        }
        QScrollBar::handle:vertical:hover { background: #95a5a6; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            border: none;
            background: none;
            height: 0px;
            width: 0px;
        }
        QScrollBar::up-arrow:vertical, QScrollBar::down-arrow:vertical { background: none; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
    )";
    this->setStyleSheet(styles);
    qDebug() << ">>> applyStyles: END - FULL FANCY STYLES APPLIED";
}

void MainWindow::loadDataOnStartup() {
    qDebug() << ">>> loadDataOnStartup: START";
    QApplication::setOverrideCursor(Qt::WaitCursor);
    qDebug() << "MainWindow: Attempting to load search engine data...";
    std::string dataPath = ".";

    if (engine.loadData(dataPath)) {
        qDebug() << QString("MainWindow: Data loaded from '%1'. Vocab: %2, Docs: %3. SVD Ready: %4 (k=%5)")
                                 .arg(QString::fromStdString(dataPath))
                                 .arg(engine.getVocabularySize())
                                 .arg(engine.getDocumentCount())
                                 .arg(engine.isSvdDataLoaded() ? "Yes" : "No")
                                 .arg(engine.isSvdDataLoaded() ? engine.getSvdKDimension() : 0);
        initialSearchButton->setEnabled(true);
        initialSearchQueryEdit->setEnabled(true);
        headerSearchButton->setEnabled(true);
        headerSearchQueryEdit->setEnabled(true);

        if (engine.isSvdDataLoaded()) {
            useLsiCheckBox->setEnabled(true);
            useLsiCheckBox->setChecked(true);
            useLsiCheckBox->setToolTip("Przełącz użycie Latent Semantic Indexing (SVD) dla wyszukiwania");
        } else {
            useLsiCheckBox->setEnabled(false);
            useLsiCheckBox->setChecked(false);
            useLsiCheckBox->setToolTip("Dane LSI/SVD nie są dostępne");
        }
    } else {
        qDebug() << "MainWindow: Error loading data from" << QString::fromStdString(dataPath) << "! Check console output from SearchEngineCore.";
        QMessageBox::critical(this, "Błąd Krytyczny", "Nie udało się załadować danych wyszukiwarki. Sprawdź pliki danych i komunikaty w konsoli.\nAplikacja może nie działać poprawnie.");
        initialSearchButton->setEnabled(false);
        initialSearchQueryEdit->setEnabled(false);
        headerSearchButton->setEnabled(false);
        headerSearchQueryEdit->setEnabled(false);
        useLsiCheckBox->setEnabled(false);
    }
    QApplication::restoreOverrideCursor();
    qDebug() << ">>> loadDataOnStartup: END";
}

void MainWindow::onSearchButtonClicked() {
    qDebug() << ">>> onSearchButtonClicked: START";
    QString query;
    if (stackedWidget->currentWidget() == initialSearchPage) {
        query = initialSearchQueryEdit->text().trimmed();
        qDebug() << "Search from initial page, query:" << query;
    } else {
        query = headerSearchQueryEdit->text().trimmed();
        qDebug() << "Search from results page, query:" << query;
    }

    if (query.isEmpty()) {
        QMessageBox::information(this, "Puste Zapytanie", "Proszę wpisać treść zapytania.");
        return;
    }
    if (!engine.isDataLoaded()){
        QMessageBox::warning(this, "Błąd Danych", "Dane wyszukiwarki nie są załadowane. Nie można przeprowadzić wyszukiwania.");
        return;
    }

    qDebug() << "MainWindow: Searching for:" << query;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    resultsListWidget->clear();

    bool useLSI = useLsiCheckBox->isChecked() && useLsiCheckBox->isEnabled();
    int topK = 20;
    qDebug() << "Search parameters: useLSI =" << useLSI << ", topK =" << topK;

    if (stackedWidget->currentWidget() == initialSearchPage) {
        headerSearchQueryEdit->setText(query);
    }

    std::vector<SearchResult> searchResults = engine.performSearchQuery(query.toStdString(), useLSI, topK);

    if (searchResults.empty()) {
        QListWidgetItem *noResultsItem = new QListWidgetItem("Nie znaleziono wyników dla Twojego zapytania.");
        noResultsItem->setTextAlignment(Qt::AlignCenter);
        noResultsItem->setFlags(noResultsItem->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled);
        resultsListWidget->addItem(noResultsItem);
    } else {
        for (const auto& res : searchResults) {
            QString displayText = QString::fromStdString(res.display_title);
            QString similarityText = QString(" (Podobieństwo: %1)").arg(QString::number(res.similarity, 'f', 3));
            QListWidgetItem *item = new QListWidgetItem(displayText + similarityText);
            item->setData(Qt::UserRole, QString::fromStdString(res.url));
            item->setToolTip(QString::fromStdString(res.url));
            resultsListWidget->addItem(item);
        }
    }
    qDebug() << QString("MainWindow: Search complete. Found %1 results.").arg(static_cast<int>(searchResults.size()));
    QApplication::restoreOverrideCursor();
    stackedWidget->setCurrentWidget(resultsPage);
    qDebug() << ">>> onSearchButtonClicked: END";
}

void MainWindow::onResultItemClicked(QListWidgetItem *item) {
    qDebug() << ">>> onResultItemClicked: START";
    if (item && item->data(Qt::UserRole).isValid()) {
        QString urlString = item->data(Qt::UserRole).toString();
        if (!urlString.isEmpty()) {
            qDebug() << "Attempting to open URL:" << urlString;
            if (!QDesktopServices::openUrl(QUrl(urlString))) {
                QMessageBox::warning(this, "Błąd otwierania URL", "Nie można otworzyć adresu URL: " + urlString);
                qDebug() << "Failed to open URL:" << urlString;
            }
        } else {
             qDebug() << "Clicked item has no URL data (UserRole is empty).";
        }
    } else {
        qDebug() << "Clicked item is invalid or has no UserRole data.";
    }
    qDebug() << ">>> onResultItemClicked: END";
}

void MainWindow::onBackButtonClicked() {
    qDebug() << ">>> onBackButtonClicked: START";
    stackedWidget->setCurrentWidget(initialSearchPage);
    qDebug() << ">>> onBackButtonClicked: END";
}