#include <QtGui>
#include <windows.h>
#include "window.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    Window *window = new Window();
    window->resize(800, 600);

    window->show();

    return app.exec();

}
