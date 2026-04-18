#include "MainWindow.h"
#include "BuildConfig.h"
#include "PropertiesPanel.h"
#include "OutlinerPanel.h"
#include "../viewport/Viewport3D.h"
#include "../model/Serializer.h"
#include <QDockWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QFile>
#include <QTimer>
#include <QTextEdit>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Road Editor 2");
    resize(1600, 900);

    m_viewport = new Viewport3D(this);
    setCentralWidget(m_viewport);

    setupDocks();
    setupMenuBar();

    // Wire viewport signals to panels
    connect(m_viewport, &Viewport3D::selectionChanged,
            m_properties, &PropertiesPanel::onSelectionChanged);
    connect(m_viewport, &Viewport3D::selectionChanged,
            m_outliner,  &OutlinerPanel::onSelectionChanged);
    connect(m_viewport, &Viewport3D::networkChanged,
            this,        &MainWindow::onNetworkChanged);

    // Wire outliner click → viewport selection highlight
    connect(m_outliner, &OutlinerPanel::roadSelected,
            this, [this](int roadIdx) {
                m_properties->onSelectionChanged(roadIdx);
                m_outliner->onSelectionChanged(roadIdx);
            });

    // Wire properties Apply → rebuild mesh with new values
    connect(m_properties, &PropertiesPanel::roadModified,
            this, [this](int roadIdx) {
                // Copy edited values back into the network
                auto& road = const_cast<RoadNetwork&>(m_viewport->network()).roads[roadIdx];
                // PropertiesPanel signals only when Apply is pressed; we push undo before rebuild.
                // For now just rebuild — future: expose network write-back via Viewport3D slot.
                (void)road;
            });

    // Auto-load sample data
    QString sample = QStringLiteral(ROAD_EDITOR_SOURCE_DIR) + "/docs/road_data_format.json";
    if (QFile::exists(sample))
        QTimer::singleShot(0, this, [this, sample]{
            m_currentPath = sample;
            m_viewport->loadNetwork(sample);
        });
}

void MainWindow::setupDocks() {
    setDockOptions(AllowTabbedDocks | AllowNestedDocks);

    // Outliner (left)
    auto* outlinerDock = new QDockWidget("Outliner", this);
    m_outliner = new OutlinerPanel(outlinerDock);
    outlinerDock->setWidget(m_outliner);
    outlinerDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::LeftDockWidgetArea, outlinerDock);

    // Properties (right)
    auto* propDock = new QDockWidget("Properties", this);
    m_properties = new PropertiesPanel(propDock);
    propDock->setWidget(m_properties);
    propDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::RightDockWidgetArea, propDock);

    // Log (bottom)
    auto* logDock = new QDockWidget("Log", this);
    logDock->setWidget(new QTextEdit(logDock));
    logDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::BottomDockWidgetArea, logDock);
    logDock->setMaximumHeight(120);
}

void MainWindow::setupMenuBar() {
    auto* file = menuBar()->addMenu("&File");

    auto* openAct = file->addAction("&Open...");
    openAct->setShortcut(QKeySequence::Open);
    connect(openAct, &QAction::triggered, this, &MainWindow::openFile);

    auto* saveAct = file->addAction("&Save");
    saveAct->setShortcut(QKeySequence::Save);
    connect(saveAct, &QAction::triggered, this, &MainWindow::saveFile);

    auto* saveAsAct = file->addAction("Save &As...");
    saveAsAct->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAct, &QAction::triggered, this, [this]{ m_currentPath.clear(); saveFile(); });

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
    if (!path.isEmpty()) {
        m_currentPath = path;
        m_viewport->loadNetwork(path);
    }
}

void MainWindow::saveFile() {
    if (m_currentPath.isEmpty()) {
        m_currentPath = QFileDialog::getSaveFileName(
            this, "Save Road Network", QString(),
            "Road Network (*.json);;All Files (*)");
    }
    if (!m_currentPath.isEmpty())
        Serializer::saveToFile(m_currentPath, m_viewport->network());
}

void MainWindow::onNetworkChanged() {
    m_properties->setNetwork(&m_viewport->network());
    m_outliner->refresh(&m_viewport->network());
}
