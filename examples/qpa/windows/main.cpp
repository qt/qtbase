#include <QGuiApplication>
#include <QScreen>

#include "window.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    Window a;
    a.setVisible(true);

    Window b;
    b.setVisible(true);

    Window child(&b);
    child.setVisible(true);

    // create one window on each additional screen as well

    QList<QScreen *> screens = app.screens();
    foreach (QScreen *screen, screens) {
        if (screen == app.primaryScreen())
            continue;
        Window *window = new Window(screen);
        window->setVisible(true);
        window->setWindowTitle(screen->name());
    }

    return app.exec();
}
