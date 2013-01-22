/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

class tst_QMimeDatabase: public QObject
{

    Q_OBJECT

private slots:
    void inheritsPerformance();
};

void tst_QMimeDatabase::inheritsPerformance()
{
    // Check performance of inherits().
    // This benchmark (which started in 2009 in kmimetypetest.cpp) uses 40 mimetypes.
    QStringList mimeTypes;
    mimeTypes << QLatin1String("image/jpeg") << QLatin1String("image/png") << QLatin1String("image/tiff") << QLatin1String("text/plain") << QLatin1String("text/html");
    mimeTypes += mimeTypes;
    mimeTypes += mimeTypes;
    mimeTypes += mimeTypes;
    QCOMPARE(mimeTypes.count(), 40);
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForName(QString::fromLatin1("text/x-chdr"));
    QVERIFY(mime.isValid());
    QBENCHMARK {
        QString match;
        foreach (const QString &mt, mimeTypes) {
            if (mime.inherits(mt)) {
                match = mt;
                // of course there would normally be a "break" here, but we're testing worse-case
                // performance here
            }
        }
        QCOMPARE(match, QString::fromLatin1("text/plain"));
    }
    // Numbers from 2011, in release mode:
    // KDE 4.7 numbers: 0.21 msec / 494,000 ticks / 568,345 instr. loads per iteration
    // QMimeBinaryProvider (with Qt 5): 0.16 msec / NA / 416,049 instr. reads per iteration
    // QMimeXmlProvider (with Qt 5): 0.062 msec / NA / 172,889 instr. reads per iteration
    //   (but the startup time is way higher)
    // And memory usage is flat at 200K with QMimeBinaryProvider, while it peaks at 6 MB when
    // parsing XML, and then keeps being around 4.5 MB for all the in-memory hashes.
}

QTEST_MAIN(tst_QMimeDatabase)
#include "main.moc"
