// Copyright (C) 2016 Robin Burchell <robin.burchell@viroteck.net>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/QCursor>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPalette>
#include <QtGui/QLinearGradient>
#include <QtGui/QPainterPath>

#include <qpa/qwindowsysteminterface.h>

#include <QtWaylandClient/private/qwaylanddecorationplugin_p.h>
#include <QtWaylandClient/private/qwaylandabstractdecoration_p.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>
#include <QtWaylandClient/private/qwaylandshellsurface_p.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

#define BUTTON_SPACING 5
#define BUTTON_WIDTH 18
#define BUTTONS_RIGHT_MARGIN 8

enum Button
{
    None,
    Close,
    Maximize,
    Minimize
};

class Q_WAYLANDCLIENT_EXPORT QWaylandBradientDecoration : public QWaylandAbstractDecoration
{
public:
    QWaylandBradientDecoration();
protected:
    QMargins margins(MarginsType marginsType = Full) const override;
    void paint(QPaintDevice *device) override;
    bool handleMouse(QWaylandInputDevice *inputDevice, const QPointF &local, const QPointF &global,Qt::MouseButtons b,Qt::KeyboardModifiers mods) override;
    bool handleTouch(QWaylandInputDevice *inputDevice, const QPointF &local, const QPointF &global, QEventPoint::State state, Qt::KeyboardModifiers mods) override;
private:
    enum class PointerType {
        Mouse,
        Touch
    };

    void processPointerTop(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b,Qt::KeyboardModifiers mods, PointerType type);
    void processPointerBottom(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b,Qt::KeyboardModifiers mods, PointerType type);
    void processPointerLeft(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b,Qt::KeyboardModifiers mods, PointerType type);
    void processPointerRight(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b,Qt::KeyboardModifiers mods, PointerType type);
    bool clickButton(Qt::MouseButtons b, Button btn);

    QRectF closeButtonRect() const;
    QRectF maximizeButtonRect() const;
    QRectF minimizeButtonRect() const;

    QStaticText m_windowTitle;
    Button m_clicking = None;
};



QWaylandBradientDecoration::QWaylandBradientDecoration()
{
    QTextOption option(Qt::AlignHCenter | Qt::AlignVCenter);
    option.setWrapMode(QTextOption::NoWrap);
    m_windowTitle.setTextOption(option);
    m_windowTitle.setTextFormat(Qt::PlainText);
}

QRectF QWaylandBradientDecoration::closeButtonRect() const
{
    const int windowRight = waylandWindow()->surfaceSize().width() - margins(ShadowsOnly).right();
    return QRectF(windowRight - BUTTON_WIDTH - BUTTON_SPACING * 0 - BUTTONS_RIGHT_MARGIN,
                  (margins().top() - BUTTON_WIDTH) / 2, BUTTON_WIDTH, BUTTON_WIDTH);
}

QRectF QWaylandBradientDecoration::maximizeButtonRect() const
{
    const int windowRight = waylandWindow()->surfaceSize().width() - margins(ShadowsOnly).right();
    return QRectF(windowRight - BUTTON_WIDTH * 2 - BUTTON_SPACING * 1 - BUTTONS_RIGHT_MARGIN,
                  (margins().top() - BUTTON_WIDTH) / 2, BUTTON_WIDTH, BUTTON_WIDTH);
}

QRectF QWaylandBradientDecoration::minimizeButtonRect() const
{
    const int windowRight = waylandWindow()->surfaceSize().width() - margins(ShadowsOnly).right();
    return QRectF(windowRight - BUTTON_WIDTH * 3 - BUTTON_SPACING * 2 - BUTTONS_RIGHT_MARGIN,
                  (margins().top() - BUTTON_WIDTH) / 2, BUTTON_WIDTH, BUTTON_WIDTH);
}

QMargins QWaylandBradientDecoration::margins(MarginsType marginsType) const
{
    if (marginsType == ShadowsOnly)
        return QMargins();

    return QMargins(3, 30, 3, 3);
}

