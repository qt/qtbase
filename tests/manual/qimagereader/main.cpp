// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

