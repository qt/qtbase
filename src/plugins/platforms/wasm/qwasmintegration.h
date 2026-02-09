// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWASMINTEGRATION_H
#define QWASMINTEGRATION_H

#include "qwasmwindow.h"

#include "qwasminputcontext.h"

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qplatforminputcontext.h>

#include <QtCore/qhash.h>

#include <private/qstdweb_p.h>

#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/val.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QWasmEventTranslator;
class QWasmFontDatabase;
class QWasmWindow;
class QWasmEventDispatcher;
class QWasmScreen;
class QWasmCompositor;
class QWasmBackingStore;
class QWasmClipboard;
class QWasmAccessibility;
class QWasmServices;
class QWasmDrag;
class QWasmSuspendResumeControl;

class QWasmIntegration : public QObject, public QPlatformIntegration
{
    Q_OBJECT
public:
    QWasmIntegration();
    ~QWasmIntegration();

    bool hasCapability(QPlatformIntegration::Capability cap) const override;
    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformWindow *createForeignWindow(QWindow *window, WId nativeHandle) const override;
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
#if QT_CONFIG(clipboard)
    QPlatformClipboard *clipboard() const override;
#endif
#ifndef QT_NO_ACCESSIBILITY
    QPlatformAccessibility *accessibility() const override;
#endif
    void initialize() override;
    QPlatformInputContext *inputContext() const override;
    QWasmInputContext *wasmInputContext() const { return m_wasmInputContext; }

#if QT_CONFIG(draganddrop)
    QPlatformDrag *drag() const override;
#endif

    QWasmClipboard *getWasmClipboard() { return m_clipboard; }
    static QWasmIntegration *get() { return s_instance; }

    void setContainerElements(emscripten::val elementArray);
    void addContainerElement(emscripten::val elementArray);
    void removeContainerElement(emscripten::val elementArray);
    void resizeScreen(const emscripten::val &canvas);
    void updateDpi();
    void resizeAllScreens();
    void loadLocalFontFamilies(emscripten::val families);
    void removeBackingStore(QWindow* window);
    void releaseRequesetUpdateHold();
    static quint64 getTimestamp();

    int touchPoints;

private:
    QWasmWindow *createWindow(QWindow *, WId nativeHandle) const;

    struct ScreenMapping {
        emscripten::val emscriptenVal;
        QWasmScreen *wasmScreen;
    };

    // m_suspendResume should be created first and destroyed early as other fields depend on it
    std::shared_ptr<QWasmSuspendResumeControl> m_suspendResume;

    mutable QWasmFontDatabase *m_fontDb;
    mutable QWasmServices *m_desktopServices;
    mutable QHash<QWindow *, QWasmBackingStore *> m_backingStores;
    QList<ScreenMapping> m_screens;
    mutable QWasmClipboard *m_clipboard;
    mutable QWasmAccessibility *m_accessibility;

    qreal m_fontDpi = -1;
    mutable QScopedPointer<QPlatformInputContext> m_inputContext;
    static QWasmIntegration *s_instance;

    QWasmInputContext *m_wasmInputContext = nullptr;

#if QT_CONFIG(draganddrop)
    std::unique_ptr<QWasmDrag> m_drag;
#endif

};

QT_END_NAMESPACE

#endif // QWASMINTEGRATION_H
