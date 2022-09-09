// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMWINDOW_H
#define QWASMWINDOW_H

#include "qwasmintegration.h"
#include <qpa/qplatformwindow.h>
#include <emscripten/html5.h>
#include "qwasmbackingstore.h"
#include "qwasmscreen.h"
#include "qwasmcompositor.h"

QT_BEGIN_NAMESPACE

class QWasmWindow : public QPlatformWindow
{
public:
    enum TitleBarControl {
        SC_None = 0x00000000,
        SC_TitleBarSysMenu = 0x00000001,
        SC_TitleBarMaxButton = 0x00000002,
        SC_TitleBarCloseButton = 0x00000004,
        SC_TitleBarNormalButton = 0x00000008,
        SC_TitleBarLabel = 0x00000010
    };
    Q_DECLARE_FLAGS(TitleBarControls, TitleBarControl);

    QWasmWindow(QWindow *w, QWasmCompositor *compositor, QWasmBackingStore *backingStore);
    ~QWasmWindow();
    void destroy();

    void initialize() override;

    void setGeometry(const QRect &) override;
    void setVisible(bool visible) override;
    bool isVisible();
    QMargins frameMargins() const override;

    WId winId() const override;

    void propagateSizeHints() override;
    void raise() override;
    void lower() override;
    QRect normalGeometry() const override;
    qreal devicePixelRatio() const override;
    void requestUpdate() override;
    void requestActivateWindow() override;

    QWasmScreen *platformScreen() const;
    void setBackingStore(QWasmBackingStore *store) { m_backingStore = store; }
    QWasmBackingStore *backingStore() const { return m_backingStore; }
    QWindow *window() const { return m_window; }

    void injectMousePressed(const QPoint &local, const QPoint &global,
                            Qt::MouseButton button, Qt::KeyboardModifiers mods);
    void injectMouseReleased(const QPoint &local, const QPoint &global,
                            Qt::MouseButton button, Qt::KeyboardModifiers mods);
    bool startSystemResize(Qt::Edges edges) final;

    bool isPointOnTitle(QPoint point) const;
    bool isPointOnResizeRegion(QPoint point) const;

    Qt::Edges resizeEdgesAtPoint(QPoint point) const;

    void setWindowState(Qt::WindowStates state) override;
    void applyWindowState();
    bool setKeyboardGrabEnabled(bool) override { return false; }
    bool setMouseGrabEnabled(bool grab) final;

    void drawTitleBar(QPainter *painter) const;

protected:
    void invalidate();
    bool hasTitleBar() const;

private:
    friend class QWasmScreen;

    struct TitleBarOptions
    {
        bool hasControl(TitleBarControl control) const;

        QRect rect;
        Qt::WindowFlags flags;
        int state;
        QPalette palette;
        QString titleBarOptionsString;
        TitleBarControls subControls;
        QIcon windowIcon;
    };

    TitleBarOptions makeTitleBarOptions() const;
    std::optional<QRect> getTitleBarControlRect(const TitleBarOptions &tb,
                                                TitleBarControl control) const;
    std::optional<QRect> getTitleBarControlRectLeftToRight(const TitleBarOptions &tb,
                                                           TitleBarControl control) const;
    QRegion titleControlRegion() const;
    QRegion titleGeometry() const;
    int borderWidth() const;
    int titleHeight() const;
    QRegion resizeRegion() const;
    TitleBarControl activeTitleBarControl() const;
    std::optional<TitleBarControl> titleBarHitTest(const QPoint &globalPoint) const;

    QWindow *m_window = nullptr;
    QWasmCompositor *m_compositor = nullptr;
    QWasmBackingStore *m_backingStore = nullptr;
    QRect m_normalGeometry {0, 0, 0 ,0};

    Qt::WindowStates m_windowState = Qt::WindowNoState;
    Qt::WindowStates m_previousWindowState = Qt::WindowNoState;
    TitleBarControl m_activeControl = SC_None;
    WId m_winid = 0;
    bool m_hasTitle = false;
    bool m_needsCompositor = false;
    long m_requestAnimationFrameId = -1;
    friend class QWasmCompositor;
    friend class QWasmEventTranslator;
    bool windowIsPopupType(Qt::WindowFlags flags) const;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QWasmWindow::TitleBarControls);
QT_END_NAMESPACE
#endif // QWASMWINDOW_H
