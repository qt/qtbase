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

#include "controllerwidget.h"
#include <controls.h>

#if QT_VERSION >= 0x050000
#    include <QtWidgets>
#    include <QWindow>
#    include <QBackingStore>
#    include <QPaintDevice>
#    include <QPainter>
#else
#    include <QtGui>
#endif

#include <QResizeEvent>

CoordinateControl::CoordinateControl(const QString &sep) : m_x(new QSpinBox), m_y(new QSpinBox)
{
    m_x->setMinimum(0);
    m_x->setMaximum(2000);
    connect(m_x, SIGNAL(valueChanged(int)), this, SLOT(spinBoxChanged()));
    m_y->setMinimum(0);
    m_y->setMaximum(2000);
    connect(m_y, SIGNAL(valueChanged(int)), this, SLOT(spinBoxChanged()));
    QHBoxLayout *l = new QHBoxLayout(this);
    l->setSpacing(2);
    l->addWidget(m_x);
    l->addWidget(new QLabel(sep));
    l->addWidget(m_y);
}

void CoordinateControl::setCoordinates(int x, int y)
{
    m_x->blockSignals(true);
    m_y->blockSignals(true);
    m_x->setValue(x);
    m_y->setValue(y);
    m_x->blockSignals(false);
    m_y->blockSignals(false);
}

QPair<int, int> CoordinateControl::coordinates() const
{
    return QPair<int, int>(m_x->value(), m_y->value());
}

void CoordinateControl::spinBoxChanged()
{
    const int x = m_x->value();
    const int y = m_y->value();
    emit pointValueChanged(QPoint(x, y));
    emit sizeValueChanged(QSize(x, y));
}

RectControl::RectControl()
    : m_point(new CoordinateControl(QLatin1String("+")))
    , m_size(new CoordinateControl(QLatin1String("x")))
{
    QHBoxLayout *l = new QHBoxLayout(this);
    l->setSpacing(0);
    l->setMargin(ControlLayoutMargin);
    connect(m_point, SIGNAL(pointValueChanged(QPoint)), this, SLOT(handleChanged()));
    connect(m_point, SIGNAL(pointValueChanged(QPoint)), this, SIGNAL(positionChanged(QPoint)));
    l->addWidget(m_point);
    l->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Ignored));
    connect(m_size, SIGNAL(sizeValueChanged(QSize)), this, SLOT(handleChanged()));
    connect(m_size, SIGNAL(sizeValueChanged(QSize)), this, SIGNAL(sizeChanged(QSize)));
    l->addWidget(m_size);
}

void RectControl::setRectValue(const QRect &r)
{
    m_point->setPointValue(r.topLeft());
    m_size->setSizeValue(r.size());
}

QRect RectControl::rectValue() const
{
    return QRect(m_point->pointValue(), m_size->sizeValue());
}

void RectControl::handleChanged()
{
    emit changed(rectValue());
}

BaseWindowControl::BaseWindowControl(QObject *w)
    : m_layout(new QGridLayout(this))
    , m_object(w)
    , m_geometry(new RectControl)
    , m_framePosition(new CoordinateControl(QLatin1String("x")))
    , m_typeControl(new TypeControl)
    , m_hintControl(new HintControl)
    , m_moveEventLabel(new QLabel(tr("Move events")))
    , m_resizeEventLabel(new QLabel(tr("Resize events")))
    , m_mouseEventLabel(new QLabel(tr("Mouse events")))
    , m_moveCount(0)
    , m_resizeCount(0)
{
    m_object->installEventFilter(this);
    m_geometry->setTitle(tr("Geometry"));
    int row = 0;
    m_layout->addWidget(m_geometry, row, 0, 1, 2);
    m_layout->setMargin(ControlLayoutMargin);
    QGroupBox *frameGB = new QGroupBox(tr("Frame"));
    QVBoxLayout *frameL = new QVBoxLayout(frameGB);
    frameL->setSpacing(0);
    frameL->setMargin(ControlLayoutMargin);
    frameL->addWidget(m_framePosition);
    m_layout->addWidget(frameGB, row, 2);

    m_layout->addWidget(m_hintControl, ++row, 0, 1, 2);
    connect(m_hintControl, SIGNAL(changed(Qt::WindowFlags)), this, SLOT(windowFlagsChanged()));
    m_layout->addWidget(m_typeControl, row, 2);
    connect(m_typeControl, SIGNAL(changed(Qt::WindowFlags)), this, SLOT(windowFlagsChanged()));

    QGroupBox *eventGroupBox = new QGroupBox(tr("Events"));
    QVBoxLayout *l = new QVBoxLayout(eventGroupBox);
    l->setSpacing(0);
    l->setMargin(ControlLayoutMargin);
    l->addWidget(m_moveEventLabel);
    l->addWidget(m_resizeEventLabel);
    l->addWidget(m_mouseEventLabel);
    m_layout->addWidget(eventGroupBox, ++row, 2);

    connect(m_geometry, SIGNAL(positionChanged(QPoint)), this, SLOT(posChanged(QPoint)));
    connect(m_geometry, SIGNAL(sizeChanged(QSize)), this, SLOT(sizeChanged(QSize)));
    connect(m_framePosition, SIGNAL(pointValueChanged(QPoint)), this, SLOT(framePosChanged(QPoint)));
}

