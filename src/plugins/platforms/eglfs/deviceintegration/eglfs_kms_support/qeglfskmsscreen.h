/****************************************************************************
**
** Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2016 Pelagicore AG
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

#ifndef QEGLFSKMSSCREEN_H
#define QEGLFSKMSSCREEN_H

#include "private/qeglfsscreen_p.h"
#include <QtCore/QList>
#include <QtCore/QMutex>

#include <QtKmsSupport/private/qkmsdevice_p.h>
#include <QtEdidSupport/private/qedidparser_p.h>

QT_BEGIN_NAMESPACE

class QEglFSKmsDevice;
class QEglFSKmsInterruptHandler;

class Q_EGLFS_EXPORT QEglFSKmsScreen : public QEglFSScreen
{
public:
    QEglFSKmsScreen(QEglFSKmsDevice *device, const QKmsOutput &output, bool headless = false);
    ~QEglFSKmsScreen();

    void setVirtualPosition(const QPoint &pos);

    QRect rawGeometry() const override;

    int depth() const override;
    QImage::Format format() const override;

    QSizeF physicalSize() const override;
    QDpi logicalDpi() const override;
    Qt::ScreenOrientation nativeOrientation() const override;
    Qt::ScreenOrientation orientation() const override;

    QString name() const override;

    QString manufacturer() const override;
    QString model() const override;
    QString serialNumber() const override;

    qreal refreshRate() const override;

    QList<QPlatformScreen *> virtualSiblings() const override { return m_siblings; }
    void setVirtualSiblings(QList<QPlatformScreen *> sl) { m_siblings = sl; }

    QVector<QPlatformScreen::Mode> modes() const override;

    int currentMode() const override;
    int preferredMode() const override;

    QEglFSKmsDevice *device() const { return m_device; }

    virtual void waitForFlip();

    QKmsOutput &output() { return m_output; }
    void restoreMode();

    SubpixelAntialiasingType subpixelAntialiasingTypeHint() const override;

    QPlatformScreen::PowerState powerState() const override;
    void setPowerState(QPlatformScreen::PowerState state) override;

    bool isCursorOutOfRange() const { return m_cursorOutOfRange; }
    void setCursorOutOfRange(bool b) { m_cursorOutOfRange = b; }

protected:
    QEglFSKmsDevice *m_device;

    QKmsOutput m_output;
    QEdidParser m_edid;
    QPoint m_pos;
    bool m_cursorOutOfRange;

    QList<QPlatformScreen *> m_siblings;

    PowerState m_powerState;

    QEglFSKmsInterruptHandler *m_interruptHandler;

    bool m_headless;
};

QT_END_NAMESPACE

#endif
