#pragma once

#include <QMainWindow>
#include <QString>
#include <QStringList>

class Viewport3D;
class PropertiesPanel;
class OutlinerPanel;
class HeightmapPanel;
class QAction;
class QMenu;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void openFile();
    void saveFile();
    void onNetworkChanged();
    void openRecentFile();
    void importHeightmap();
    void clearHeightmap();

private:
    static constexpr int kMaxRecentFiles = 8;

    Viewport3D*      m_viewport    = nullptr;
    PropertiesPanel* m_properties  = nullptr;
    OutlinerPanel*   m_outliner    = nullptr;
    HeightmapPanel*  m_heightmap   = nullptr;

    QAction* m_selectModeAct  = nullptr;
    QAction* m_editModeAct    = nullptr;
    QAction* m_verticalModeAct = nullptr;
    QAction* m_bankModeAct    = nullptr;
    QAction* m_laneModeAct    = nullptr;
    QAction* m_createRoadAct  = nullptr;
    QAction* m_createIntersectionAct = nullptr;
    QAction* m_wireframeAct   = nullptr;
    QMenu* m_recentFilesMenu  = nullptr;

    QString m_currentPath;
    QStringList m_recentFiles;

    void setupDocks();
    void setupMenuBar();
    void setupToolBar();
    void loadFile(const QString& path, bool addToRecent = true);
    void addRecentFile(const QString& path);
    void loadRecentFiles();
    void saveRecentFiles() const;
    void refreshRecentFilesMenu();
};
