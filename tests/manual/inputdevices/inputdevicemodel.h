// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef INPUTDEVICEMODEL_H
#define INPUTDEVICEMODEL_H

#include <QAbstractItemModel>

class QInputDevice;

class InputDeviceModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Role {
        Name = Qt::UserRole + 1,
        DeviceType,
        PointerType,
        Capabilities,
        SystemId,
        SeatName,
        AvailableVirtualGeometry,
        MaximumPoints,
        ButtonCount,
        UniqueId,
        NRoles
    };
    Q_ENUM(Role);

    explicit InputDeviceModel(QObject *parent = nullptr);
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

signals:
    void deviceAdded(const QObject *o);

private slots:
    void onDeviceAdded(const QObject *o);

private:
    void watchDevice(const QInputDevice *dev) const;
    void deviceDestroyed(QObject *o);

    mutable QList<const QInputDevice *> m_known;
};

#endif // INPUTDEVICEMODEL_H