bool BaseWindowControl::eventFilter(QObject *, QEvent *e)
{
    switch (e->type()) {
    case QEvent::Resize: {
        const QResizeEvent *re = static_cast<const QResizeEvent *>(e);
        m_resizeEventLabel->setText(tr("Resize %1x%2 (#%3)")
                                    .arg(re->size().width()).arg(re->size().height())
                                    .arg(++m_resizeCount));
        refresh();
    }
        break;
    case QEvent::Move: {
        const QMoveEvent *me = static_cast<const QMoveEvent *>(e);
        m_moveEventLabel->setText(tr("Move %1,%2 (#%3)")
                                  .arg(me->pos().x()).arg(me->pos().y())
                                  .arg(++m_moveCount));
        refresh();
    }
        break;
    case QEvent::MouseMove: {
        const QMouseEvent *me = static_cast<const QMouseEvent *>(e);
        const QPoint pos = me->pos();
        QPoint globalPos = objectMapToGlobal(m_object, pos);
        m_mouseEventLabel->setText(tr("Mouse: %1,%2 Global: %3,%4 ").
                                   arg(pos.x()).arg(pos.y()).arg(globalPos.x()).arg(globalPos.y()));
    }
        break;
    case QEvent::WindowStateChange:
        refresh();
    default:
        break;
    }
    return false;
}

void BaseWindowControl::posChanged(const QPoint &p)
{
    QRect geom = objectGeometry(m_object);
    geom.moveTopLeft(p);
    setObjectGeometry(m_object, geom);
}

void BaseWindowControl::sizeChanged(const QSize &s)
{
    QRect geom = objectGeometry(m_object);
    geom.setSize(s);
    setObjectGeometry(m_object, geom);
}

void BaseWindowControl::framePosChanged(const QPoint &p)
{
    setObjectFramePosition(m_object, p);
}

void BaseWindowControl::windowFlagsChanged()
{
    const Qt::WindowFlags f = m_typeControl->type() | m_hintControl->hints();
    setObjectWindowFlags(m_object, f);
}

void BaseWindowControl::refresh()
{
    m_geometry->setRectValue(objectGeometry(m_object));
    m_framePosition->setPointValue(objectFramePosition(m_object));
    const Qt::WindowFlags flags = objectWindowFlags(m_object);
    m_typeControl->setType(flags);
    m_hintControl->setHints(flags);
}

// A control for a QWidget
class WidgetWindowControl : public BaseWindowControl
{
    Q_OBJECT
public:
    explicit WidgetWindowControl(QWidget *w);

    virtual void refresh();

private slots:
    void statesChanged();

private:
    virtual QRect objectGeometry(const QObject *o) const
        { return static_cast<const QWidget *>(o)->geometry(); }
    virtual void setObjectGeometry(QObject *o, const QRect &r) const
        { static_cast<QWidget *>(o)->setGeometry(r); }
    virtual QPoint objectFramePosition(const QObject *o) const
        { return static_cast<const QWidget *>(o)->pos(); }
    virtual void setObjectFramePosition(QObject *o, const QPoint &p) const
        { static_cast<QWidget *>(o)->move(p); }
    virtual QPoint objectMapToGlobal(const QObject *o, const QPoint &p) const
        { return static_cast<const QWidget *>(o)->mapToGlobal(p); }
    virtual Qt::WindowFlags objectWindowFlags(const QObject *o) const
        { return static_cast<const QWidget *>(o)->windowFlags(); }
    virtual void setObjectWindowFlags(QObject *o, Qt::WindowFlags f);

