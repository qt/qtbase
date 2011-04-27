/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qapplication.h>
#include <qlabel.h>
#include <qpixmap.h>
#include <qprinter.h>
#include <qpainter.h>
#include <qfile.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <QtGui>
#include <QtCore>

class MyClass : public QWidget
{
public:
     MyClass(QWidget *parent = 0) : QWidget(parent) { }

protected:
     void paintEvent(QPaintEvent *e)
     {
        /*QRadialGradient rg(50, 50, 50, 50, 50);
         rg.setColorAt(0, QColor::fromRgbF(1, 0, 0, 1));
         rg.setColorAt(1, QColor::fromRgbF(0, 0, 0, 0));
         QPainter pmp(&pm);
         pmp.fillRect(0, 0, 100, 100, rg);
         pmp.end();*/

	createImage();

	QPainter p(this);
	p.fillRect(rect(), Qt::white);

	p.drawPixmap(0, 0, pixmap);

	p.drawPixmap(100, 0, channelImage);
     }

    void createImage()
    {
//! [0]
	pixmap = QPixmap(100, 100);
	pixmap.fill(Qt::transparent);

	QRadialGradient gradient(50, 50, 50, 50, 50);
	gradient.setColorAt(0, QColor::fromRgbF(1, 0, 0, 1));
	gradient.setColorAt(1, QColor::fromRgbF(0, 0, 0, 0));
	QPainter painter(&pixmap);
	painter.fillRect(0, 0, 100, 100, gradient);

	channelImage = pixmap.alphaChannel();
	update();
//! [0]
    }

    QPixmap channelImage, pixmap;
    QSize sizeHint() const { return QSize(500, 500); }
};

int main(int argc, char **argv)
{
     QApplication app(argc, argv);

     MyClass cl;
     cl.show();
     QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

     int ret = app.exec();
     return ret;
}
