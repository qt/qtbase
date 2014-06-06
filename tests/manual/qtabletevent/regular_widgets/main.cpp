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
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QVector>
#include <QPainter>
#include <QCursor>

enum TabletPointType {
    TabletButtonPress,
    TabletButtonRelease,
    TabletMove
};

struct TabletPoint
{
    TabletPoint(const QPointF &p = QPointF(), TabletPointType t = TabletMove,
                Qt::MouseButton b = Qt::LeftButton, QTabletEvent::PointerType pt = QTabletEvent::UnknownPointer, qreal prs = 0) :
        pos(p), type(t), button(b), ptype(pt), pressure(prs) {}

    QPointF pos;
    TabletPointType type;
    Qt::MouseButton button;
    QTabletEvent::PointerType ptype;
    qreal pressure;
};

class EventReportWidget : public QWidget
{
    Q_OBJECT
public:
    EventReportWidget();

public slots:
    void clearPoints() { m_points.clear(); update(); }

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) { outputMouseEvent(event); }
    void mouseMoveEvent(QMouseEvent *event) { outputMouseEvent(event); }
    void mousePressEvent(QMouseEvent *event) { outputMouseEvent(event); }
    void mouseReleaseEvent(QMouseEvent *event) { outputMouseEvent(event); }

    void tabletEvent(QTabletEvent *);

    void paintEvent(QPaintEvent *);

private:
    void outputMouseEvent(QMouseEvent *event);

    bool m_lastIsMouseMove;
    bool m_lastIsTabletMove;
    Qt::MouseButton m_lastButton;
    QVector<TabletPoint> m_points;
};

EventReportWidget::EventReportWidget()
    : m_lastIsMouseMove(false)
    , m_lastIsTabletMove(false)
    , m_lastButton(Qt::NoButton)
{ }

void EventReportWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    const QRectF geom = QRectF(QPoint(0, 0), size());
    p.fillRect(geom, Qt::white);
    p.drawRect(QRectF(geom.topLeft(), geom.bottomRight() - QPointF(1,1)));
    p.setPen(Qt::white);
    foreach (const TabletPoint &t, m_points) {
        if (geom.contains(t.pos)) {
              QPainterPath pp;
              pp.addEllipse(t.pos, 8, 8);
              QRectF pointBounds(t.pos.x() - 10, t.pos.y() - 10, 20, 20);
              switch (t.type) {
              case TabletButtonPress:
                  p.fillPath(pp, Qt::darkGreen);
                  if (t.button != Qt::NoButton)
                    p.drawText(pointBounds, Qt::AlignCenter, QString::number(t.button));
                  break;
              case TabletButtonRelease:
                  p.fillPath(pp, Qt::red);
                  if (t.button != Qt::NoButton)
                    p.drawText(pointBounds, Qt::AlignCenter, QString::number(t.button));
                  break;
              case TabletMove:
                  if (t.pressure > 0.0) {
                      p.setPen(t.ptype == QTabletEvent::Eraser ? Qt::red : Qt::black);
                      p.drawEllipse(t.pos, t.pressure * 10.0, t.pressure * 10.0);
                      p.setPen(Qt::white);
                  } else {
                      p.fillRect(t.pos.x() - 2, t.pos.y() - 2, 4, 4, Qt::black);
                  }
                  break;
              }
          }
    }
}

void EventReportWidget::tabletEvent(QTabletEvent *event)
{

    QWidget::tabletEvent(event);
    QString type;
    switch (event->type()) {
    case QEvent::TabletEnterProximity:
        type = QString::fromLatin1("TabletEnterProximity");
        break;
    case QEvent::TabletLeaveProximity:
        type = QString::fromLatin1("TabletLeaveProximity");
        break;
    case QEvent::TabletMove:
        type = QString::fromLatin1("TabletMove");
        m_points.push_back(TabletPoint(event->pos(), TabletMove, m_lastButton, event->pointerType(), event->pressure()));
        update();
        break;
    case QEvent::TabletPress:
        type = QString::fromLatin1("TabletPress");
        m_points.push_back(TabletPoint(event->pos(), TabletButtonPress, event->button(), event->pointerType()));
        m_lastButton = event->button();
        update();
        break;
    case QEvent::TabletRelease:
        type = QString::fromLatin1("TabletRelease");
        m_points.push_back(TabletPoint(event->pos(), TabletButtonRelease, event->button(), event->pointerType()));
        update();
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    QString pointerType = "UNKNOWN";
    switch (event->pointerType()) {
    case QTabletEvent::Pen:
        pointerType = "Pen";
        break;
    case QTabletEvent::Cursor:
        pointerType = "Cursor";
        break;
    case QTabletEvent::Eraser:
        pointerType = "Eraser";
        break;
    default:
        break;
    }

    QString device = "UNKNOWN";
    switch (event->device()) {
    case QTabletEvent::Puck:
        pointerType = "Puck";
        break;
    case QTabletEvent::Stylus:
        pointerType = "Stylus";
        break;
    case QTabletEvent::Airbrush:
        pointerType = "Airbrush";
        break;
    case QTabletEvent::FourDMouse:
        pointerType = "FourDMouse";
        break;
    case QTabletEvent::RotationStylus:
        pointerType = "RotationStylus";
        break;
    default:
        break;
    }

    if (!m_lastIsTabletMove)
        qDebug() << "Tablet event, type = " << type
                 << " position = " << event->pos()
                 << " global position = " << event->globalPos()
                 << " cursor at " << QCursor::pos()
                 << " buttons " << event->buttons() << " changed " << event->button()
                 << " pointer type " << pointerType << " device " << device;

    m_lastIsTabletMove = (event->type() == QEvent::TabletMove);
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
    QMainWindow mainWindow;
    mainWindow.setWindowTitle(QString::fromLatin1("Tablet Test %1").arg(QT_VERSION_STR));
    EventReportWidget *widget = new EventReportWidget;
    widget->setMinimumSize(640, 480);
    QMenu *fileMenu = mainWindow.menuBar()->addMenu("File");
    QObject::connect(fileMenu->addAction("Clear"), SIGNAL(triggered()), widget, SLOT(clearPoints()));
    QAction *quitAction = fileMenu->addAction("Quit");
    QObject::connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    quitAction->setShortcut(Qt::CTRL + Qt::Key_Q);
    mainWindow.setCentralWidget(widget);
    mainWindow.show();
    return app.exec();
}

#include "main.moc"
