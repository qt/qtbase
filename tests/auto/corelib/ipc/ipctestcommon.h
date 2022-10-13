// Copyright (C) 2022 Intel Corporation.

#include <QtTest/QTest>
#include <QtCore/QNativeIpcKey>

namespace IpcTestCommon {
static QList<QNativeIpcKey::Type> supportedKeyTypes;

template <typename IpcClass> void addGlobalTestRows()
{
    qDebug() << "Default key type is" << QNativeIpcKey::DefaultTypeForOs
             << "and legacy key type is" << QNativeIpcKey::legacyDefaultTypeForOs();

#if defined(Q_OS_FREEBSD) || defined(Q_OS_DARWIN) || defined(Q_OS_WIN) || (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID))
    // only enforce that IPC works on the platforms above; other platforms may
    // have no working backends (notably, Android)
    QVERIFY(IpcClass::isKeyTypeSupported(QNativeIpcKey::DefaultTypeForOs));
    QVERIFY(IpcClass::isKeyTypeSupported(QNativeIpcKey::legacyDefaultTypeForOs()));
#endif

    auto addRowIfSupported = [](const char *name, QNativeIpcKey::Type type) {
        if (IpcClass::isKeyTypeSupported(type)) {
            supportedKeyTypes << type;
            QTest::newRow(name) << type;
        }
    };

    QTest::addColumn<QNativeIpcKey::Type>("keyType");

    addRowIfSupported("Windows", QNativeIpcKey::Type::Windows);
    addRowIfSupported("POSIX", QNativeIpcKey::Type::PosixRealtime);
    addRowIfSupported("SystemV-Q", QNativeIpcKey::Type::SystemV);
    addRowIfSupported("SystemV-T", QNativeIpcKey::Type('T'));

    if (supportedKeyTypes.isEmpty())
        QSKIP("System reports no supported IPC types.");
}

// rotate through the supported types and find another
inline QNativeIpcKey::Type nextKeyType(QNativeIpcKey::Type type)
{
    qsizetype idx = supportedKeyTypes.indexOf(type);
    Q_ASSERT(idx >= 0);

    ++idx;
    if (idx == supportedKeyTypes.size())
        idx = 0;
    return supportedKeyTypes.at(idx);
}
} // namespace IpcTestCommon

QT_BEGIN_NAMESPACE
namespace QTest {
template<> inline char *toString(const QNativeIpcKey::Type &type)
{
    switch (type) {
    case QNativeIpcKey::Type::SystemV:  return qstrdup("SystemV");
    case QNativeIpcKey::Type::PosixRealtime: return qstrdup("PosixRealTime");
    case QNativeIpcKey::Type::Windows:  return qstrdup("Windows");
    }
    if (type == QNativeIpcKey::Type{})
        return qstrdup("Invalid");

    char buf[32];
    qsnprintf(buf, sizeof(buf), "%u", unsigned(type));
    return qstrdup(buf);
}

template<> inline char *toString(const QNativeIpcKey &key)
{
    if (!key.isValid())
        return qstrdup("<invalid>");

    const char *type = toString(key.type());
    const char *text = toString(key.nativeKey());
    char buf[256];
    qsnprintf(buf, sizeof(buf), "QNativeIpcKey(%s, %s)", text, type);
    delete[] type;
    delete[] text;
    return qstrdup(buf);
}
} // namespace QTest
QT_END_NAMESPACE
