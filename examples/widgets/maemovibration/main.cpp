
#include "buttonwidget.h"
#include "mcevibrator.h"

#include <QtDebug>
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QTextStream>

#include <cstdlib>

//! [0]
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
	QString path = MceVibrator::defaultMceFilePath;

    QFile file(path);
    QStringList names;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        names = MceVibrator::parsePatternNames(stream);
        file.close();
    }

    if (names.isEmpty()){
        qDebug() << "Could not read vibration pattern names from " << path;
        a.exit(-1);
    }
//! [0]

//! [1]
    ButtonWidget buttonWidget(names);
    MceVibrator vibrator;
    QObject::connect(&buttonWidget, SIGNAL(clicked(const QString &)),
                     &vibrator, SLOT(vibrate(const QString &)));
    buttonWidget.show();

    return a.exec();
}
//! [1]

