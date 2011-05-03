#include <QGuiApplication>

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

    return app.exec();
}
