/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore>
#include <QtGui>
#include <QtTest>

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

