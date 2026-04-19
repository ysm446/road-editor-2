#include "MainWindow.h"
#include "BuildConfig.h"
#include "PropertiesPanel.h"
#include "OutlinerPanel.h"
#include "../viewport/Viewport3D.h"
#include "../editor/EditorState.h"
#include "../model/Serializer.h"
#include <QDockWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QToolBar>
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
    setupToolBar();

    // Wire viewport signals to panels
    connect(m_viewport, &Viewport3D::selectionChanged,
            m_properties, &PropertiesPanel::onSelectionChanged);
    connect(m_viewport, &Viewport3D::selectionStateChanged,
            m_properties, &PropertiesPanel::onSelectionStateChanged);
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

    // Wire properties Apply → write back road values and rebuild mesh
    connect(m_properties, &PropertiesPanel::roadModified,
            m_viewport,   &Viewport3D::applyRoadProperties);
    connect(m_properties, &PropertiesPanel::selectedVerticalCurveModified,
            m_viewport,   &Viewport3D::applySelectedVerticalCurveProperties);
    connect(m_properties, &PropertiesPanel::removeSelectedVerticalCurveRequested,
            m_viewport,   &Viewport3D::removeSelectedVerticalCurve);
    connect(m_properties, &PropertiesPanel::selectedBankAngleModified,
            m_viewport,   &Viewport3D::applySelectedBankAngleProperties);
    connect(m_properties, &PropertiesPanel::removeSelectedBankAngleRequested,
            m_viewport,   &Viewport3D::removeSelectedBankAngle);
    connect(m_properties, &PropertiesPanel::selectedLaneSectionModified,
            m_viewport,   &Viewport3D::applySelectedLaneSectionProperties);
    connect(m_properties, &PropertiesPanel::removeSelectedLaneSectionRequested,
            m_viewport,   &Viewport3D::removeSelectedLaneSection);
    connect(m_properties, &PropertiesPanel::selectedSocketModified,
            m_viewport,   &Viewport3D::applySelectedSocketProperties);
    connect(m_properties, &PropertiesPanel::addSocketRequested,
            m_viewport,   &Viewport3D::addSocketToSelectedIntersection);
    connect(m_properties, &PropertiesPanel::removeSocketRequested,
            m_viewport,   &Viewport3D::removeSelectedSocket);

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

void MainWindow::setupToolBar() {
    auto* tb = addToolBar("Tools");
    tb->setMovable(false);
    tb->setIconSize({20, 20});

    auto* group = new QActionGroup(this);
    group->setExclusive(true);

    m_selectModeAct = tb->addAction("Select");
    m_selectModeAct->setCheckable(true);
    m_selectModeAct->setToolTip("Select roads  [S]");
    m_selectModeAct->setShortcut(Qt::Key_S);
    group->addAction(m_selectModeAct);

    m_editModeAct = tb->addAction("Edit");
    m_editModeAct->setCheckable(true);
    m_editModeAct->setChecked(true);
    m_editModeAct->setToolTip("Edit control points  [E]");
    m_editModeAct->setShortcut(Qt::Key_E);
    group->addAction(m_editModeAct);

    m_verticalModeAct = tb->addAction("Vertical");
    m_verticalModeAct->setCheckable(true);
    m_verticalModeAct->setToolTip("Edit vertical curve points  [V]");
    m_verticalModeAct->setShortcut(Qt::Key_V);
    group->addAction(m_verticalModeAct);

    m_bankModeAct = tb->addAction("Bank");
    m_bankModeAct->setCheckable(true);
    m_bankModeAct->setToolTip("Edit bank angle points  [B]");
    m_bankModeAct->setShortcut(Qt::Key_B);
    group->addAction(m_bankModeAct);

    m_laneModeAct = tb->addAction("Lane");
    m_laneModeAct->setCheckable(true);
    m_laneModeAct->setToolTip("Edit lane section points  [L]");
    m_laneModeAct->setShortcut(Qt::Key_L);
    group->addAction(m_laneModeAct);

    tb->addSeparator();
    m_createRoadAct = tb->addAction("Create Road");
    m_createRoadAct->setToolTip("Create a new road in Edit mode  [R]");
    m_createRoadAct->setShortcut(Qt::Key_R);

    m_createIntersectionAct = tb->addAction("Create Intersection");
    m_createIntersectionAct->setToolTip("Create a new intersection in Edit mode  [I]");
    m_createIntersectionAct->setShortcut(Qt::Key_I);

    connect(m_selectModeAct, &QAction::triggered, this, [this]{
        m_viewport->setToolMode(ToolMode::Select);
    });
    connect(m_editModeAct, &QAction::triggered, this, [this]{
        m_viewport->setToolMode(ToolMode::Edit);
        m_viewport->setEditSubTool(EditSubTool::None);
    });
    connect(m_verticalModeAct, &QAction::triggered, this, [this]{
        m_viewport->setToolMode(ToolMode::VerticalCurve);
        m_viewport->setEditSubTool(EditSubTool::None);
    });
    connect(m_bankModeAct, &QAction::triggered, this, [this]{
        m_viewport->setToolMode(ToolMode::BankAngle);
        m_viewport->setEditSubTool(EditSubTool::None);
    });
    connect(m_laneModeAct, &QAction::triggered, this, [this]{
        m_viewport->setToolMode(ToolMode::LaneSection);
        m_viewport->setEditSubTool(EditSubTool::None);
    });
    connect(m_createRoadAct, &QAction::triggered, this, [this]{
        m_editModeAct->setChecked(true);
        m_viewport->setToolMode(ToolMode::Edit);
        m_viewport->setEditSubTool(EditSubTool::CreateRoad);
    });
    connect(m_createIntersectionAct, &QAction::triggered, this, [this]{
        m_editModeAct->setChecked(true);
        m_viewport->setToolMode(ToolMode::Edit);
        m_viewport->setEditSubTool(EditSubTool::CreateIntersection);
    });

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

    auto* view = menuBar()->addMenu("&View");
    m_wireframeAct = view->addAction("Wireframe");
    m_wireframeAct->setCheckable(true);
    m_wireframeAct->setToolTip("Toggle wireframe display  [W]");
    m_wireframeAct->setShortcut(Qt::Key_W);
    connect(m_wireframeAct, &QAction::toggled, m_viewport, &Viewport3D::setWireframe);
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
