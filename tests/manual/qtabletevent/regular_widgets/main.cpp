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

#include <QApplication>
#include <QDebug>
#include <QMouseEvent>
#include <QTabletEvent>
#include <QWidget>

class EventReportWidget : public QWidget
{
public:
    EventReportWidget();
protected:
    void mouseDoubleClickEvent(QMouseEvent *event) { outputMouseEvent(event); }
    void mouseMoveEvent(QMouseEvent *event) { outputMouseEvent(event); }
    void mousePressEvent(QMouseEvent *event) { outputMouseEvent(event); }
    void mouseReleaseEvent(QMouseEvent *event) { outputMouseEvent(event); }

    void tabletEvent(QTabletEvent *);

private:
    void outputMouseEvent(QMouseEvent *event);

    bool m_lastIsMouseMove;
    bool m_lastIsTabletMove;
};

EventReportWidget::EventReportWidget()
    : m_lastIsMouseMove(false)
    , m_lastIsTabletMove(false)
{ }

void EventReportWidget::tabletEvent(QTabletEvent *event)
{
    QWidget::tabletEvent(event);

    QString type;
    switch (event->type()) {
    case QEvent::TabletEnterProximity:
        m_lastIsTabletMove = false;
        type = QString::fromLatin1("TabletEnterProximity");
        break;
    case QEvent::TabletLeaveProximity:
        m_lastIsTabletMove = false;
        type = QString::fromLatin1("TabletLeaveProximity");
        break;
    case QEvent::TabletMove:
        if (m_lastIsTabletMove)
            return;

        m_lastIsTabletMove = true;
        type = QString::fromLatin1("TabletMove");
        break;
    case QEvent::TabletPress:
        m_lastIsTabletMove = false;
        type = QString::fromLatin1("TabletPress");
        break;
    case QEvent::TabletRelease:
        m_lastIsTabletMove = false;
        type = QString::fromLatin1("TabletRelease");
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    qDebug() << "Tablet event, type = " << type
             << " position = " << event->pos()
             << " global position = " << event->globalPos();
}

void EventReportWidget::outputMouseEvent(QMouseEvent *event)
{
    QString type;
    switch (event->type()) {
    case QEvent::MouseButtonDblClick:
        m_lastIsMouseMove = false;
        type = QString::fromLatin1("MouseButtonDblClick");
        break;
    case QEvent::MouseButtonPress:
        m_lastIsMouseMove = false;
        type = QString::fromLatin1("MouseButtonPress");
        break;
    case QEvent::MouseButtonRelease:
        m_lastIsMouseMove = false;
        type = QString::fromLatin1("MouseButtonRelease");
        break;
    case QEvent::MouseMove:
        if (m_lastIsMouseMove)
            return; // only show one move to keep things readable

        m_lastIsMouseMove = true;
        type = QString::fromLatin1("MouseMove");
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    qDebug() << "Mouse event, type = " << type
             << " position = " << event->pos()
             << " global position = " << event->globalPos();
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    EventReportWidget widget;
    widget.show();
    return app.exec();
}
