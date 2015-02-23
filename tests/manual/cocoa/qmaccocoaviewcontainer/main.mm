/****************************************************************************
 **
 ** Copyright (C) 2015 The Qt Company Ltd.
 ** Contact: http://www.qt.io/licensing/
 **
 ** This file is part of the test suite of the Qt Toolkit.
 **
 ** $QT_BEGIN_LICENSE:LGPL21$
 ** Commercial License Usage
 ** Licensees holding valid commercial Qt licenses may use this file in
 ** accordance with the commercial license agreement provided with the
 ** Software or, alternatively, in accordance with the terms contained in
 ** a written agreement between you and The Qt Company. For licensing terms
 ** and conditions see http://www.qt.io/terms-conditions. For further
 ** information use the contact form at http://www.qt.io/contact-us.
 **
 ** GNU Lesser General Public License Usage
 ** Alternatively, this file may be used under the terms of the GNU Lesser
 ** General Public License version 2.1 or version 3 as published by the Free
 ** Software Foundation and appearing in the file LICENSE.LGPLv21 and
 ** LICENSE.LGPLv3 included in the packaging of this file. Please review the
 ** following information to ensure the GNU Lesser General Public License
 ** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
 ** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
 **
 ** As a special exception, The Qt Company gives you certain additional
 ** rights. These rights are described in The Qt Company LGPL Exception
 ** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
 **
 ** $QT_END_LICENSE$
 **
 ****************************************************************************/


#import "TestMouseMovedNSView.h"
#include <QtGui>
#include <QtWidgets>
#include <QMacCocoaViewContainer>

class MyWidget : public QWidget
{
    Q_OBJECT
public:
    MyWidget(QMacCocoaViewContainer *c, QWidget *parent = 0) : QWidget(parent), container(c), currentlyVisible(true)
    {
        QVBoxLayout *vbox = new QVBoxLayout;
        QLabel *label = new QLabel("1: Check that the cross hairs move when the mouse is moved over the widget\n"
                                   "2: Check that clicking on change visibility causes the NSView to hide/show as appropriate\n"
                                   "3: Check that clicking on resize makes the view be 200x200");
        vbox->addWidget(label);
        QPushButton *button = new QPushButton("Change visibility");
        connect(button, SIGNAL(clicked()), this, SLOT(changeVisibility()));
        vbox->addWidget(button);
        button = new QPushButton("Change size");
        connect(button, SIGNAL(clicked()), this, SLOT(changeSize()));
        vbox->addWidget(button);
        setLayout(vbox);
    }
public slots:
    void changeVisibility()
    {
        currentlyVisible = !currentlyVisible;
        if (!currentlyVisible)
            container->hide();
        else
            container->show();
        bool b = !([(NSView *)container->cocoaView() isHidden]);
        QMessageBox::information(this, "Is visible", QString("NSView visibility: %1").arg(b));
    }
    void changeSize()
    {
        NSRect r = NSMakeRect(0, 0, 200, 200);
        [(NSView *)container->cocoaView() setFrame:r];
    }
private:
    QMacCocoaViewContainer *container;
    bool currentlyVisible;
};

#include "main.moc"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    QPoint pos(100,100);
    QWidget w;
    w.move(pos);
    w.setWindowTitle("QMacCocoaViewContainer");
    NSRect r = NSMakeRect(0, 0, 100, 100);
    NSView *view = [[TestMouseMovedNSView alloc] initWithFrame: r];
    QMacCocoaViewContainer *nativeChild = new QMacCocoaViewContainer(view, &w);
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(nativeChild);
    w.setLayout(vbox);
    w.show();
    MyWidget w2(nativeChild);
    w2.show();
    return a.exec();
}

