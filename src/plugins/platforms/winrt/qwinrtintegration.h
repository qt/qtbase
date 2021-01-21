/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
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
    ~QWinRTIntegration() override;

    static QWinRTIntegration *create()
    {
        QScopedPointer<QWinRTIntegration> integration(new QWinRTIntegration);
        return integration->succeeded() ? integration.take() : nullptr;
    }

    bool succeeded() const;

    bool hasCapability(QPlatformIntegration::Capability cap) const override;
    QVariant styleHint(StyleHint hint) const override;

    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const override;
    QAbstractEventDispatcher *createEventDispatcher() const override;
    void initialize() override;
    QPlatformFontDatabase *fontDatabase() const override;
    QPlatformInputContext *inputContext() const override;
    QPlatformServices *services() const override;
    QPlatformClipboard *clipboard() const override;
#if QT_CONFIG(draganddrop)
    QPlatformDrag *drag() const override;
#endif
#if QT_CONFIG(accessibility)
    QPlatformAccessibility *accessibility() const override;
#endif

    Qt::KeyboardModifiers queryKeyboardModifiers() const override;

    QStringList themeNames() const override;
    QPlatformTheme *createPlatformTheme(const QString &name) const override;

    QPlatformOffscreenSurface *createPlatformOffscreenSurface(QOffscreenSurface *surface) const override;
private:
    HRESULT onSuspended(IInspectable *, ABI::Windows::ApplicationModel::ISuspendingEventArgs *);
    HRESULT onResume(IInspectable *, IInspectable *);

    QScopedPointer<QWinRTIntegrationPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTIntegration)
};

QT_END_NAMESPACE

#endif // QWINRTINTEGRATION_H
