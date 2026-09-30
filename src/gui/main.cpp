#include <QApplication>
#include <QIcon>

#include "MainWindow.h"

namespace {

// Prefer the installed theme icon, so a user's icon theme can override it, and
// fall back to the copies compiled into the binary. The fallback is not
// belt-and-braces: running from the build directory is the normal case during
// development and there is no installed icon then. Qt6 here is built without
// the Svg module, so these are the PNGs rather than icons/vole.svg.
QIcon applicationIcon()
{
    QIcon embedded;
    for (int px : {16, 22, 24, 32, 48, 64, 128, 256, 512}) {
        embedded.addFile(QStringLiteral(":/icons/%1x%1/vole.png").arg(px));
    }
    return QIcon::fromTheme(QStringLiteral("vole"), embedded);
}

}  // namespace

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
    QApplication::setWindowIcon(applicationIcon());

    MainWindow window;
    window.show();
    return QApplication::exec();
}
