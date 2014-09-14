#include <stdio.h>
#include <unistd.h>

#include <QtCore>
#include <QtGui>

#include "rasterwindow.h"

RasterWindow *window;

class First
{
public:
    First() {
        QLoggingCategory::setFilterRules(QStringLiteral("qt.platform.pepper.*=true"));
    }
};
First first;


// App-provided init and exit functions:
void app_init(const QStringList &arguments)
{
    qDebug() << "Running app_init";

//    QLoggingCategory::setFilterRules(QStringLiteral("qt.platform.pepper*=true"));
//    QLoggingCategory::setFilterRules(QStringLiteral("qt.*=true"));

    window = new RasterWindow;
    window->show();
}

void app_exit()
{
    qDebug() << "Running app_exit";
    delete window;
}

// Register functions with Qt. The type of register function
// selects the QApplicaiton type.
Q_GUI_MAIN(app_init, app_exit);
