// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <storeloader.h>

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

#include <QtCore/QVariant>

using namespace Qt::StringLiterals;

class QtDynamicFeatureTest : public QObject
{
    Q_OBJECT
public:
    QtDynamicFeatureTest(){}

private Q_SLOTS:
    void loadResourcesFeature();
};

constexpr int APP_NOT_OWNED = -15;

void QtDynamicFeatureTest::loadResourcesFeature()
{
    QVERIFY(!QFile::exists(":/dynamic_resources/qtlogo.png"));

    auto handler = StoreLoader::loadModule("tst_android_dynamic_feature_resources"_L1);
    QVERIFY(handler);

    QSignalSpy finishedSpy(handler.get(), &StoreLoaderHandler::finished);
    QSignalSpy errorSpy(handler.get(), &StoreLoaderHandler::errorOccured);

    QVERIFY(finishedSpy.wait(20000));

    if (!errorSpy.isEmpty()) {
        const auto args = errorSpy.takeFirst();
        const int errorCode = args.at(0).toInt();
        if (errorCode == APP_NOT_OWNED)
            QSKIP("The app needs to be installed from the Play Store to perform dynamic loading.");
        else
            QSKIP("splitInstall() failed, potentially the SplitInstall API are not available.");
    }

    QVERIFY(QFile::exists(":/dynamic_resources/qtlogo.png"));
}

QTEST_MAIN(QtDynamicFeatureTest)

#include "tst_android_dynamic_feature.moc"
