#include <QApplication>

#include "MainWindow.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("endgame-op1w"));
    // Qt appends this to every window title, so it must not name a model: the
    // same binary drives the v1 and the v2. MainWindow supplies the model half.
    QApplication::setApplicationDisplayName(QStringLiteral("Endgame Gear"));

    MainWindow window;
    window.show();
    return QApplication::exec();
}
