// Copyright (C) 2015 Green Hills Software
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qintegrityhidmanager.h"
#include <QList>
#include <QPoint>
#include <QGuiApplication>
#include <qpa/qwindowsysteminterface.h>
#include <device/hiddriver.h>
#include <private/qguiapplication_p.h>
#include <private/qinputdevicemanager_p_p.h>

QT_BEGIN_NAMESPACE

class IntNotifier
{
    static const Value ActivityPriority = 2;
protected:
    Activity act;
public:
    IntNotifier()
    {
        CheckSuccess(CreateActivity(CurrentTask(), ActivityPriority, false, (Value)this, &act));
    };
    ~IntNotifier()
    {
        CheckSuccess(CloseActivity(act));
    };
    virtual void process_event() = 0;
    virtual void async_wait() = 0;
};

class HIDDeviceHandler : IntNotifier
{
public:
    HIDDeviceHandler(HIDDriver *hidd, HIDHandle hidh)
        : driver(hidd), handle(hidh), currentPos(0, 0) { }
    ~HIDDeviceHandler()
    {
        CheckSuccess(gh_hid_close(handle));
    };
    void process_event(void) override;
    void async_wait(void) override;
    HIDDriver *get_driver(void) { return driver; };
    HIDHandle get_handle(void) { return handle; };
private:
    HIDDriver *driver;
    HIDHandle handle;
    QPoint currentPos;
    Qt::MouseButtons buttons;
};

class HIDDriverHandler : IntNotifier
{
public:
    HIDDriverHandler(HIDDriver *hidd) : IntNotifier(), driver(hidd) { }
    ~HIDDriverHandler()
    {
        qDeleteAll(devices);
    };
    void process_event(void) override;
    void async_wait(void) override;
    void find_devices(void);
private:
    QHash<Value, HIDDeviceHandler *> devices;
    HIDDriver *driver;
};

void HIDDriverHandler::process_event()
{
    find_devices();
}

void HIDDriverHandler::async_wait()
{
    gh_hid_wait_for_new_device(driver, act);
}

void HIDDriverHandler::find_devices()
{
    Error err;
    uintptr_t devicecontext;
    uint32_t device_id;
    HIDHandle handle;
    HIDDeviceHandler *hidnot;
    int deviceCount = 0;

    devicecontext = 0;
    forever {
        err = gh_hid_enum_devices(driver, &device_id, &devicecontext);
        if (err == OperationNotImplemented)
            break;
        else if (err == Failure)
            break;
        if (!devices.contains(device_id)) {
            err = gh_hid_init_device(driver, device_id, &handle);
            if (err == Success) {
                hidnot = new HIDDeviceHandler(driver, handle);
                devices.insert(device_id, hidnot);
                QInputDeviceManagerPrivate::get(QGuiApplicationPrivate::inputDeviceManager())->setDeviceCount(
                            QInputDeviceManager::DeviceTypePointer,++deviceCount);
                hidnot->async_wait();
            }
        }
    }
    if (err == OperationNotImplemented) {
        /* fallback on legacy enumeration where we assume 0-based
         * contiguous indexes */
        device_id = 0;
        err = Success;
        do {
            if (!devices.contains(device_id)) {
                err = gh_hid_init_device(driver, device_id, &handle);
                if (err != Success)
                    break;
                hidnot = new HIDDeviceHandler(driver, handle);
                devices.insert(device_id, hidnot);
                hidnot->async_wait();
            }
            device_id++;
        } while (err == Success);
    }

    async_wait();
}


void HIDDeviceHandler::process_event()
{
    HIDEvent event;
    uint32_t num_events = 1;

    while (gh_hid_get_event(handle, &event, &num_events) == Success) {
        if (event.type == HID_TYPE_AXIS) {
            switch (event.index) {
            case HID_AXIS_ABSX:
                currentPos.setX(event.value);
                break;
            case HID_AXIS_ABSY:
                currentPos.setY(event.value);
                break;
            case HID_AXIS_RELX:
                currentPos.setX(currentPos.x() + event.value);
                break;
            case HID_AXIS_RELY:
                currentPos.setY(currentPos.y() + event.value);
                break;
            default:
                /* ignore the rest for now */
                break;
            }
        } else if (event.type == HID_TYPE_KEY) {
            switch (event.index) {
            case HID_BUTTON_LEFT:
                if (event.value)
                    buttons |= Qt::LeftButton;
                else
                    buttons &= ~Qt::LeftButton;
                break;
            case HID_BUTTON_MIDDLE:
                if (event.value)
                    buttons |= Qt::MiddleButton;
                else
                    buttons &= ~Qt::MiddleButton;
                break;
            case HID_BUTTON_RIGHT:
                if (event.value)
                    buttons |= Qt::RightButton;
                else
                    buttons &= ~Qt::RightButton;
                break;
            default:
                /* ignore the rest for now */
                break;
            }
        } else if (event.type == HID_TYPE_SYNC) {
            QWindowSystemInterface::handleMouseEvent(0, currentPos, currentPos, buttons,
                                                     QGuiApplication::keyboardModifiers());
        } else if (event.type == HID_TYPE_DISCONNECT) {
            /* FIXME */
        }
    }
    async_wait();
}

void HIDDeviceHandler::async_wait()
{
    CheckSuccess(gh_hid_async_wait_for_event(handle, act));
}

void QIntegrityHIDManager::open_devices()
{
    HIDDriver *hidd;
    uintptr_t context = 0;
    HIDDriverHandler *hidnot;

    while (gh_hid_enum_drivers(&hidd, &context) == Success) {
        hidnot = new HIDDriverHandler(hidd);
        m_drivers.append(hidnot);
        hidnot->find_devices();
    }
}

void QIntegrityHIDManager::run()
{
    IntNotifier *notifier;
    open_devices();
    /* main loop */
    forever {
        WaitForActivity((Value *)&notifier);
        notifier->process_event();
    }
}

QIntegrityHIDManager::QIntegrityHIDManager(const QString &key, const QString &spec, QObject *parent)
    : QThread(parent)
{
    start();
}

QIntegrityHIDManager::~QIntegrityHIDManager()
{
    terminate();
    qDeleteAll(m_drivers);
}

QT_END_NAMESPACE