void QWaylandBradientDecoration::paint(QPaintDevice *device)
{
    bool active = window()->handle()->isActive();
    QRect wg = QRect(QPoint(), waylandWindow()->surfaceSize()).marginsRemoved(margins(ShadowsOnly));
    QRect cg = wg.marginsRemoved(margins(ShadowsExcluded));
    QRect clips[] =
    {
        QRect(wg.left(), wg.top(), wg.width(), margins(ShadowsExcluded).top()),
        QRect(wg.left(), cg.bottom() + 1, wg.width(), margins(ShadowsExcluded).bottom()),
        QRect(wg.left(), cg.top(), margins(ShadowsExcluded).left(), cg.height()),
        QRect(cg.right() + 1, cg.top(), margins(ShadowsExcluded).right(), cg.height())
    };

    QRect top = clips[0];

    QPalette palette;
    const QColor foregroundColor = palette.color(QPalette::Active, QPalette::WindowText);
    const QColor backgroundColor = palette.color(QPalette::Active, QPalette::Window);
    const QColor foregroundInactiveColor = palette.color(QPalette::Disabled, QPalette::WindowText);

    QPainter p(device);
    p.setRenderHint(QPainter::Antialiasing);

    // Title bar
    QPainterPath roundedRect;
    roundedRect.addRoundedRect(wg, 3, 3);
    for (int i = 0; i < 4; ++i) {
        p.save();
        p.setClipRect(clips[i]);
        p.fillPath(roundedRect, backgroundColor);
        p.restore();
    }

    // Window icon
    QIcon icon = waylandWindow()->windowIcon();
    if (!icon.isNull()) {
        QRectF iconRect(0, 0, 22, 22);
        iconRect.adjust(margins().left() + BUTTON_SPACING, 4,
                        margins().left() + BUTTON_SPACING, 4),
        icon.paint(&p, iconRect.toRect());
    }

    // Window title
    QString windowTitleText = waylandWindow()->windowTitle();
    if (!windowTitleText.isEmpty()) {
        if (m_windowTitle.text() != windowTitleText) {
            m_windowTitle.setText(windowTitleText);
            m_windowTitle.prepare();
        }

        QRect titleBar = top;
        titleBar.setLeft(margins().left() + BUTTON_SPACING +
            (icon.isNull() ? 0 : 22 + BUTTON_SPACING));
        titleBar.setRight(minimizeButtonRect().left() - BUTTON_SPACING);

        p.save();
        p.setClipRect(titleBar);
        p.setPen(active ? foregroundColor : foregroundInactiveColor);
        QSizeF size = m_windowTitle.size();
        int dx = (top.width() - size.width()) /2;
        int dy = (top.height()- size.height()) /2;
        QFont font = p.font();
        font.setPixelSize(14);
        p.setFont(font);
        QPoint windowTitlePoint(top.topLeft().x() + dx,
                 top.topLeft().y() + dy);
        p.drawStaticText(windowTitlePoint, m_windowTitle);
        p.restore();
    }

    QRectF rect;

    // Default pen
    QPen pen(active ? foregroundColor : foregroundInactiveColor);
    p.setPen(pen);

    // Close button
    p.save();
    rect = closeButtonRect();
    qreal crossSize = rect.height() / 2.3;
    QPointF crossCenter(rect.center());
    QRectF crossRect(crossCenter.x() - crossSize / 2, crossCenter.y() - crossSize / 2, crossSize, crossSize);
    pen.setWidth(2);
    p.setPen(pen);
    p.drawLine(crossRect.topLeft(), crossRect.bottomRight());
    p.drawLine(crossRect.bottomLeft(), crossRect.topRight());
    p.restore();

    // Maximize button
    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    rect = maximizeButtonRect().adjusted(4, 5, -4, -5);
    if ((window()->windowStates() & Qt::WindowMaximized)) {
        qreal inset = 2;
        QRectF rect1 = rect.adjusted(inset, 0, 0, -inset);
        QRectF rect2 = rect.adjusted(0, inset, -inset, 0);
        p.drawRect(rect1);
        p.setBrush(backgroundColor); // need to cover up some lines from the other rect
        p.drawRect(rect2);
    } else {
        p.drawRect(rect);
        p.drawLine(rect.left(), rect.top() + 1, rect.right(), rect.top() + 1);
    }
    p.restore();

    // Minimize button
    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    rect = minimizeButtonRect().adjusted(5, 5, -5, -5);
    pen.setWidth(2);
    p.setPen(pen);
    p.drawLine(rect.bottomLeft(), rect.bottomRight());
    p.restore();
}

