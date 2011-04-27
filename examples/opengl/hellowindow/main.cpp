#include <QGuiApplication>

#include "hellowindow.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    HelloWindow window;
    window.setVisible(true);

    return app.exec();
}
