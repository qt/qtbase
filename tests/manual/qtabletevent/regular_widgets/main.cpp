// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QAction>
#include <QApplication>
#include <QCursor>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QList>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QPointingDevice>
#include <QPointer>
#include <QPushButton>
#include <QStatusBar>
#include <QTabletEvent>
#include <QVBoxLayout>

#ifdef Q_OS_WIN
#  include <QtGui/private/qguiapplication_p.h>
#  include <QtGui/qpa/qplatformintegration.h>
#endif

enum TabletPointType {
    TabletButtonPress,
    TabletButtonRelease,
    TabletMove
};

#ifdef Q_OS_WIN
using QWindowsApplication = QNativeInterface::Private::QWindowsApplication;

static void setWinTabEnabled(bool e)
{
    if (auto nativeWindowsApp = dynamic_cast<QWindowsApplication *>(QGuiApplicationPrivate::platformIntegration()))
        nativeWindowsApp->setWinTabEnabled(e);
}

static bool isWinTabEnabled()
{
    auto nativeWindowsApp = dynamic_cast<QWindowsApplication *>(QGuiApplicationPrivate::platformIntegration());
    return nativeWindowsApp && nativeWindowsApp->isWinTabEnabled();
}
#endif // Q_OS_WIN

struct TabletPoint
{
    TabletPoint(const QPointF &p = QPointF(), TabletPointType t = TabletMove, Qt::MouseButton b = Qt::LeftButton,
                QPointingDevice::PointerType pt = QPointingDevice::PointerType::Unknown, qreal prs = 0, qreal rotation = 0) :
        pos(p), type(t), button(b), ptype(pt), pressure(prs), angle(rotation) {}

    QPointF pos;
    TabletPointType type;
    Qt::MouseButton button;
    QPointingDevice::PointerType ptype;
    qreal pressure;
    qreal angle;
};

class ProximityEventFilter : public QObject
{
    Q_OBJECT
public:
    explicit ProximityEventFilter(QObject *parent) : QObject(parent) { }

    bool eventFilter(QObject *, QEvent *event) override;

    static bool tabletPenProximity() { return m_tabletPenProximity; }

signals:
    void proximityChanged();

private:
    static bool m_tabletPenProximity;
};

bool ProximityEventFilter::eventFilter(QObject *, QEvent *event)
{
    switch (event->type()) {
    case QEvent::TabletEnterProximity:
    case QEvent::TabletLeaveProximity:
        ProximityEventFilter::m_tabletPenProximity = event->type() == QEvent::TabletEnterProximity;
        emit proximityChanged();
        qDebug() << event;
        break;
    default:
        break;
    }
    return false;
}

bool ProximityEventFilter::m_tabletPenProximity = false;

class EventReportWidget : public QWidget
{
    Q_OBJECT
public:
    EventReportWidget();

public slots:
    void clearPoints() { m_points.clear(); update(); }

signals:
    void stats(QString s, int timeOut = 0);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override { outputMouseEvent(event); }
    void mouseMoveEvent(QMouseEvent *event) override { outputMouseEvent(event); }
    void mousePressEvent(QMouseEvent *event) override { outputMouseEvent(event); }
    void mouseReleaseEvent(QMouseEvent *event) override { outputMouseEvent(event); }

    void tabletEvent(QTabletEvent *) override;

    bool event(QEvent *event) override;

    void paintEvent(QPaintEvent *) override;
    void timerEvent(QTimerEvent *) override;

private:
    void outputMouseEvent(QMouseEvent *event);

    bool m_lastIsMouseMove = false;
    bool m_lastIsTabletMove = false;
    Qt::MouseButton m_lastButton = Qt::NoButton;
    QList<TabletPoint> m_points;
    QList<QPointF> m_touchPoints;
    QPointF m_tabletPos;
    int m_tabletMoveCount = 0;
    int m_paintEventCount = 0;
};

