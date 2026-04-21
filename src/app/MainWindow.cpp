#include "MainWindow.h"
#include "BuildConfig.h"
#include "HeightmapPanel.h"
#include "PropertiesPanel.h"
#include "OutlinerPanel.h"
#include "../viewport/Viewport3D.h"
#include "../editor/EditorState.h"
#include "../model/EnvironmentSerializer.h"
#include "../model/Serializer.h"
#include <QDockWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QToolBar>
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStatusBar>
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
    m_viewport->setToolMode(ToolMode::Select);
    loadRecentFiles();
    refreshRecentFilesMenu();

    // Wire viewport signals to panels
    connect(m_viewport, &Viewport3D::selectionChanged,
            m_properties, &PropertiesPanel::onSelectionChanged);
    connect(m_viewport, &Viewport3D::selectionStateChanged,
            m_properties, &PropertiesPanel::onSelectionStateChanged);
    connect(m_viewport, &Viewport3D::selectionChanged,
            m_outliner,  &OutlinerPanel::onSelectionChanged);
    connect(m_viewport, &Viewport3D::networkChanged,
            this,        &MainWindow::onNetworkChanged);
    connect(m_viewport, &Viewport3D::networkChanged,
            m_heightmap, [this] { m_heightmap->setNetwork(&m_viewport->network()); });

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
    connect(m_heightmap, &HeightmapPanel::importRequested,
            this, &MainWindow::importHeightmap);
    connect(m_heightmap, &HeightmapPanel::clearRequested,
            this, &MainWindow::clearHeightmap);
    connect(m_heightmap, &HeightmapPanel::importTextureRequested,
            this, &MainWindow::importTerrainTexture);
    connect(m_heightmap, &HeightmapPanel::clearTextureRequested,
            this, &MainWindow::clearTerrainTexture);
    connect(m_heightmap, &HeightmapPanel::snapWhileMovingChanged,
            m_viewport, &Viewport3D::setSnapToTerrainWhileMoving);
    connect(m_heightmap, &HeightmapPanel::settingsChanged,
            this, [this](const TerrainSettings& settings) {
                QString errorMessage;
                if (!m_viewport->updateTerrainSettings(settings, &errorMessage)) {
                    statusBar()->showMessage(
                        errorMessage.isEmpty() ? QStringLiteral("ハイトマップ設定の更新に失敗しました。")
                                               : errorMessage,
                        6000);
                }
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

    auto* heightmapDock = new QDockWidget("Terrain", this);
    m_heightmap = new HeightmapPanel(heightmapDock);
    heightmapDock->setWidget(m_heightmap);
    heightmapDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::LeftDockWidgetArea, heightmapDock);
    splitDockWidget(outlinerDock, heightmapDock, Qt::Vertical);

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
    m_selectModeAct->setChecked(true);
    m_selectModeAct->setToolTip("Select roads  [1]");
    m_selectModeAct->setShortcut(Qt::Key_1);
    group->addAction(m_selectModeAct);

    m_editModeAct = tb->addAction("Edit");
    m_editModeAct->setCheckable(true);
    m_editModeAct->setToolTip("Edit control points  [2]");
    m_editModeAct->setShortcut(Qt::Key_2);
    group->addAction(m_editModeAct);

    m_verticalModeAct = tb->addAction("Vertical");
    m_verticalModeAct->setCheckable(true);
    m_verticalModeAct->setToolTip("Edit vertical curve points  [3]");
    m_verticalModeAct->setShortcut(Qt::Key_3);
    group->addAction(m_verticalModeAct);

    m_bankModeAct = tb->addAction("Bank");
    m_bankModeAct->setCheckable(true);
    m_bankModeAct->setToolTip("Edit bank angle points  [4]");
    m_bankModeAct->setShortcut(Qt::Key_4);
    group->addAction(m_bankModeAct);

    m_laneModeAct = tb->addAction("Lane");
    m_laneModeAct->setCheckable(true);
    m_laneModeAct->setToolTip("Edit lane section points  [5]");
    m_laneModeAct->setShortcut(Qt::Key_5);
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

    auto* openEnvAct = file->addAction("Open &Environment...");
    connect(openEnvAct, &QAction::triggered, this, &MainWindow::openEnvironmentFile);

    m_recentFilesMenu = file->addMenu("Recent Files");
    refreshRecentFilesMenu();

    auto* saveAct = file->addAction("&Save");
    saveAct->setShortcut(QKeySequence::Save);
    connect(saveAct, &QAction::triggered, this, &MainWindow::saveFile);

    auto* saveEnvAct = file->addAction("Save En&vironment...");
    connect(saveEnvAct, &QAction::triggered, this, &MainWindow::saveEnvironmentFile);

    auto* saveAsAct = file->addAction("Save &As...");
    saveAsAct->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAct, &QAction::triggered, this, [this]{ m_currentPath.clear(); saveFile(); });

    file->addSeparator();
    auto* exitAct = file->addAction("E&xit");
    exitAct->setShortcut(QKeySequence::Quit);
    connect(exitAct, &QAction::triggered, qApp, &QApplication::quit);

    menuBar()->addMenu("&Edit");

    auto* view = menuBar()->addMenu("&View");
    m_gridAct = view->addAction("Grid");
    m_gridAct->setCheckable(true);
    m_gridAct->setChecked(true);
    connect(m_gridAct, &QAction::toggled, m_viewport, &Viewport3D::setGridVisible);

    m_wireframeAct = view->addAction("Wireframe");
    m_wireframeAct->setCheckable(true);
    m_wireframeAct->setToolTip("Toggle wireframe display  [W]");
    m_wireframeAct->setShortcut(Qt::Key_W);
    connect(m_wireframeAct, &QAction::toggled, m_viewport, &Viewport3D::setWireframe);

    m_directionArrowsAct = view->addAction("Road Direction Arrows");
    m_directionArrowsAct->setCheckable(true);
    m_directionArrowsAct->setToolTip("Toggle road direction arrows");
    connect(m_directionArrowsAct, &QAction::toggled, m_viewport, &Viewport3D::setRoadDirectionArrowsVisible);
}

