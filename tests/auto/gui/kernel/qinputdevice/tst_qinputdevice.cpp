/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QTest>
#include <qpa/qwindowsysteminterface.h>
#include <QtGui/qinputdevice.h>
#include <QtGui/qpointingdevice.h>
#include <QtGui/private/qinputdevice_p.h>

class tst_QInputDevice : public QObject
{
    Q_OBJECT
public:
    tst_QInputDevice() {}
    virtual ~tst_QInputDevice() {}
private slots:
    void initTestCase();
    void multiSeatDevices();

private:
    const QInputDevice *getPrimaryKeyboard(const QString& seatName = QString());
    const QPointingDevice *getPrimaryPointingDevice(const QString& seatName = QString());
};

void tst_QInputDevice::initTestCase()
{
}

const QInputDevice *tst_QInputDevice::getPrimaryKeyboard(const QString& seatName)
{
    QList<const QInputDevice *> devices = QInputDevice::devices();
    const QInputDevice *ret = nullptr;
    for (const QInputDevice *d : devices) {
        if (d->type() != QInputDevice::DeviceType::Keyboard)
            continue;
        if (seatName.isNull() || d->seatName() == seatName) {
            // the master keyboard's parent is not another input device
            if (!d->parent() || !qobject_cast<const QInputDevice *>(d->parent()))
                return d;
            if (!ret)
                ret = d;
        }
    }
    return ret;
}

const QPointingDevice *tst_QInputDevice::getPrimaryPointingDevice(const QString& seatName)
{
    QList<const QInputDevice *> devices = QInputDevice::devices();
    const QPointingDevice *mouse = nullptr;
    const QPointingDevice *touchpad = nullptr;
    for (const QInputDevice *dev : devices) {
        if (!seatName.isNull() && dev->seatName() != seatName)
            continue;
        if (dev->type() == QInputDevice::DeviceType::Mouse) {
            if (!mouse)
                mouse = static_cast<const QPointingDevice *>(dev);
            // the core pointer is likely a mouse, and its parent is not another input device
            if (!mouse->parent() || !qobject_cast<const QInputDevice *>(mouse->parent()))
                return mouse;
        } else if (dev->type() == QInputDevice::DeviceType::TouchPad) {
            if (!touchpad || !dev->parent() || dev->parent()->metaObject() != dev->metaObject())
                touchpad = static_cast<const QPointingDevice *>(dev);
        }
    }
    if (mouse)
        return mouse;
    return touchpad;
}

void tst_QInputDevice::multiSeatDevices()
{
    QWindowSystemInterface::registerInputDevice(new QInputDevice("seat 1 kbd", 1000, QInputDevice::DeviceType::Keyboard, "seat 1", this));
    QWindowSystemInterface::registerInputDevice(new QPointingDevice("seat 1 mouse", 1010, QInputDevice::DeviceType::Mouse, QPointingDevice::PointerType::Generic,
                                                                    QInputDevice::Capability::Position | QInputDevice::Capability::Hover | QInputDevice::Capability::Scroll,
                                                                    1, 5, "seat 1", QPointingDeviceUniqueId(), this));
    QWindowSystemInterface::registerInputDevice(new QInputDevice("seat 2 kbd", 2000, QInputDevice::DeviceType::Keyboard, "seat 2", this));
    QWindowSystemInterface::registerInputDevice(new QPointingDevice("seat 2 mouse", 2010, QInputDevice::DeviceType::Mouse, QPointingDevice::PointerType::Generic,
                                                                    QInputDevice::Capability::Position | QInputDevice::Capability::Hover,
                                                                    1, 2, "seat 2", QPointingDeviceUniqueId(), this));
    QVERIFY(QInputDevice::devices().count() >= 4);
    QVERIFY(QInputDevicePrivate::fromId(1010));
    QVERIFY(QInputDevicePrivate::fromId(1010)->hasCapability(QInputDevice::Capability::Scroll));
    QVERIFY(QInputDevicePrivate::fromId(2010));
    QVERIFY(!QInputDevicePrivate::fromId(2010)->hasCapability(QInputDevice::Capability::Scroll));
    QVERIFY(QInputDevice::primaryKeyboard());
    if (!getPrimaryKeyboard())
        QCOMPARE(QInputDevice::primaryKeyboard()->systemId(), qint64(1) << 33);
    QVERIFY(QPointingDevice::primaryPointingDevice());
    if (!getPrimaryPointingDevice())
        QCOMPARE(QPointingDevice::primaryPointingDevice()->systemId(), 1);
    QVERIFY(QInputDevice::primaryKeyboard("seat 1"));
    QCOMPARE(QInputDevice::primaryKeyboard("seat 1")->systemId(), 1000);
    QVERIFY(QPointingDevice::primaryPointingDevice("seat 1"));
    QCOMPARE(QPointingDevice::primaryPointingDevice("seat 1")->systemId(), 1010);
    QVERIFY(QInputDevice::primaryKeyboard("seat 2"));
    QCOMPARE(QInputDevice::primaryKeyboard("seat 2")->systemId(), 2000);
    QVERIFY(QPointingDevice::primaryPointingDevice("seat 2"));
    QCOMPARE(QPointingDevice::primaryPointingDevice("seat 2")->systemId(), 2010);
}

QTEST_MAIN(tst_QInputDevice)
#include "tst_qinputdevice.moc"
