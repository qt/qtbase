#include <QGuiApplication>
#include <QRect>
#include <QScreen>

#include "paintedwindow.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    QScreen *screen = QGuiApplication::primaryScreen();

    QRect screenGeometry = screen->availableGeometry();

    QPoint center = screenGeometry.center();
    QRect windowRect(0, 0, 640, 480);

    PaintedWindow window;
    window.setGeometry(QRect(center - windowRect.center(), windowRect.size()));
    window.show();

    app.exec();
}
