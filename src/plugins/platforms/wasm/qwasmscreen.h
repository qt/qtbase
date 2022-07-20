// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMSCREEN_H
#define QWASMSCREEN_H

#include "qwasmcursor.h"

#include <qpa/qplatformscreen.h>

#include <QtCore/qscopedpointer.h>
#include <QtCore/qtextstream.h>
#include <QtCore/private/qstdweb_p.h>

#include <emscripten/val.h>

QT_BEGIN_NAMESPACE

class QPlatformOpenGLContext;
class QWasmWindow;
class QWasmBackingStore;
class QWasmCompositor;
class QWasmEventTranslator;
class QOpenGLContext;

class QWasmScreen : public QObject, public QPlatformScreen
{
    Q_OBJECT
public:
    QWasmScreen(const emscripten::val &containerOrCanvas);
    ~QWasmScreen();
    void deleteScreen();

    static QWasmScreen *get(QPlatformScreen *screen);
    static QWasmScreen *get(QScreen *screen);
    emscripten::val container() const;
    emscripten::val canvas() const;
    QString canvasId() const;
    QString canvasTargetId() const;

    QWasmCompositor *compositor();
    QWasmEventTranslator *eventTranslator();

    QRect geometry() const override;
    int depth() const override;
    QImage::Format format() const override;
    QDpi logicalDpi() const override;
    qreal devicePixelRatio() const override;
    QString name() const override;
    QPlatformCursor *cursor() const override;

    void resizeMaximizedWindows();
    QWindow *topWindow() const;
    QWindow *topLevelAt(const QPoint &p) const override;

    QPoint translateAndClipGlobalPoint(const QPoint &p) const;

    void invalidateSize();
    void updateQScreenAndCanvasRenderSize();
    void installCanvasResizeObserver();
    static void canvasResizeObserverCallback(emscripten::val entries, emscripten::val);

public slots:
    void setGeometry(const QRect &rect);

private:
    std::string canvasSpecialHtmlTargetId() const;
    bool hasSpecialHtmlTargets() const;

    emscripten::val m_container;
    emscripten::val m_canvas;
    std::unique_ptr<QWasmCompositor> m_compositor;
    std::unique_ptr<QWasmEventTranslator> m_eventTranslator;
    QRect m_geometry = QRect(0, 0, 100, 100);
    int m_depth = 32;
    QImage::Format m_format = QImage::Format_RGB32;
    QWasmCursor m_cursor;
    static const char * m_canvasResizeObserverCallbackContextPropertyName;
    std::unique_ptr<qstdweb::EventCallback> m_onContextMenu;
};

QT_END_NAMESPACE
#endif // QWASMSCREEN_H
