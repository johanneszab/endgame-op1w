#include <QApplication>

#include "MainWindow.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("endgame-op1w"));
    QApplication::setApplicationDisplayName(
        QStringLiteral("Endgame Gear OP1w 4k v2"));

    MainWindow window;
    window.show();
    return QApplication::exec();
}
