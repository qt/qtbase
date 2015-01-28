/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <private/qmimetype_p.h>

#include <qmimetype.h>
#include <qmimedatabase.h>

#include <QtTest/QtTest>


class tst_qmimetype : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void isValid();
    void name();
    void genericIconName();
    void iconName();
    void suffixes();
};

// ------------------------------------------------------------------------------------------------

void tst_qmimetype::initTestCase()
{
    qputenv("XDG_DATA_DIRS", "doesnotexist");
}

// ------------------------------------------------------------------------------------------------

static QString qMimeTypeName()
{
    static const QString result ("No name of the MIME type");
    return result;
}

static QString qMimeTypeGenericIconName()
{
    static const QString result ("No file name of an icon image that represents the MIME type");
    return result;
}

static QString qMimeTypeIconName()
{
    static const QString result ("No file name of an icon image that represents the MIME type");
    return result;
}

static QStringList buildQMimeTypeFilenameExtensions()
{
    QStringList result;
    result << QString::fromLatin1("*.png");
    return result;
}

static QStringList qMimeTypeGlobPatterns()
{
    static const QStringList result (buildQMimeTypeFilenameExtensions());
    return result;
}

// ------------------------------------------------------------------------------------------------

#ifndef Q_COMPILER_RVALUE_REFS
QMIMETYPE_BUILDER
#else
QMIMETYPE_BUILDER_FROM_RVALUE_REFS
#endif

// ------------------------------------------------------------------------------------------------

void tst_qmimetype::isValid()
{
    QMimeType instantiatedQMimeType (
                  buildQMimeType (
                      qMimeTypeName(),
                      qMimeTypeGenericIconName(),
                      qMimeTypeIconName(),
                      qMimeTypeGlobPatterns()
                  )
              );

    QVERIFY(instantiatedQMimeType.isValid());

    QMimeType otherQMimeType (instantiatedQMimeType);

    QVERIFY(otherQMimeType.isValid());
    QCOMPARE(instantiatedQMimeType, otherQMimeType);

    QMimeType defaultQMimeType;

    QVERIFY(!defaultQMimeType.isValid());
}

// ------------------------------------------------------------------------------------------------

void tst_qmimetype::name()
{
    QMimeType instantiatedQMimeType (
                  buildQMimeType (
                      qMimeTypeName(),
                      qMimeTypeGenericIconName(),
                      qMimeTypeIconName(),
                      qMimeTypeGlobPatterns()
                  )
              );

    QMimeType otherQMimeType (
                  buildQMimeType (
                      QString(),
                      qMimeTypeGenericIconName(),
                      qMimeTypeIconName(),
                      qMimeTypeGlobPatterns()
                  )
              );

    // Verify that the Name is part of the equality test:
    QCOMPARE(instantiatedQMimeType.name(), qMimeTypeName());

    QVERIFY(instantiatedQMimeType != otherQMimeType);
    QVERIFY(!(instantiatedQMimeType == otherQMimeType));
}

// ------------------------------------------------------------------------------------------------

void tst_qmimetype::genericIconName()
{
    QMimeType instantiatedQMimeType (
                  buildQMimeType (
                      qMimeTypeName(),
                      qMimeTypeGenericIconName(),
                      qMimeTypeIconName(),
                      qMimeTypeGlobPatterns()
                  )
              );

    QCOMPARE(instantiatedQMimeType.genericIconName(), qMimeTypeGenericIconName());
}

// ------------------------------------------------------------------------------------------------

void tst_qmimetype::iconName()
{
    QMimeType instantiatedQMimeType (
                  buildQMimeType (
                      qMimeTypeName(),
                      qMimeTypeGenericIconName(),
                      qMimeTypeIconName(),
                      qMimeTypeGlobPatterns()
                  )
              );

    QCOMPARE(instantiatedQMimeType.iconName(), qMimeTypeIconName());
}

// ------------------------------------------------------------------------------------------------

void tst_qmimetype::suffixes()
{
    QMimeType instantiatedQMimeType (
                  buildQMimeType (
                      qMimeTypeName(),
                      qMimeTypeGenericIconName(),
                      qMimeTypeIconName(),
                      qMimeTypeGlobPatterns()
                  )
              );

    QCOMPARE(instantiatedQMimeType.globPatterns(), qMimeTypeGlobPatterns());
    QCOMPARE(instantiatedQMimeType.suffixes(), QStringList() << QString::fromLatin1("png"));
}

// ------------------------------------------------------------------------------------------------

QTEST_GUILESS_MAIN(tst_qmimetype)
#include "tst_qmimetype.moc"