bool QWaylandBradientDecoration::clickButton(Qt::MouseButtons b, Button btn)
{
    if (isLeftClicked(b)) {
        m_clicking = btn;
        return false;
    } else if (isLeftReleased(b)) {
        if (m_clicking == btn) {
            m_clicking = None;
            return true;
        } else {
            m_clicking = None;
        }
    }
    return false;
}

bool QWaylandBradientDecoration::handleMouse(QWaylandInputDevice *inputDevice, const QPointF &local, const QPointF &global, Qt::MouseButtons b, Qt::KeyboardModifiers mods)

{
    Q_UNUSED(global);

    // Figure out what area mouse is in
    QSize ss = waylandWindow()->surfaceSize();
    if (local.y() <= margins().top()) {
        processPointerTop(inputDevice, local, b, mods, PointerType::Mouse);
    } else if (local.y() >= ss.height() - margins().bottom()) {
        processPointerBottom(inputDevice, local, b, mods, PointerType::Mouse);
    } else if (local.x() <= margins().left()) {
        processPointerLeft(inputDevice, local, b, mods, PointerType::Mouse);
    } else if (local.x() >= ss.width() - margins().right()) {
        processPointerRight(inputDevice, local, b, mods, PointerType::Mouse);
    } else {
#if QT_CONFIG(cursor)
        waylandWindow()->restoreMouseCursor(inputDevice);
#endif
        setMouseButtons(b);
        return false;
    }

    setMouseButtons(b);
    return true;
}

bool QWaylandBradientDecoration::handleTouch(QWaylandInputDevice *inputDevice, const QPointF &local, const QPointF &global, QEventPoint::State state, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(global);
    QSize ss = waylandWindow()->surfaceSize();

    bool handled = state == QEventPoint::Pressed;
    if (handled) {
        if (local.y() <= margins().top()) {
            processPointerTop(inputDevice, local, Qt::LeftButton, mods, PointerType::Touch);
        } else if (local.y() >= ss.height() - margins().bottom()) {
            processPointerBottom(inputDevice, local, Qt::LeftButton, mods, PointerType::Touch);
        } else if (local.x() <= margins().left()) {
            processPointerLeft(inputDevice, local, Qt::LeftButton, mods, PointerType::Touch);
        } else if (local.x() >= ss.width() - margins().right()) {
            processPointerRight(inputDevice, local, Qt::LeftButton, mods, PointerType::Touch);
        } else {
            handled = false;
        }
    }

    return handled;
}

void QWaylandBradientDecoration::processPointerTop(QWaylandInputDevice *inputDevice,
                                                 const QPointF &local,
                                                 Qt::MouseButtons b,
                                                 Qt::KeyboardModifiers mods,
                                                 PointerType type)
{
#if !QT_CONFIG(cursor)
    Q_UNUSED(type);
#endif

    QSize ss = waylandWindow()->surfaceSize();
    Q_UNUSED(mods);
    if (local.y() <= margins().bottom()) {
        if (local.x() <= margins().left()) {
            //top left bit
#if QT_CONFIG(cursor)
            if (type == PointerType::Mouse)
                waylandWindow()->applyCursor(inputDevice, Qt::SizeFDiagCursor);
#endif
            startResize(inputDevice, Qt::TopEdge | Qt::LeftEdge, b);
        } else if (local.x() >= ss.width() - margins().right()) {
            //top right bit
#if QT_CONFIG(cursor)
            if (type == PointerType::Mouse)
                waylandWindow()->applyCursor(inputDevice, Qt::SizeBDiagCursor);
#endif
            startResize(inputDevice, Qt::TopEdge | Qt::RightEdge, b);
        } else {
            //top resize bit
#if QT_CONFIG(cursor)
            if (type == PointerType::Mouse)
                waylandWindow()->applyCursor(inputDevice, Qt::SplitVCursor);
#endif
            startResize(inputDevice, Qt::TopEdge, b);
        }
    } else if (local.x() <= margins().left()) {
        processPointerLeft(inputDevice, local, b, mods, type);
    } else if (local.x() >= ss.width() - margins().right()) {
        processPointerRight(inputDevice, local, b, mods, type);
    } else if (isRightClicked(b)) {
        showWindowMenu(inputDevice);
    } else if (closeButtonRect().contains(local)) {
        if (type == PointerType::Touch || clickButton(b, Close))
            QWindowSystemInterface::handleCloseEvent(window());
    } else if (maximizeButtonRect().contains(local)) {
        if (type == PointerType::Touch || clickButton(b, Maximize))
            window()->setWindowStates(window()->windowStates() ^ Qt::WindowMaximized);
    } else if (minimizeButtonRect().contains(local)) {
        if (type == PointerType::Touch || clickButton(b, Minimize))
            window()->setWindowState(Qt::WindowMinimized);
    } else {
#if QT_CONFIG(cursor)
        if (type == PointerType::Mouse)
            waylandWindow()->restoreMouseCursor(inputDevice);
#endif
        startMove(inputDevice,b);
    }
}