void MainWindow::openFile() {
    QString path = QFileDialog::getOpenFileName(
        this, "Open Road Network", QString(),
        "Road Network (*.json);;All Files (*)");
    if (!path.isEmpty())
        loadFile(path);
}

void MainWindow::saveFile() {
    if (m_currentPath.isEmpty()) {
        m_currentPath = QFileDialog::getSaveFileName(
            this, "Save Road Network", QString(),
            "Road Network (*.json);;All Files (*)");
    }
    if (!m_currentPath.isEmpty()) {
        Serializer::saveToFile(m_currentPath, m_viewport->network());
        addRecentFile(m_currentPath);
    }
}

void MainWindow::openEnvironmentFile() {
    const QString path = QFileDialog::getOpenFileName(
        this, "Open Environment", QString(),
        "Environment (*.json);;All Files (*)");
    if (path.isEmpty())
        return;

    EnvironmentState env;
    if (!EnvironmentSerializer::loadFromFile(path, env)) {
        statusBar()->showMessage(QStringLiteral("環境ファイルの読み込みに失敗しました。"), 6000);
        return;
    }

    QString errorMessage;
    if (!m_viewport->applyEnvironmentState(env, &errorMessage)) {
        statusBar()->showMessage(
            errorMessage.isEmpty() ? QStringLiteral("環境ファイルの適用に失敗しました。")
                                   : errorMessage,
            6000);
        return;
    }

    m_currentEnvironmentPath = path;
    statusBar()->showMessage(QString("Loaded environment: %1").arg(QFileInfo(path).fileName()), 5000);
}

void MainWindow::saveEnvironmentFile() {
    if (m_currentEnvironmentPath.isEmpty()) {
        m_currentEnvironmentPath = QFileDialog::getSaveFileName(
            this, "Save Environment", QString(),
            "Environment (*.json);;All Files (*)");
    }
    if (m_currentEnvironmentPath.isEmpty())
        return;

    if (!EnvironmentSerializer::saveToFile(m_currentEnvironmentPath, m_viewport->environmentState())) {
        statusBar()->showMessage(QStringLiteral("環境ファイルの保存に失敗しました。"), 6000);
        return;
    }

    statusBar()->showMessage(
        QString("Saved environment: %1").arg(QFileInfo(m_currentEnvironmentPath).fileName()),
        5000);
}

void MainWindow::importHeightmap() {
    const QString path = QFileDialog::getOpenFileName(
        this,
        "Import Heightmap",
        QString(),
        "Image Files (*.png *.bmp *.jpg *.jpeg *.tif *.tiff);;All Files (*)");
    if (path.isEmpty())
        return;

    QString errorMessage;
    if (m_viewport->importHeightmap(path, &errorMessage)) {
        statusBar()->showMessage(QString("Imported heightmap: %1").arg(QFileInfo(path).fileName()), 5000);
    } else {
        statusBar()->showMessage(errorMessage.isEmpty() ? QStringLiteral("ハイトマップの読み込みに失敗しました。")
                                                        : errorMessage,
                                 6000);
    }
}

