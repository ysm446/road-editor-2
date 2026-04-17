#include "MainWindow.h"
#include "../viewport/Viewport3D.h"
#include <QDockWidget>
#include <QLabel>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QFile>
#include <QTimer>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Road Editor 2");
    resize(1600, 900);

    m_viewport = new Viewport3D(this);
    setCentralWidget(m_viewport);

    setupDocks();
    setupMenuBar();

    // Auto-load sample data on first run
    QString sample = QString(SOURCE_DIR) + "/docs/road_data_format.json";
    if (QFile::exists(sample))
        QTimer::singleShot(0, this, [this, sample]{ m_viewport->loadNetwork(sample); });
}

void MainWindow::setupDocks() {
    setDockOptions(AllowTabbedDocks | AllowNestedDocks);

    auto mkDock = [this](const QString& title, Qt::DockWidgetArea area) {
        auto* dock = new QDockWidget(title, this);
        dock->setWidget(new QLabel(QString("(%1)").arg(title.toLower()), dock));
        dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
        addDockWidget(area, dock);
        return dock;
    };

    mkDock("Tools",      Qt::LeftDockWidgetArea);
    mkDock("Properties", Qt::RightDockWidgetArea);
    mkDock("Log",        Qt::BottomDockWidgetArea);
}

void MainWindow::setupMenuBar() {
    auto* file = menuBar()->addMenu("&File");
    auto* openAct = file->addAction("&Open...");
    openAct->setShortcut(QKeySequence::Open);
    connect(openAct, &QAction::triggered, this, &MainWindow::openFile);
    file->addAction("&Save"   )->setShortcut(QKeySequence::Save);
    file->addSeparator();
    auto* exitAct = file->addAction("E&xit");
    exitAct->setShortcut(QKeySequence::Quit);
    connect(exitAct, &QAction::triggered, qApp, &QApplication::quit);

    menuBar()->addMenu("&Edit");
    menuBar()->addMenu("&View");
}

void MainWindow::openFile() {
    QString path = QFileDialog::getOpenFileName(
        this, "Open Road Network", QString(),
        "Road Network (*.json);;All Files (*)");
    if (!path.isEmpty())
        m_viewport->loadNetwork(path);
}
