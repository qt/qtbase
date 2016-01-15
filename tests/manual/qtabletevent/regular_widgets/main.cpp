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

#include <QApplication>
#include <QDebug>
#include <QMouseEvent>
#include <QTabletEvent>
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
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
                Qt::MouseButton b = Qt::LeftButton, QTabletEvent::PointerType pt = QTabletEvent::UnknownPointer, qreal prs = 0, qreal rotation = 0) :
        pos(p), type(t), button(b), ptype(pt), pressure(prs), angle(rotation) {}

    QPointF pos;
    TabletPointType type;
    Qt::MouseButton button;
    QTabletEvent::PointerType ptype;
    qreal pressure;
    qreal angle;
};

class EventReportWidget : public QWidget
{
    Q_OBJECT
public:
    EventReportWidget();

public slots:
    void clearPoints() { m_points.clear(); update(); }

signals:
    void stats(QString s);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) { outputMouseEvent(event); }
    void mouseMoveEvent(QMouseEvent *event) { outputMouseEvent(event); }
    void mousePressEvent(QMouseEvent *event) { outputMouseEvent(event); }
    void mouseReleaseEvent(QMouseEvent *event) { outputMouseEvent(event); }

    void tabletEvent(QTabletEvent *);

    void paintEvent(QPaintEvent *);
    void timerEvent(QTimerEvent *);

private:
    void outputMouseEvent(QMouseEvent *event);

    bool m_lastIsMouseMove;
    bool m_lastIsTabletMove;
    Qt::MouseButton m_lastButton;
    QVector<TabletPoint> m_points;
    int m_tabletMoveCount;
    int m_paintEventCount;
};

EventReportWidget::EventReportWidget()
    : m_lastIsMouseMove(false)
    , m_lastIsTabletMove(false)
    , m_lastButton(Qt::NoButton)
    , m_tabletMoveCount(0)
    , m_paintEventCount(0)
{
    startTimer(1000);
}

void EventReportWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    int lineSpacing = fontMetrics().lineSpacing();
    int halfLineSpacing = lineSpacing / 2;
    const QRectF geom = QRectF(QPoint(0, 0), size());
    p.fillRect(geom, Qt::white);
    p.drawRect(QRectF(geom.topLeft(), geom.bottomRight() - QPointF(1,1)));
    p.setPen(Qt::white);
    QPainterPath ellipse;
    ellipse.addEllipse(0, 0, halfLineSpacing * 5, halfLineSpacing);
    foreach (const TabletPoint &t, m_points) {
        if (geom.contains(t.pos)) {
              QPainterPath pp;
              pp.addEllipse(t.pos, halfLineSpacing, halfLineSpacing);
              QRectF pointBounds(t.pos.x() - halfLineSpacing, t.pos.y() - halfLineSpacing, lineSpacing, lineSpacing);
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
                      if (t.angle != 0.0) {
                          p.save();
                          p.translate(t.pos);
                          p.scale(t.pressure, t.pressure);
                          p.rotate(t.angle);
                          p.drawPath(ellipse);
                          p.restore();
                      } else {
                          p.drawEllipse(t.pos, t.pressure * halfLineSpacing, t.pressure * halfLineSpacing);
                      }
                      p.setPen(Qt::white);
                  } else {
                      p.fillRect(t.pos.x() - 2, t.pos.y() - 2, 4, 4, Qt::black);
                  }
                  break;
              }
          }
    }
    ++m_paintEventCount;
}

void EventReportWidget::tabletEvent(QTabletEvent *event)
{
    QWidget::tabletEvent(event);
    bool isMove = false;
    switch (event->type()) {
    case QEvent::TabletEnterProximity:
    case QEvent::TabletLeaveProximity:
        qDebug() << "proximity" << event;
        break;
    case QEvent::TabletMove:
        m_points.push_back(TabletPoint(event->pos(), TabletMove, m_lastButton, event->pointerType(), event->pressure(), event->rotation()));
        update();
        isMove = true;
        ++m_tabletMoveCount;
        break;
    case QEvent::TabletPress:
        m_points.push_back(TabletPoint(event->pos(), TabletButtonPress, event->button(), event->pointerType(), event->rotation()));
        m_lastButton = event->button();
        update();
        break;
    case QEvent::TabletRelease:
        m_points.push_back(TabletPoint(event->pos(), TabletButtonRelease, event->button(), event->pointerType(), event->rotation()));
        update();
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    if (!(isMove && m_lastIsTabletMove)) {
        QDebug d = qDebug();
        d << event << " global position = " << event->globalPos()
                   << " cursor at " << QCursor::pos();
        if (event->button() != Qt::NoButton)
            d << " changed button " << event->button();
    }
    m_lastIsTabletMove = isMove;
}

void EventReportWidget::outputMouseEvent(QMouseEvent *event)
{
    if (event->type() == QEvent::MouseMove)  {
        if (m_lastIsMouseMove)
            return; // only show one move to keep things readable
        m_lastIsMouseMove = true;
    }
    qDebug() << event;
}

void EventReportWidget::timerEvent(QTimerEvent *)
{
    emit stats(QString("%1 moves/sec, %2 frames/sec").arg(m_tabletMoveCount).arg(m_paintEventCount));
    m_tabletMoveCount = 0;
    m_paintEventCount = 0;
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
    QObject::connect(widget, SIGNAL(stats(QString)), mainWindow.statusBar(), SLOT(showMessage(QString)));
    QAction *quitAction = fileMenu->addAction("Quit");
    QObject::connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    quitAction->setShortcut(Qt::CTRL + Qt::Key_Q);
    mainWindow.setCentralWidget(widget);
    mainWindow.show();
    return app.exec();
}

#include "main.moc"
