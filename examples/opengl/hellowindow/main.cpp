#include <QGuiApplication>
#include <QScreen>
#include <QThread>

#include "hellowindow.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    QScreen *screen = QGuiApplication::primaryScreen();

    QRect screenGeometry = screen->availableGeometry();

    QSurfaceFormat format;
    format.setDepthBufferSize(16);
    format.setSamples(4);

    QPoint center = QPoint(screenGeometry.center().x(), screenGeometry.top() + 80);
    QSize windowSize(400, 320);
    int delta = 40;

    Renderer rendererA(format);
    Renderer rendererB(format, &rendererA);

    QThread renderThread;
    rendererB.moveToThread(&renderThread);
    renderThread.start();

    QObject::connect(qGuiApp, SIGNAL(lastWindowClosed()), &renderThread, SLOT(quit()));

    HelloWindow windowA(&rendererA);
    windowA.setGeometry(QRect(center, windowSize).translated(-windowSize.width() - delta / 2, 0));
    windowA.setWindowTitle(QLatin1String("Thread A - Context A"));
    windowA.setVisible(true);

    HelloWindow windowB(&rendererA);
    windowB.setGeometry(QRect(center, windowSize).translated(delta / 2, 0));
    windowB.setWindowTitle(QLatin1String("Thread A - Context A"));
    windowB.setVisible(true);

    HelloWindow windowC(&rendererB);
    windowC.setGeometry(QRect(center, windowSize).translated(-windowSize.width() / 2, windowSize.height() + delta));
    windowC.setWindowTitle(QLatin1String("Thread B - Context B"));
    windowC.setVisible(true);

    app.exec();

    renderThread.wait();
}
