// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QEventLoop>
#include <QTimer>
#include <QJniObject>
#include <QtCore/private/qandroidextras_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace QtJniTypes;

Q_DECLARE_JNI_CLASS(ComponentName, "android/content/ComponentName")

class tst_AndroidActivityAlias : public QObject
{
    Q_OBJECT

private slots:
    void launchAliasActivity_data();
    void launchAliasActivity();
};

void tst_AndroidActivityAlias::launchAliasActivity_data()
{
    QTest::addColumn<QString>("aliasClass");
    QTest::addColumn<int>("requestCode");

    QTest::newRow("alias_without_metadata")
        << QStringLiteral("Alias")
        << 12345;
    QTest::newRow("alias_with_metadata")
        << QStringLiteral("AliasWithMetaData")
        << 12346;
    QTest::newRow("alias_with_invalid_metadata")
        << QStringLiteral("AliasWithInvalidMetaData")
        << 12347;
}

void tst_AndroidActivityAlias::launchAliasActivity()
{
    QFETCH(QString, aliasClass);
    QFETCH(int, requestCode);

    struct AliasActivityResult
    {
        bool finished = false;
        int resultCode = 0;
        QString aliasComponent;
    };

    AliasActivityResult result;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeout.start(5000);

    static QString EXTRA_FINISH_IMMEDIATELY = "finish_immediately";
    static QString EXTRA_ALIAS_COMPONENT = "alias_component";
    static int RESULT_OK = -1; // android.app.Activity.RESULT_OK

    using namespace QNativeInterface;
    const auto packageName = QAndroidApplication::context().callMethod<QString>("getPackageName");
    QVERIFY2(!packageName.isEmpty(), "Failed to obtain application package name.");

    Intent intent = Intent::construct();
    const QString fullAliasClass = QLatin1String("%1.%2").arg(packageName, aliasClass);
    ComponentName component = ComponentName::construct(packageName, fullAliasClass);
    intent.callMethod<Intent>("setComponent", component);
    intent.callMethod<Intent>("putExtra", EXTRA_FINISH_IMMEDIATELY, true);

    QtAndroidPrivate::startActivity(intent, requestCode,
        [&](int, int resultCode, const Intent &data) {
            result.finished = true;
            result.resultCode = resultCode;
            if (data.isValid()) {
                result.aliasComponent =
                    data.callMethod<QString>("getStringExtra", EXTRA_ALIAS_COMPONENT);
            }
            QMetaObject::invokeMethod(&loop, &QEventLoop::quit, Qt::QueuedConnection);
        }
    );

    loop.exec();
    timeout.stop();

    QVERIFY2(result.finished, qPrintable(u"Timed out waiting for %1 to finish."_s.arg(aliasClass)));
    QCOMPARE(result.resultCode, RESULT_OK);
    QCOMPARE(result.aliasComponent, fullAliasClass);
}

QT_END_NAMESPACE

QTEST_MAIN(tst_AndroidActivityAlias)
#include "tst_androidactivityalias.moc"