void QWaylandBradientDecoration::processPointerBottom(QWaylandInputDevice *inputDevice,
                                                      const QPointF &local,
                                                      Qt::MouseButtons b,
                                                      Qt::KeyboardModifiers mods,
                                                      PointerType type)
{
    Q_UNUSED(mods);
#if !QT_CONFIG(cursor)
    Q_UNUSED(type);
#endif

    QSize ss = waylandWindow()->surfaceSize();
    if (local.x() <= margins().left()) {
        //bottom left bit
#if QT_CONFIG(cursor)
        if (type == PointerType::Mouse)
            waylandWindow()->applyCursor(inputDevice, Qt::SizeBDiagCursor);
#endif
        startResize(inputDevice, Qt::BottomEdge | Qt::LeftEdge, b);
    } else if (local.x() >= ss.width() - margins().right()) {
        //bottom right bit
#if QT_CONFIG(cursor)
        if (type == PointerType::Mouse)
            waylandWindow()->applyCursor(inputDevice, Qt::SizeFDiagCursor);
#endif
        startResize(inputDevice, Qt::BottomEdge | Qt::RightEdge, b);
    } else {
        //bottom bit
#if QT_CONFIG(cursor)
        if (type == PointerType::Mouse)
            waylandWindow()->applyCursor(inputDevice, Qt::SizeVerCursor);
#endif
        startResize(inputDevice, Qt::BottomEdge, b);
    }
}

void QWaylandBradientDecoration::processPointerLeft(QWaylandInputDevice *inputDevice,
                                                    const QPointF &local,
                                                    Qt::MouseButtons b,
                                                    Qt::KeyboardModifiers mods,
                                                    PointerType type)
{
    Q_UNUSED(local);
    Q_UNUSED(mods);
#if QT_CONFIG(cursor)
    if (type == PointerType::Mouse)
        waylandWindow()->applyCursor(inputDevice, Qt::SizeHorCursor);
#else
    Q_UNUSED(type);
#endif
    startResize(inputDevice, Qt::LeftEdge, b);
}

void QWaylandBradientDecoration::processPointerRight(QWaylandInputDevice *inputDevice,
                                                     const QPointF &local,
                                                     Qt::MouseButtons b,
                                                     Qt::KeyboardModifiers mods,
                                                     PointerType type)
{
    Q_UNUSED(local);
    Q_UNUSED(mods);
#if QT_CONFIG(cursor)
    if (type == PointerType::Mouse)
        waylandWindow()->applyCursor(inputDevice, Qt::SizeHorCursor);
#else
    Q_UNUSED(type);
#endif
    startResize(inputDevice, Qt::RightEdge, b);
}

class QWaylandBradientDecorationPlugin : public QWaylandDecorationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QWaylandDecorationFactoryInterface_iid FILE "bradient.json")
public:
    QWaylandAbstractDecoration *create(const QString&, const QStringList&) override;
};

QWaylandAbstractDecoration *QWaylandBradientDecorationPlugin::create(const QString& system, const QStringList& paramList)
{
    Q_UNUSED(paramList);
    Q_UNUSED(system);
    return new QWaylandBradientDecoration();
}

}

QT_END_NAMESPACE

#include "main.moc"
