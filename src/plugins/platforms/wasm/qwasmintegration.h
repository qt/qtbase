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

#ifndef QWASMINTEGRATION_H
#define QWASMINTEGRATION_H

#include "qwasmwindow.h"

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qplatforminputcontext.h>

#include <QtCore/qhash.h>

#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/val.h>

QT_BEGIN_NAMESPACE

class QWasmEventTranslator;
class QWasmFontDatabase;
class QWasmWindow;
class QWasmEventDispatcher;
class QWasmScreen;
class QWasmCompositor;
class QWasmBackingStore;
class QWasmClipboard;
class QWasmServices;

class QWasmIntegration : public QObject, public QPlatformIntegration
{
    Q_OBJECT
public:
    QWasmIntegration();
    ~QWasmIntegration();

    bool hasCapability(QPlatformIntegration::Capability cap) const override;
    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;
#ifndef QT_NO_OPENGL
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const override;
#endif
    QPlatformOffscreenSurface *createPlatformOffscreenSurface(QOffscreenSurface *surface) const override;
    QPlatformFontDatabase *fontDatabase() const override;
    QAbstractEventDispatcher *createEventDispatcher() const override;
    QVariant styleHint(QPlatformIntegration::StyleHint hint) const override;
    Qt::WindowState defaultWindowState(Qt::WindowFlags flags) const override;
    QStringList themeNames() const override;
    QPlatformTheme *createPlatformTheme(const QString &name) const override;
    QPlatformServices *services() const override;
    QPlatformClipboard *clipboard() const override;
    void initialize() override;
    QPlatformInputContext *inputContext() const override;

    QWasmClipboard *getWasmClipboard() { return m_clipboard; }

    static QWasmIntegration *get() { return s_instance; }
    static void QWasmBrowserExit();

    void addScreen(const emscripten::val &canvas);
    void removeScreen(const emscripten::val &canvas);
    void resizeScreen(const emscripten::val &canvas);
    void resizeAllScreens();
    void updateDpi();
    void removeBackingStore(QWindow* window);

private:
    mutable QWasmFontDatabase *m_fontDb;
    mutable QWasmServices *m_desktopServices;
    mutable QHash<QWindow *, QWasmBackingStore *> m_backingStores;
    QVector<QPair<emscripten::val, QWasmScreen *>> m_screens;
    mutable QWasmClipboard *m_clipboard;
    qreal m_fontDpi = -1;
    mutable QScopedPointer<QPlatformInputContext> m_inputContext;
    static QWasmIntegration *s_instance;
};

QT_END_NAMESPACE

#endif // QWASMINTEGRATION_H
