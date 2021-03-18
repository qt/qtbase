/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINRTSCREEN_H
#define QWINRTSCREEN_H

#include <qpa/qplatformscreen.h>
#include <qpa/qwindowsysteminterface.h>

namespace ABI {
    namespace Windows {
        namespace ApplicationModel {
            struct ISuspendingEventArgs;
        }
        namespace UI {
            namespace Core {
                struct IAutomationProviderRequestedEventArgs;
                struct ICharacterReceivedEventArgs;
                struct ICorePointerRedirector;
                struct ICoreWindow;
                struct ICoreWindowEventArgs;
                struct IKeyEventArgs;
                struct IPointerEventArgs;
                struct IVisibilityChangedEventArgs;
                struct IWindowActivatedEventArgs;
            }
            namespace Xaml {
                struct IDependencyObject;
                struct IWindow;
            }
            namespace ViewManagement {
                struct IApplicationView;
            }
        }
        namespace Graphics {
            namespace Display {
                struct IDisplayInformation;
            }
        }
    }
}
struct IInspectable;

QT_BEGIN_NAMESPACE

class QTouchDevice;
class QWinRTCursor;
class QWinRTInputContext;
class QWinRTScreenPrivate;
class QWinRTWindow;
class QWinRTScreen : public QPlatformScreen
{
public:
    explicit QWinRTScreen();
    ~QWinRTScreen() override;

    QRect geometry() const override;
    QRect availableGeometry() const override;
    int depth() const override;
    QImage::Format format() const override;
    QSizeF physicalSize() const override;
    QDpi logicalDpi() const override;
    qreal scaleFactor() const;
    QPlatformCursor *cursor() const override;
    Qt::KeyboardModifiers keyboardModifiers() const;

    Qt::ScreenOrientation nativeOrientation() const override;
    Qt::ScreenOrientation orientation() const override;

    QWindow *topWindow() const;
    QWindow *windowAt(const QPoint &pos);
    void addWindow(QWindow *window);
    void removeWindow(QWindow *window);
    void raise(QWindow *window);
    void lower(QWindow *window);

    bool setMouseGrabWindow(QWinRTWindow *window, bool grab);
    QWinRTWindow* mouseGrabWindow() const;

    bool setKeyboardGrabWindow(QWinRTWindow *window, bool grab);
    QWinRTWindow* keyboardGrabWindow() const;

    void updateWindowTitle(const QString &title);

    ABI::Windows::UI::Core::ICoreWindow *coreWindow() const;
    ABI::Windows::UI::Xaml::IDependencyObject *canvas() const;

    void initialize();

    void setCursorRect(const QRectF &cursorRect);
    void setKeyboardRect(const QRectF &keyboardRect);

    enum class MousePositionTransition {
        MovedOut,
        MovedIn,
        StayedIn,
        StayedOut
    };

    void emulateMouseMove(const QPointF &point, MousePositionTransition transition);

    void setResizePending();
    bool resizePending() const;

private:
    void handleExpose();

    HRESULT onKeyDown(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IKeyEventArgs *);
    HRESULT onKeyUp(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IKeyEventArgs *);
    HRESULT onCharacterReceived(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::ICharacterReceivedEventArgs *);
    HRESULT onPointerEntered(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IPointerEventArgs *);
    HRESULT onPointerExited(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IPointerEventArgs *);
    HRESULT onPointerUpdated(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IPointerEventArgs *);

    HRESULT onActivated(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IWindowActivatedEventArgs *);

    HRESULT onClosed(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::ICoreWindowEventArgs *);
    HRESULT onVisibilityChanged(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IVisibilityChangedEventArgs *);

    HRESULT onOrientationChanged(ABI::Windows::Graphics::Display::IDisplayInformation *, IInspectable *);
    HRESULT onDpiChanged(ABI::Windows::Graphics::Display::IDisplayInformation *, IInspectable *);
    HRESULT onWindowSizeChanged(ABI::Windows::UI::ViewManagement::IApplicationView *, IInspectable *);
    HRESULT onRedirectReleased(ABI::Windows::UI::Core::ICorePointerRedirector *, ABI::Windows::UI::Core::IPointerEventArgs *);

    QScopedPointer<QWinRTScreenPrivate> d_ptr;
    QRectF mCursorRect;
    Q_DECLARE_PRIVATE(QWinRTScreen)
};

QT_END_NAMESPACE

#endif // QWINRTSCREEN_H
