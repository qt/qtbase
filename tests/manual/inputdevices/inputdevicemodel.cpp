// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "inputdevicemodel.h"

#include <QEvent>
#include <QInputDevice>
#include <QLoggingCategory>
#include <QMetaEnum>
#include <QPointingDevice>

Q_LOGGING_CATEGORY(lcIPDM, "qt.inputdevicemodel")

static QString enumToString(const QObject *obj, const char* enumName, int enumValue)
{
    const auto *metaobj = obj->metaObject();
    Q_ASSERT(metaobj);
    const int enumIdx = metaobj->indexOfEnumerator(enumName);
    if (enumIdx < 0)
        return {};
    Q_ASSERT(metaobj->enumerator(enumIdx).isValid());
    const char *ret = metaobj->enumerator(enumIdx).valueToKey(enumValue);
    if (!ret)
        return {};
    return QString::fromUtf8(ret);
}

static QString capabilitiesString(const QInputDevice *dev)
{
    QStringList ret;
    const auto caps = dev->capabilities();
    if (caps.testFlag(QInputDevice::Capability::Position))
        ret << InputDeviceModel::tr("pos");
    if (caps.testFlag(QInputDevice::Capability::Area))
        ret << InputDeviceModel::tr("area");
    if (caps.testFlag(QInputDevice::Capability::Pressure))
        ret << InputDeviceModel::tr("press");
    if (caps.testFlag(QInputDevice::Capability::Velocity))
        ret << InputDeviceModel::tr("vel");
    if (caps.testFlag(QInputDevice::Capability::NormalizedPosition))
        ret << InputDeviceModel::tr("norm");
    if (caps.testFlag(QInputDevice::Capability::MouseEmulation))
        ret << InputDeviceModel::tr("m-emu");
    if (caps.testFlag(QInputDevice::Capability::Scroll))
        ret << InputDeviceModel::tr("scroll");
    if (caps.testFlag(QInputDevice::Capability::PixelScroll))
        ret << InputDeviceModel::tr("pxscroll");
    if (caps.testFlag(QInputDevice::Capability::Hover))
        ret << InputDeviceModel::tr("hover");
    if (caps.testFlag(QInputDevice::Capability::Rotation))
        ret << InputDeviceModel::tr("rot");
    if (caps.testFlag(QInputDevice::Capability::XTilt))
        ret << InputDeviceModel::tr("xtilt");
    if (caps.testFlag(QInputDevice::Capability::YTilt))
        ret << InputDeviceModel::tr("ytilt");
    if (caps.testFlag(QInputDevice::Capability::TangentialPressure))
        ret << InputDeviceModel::tr("tan");
    if (caps.testFlag(QInputDevice::Capability::ZPosition))
        ret << InputDeviceModel::tr("z");
    return ret.join(u'|');
}

/*!
    Returns true only if the given \a device is a master:
    that is, if its parent is \e not another QInputDevice.
*/
static const auto masterDevicePred = [](const QInputDevice *device)
{
    return !qobject_cast<QInputDevice*>(device->parent());
};

/*!
    Returns the master device at index \a i:
    that is, the i'th of the devices that satisfy masterDevicePred().
*/
static const QInputDevice *masterDevice(int i)
{
    const auto devices = QInputDevice::devices();
    auto it = std::find_if(devices.constBegin(), devices.constEnd(), masterDevicePred);
    it += i;
    return (it == devices.constEnd() ? nullptr : *it);
}

/*!
    Returns the index of the master \a device: that is, the index within the
    subset of QInputDevice::devices() that satisfy masterDevicePred().
*/
static const int masterDeviceIndex(const QInputDevice *device)
{
    Q_ASSERT(masterDevicePred(device)); // assume dev is not a slave
    const auto devices = QInputDevice::devices();
    auto it = std::find_if(devices.constBegin(), devices.constEnd(), masterDevicePred);
    for (int i = 0; it != devices.constEnd(); ++i, ++it)
        if (*it == device)
            return i;
    return -1;
}

InputDeviceModel::InputDeviceModel(QObject *parent)
    : QAbstractItemModel{parent}
{
    connect(this, &InputDeviceModel::deviceAdded, this, &InputDeviceModel::onDeviceAdded, Qt::QueuedConnection);
}

// invariant: always call createIndex(row, column, QInputDevice*) or else nullptr for the last argument

QModelIndex InputDeviceModel::index(int row, int column, const QModelIndex &parent) const
{
    const QInputDevice *par = static_cast<QInputDevice *>(parent.internalPointer());
    const QInputDevice *ret = par ? qobject_cast<const QInputDevice *>(par->children().at(row)) : masterDevice(row);
    qCDebug(lcIPDM) << row << column << "under parent" << par << ":" << ret;
    return createIndex(row, column, ret);
}

