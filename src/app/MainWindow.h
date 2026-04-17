#pragma once

#include <QMainWindow>

class Viewport3D;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void openFile();

private:
    Viewport3D* m_viewport = nullptr;

    void setupDocks();
    void setupMenuBar();
};
