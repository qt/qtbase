#include "testwidget.h"

#include <QtGui/QApplication>

//! [0]
int main( int argc, char *argv[] )
{
    QApplication application( argc, argv );
    TestWidget w;
    w.showFullScreen();
    return application.exec();
}
//! [0]
