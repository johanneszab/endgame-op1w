#include <QApplication>
#include <QIcon>

#include "MainWindow.h"

namespace {

// Prefer the installed theme icon, so a user's icon theme can override it, and
// fall back to the copies compiled into the binary. The fallback is not
// belt-and-braces: running from the build directory is the normal case during
// development and there is no installed icon then. Qt6 here is built without
// the Svg module, so these are the PNGs rather than icons/vole.svg.
//
// None of this reaches the window decoration on WAYLAND, where a client cannot
// hand the compositor an icon at all -- see setDesktopFileName() below.
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

    // THE line that makes an icon appear on Wayland, and it is not the one
    // below. A Wayland client cannot give the compositor an icon: it announces
    // an `app_id`, and the shell looks up <app_id>.desktop and takes the icon
    // from there. Qt derives app_id from this desktop file name, and when it is
    // unset falls back to the EXECUTABLE's basename -- "vole-gui", for which no
    // .desktop exists, so GNOME labelled the window "vole-gui" and drew the
    // generic placeholder. Naming it "vole" makes it match vole.desktop.
    //
    // Two consequences worth knowing, because neither is a bug to chase:
    //  - The .desktop file and the hicolor icons must be INSTALLED. A build-tree
    //    run has no icon on Wayland no matter what this program does.
    //  - Renaming vole.desktop means changing this string too, or the match
    //    silently breaks again.
    QApplication::setDesktopFileName(QStringLiteral("vole"));

    // Still needed: X11 and XWayland take the icon from the window itself, as
    // does Windows, and it is what the task switcher uses there.
    QApplication::setWindowIcon(applicationIcon());

    MainWindow window;
    window.show();
    return QApplication::exec();
}