EventReportWidget::EventReportWidget()
{
    setAttribute(Qt::WA_AcceptTouchEvents);
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
    for (const TabletPoint &t : std::as_const(m_points)) {
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
                      p.setPen(t.ptype == QPointingDevice::PointerType::Eraser ? Qt::red : Qt::black);
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

    // Draw haircross when tablet pen is in proximity
    if (ProximityEventFilter::tabletPenProximity() && geom.contains(m_tabletPos)) {
        p.setPen(Qt::black);
        p.drawLine(QPointF(0, m_tabletPos.y()), QPointF(geom.width(), m_tabletPos.y()));
        p.drawLine(QPointF(m_tabletPos.x(), 0), QPointF(m_tabletPos.x(), geom.height()));
    }
    p.setPen(Qt::blue);
    for (QPointF t : m_touchPoints) {
        p.drawLine(t.x() - 40, t.y(), t.x() + 40, t.y());
        p.drawLine(t.x(), t.y() - 40, t.x(), t.y() + 40);
    }
    ++m_paintEventCount;
}

void EventReportWidget::tabletEvent(QTabletEvent *event)
{
    QWidget::tabletEvent(event);
    bool isMove = false;
    m_tabletPos = event->position();
    switch (event->type()) {
    case QEvent::TabletMove:
        m_points.push_back(TabletPoint(m_tabletPos, TabletMove, m_lastButton, event->pointerType(), event->pressure(), event->rotation()));
        update();
        isMove = true;
        ++m_tabletMoveCount;
        break;
    case QEvent::TabletPress:
        m_points.push_back(TabletPoint(m_tabletPos, TabletButtonPress, event->button(), event->pointerType(), event->rotation()));
        m_lastButton = event->button();
        update();
        break;
    case QEvent::TabletRelease:
        m_points.push_back(TabletPoint(m_tabletPos, TabletButtonRelease, event->button(), event->pointerType(), event->rotation()));
        update();
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    if (!(isMove && m_lastIsTabletMove)) {
        QDebug d = qDebug();
        d << event << " global position = " << event->globalPosition()
                   << " cursor at " << QCursor::pos();
        if (event->button() != Qt::NoButton)
            d << " changed button " << event->button();
    }
    m_lastIsTabletMove = isMove;
}

bool EventReportWidget::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
        event->accept();
        m_touchPoints.clear();
        for (const QEventPoint &p : static_cast<const QPointerEvent *>(event)->points())
            m_touchPoints.append(p.position());
        update();
        break;
    case QEvent::TouchEnd:
        m_touchPoints.clear();
        update();
        break;
    default:
        return QWidget::event(event);
    }
    return true;
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

class DevicesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DevicesDialog(QWidget *p);

public slots:
    void refresh();

private:
    QPlainTextEdit *m_edit;
};

DevicesDialog::DevicesDialog(QWidget *p) : QDialog(p)
{
    auto layout = new QVBoxLayout(this);
    m_edit = new QPlainTextEdit(this);
    m_edit->setReadOnly(true);
    layout->addWidget(m_edit);
    auto box = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    auto refreshButton = box->addButton("Refresh", QDialogButtonBox::ActionRole);
    connect(refreshButton, &QAbstractButton::clicked, this, &DevicesDialog::refresh);
    layout->addWidget(box);
    setWindowTitle("Devices");
    refresh();
}

void DevicesDialog::refresh()
{
    QString text;
    QDebug d(&text);
    d.noquote();
    d.nospace();
    for (auto device : QInputDevice::devices())
        d << device<< "\n\n";
    m_edit->setPlainText(text);
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(ProximityEventFilter *proximityEventFilter);

public slots:
    void showDevices();

private:
    QPointer<DevicesDialog> m_devicesDialog;
};

MainWindow::MainWindow(ProximityEventFilter *proximityEventFilter)
{
    setWindowTitle(QString::fromLatin1("Tablet Test %1").arg(QT_VERSION_STR));
    auto widget = new EventReportWidget;
    QObject::connect(proximityEventFilter, &ProximityEventFilter::proximityChanged,
                     widget, QOverload<>::of(&QWidget::update));
    widget->setMinimumSize(640, 480);
    auto fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction("Clear", widget, &EventReportWidget::clearPoints);
    auto showAction = fileMenu->addAction("Show Devices", this, &MainWindow::showDevices);
    showAction->setShortcut(Qt::CTRL | Qt::Key_D);
    QObject::connect(widget, &EventReportWidget::stats,
                     statusBar(), &QStatusBar::showMessage);
    QAction *quitAction = fileMenu->addAction("Quit", qApp, &QCoreApplication::quit);
    quitAction->setShortcut(Qt::CTRL | Qt::Key_Q);

    auto settingsMenu = menuBar()->addMenu("Settings");
    auto winTabAction = settingsMenu->addAction("WinTab");
    winTabAction->setCheckable(true);
#ifdef Q_OS_WIN
    winTabAction->setChecked(isWinTabEnabled());
    connect(winTabAction, &QAction::toggled, this, setWinTabEnabled);
#else
    winTabAction->setEnabled(false);
#endif

    setCentralWidget(widget);
}

void MainWindow::showDevices()
{
    if (m_devicesDialog.isNull()) {
        m_devicesDialog = new DevicesDialog(nullptr);
        m_devicesDialog->setModal(false);
        m_devicesDialog->resize(500, 300);
        m_devicesDialog->move(frameGeometry().topRight() + QPoint(20, 0));
        m_devicesDialog->setAttribute(Qt::WA_DeleteOnClose);
    }
    m_devicesDialog->show();
    m_devicesDialog->raise();
    m_devicesDialog->refresh();
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    ProximityEventFilter *proximityEventFilter = new ProximityEventFilter(&app);
    app.installEventFilter(proximityEventFilter);
    MainWindow mainWindow(proximityEventFilter);
    mainWindow.show();
    return app.exec();
}

#include "main.moc"
