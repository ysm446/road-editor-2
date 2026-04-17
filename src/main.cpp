#include <QApplication>
#include <QSurfaceFormat>
#include "app/MainWindow.h"

int main(int argc, char* argv[]) {
    // Must be set before QApplication for correct OpenGL context on all platforms
    QSurfaceFormat fmt;
    fmt.setVersion(4, 1);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setDepthBufferSize(24);
    fmt.setSamples(4);
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);
    app.setApplicationName("Road Editor 2");

    MainWindow window;
    window.show();

    return app.exec();
}
