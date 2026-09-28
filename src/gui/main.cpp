#include <QApplication>

#include "MainWindow.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("vole"));
    // Qt appends this to every window title, so it must not name a model: the
    // same binary drives the v1 and the v2. MainWindow supplies the model half.
    //
    // It must not name the vendor either. This is an unaffiliated,
    // unendorsed reimplementation, and a third-party window titled with the
    // manufacturer's own name is the one place a user could reasonably mistake
    // it for something official. Naming the hardware in prose is fine — that is
    // what the README does — but the application's own identity is "vole".
    QApplication::setApplicationDisplayName(QStringLiteral("vole"));

    MainWindow window;
    window.show();
    return QApplication::exec();
}
