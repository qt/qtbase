/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QWidget>
#include <QHBoxLayout>
#include <QApplication>
#include <QPainter>
#include <QImage>
#include <QImageReader>

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

