#include <QtGui/QApplication>
#include <QtGui/QLabel>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QLabel label(QObject::tr("Hello, world!"));
#if defined(Q_WS_S60)
    label.showMaximized();
#else
    label.show();
#endif
    return a.exec();
}
