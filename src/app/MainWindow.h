#pragma once

#include <QMainWindow>
#include <QString>

class Viewport3D;
class PropertiesPanel;
class OutlinerPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void openFile();
    void saveFile();
    void onNetworkChanged();

private:
    Viewport3D*      m_viewport    = nullptr;
    PropertiesPanel* m_properties  = nullptr;
    OutlinerPanel*   m_outliner    = nullptr;

    QString m_currentPath;

    void setupDocks();
    void setupMenuBar();
};
