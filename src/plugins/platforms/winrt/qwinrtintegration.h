/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINRTINTEGRATION_H
#define QWINRTINTEGRATION_H

#include <qpa/qplatformintegration.h>

namespace ABI {
    namespace Windows {
        namespace ApplicationModel {
            struct ISuspendingEventArgs;
        }
        namespace Foundation {
            struct IAsyncAction;
        }
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
        namespace Phone {
            namespace UI {
                namespace Input {
                    struct IBackPressedEventArgs;
                    struct ICameraEventArgs;
                }
            }
        }
#endif
    }
}
struct IAsyncInfo;
struct IInspectable;

QT_BEGIN_NAMESPACE

class QAbstractEventDispatcher;

class QWinRTIntegrationPrivate;
class QWinRTIntegration : public QPlatformIntegration
{
private:
    explicit QWinRTIntegration();
public:
    ~QWinRTIntegration();

    static QWinRTIntegration *create()
    {
        QScopedPointer<QWinRTIntegration> integration(new QWinRTIntegration);
        return integration->succeeded() ? integration.take() : nullptr;
    }

    bool succeeded() const;

    bool hasCapability(QPlatformIntegration::Capability cap) const Q_DECL_OVERRIDE;
    QVariant styleHint(StyleHint hint) const Q_DECL_OVERRIDE;

    QPlatformWindow *createPlatformWindow(QWindow *window) const Q_DECL_OVERRIDE;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const Q_DECL_OVERRIDE;
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const Q_DECL_OVERRIDE;
    QAbstractEventDispatcher *createEventDispatcher() const Q_DECL_OVERRIDE;
    void initialize() Q_DECL_OVERRIDE;
    QPlatformFontDatabase *fontDatabase() const Q_DECL_OVERRIDE;
    QPlatformInputContext *inputContext() const Q_DECL_OVERRIDE;
    QPlatformServices *services() const Q_DECL_OVERRIDE;
    Qt::KeyboardModifiers queryKeyboardModifiers() const Q_DECL_OVERRIDE;

    QStringList themeNames() const Q_DECL_OVERRIDE;
    QPlatformTheme *createPlatformTheme(const QString &name) const Q_DECL_OVERRIDE;

    QPlatformOffscreenSurface *createPlatformOffscreenSurface(QOffscreenSurface *surface) const Q_DECL_OVERRIDE;
private:
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
    HRESULT onBackButtonPressed(IInspectable *, ABI::Windows::Phone::UI::Input::IBackPressedEventArgs *args);
    HRESULT onCameraPressed(IInspectable *, ABI::Windows::Phone::UI::Input::ICameraEventArgs *);
    HRESULT onCameraHalfPressed(IInspectable *, ABI::Windows::Phone::UI::Input::ICameraEventArgs *);
    HRESULT onCameraReleased(IInspectable *, ABI::Windows::Phone::UI::Input::ICameraEventArgs *);
#endif
    HRESULT onSuspended(IInspectable *, ABI::Windows::ApplicationModel::ISuspendingEventArgs *);
    HRESULT onResume(IInspectable *, IInspectable *);

    QScopedPointer<QWinRTIntegrationPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTIntegration)
};

QT_END_NAMESPACE

#endif // QWINRTINTEGRATION_H
