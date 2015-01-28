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

#include <QApplication>
#include <QSlider>
#include <QScrollArea>
#include <QScrollBar>
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGestureEvent>
#include <QPanGesture>
#include <QDebug>

#include "mousepangesturerecognizer.h"

class ScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    ScrollArea(QWidget *parent = 0)
        : QScrollArea(parent), outside(false)
    {
        viewport()->grabGesture(Qt::PanGesture, Qt::ReceivePartialGestures);
    }

protected:
    bool viewportEvent(QEvent *event)
    {
        if (event->type() == QEvent::Gesture) {
            gestureEvent(static_cast<QGestureEvent *>(event));
            return true;
        } else if (event->type() == QEvent::GestureOverride) {
            QGestureEvent *ge = static_cast<QGestureEvent *>(event);
            if (QPanGesture *pan = static_cast<QPanGesture *>(ge->gesture(Qt::PanGesture)))
                if (pan->state() == Qt::GestureStarted) {
                    outside = false;
                }
        }
        return QScrollArea::viewportEvent(event);
    }
    void gestureEvent(QGestureEvent *event)
    {
        QPanGesture *pan = static_cast<QPanGesture *>(event->gesture(Qt::PanGesture));
        if (pan) {
            switch(pan->state()) {
            case Qt::GestureStarted: qDebug() << this << "Pan: started"; break;
            case Qt::GestureFinished: qDebug() << this << "Pan: finished"; break;
            case Qt::GestureCanceled: qDebug() << this << "Pan: canceled"; break;
            case Qt::GestureUpdated: break;
            default: qDebug() << this << "Pan: <unknown state>"; break;
            }

            if (pan->state() == Qt::GestureStarted)
                outside = false;
            event->ignore();
            event->ignore(pan);
            if (outside)
                return;

            const QPointF delta = pan->delta();
            const QPointF totalOffset = pan->offset();
            QScrollBar *vbar = verticalScrollBar();
            QScrollBar *hbar = horizontalScrollBar();

            if ((vbar->value() == vbar->minimum() && totalOffset.y() > 10) ||
                (vbar->value() == vbar->maximum() && totalOffset.y() < -10)) {
                outside = true;
                return;
            }
            if ((hbar->value() == hbar->minimum() && totalOffset.x() > 10) ||
                (hbar->value() == hbar->maximum() && totalOffset.x() < -10)) {
                outside = true;
                return;
            }
            vbar->setValue(vbar->value() - delta.y());
            hbar->setValue(hbar->value() - delta.x());
            event->accept(pan);
        }
    }

private:
    bool outside;
};

class Slider : public QSlider
{
public:
    Slider(Qt::Orientation orientation, QWidget *parent = 0)
        : QSlider(orientation, parent)
    {
        grabGesture(Qt::PanGesture);
    }
protected:
    bool event(QEvent *event)
    {
        if (event->type() == QEvent::Gesture) {
            gestureEvent(static_cast<QGestureEvent *>(event));
            return true;
        }
        return QSlider::event(event);
    }
    void gestureEvent(QGestureEvent *event)
    {
        QPanGesture *pan = static_cast<QPanGesture *>(event->gesture(Qt::PanGesture));
        if (pan) {
            switch (pan->state()) {
            case Qt::GestureStarted: qDebug() << this << "Pan: started"; break;
            case Qt::GestureFinished: qDebug() << this << "Pan: finished"; break;
            case Qt::GestureCanceled: qDebug() << this << "Pan: canceled"; break;
            case Qt::GestureUpdated: break;
            default: qDebug() << this << "Pan: <unknown state>"; break;
            }

            if (pan->state() == Qt::GestureStarted)
                outside = false;
            event->ignore();
            event->ignore(pan);
            if (outside)
                return;
            const QPointF delta = pan->delta();
            const QPointF totalOffset = pan->offset();
            if (orientation() == Qt::Horizontal) {
                if ((value() == minimum() && totalOffset.x() < -10) ||
                    (value() == maximum() && totalOffset.x() > 10)) {
                    outside = true;
                    return;
                }
                if (totalOffset.y() < 40 && totalOffset.y() > -40) {
                    setValue(value() + delta.x());
                    event->accept(pan);
                } else {
                    outside = true;
                }
            } else if (orientation() == Qt::Vertical) {
                if ((value() == maximum() && totalOffset.y() < -10) ||
                    (value() == minimum() && totalOffset.y() > 10)) {
                    outside = true;
                    return;
                }
                if (totalOffset.x() < 40 && totalOffset.x() > -40) {
                    setValue(value() - delta.y());
                    event->accept(pan);
                } else {
                    outside = true;
                }
            }
        }
    }
private:
    bool outside;
};

class MainWindow : public QMainWindow
{
public:
    MainWindow()
    {
        rootScrollArea = new ScrollArea;
        rootScrollArea->setObjectName(QLatin1String("rootScrollArea"));
        setCentralWidget(rootScrollArea);

        QWidget *root = new QWidget;
        root->setFixedSize(3000, 3000);
        rootScrollArea->setWidget(root);

        Slider *verticalSlider = new Slider(Qt::Vertical, root);
        verticalSlider->setObjectName(QLatin1String("verticalSlider"));
        verticalSlider ->move(650, 1100);
        Slider *horizontalSlider = new Slider(Qt::Horizontal, root);
        horizontalSlider->setObjectName(QLatin1String("horizontalSlider"));
        horizontalSlider ->move(600, 1000);

        childScrollArea = new ScrollArea(root);
        childScrollArea->setObjectName(QLatin1String("childScrollArea"));
        childScrollArea->move(500, 500);
        QWidget *w = new QWidget;
        w->setMinimumWidth(700);
        QVBoxLayout *l = new QVBoxLayout(w);
        l->setMargin(20);
        for (int i = 0; i < 100; ++i) {
            QWidget *w = new QWidget;
            QHBoxLayout *ll = new QHBoxLayout(w);
            ll->addWidget(new QLabel(QString("Label %1").arg(i)));
            ll->addWidget(new QPushButton(QString("Button %1").arg(i)));
            l->addWidget(w);
        }
        childScrollArea->setWidget(w);
#if defined(Q_OS_WIN)
        // Windows can force Qt to create a native window handle for an
        // intermediate widget and that will block gesture to get touch events.
        // So this hack to make sure gestures get all touch events they need.
        foreach (QObject *w, children())
            if (w->isWidgetType())
                static_cast<QWidget *>(w)->setAttribute(Qt::WA_AcceptTouchEvents);
#endif
    }
private:
    ScrollArea *rootScrollArea;
    ScrollArea *childScrollArea;
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QGestureRecognizer::registerRecognizer(new MousePanGestureRecognizer);
    MainWindow w;
    w.show();
    return app.exec();
}

#include "main.moc"
