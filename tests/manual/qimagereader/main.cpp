/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
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

