/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwasmscreen.h"
#include "qwasmwindow.h"
#include "qwasmeventtranslator.h"
#include "qwasmcompositor.h"
#include "qwasmintegration.h"
#include "qwasmstring.h"

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <QtEglSupport/private/qeglconvenience_p.h>
#ifndef QT_NO_OPENGL
# include <QtEglSupport/private/qeglplatformcontext_p.h>
#endif
#include <qpa/qwindowsysteminterface.h>
#include <QtCore/qcoreapplication.h>
#include <QtGui/qguiapplication.h>
#include <private/qhighdpiscaling_p.h>

using namespace emscripten;

QT_BEGIN_NAMESPACE

QWasmScreen::QWasmScreen(const QString &canvasId)
    : m_canvasId(canvasId)

{
    m_compositor = new QWasmCompositor(this);
    m_eventTranslator = new QWasmEventTranslator(this);
    updateQScreenAndCanvasRenderSize();
}

QWasmScreen::~QWasmScreen()
{
    destroy();
}

void QWasmScreen::destroy()
{
    m_compositor->destroy();
}

QWasmScreen *QWasmScreen::get(QPlatformScreen *screen)
{
    return static_cast<QWasmScreen *>(screen);
}

QWasmScreen *QWasmScreen::get(QScreen *screen)
{
    return get(screen->handle());
}

QWasmCompositor *QWasmScreen::compositor()
{
    return m_compositor;
}

QWasmEventTranslator *QWasmScreen::eventTranslator()
{
    return m_eventTranslator;
}

QString QWasmScreen::canvasId() const
{
    return m_canvasId;
}

QRect QWasmScreen::geometry() const
{
    return m_geometry;
}

int QWasmScreen::depth() const
{
    return m_depth;
}

QImage::Format QWasmScreen::format() const
{
    return m_format;
}

QDpi QWasmScreen::logicalDpi() const
{
    emscripten::val dpi = emscripten::val::module_property("qtFontDpi");
    if (!dpi.isUndefined()) {
        qreal dpiValue = dpi.as<qreal>();
        return QDpi(dpiValue, dpiValue);
    }
    const qreal defaultDpi = 96;
    return QDpi(defaultDpi, defaultDpi);
}

qreal QWasmScreen::devicePixelRatio() const
{
    // FIXME: The effective device pixel ratio may be different from the
    // HTML window dpr if the OpenGL driver/GPU allocates a less than
    // full resolution surface. Use emscripten_webgl_get_drawing_buffer_size()
    // and compute the dpr instead.
    double htmlWindowDpr = emscripten::val::global("window")["devicePixelRatio"].as<double>();
    return qreal(htmlWindowDpr);
}

QString QWasmScreen::name() const
{
    return m_canvasId;
}

QPlatformCursor *QWasmScreen::cursor() const
{
    return const_cast<QWasmCursor *>(&m_cursor);
}

void QWasmScreen::resizeMaximizedWindows()
{
    if (!screen())
        return;
    QPlatformScreen::resizeMaximizedWindows();
}

QWindow *QWasmScreen::topWindow() const
{
    return m_compositor->keyWindow();
}

QWindow *QWasmScreen::topLevelAt(const QPoint &p) const
{
    return m_compositor->windowAt(p);
}

void QWasmScreen::invalidateSize()
{
    m_geometry = QRect();
}

void QWasmScreen::setGeometry(const QRect &rect)
{
    m_geometry = rect;
    QWindowSystemInterface::handleScreenGeometryChange(QPlatformScreen::screen(), geometry(), availableGeometry());
    resizeMaximizedWindows();
}

void QWasmScreen::updateQScreenAndCanvasRenderSize()
{
    // The HTML canvas has two sizes: the CSS size and the canvas render size.
    // The CSS size is determined according to standard CSS rules, while the
    // render size is set using the "width" and "height" attributes. The render
    // size must be set manually and is not auto-updated on CSS size change.
    // Setting the render size to a value larger than the CSS size enables high-dpi
    // rendering.

    QByteArray canvasId = m_canvasId.toUtf8();
    double css_width;
    double css_height;
    emscripten_get_element_css_size(canvasId.constData(), &css_width, &css_height);
    QSizeF cssSize(css_width, css_height);

    QSizeF canvasSize = cssSize * devicePixelRatio();
    val document = val::global("document");
    val canvas = document.call<val>("getElementById", QWasmString::fromQString(m_canvasId));

    canvas.set("width", canvasSize.width());
    canvas.set("height", canvasSize.height());

    QPoint offset;
    offset.setX(canvas["offsetTop"].as<int>());
    offset.setY(canvas["offsetLeft"].as<int>());

    emscripten::val rect = canvas.call<emscripten::val>("getBoundingClientRect");
    QPoint position(rect["left"].as<int>() - offset.x(), rect["top"].as<int>() - offset.y());

    setGeometry(QRect(position, cssSize.toSize()));
    m_compositor->redrawWindowContent();
}

QT_END_NAMESPACE