QModelIndex InputDeviceModel::parent(const QModelIndex &index) const
{
    if (!index.internalPointer())
        return {};
    const QInputDevice *par = qobject_cast<const QInputDevice *>(
                static_cast<QInputDevice *>(index.internalPointer())->parent());
    if (par)
        return createIndex(masterDeviceIndex(par), index.column(), par);
    return {};
}

int InputDeviceModel::rowCount(const QModelIndex &parent) const
{
    int ret = 0;
    const QInputDevice *par = qobject_cast<const QInputDevice *>(static_cast<QObject *>(parent.internalPointer()));
    if (par) {
        ret = par->children().count();
    } else {
        const auto devices = QInputDevice::devices();
        ret = std::count_if(devices.constBegin(), devices.constEnd(), masterDevicePred);
    }
    qCDebug(lcIPDM) << ret << "under parent" << parent << par;
    return ret;
}

int InputDeviceModel::columnCount(const QModelIndex &) const
{
    return NRoles;
}

QVariant InputDeviceModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section + Name) {
        case Name:
            return tr("Device Name");
        case DeviceType:
            return tr("Device Type");
        case PointerType:
            return tr("Pointer Type");
        case Capabilities:
            return tr("Capabilities");
        case SystemId:
            return tr("System ID");
        case SeatName:
            return tr("Seat Name");
        case AvailableVirtualGeometry:
            return tr("Available Virtual Geometry");
        case MaximumPoints:
            return tr("Maximum Points");
        case ButtonCount:
            return tr("Button Count");
        case UniqueId:
            return tr("Unique ID");
        case NRoles:
            break;
        }
    }
    return {};
}

bool InputDeviceModel::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::ChildAdded)
        // At this time, the child is not fully-constructed.
        // Emit a signal which is connected to onDeviceAdded via queued connection, to delay row insertion.
        emit deviceAdded(static_cast<QChildEvent *>(event)->child());
    return false;
}

void InputDeviceModel::onDeviceAdded(const QObject *o)
{
    const QInputDevice *child = qobject_cast<const QInputDevice *>(o);
    const QInputDevice *parent = qobject_cast<const QInputDevice *>(child->parent());
    const int idx = parent->children().indexOf(child);
    qCDebug(lcIPDM) << parent << "has a baby!" << child << "@" << idx;
    beginInsertRows(createIndex(masterDeviceIndex(parent), 0, parent), idx, idx);
    endInsertRows();
}

void InputDeviceModel::watchDevice(const QInputDevice *dev) const
{
    if (!m_known.contains(dev)) {
        m_known << dev;
        connect(dev, &QObject::destroyed, this, &InputDeviceModel::deviceDestroyed);
        if (masterDevicePred(dev))
            const_cast<QInputDevice *>(dev)->installEventFilter(const_cast<InputDeviceModel *>(this));
    }
}

void InputDeviceModel::deviceDestroyed(QObject *o)
{
    beginResetModel();
    const QInputDevice *dev = static_cast<QInputDevice *>(o);
    bool needsReset = true;
    if (!masterDevicePred(dev)) {
        const QInputDevice *parent = static_cast<const QInputDevice *>(dev->parent());
        const int idx = parent->children().indexOf(dev);
        Q_ASSERT(idx >= 0);
        beginRemoveRows(createIndex(masterDeviceIndex(parent), 0, parent), idx, idx);
        endRemoveRows();
        needsReset = false;
    }
    m_known.removeOne(dev);
    if (needsReset)
        endResetModel();
}

QVariant InputDeviceModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
        role = index.column() + Role::Name;
    if (role >= NRoles)
        return {};
    const QInputDevice *dev = static_cast<QInputDevice *>(index.internalPointer());
    watchDevice(dev);
    if (role < Name)
        qCDebug(lcIPDM) << index << Qt::ItemDataRole(role) << dev;
    else
        qCDebug(lcIPDM) << index << Role(role) << dev;
    const QPointingDevice *pdev = qobject_cast<const QPointingDevice *>(dev);
    switch (role) {
    case Name:
        return dev->name();
    case DeviceType:
        return enumToString(dev, "DeviceType", int(dev->type()));
    case PointerType:
        return pdev ? enumToString(pdev, "PointerType", int(pdev->pointerType()))
                    : QString();
    case Capabilities:
        return capabilitiesString(dev);
    case SystemId:
        return dev->systemId();
    case SeatName:
        return dev->seatName();
    case AvailableVirtualGeometry: {
        const auto rect = dev->availableVirtualGeometry();
        return tr("%1 x %2 %3 %4").arg(rect.width()).arg(rect.height()).arg(rect.x()).arg(rect.y());
    }
    case MaximumPoints:
        return pdev ? pdev->maximumPoints() : 0;
    case ButtonCount:
        return pdev ? pdev->buttonCount() : 0;
    case UniqueId:
        return pdev ? pdev->uniqueId().numericId() : 0;
    }
    return {};
}

#include "moc_inputdevicemodel.cpp"
