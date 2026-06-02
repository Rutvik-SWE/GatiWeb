#include <QApplication>
#include "ui/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("GATIWEB"));
    app.setOrganizationName(QStringLiteral("GatiTeam"));

    MainWindow window;
    window.resize(1100, 800);
    window.setWindowTitle(QStringLiteral("GATIWEB Engine"));
    window.show();

    return app.exec();
}