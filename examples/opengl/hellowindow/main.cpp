#include <QGuiApplication>

#include "hellowindow.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    Renderer renderer;

    HelloWindow windowA(&renderer);
    windowA.setVisible(true);

    HelloWindow windowB(&renderer);
    windowB.setVisible(true);

    return app.exec();
}