    WindowStatesControl *m_statesControl;
};

WidgetWindowControl::WidgetWindowControl(QWidget *w )
    : BaseWindowControl(w)
    , m_statesControl(new WindowStatesControl(WindowStatesControl::WantVisibleCheckBox | WindowStatesControl::WantActiveCheckBox))
{
    setTitle(w->windowTitle());
    m_layout->addWidget(m_statesControl, 2, 0);
    connect(m_statesControl, SIGNAL(changed()), this, SLOT(statesChanged()));
}

void WidgetWindowControl::setObjectWindowFlags(QObject *o, Qt::WindowFlags f)
{
    QWidget *w = static_cast<QWidget *>(o);
    const bool visible = w->isVisible();
    w->setWindowFlags(f); // hides.
    if (visible)
        w->show();
}

void WidgetWindowControl::refresh()
{
    const QWidget *w = static_cast<const QWidget *>(m_object);
    m_statesControl->setVisibleValue(w->isVisible());
    m_statesControl->setStates(w->windowState());
    BaseWindowControl::refresh();
}

void WidgetWindowControl::statesChanged()
{
    QWidget *w = static_cast<QWidget *>(m_object);
    w->setVisible(m_statesControl->visibleValue());
    w->setWindowState(m_statesControl->states());
}

#if QT_VERSION >= 0x050000

// Test window drawing diagonal lines
class Window : public QWindow
{
public:
    explicit Window(QWindow *parent = 0)
        : QWindow(parent)
        , m_backingStore(new QBackingStore(this))
    {
        setObjectName(QStringLiteral("window"));
        setTitle(tr("TestWindow"));
    }

protected:
    void exposeEvent(QExposeEvent *)
        { render(); }

private:
    QBackingStore *m_backingStore;
    void render();
};

void Window::render()
{
    QRect rect(QPoint(), geometry().size());
    m_backingStore->resize(rect.size());
    m_backingStore->beginPaint(rect);
    if (!rect.size().isEmpty()) {
        QPaintDevice *device = m_backingStore->paintDevice();
        QPainter p(device);
        p.fillRect(rect, Qt::white);
        p.drawLine(0, 0, rect.width(), rect.height());
        p.drawLine(0, rect.height(), rect.width(), 0);
    }
    m_backingStore->endPaint();
    m_backingStore->flush(rect);
}

// A control for a QWindow
class WindowControl : public BaseWindowControl
{
    Q_OBJECT
public:
    explicit WindowControl(QWindow *w);

    virtual void refresh();

private slots:
    void stateChanged();

private:
    virtual QRect objectGeometry(const QObject *o) const
        { return static_cast<const QWindow *>(o)->geometry(); }
    virtual void setObjectGeometry(QObject *o, const QRect &r) const
        { static_cast<QWindow *>(o)->setGeometry(r); }
    virtual QPoint objectFramePosition(const QObject *o) const
        { return static_cast<const QWindow *>(o)->framePosition(); }
    virtual void setObjectFramePosition(QObject *o, const QPoint &p) const
        { static_cast<QWindow *>(o)->setFramePosition(p); }
    virtual QPoint objectMapToGlobal(const QObject *o, const QPoint &p) const
        { return static_cast<const QWindow *>(o)->mapToGlobal(p); }
    virtual Qt::WindowFlags objectWindowFlags(const QObject *o) const
        { return static_cast<const QWindow *>(o)->flags(); }
    virtual void setObjectWindowFlags(QObject *o, Qt::WindowFlags f)
        { static_cast<QWindow *>(o)->setFlags(f); }

    WindowStateControl *m_stateControl;
};

WindowControl::WindowControl(QWindow *w )
    : BaseWindowControl(w)
    , m_stateControl(new WindowStateControl(WindowStateControl::WantVisibleCheckBox | WindowStateControl::WantMinimizeRadioButton))
{
    setTitle(w->title());
    QGroupBox *stateGroupBox = new QGroupBox(tr("State"));
    QVBoxLayout *l = new QVBoxLayout(stateGroupBox);
    l->addWidget(m_stateControl);
    m_layout->addWidget(stateGroupBox, 2, 0);
    connect(m_stateControl, SIGNAL(changed()), this, SLOT(stateChanged()));
}

