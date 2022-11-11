// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    QVERIFY(QInputDevice::devices().size() >= 4);
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
