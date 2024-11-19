// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <QtCore/qobject.h>
#include <QtCore/qmetaobject.h>

#include <qpa/qplatforminputcontextfactory_p.h>
#include <qpa/qplatforminputcontext.h>

class tst_XkbKeyboard : public QObject
{
    Q_OBJECT
private slots:
    void verifyComposeInputContextInterface();
};

void tst_XkbKeyboard::verifyComposeInputContextInterface()
{
    QPlatformInputContext *inputContext = QPlatformInputContextFactory::create(QStringLiteral("compose"));
    QVERIFY(inputContext);

    const char *const inputContextClassName = "QComposeInputContext";
    const char *const normalizedSignature = "setXkbContext(xkb_context*)";

    QVERIFY(inputContext->objectName() == QLatin1String(inputContextClassName));

    int methodIndex = inputContext->metaObject()->indexOfMethod(normalizedSignature);
    QMetaMethod method = inputContext->metaObject()->method(methodIndex);
    Q_ASSERT(method.isValid());
}

QTEST_MAIN(tst_XkbKeyboard)
#include "tst_xkbkeyboard.moc"