void WindowControl::refresh()
{
    const QWindow *w = static_cast<const QWindow *>(m_object);
    BaseWindowControl::refresh();
    m_stateControl->setVisibleValue(w->isVisible());
    m_stateControl->setState(w->windowState());
}

void WindowControl::stateChanged()
{
    QWindow *w = static_cast<QWindow *>(m_object);
    w->setVisible(m_stateControl->visibleValue());
    w->setWindowState(m_stateControl->state());
}

#endif

ControllerWidget::ControllerWidget(QWidget *parent)
    : QMainWindow(parent)
    , m_testWidget(new QWidget)
#if QT_VERSION >= 0x050000
    , m_testWindow(new Window)
#endif
{
    QMenu *fileMenu = menuBar()->addMenu(tr("File"));
    QAction *exitAction = fileMenu->addAction(tr("Exit"));
    exitAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
    connect(exitAction, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));

    QString title = QLatin1String("Geometry test, (Qt ");
    title += QLatin1String(QT_VERSION_STR);
#if QT_VERSION >= 0x050000
    title += QLatin1String(", ");
    title += qApp->platformName();
#endif
    title += QLatin1Char(')');
    setWindowTitle(title);

    int x = 100;
    int y = 100;
    const QStringList args = QApplication::arguments();
    const int offsetArgIndex = args.indexOf(QLatin1String("-offset"));
    if (offsetArgIndex >=0 && offsetArgIndex < args.size() - 1) {
        y += args.at(offsetArgIndex + 1).toInt();
    } else {
        if (QT_VERSION < 0x050000)
            y += 400;
    }

    move(x, y);

    x += 800;
    m_testWidget->setWindowTitle(tr("TestWidget"));
    if (args.contains(QLatin1String("-layout"))) {
        QVBoxLayout *layout = new QVBoxLayout(m_testWidget.data());
        QLabel *label = new QLabel("Hallo");
        layout->addWidget(label);
    }
    m_testWidget->move(x, y);
    m_testWidget->resize(200, 200);

    if (args.contains(QLatin1String("-widgetminimized"), Qt::CaseInsensitive))
        m_testWidget->showMinimized();
    else if (args.contains(QLatin1String("-widgetmaximized"), Qt::CaseInsensitive))
        m_testWidget->showMaximized();
    else if (args.contains(QLatin1String("-widgetfullscreen"), Qt::CaseInsensitive))
        m_testWidget->showFullScreen();
    else
        m_testWidget->show();

#if QT_VERSION >= 0x050000
    x += 300;
    m_testWindow->setFlags(Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint
                                 | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint
                                 | Qt::WindowTitleHint | Qt::WindowFullscreenButtonHint);
    m_testWindow->setFramePosition(QPoint(x, y));
    m_testWindow->resize(200, 200);
    if (args.contains(QLatin1String("-windowminimized"), Qt::CaseInsensitive))
        m_testWindow->showMinimized();
    else if (args.contains(QLatin1String("-windowmaximized"), Qt::CaseInsensitive))
        m_testWindow->showMaximized();
    else if (args.contains(QLatin1String("-windowfullscreen"), Qt::CaseInsensitive))
        m_testWindow->showFullScreen();
    else
        m_testWindow->show();
    m_testWindow->setTitle(tr("TestWindow"));
#endif

    QWidget *central = new QWidget ;
    QVBoxLayout *l = new QVBoxLayout(central);

    const QString labelText = tr(
        "<html><head/><body><p>This example lets you control the geometry"
        " of a QWidget and a QWindow (Qt 5) for testing out"
        " QPA plugins.</p>"
        "<p>It compiles with Qt 4 and Qt 5 for comparison.</p>"
        "<p>The command line option <code>-offset &lt;value&gt;</code> specifies"
        " a vertical offset.</p></body></html>");

    l->addWidget(new QLabel(labelText));

    BaseWindowControl *widgetControl = new WidgetWindowControl(m_testWidget.data());
    widgetControl->refresh();
    l->addWidget(widgetControl);

#if QT_VERSION >= 0x050000
    BaseWindowControl *windowControl = new WindowControl(m_testWindow.data());
    windowControl->refresh();
    l->addWidget(windowControl);
#endif

    setCentralWidget(central);
}

ControllerWidget::~ControllerWidget()
{
}

#include "controllerwidget.moc"
