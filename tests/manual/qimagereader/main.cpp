/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>

class MyWidget : public QWidget
{
public:
    MyWidget(QWidget * parent, const QString &imagefname, bool scaleImage)
            : QWidget(parent), fileName(imagefname), scale(scaleImage)
    {

    }

    virtual void paintEvent(QPaintEvent * /*event*/)
    {
        QPainter painter(this);
        QImageReader reader(fileName);
        if (!reader.canRead()) {
            qWarning("Unable to read image file %s", fileName.toLocal8Bit().constData());
            return;
        }
        if (!scale){
            QImage image = reader.read();
            painter.drawImage(rect(), image);
        }else{
            reader.setScaledSize( QSize(rect().width(), rect().height()) );
            QImage image = reader.read();
            painter.drawImage(rect(), image);
        }
    }

private:
    QString fileName;
    bool scale;

};



// both the scaled and unscaled version of the CMYK encoded JPEG
//      should have the same colors and not look corrupted.

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    QWidget mainWidget;
    mainWidget.setWindowTitle("Colors in images are identical?");
    mainWidget.setMinimumSize(400,400);
    QHBoxLayout *l = new QHBoxLayout;
    MyWidget *w1 = new MyWidget(&mainWidget,"Qt_logostrap_CMYK.jpg", false);
    MyWidget *w2 = new MyWidget(&mainWidget,"Qt_logostrap_CMYK.jpg", true);
    l->addWidget(w1);
    l->addWidget(w2);
    mainWidget.setLayout(l);
    mainWidget.show();

    return app.exec();
}

