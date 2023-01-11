// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QPermission>

#include <QTest>

struct DummyPermission // a minimal QPermission-compatible type
{
    using QtPermissionHelper = void;
    int state = 0;
};
Q_DECLARE_METATYPE(DummyPermission)

class tst_QPermission : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void converting_Dummy() const { return converting_impl<DummyPermission>(); }
    void converting_Location() const { return converting_impl<QLocationPermission>(); }
    void converting_Calendar() const { return converting_impl<QCalendarPermission>(); }
    void converting_Contacts() const { return converting_impl<QContactsPermission>(); }
    void converting_Camera() const { return converting_impl<QCameraPermission>(); }
    void converting_Microphone() const { return converting_impl<QMicrophonePermission>(); }
    void converting_Bluetooth() const { return converting_impl<QBluetoothPermission>(); }

private:
    template <typename T>
    void converting_impl() const;
};

template <typename T>
void tst_QPermission::converting_impl() const
{
    T concrete;
    const T cconcrete = concrete;
    const auto metaType = QMetaType::fromType<T>();

    // construction is implicit:
    // from rvalue:
    {
        QPermission p = T();
        QCOMPARE_EQ(p.type(), metaType);
    }
    // from mutable lvalue:
    {
        QPermission p = concrete;
        QCOMPARE_EQ(p.type(), metaType);
    }
    // from const lvalue:
    {
        QPermission p = cconcrete;
        QCOMPARE_EQ(p.type(), metaType);
    }

    // data<>() compiles:
    {
        const QPermission p = concrete;
        [[maybe_unused]] auto r = p.data<T>();
        static_assert(std::is_same_v<decltype(r), T>);
    }
}

QTEST_APPLESS_MAIN(tst_QPermission)
#include "tst_qpermission.moc"