void MainWindow::clearHeightmap() {
    m_viewport->clearHeightmap();
    statusBar()->showMessage(QStringLiteral("ハイトマップをクリアしました。"), 4000);
}

void MainWindow::importTerrainTexture() {
    const QString path = QFileDialog::getOpenFileName(
        this,
        "Import Terrain Texture",
        QString(),
        "Image Files (*.png *.bmp *.jpg *.jpeg *.tif *.tiff);;All Files (*)");
    if (path.isEmpty())
        return;

    QString errorMessage;
    if (m_viewport->importTerrainTexture(path, &errorMessage)) {
        statusBar()->showMessage(QString("Imported terrain texture: %1").arg(QFileInfo(path).fileName()), 5000);
    } else {
        statusBar()->showMessage(errorMessage.isEmpty() ? QStringLiteral("地形テクスチャの読み込みに失敗しました。")
                                                        : errorMessage,
                                 6000);
    }
}

void MainWindow::clearTerrainTexture() {
    m_viewport->clearTerrainTexture();
    statusBar()->showMessage(QStringLiteral("地形テクスチャをクリアしました。"), 4000);
}

void MainWindow::onNetworkChanged() {
    m_properties->setNetwork(&m_viewport->network());
    m_outliner->refresh(&m_viewport->network());
    m_heightmap->setNetwork(&m_viewport->network());
}

void MainWindow::openRecentFile() {
    auto* action = qobject_cast<QAction*>(sender());
    if (!action)
        return;

    const QString path = action->data().toString();
    if (!path.isEmpty())
        loadFile(path, true);
}

void MainWindow::loadFile(const QString& path, bool addToRecent) {
    if (path.isEmpty())
        return;

    if (!QFile::exists(path)) {
        if (addToRecent) {
            m_recentFiles.removeAll(QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath()));
            saveRecentFiles();
            refreshRecentFilesMenu();
        }
        return;
    }

    m_currentPath = path;
    m_viewport->loadNetwork(path);
    if (addToRecent)
        addRecentFile(path);
}

void MainWindow::addRecentFile(const QString& path) {
    const QString normalizedPath = QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath());
    m_recentFiles.removeAll(normalizedPath);
    m_recentFiles.prepend(normalizedPath);
    while (m_recentFiles.size() > kMaxRecentFiles)
        m_recentFiles.removeLast();

    saveRecentFiles();
    refreshRecentFilesMenu();
}

void MainWindow::loadRecentFiles() {
    QSettings settings;
    m_recentFiles = settings.value("file/recentFiles").toStringList();

    QStringList normalized;
    normalized.reserve(m_recentFiles.size());
    for (const QString& path : std::as_const(m_recentFiles)) {
        const QString absolutePath = QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath());
        if (absolutePath.isEmpty() || normalized.contains(absolutePath))
            continue;
        normalized.push_back(absolutePath);
        if (normalized.size() >= kMaxRecentFiles)
            break;
    }
    m_recentFiles = normalized;
}

void MainWindow::saveRecentFiles() const {
    QSettings settings;
    settings.setValue("file/recentFiles", m_recentFiles);
}

void MainWindow::refreshRecentFilesMenu() {
    if (!m_recentFilesMenu)
        return;

    m_recentFilesMenu->clear();
    QStringList existingFiles;
    existingFiles.reserve(m_recentFiles.size());
    for (const QString& path : std::as_const(m_recentFiles)) {
        if (QFile::exists(path))
            existingFiles.push_back(path);
    }
    if (existingFiles != m_recentFiles) {
        m_recentFiles = existingFiles;
        saveRecentFiles();
    }

    if (m_recentFiles.isEmpty()) {
        auto* emptyAct = m_recentFilesMenu->addAction("(No Recent Files)");
        emptyAct->setEnabled(false);
        return;
    }

    for (int i = 0; i < m_recentFiles.size(); ++i) {
        const QString& path = m_recentFiles[i];
        const QString fileName = QFileInfo(path).fileName();
        auto* action = m_recentFilesMenu->addAction(
            QString("&%1 %2").arg(i + 1).arg(fileName));
        action->setData(path);
        action->setToolTip(path);
        action->setStatusTip(path);
        connect(action, &QAction::triggered, this, &MainWindow::openRecentFile);
    }

    m_recentFilesMenu->addSeparator();
    auto* clearAct = m_recentFilesMenu->addAction("Clear Menu");
    connect(clearAct, &QAction::triggered, this, [this] {
        m_recentFiles.clear();
        saveRecentFiles();
        refreshRecentFilesMenu();
    });
}
