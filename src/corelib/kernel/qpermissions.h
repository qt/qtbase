// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPERMISSIONS_H
#define QPERMISSIONS_H

#if 0
#pragma qt_class(QPermissions)
#endif

#include <QtCore/qglobal.h>
#include <QtCore/qtmetamacros.h>
#include <QtCore/qvariant.h>

#include <QtCore/qshareddata_impl.h>
#include <QtCore/qtypeinfo.h>
#include <QtCore/qmetatype.h>

#if !defined(Q_QDOC)
QT_REQUIRE_CONFIG(permissions);
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_DEBUG_STREAM
class QDebug;
#endif

struct QMetaObject;
class QCoreApplication;

class QPermission
{
    Q_GADGET_EXPORT(Q_CORE_EXPORT)

    template <typename T, typename Enable = void>
    static constexpr inline bool is_permission_v = false;

    template <typename T>
    static constexpr inline bool is_permission_v<T, typename T::QtPermissionHelper> = true;

    template <typename T>
    using if_permission = std::enable_if_t<is_permission_v<T>, bool>;
public:
    explicit QPermission() = default;

    template <typename T, if_permission<T> = true>
    QPermission(const T &t) : m_data(QVariant::fromValue(t)) {}

    Qt::PermissionStatus status() const { return m_status; }

    QMetaType type() const { return m_data.metaType(); }

    template <typename T, if_permission<T> = true>
    T data() const
    {
        if (auto p = data(QMetaType::fromType<T>()))
            return *static_cast<const T *>(p);
        else
            return T{};
    }

#ifndef QT_NO_DEBUG_STREAM
    friend Q_CORE_EXPORT QDebug operator<<(QDebug debug, const QPermission &);
#endif

private:
    Q_CORE_EXPORT const void *data(QMetaType id) const;

    Qt::PermissionStatus m_status = Qt::PermissionStatus::Undetermined;
    QVariant m_data;

    friend class QCoreApplication;
};

#define QT_PERMISSION(ClassName) \
    Q_GADGET_EXPORT(Q_CORE_EXPORT) \
    using QtPermissionHelper = void; \
    friend class QPermission; \
public: \
    Q_CORE_EXPORT ClassName(); \
    Q_CORE_EXPORT ClassName(const ClassName &other) noexcept; \
    Q_CORE_EXPORT ClassName(ClassName &&other) noexcept; \
    Q_CORE_EXPORT ~ClassName() noexcept; \
    Q_CORE_EXPORT ClassName &operator=(const ClassName &other) noexcept; \
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(ClassName) \
    void swap(ClassName &other) noexcept { d.swap(other.d); } \
private: \
    QtPrivate::QExplicitlySharedDataPointerV2<ClassName##Private> d;

class QLocationPermissionPrivate;
class QLocationPermission
{
    QT_PERMISSION(QLocationPermission)
public:
    enum Accuracy { Approximate, Precise };
    Q_ENUM(Accuracy)

    Q_CORE_EXPORT void setAccuracy(Accuracy accuracy);
    Q_CORE_EXPORT Accuracy accuracy() const;

    enum Availability { WhenInUse, Always };
    Q_ENUM(Availability)

    Q_CORE_EXPORT void setAvailability(Availability availability);
    Q_CORE_EXPORT Availability availability() const;
};
Q_DECLARE_SHARED(QLocationPermission)

class QCalendarPermissionPrivate;
class QCalendarPermission
{
    QT_PERMISSION(QCalendarPermission)
public:
    Q_CORE_EXPORT void setReadWrite(bool enable);
    Q_CORE_EXPORT bool isReadWrite() const;
};
Q_DECLARE_SHARED(QCalendarPermission)

class QContactsPermissionPrivate;
class QContactsPermission
{
    QT_PERMISSION(QContactsPermission)
public:
    Q_CORE_EXPORT void setReadWrite(bool enable);
    Q_CORE_EXPORT bool isReadWrite() const;
};
Q_DECLARE_SHARED(QContactsPermission)

#define Q_DECLARE_MINIMAL_PERMISSION(ClassName) \
    class ClassName##Private; \
    class ClassName \
    { \
        QT_PERMISSION(ClassName) \
    }; \
    Q_DECLARE_SHARED(ClassName)

Q_DECLARE_MINIMAL_PERMISSION(QCameraPermission)
Q_DECLARE_MINIMAL_PERMISSION(QMicrophonePermission)
Q_DECLARE_MINIMAL_PERMISSION(QBluetoothPermission)

#undef QT_PERMISSION
#undef Q_DECLARE_MINIMAL_PERMISSION

QT_END_NAMESPACE

#endif // QPERMISSIONS_H
