/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtTest/QtTest>
#include <QtCore/qdiriterator.h>

class tst_Android : public QObject
{
Q_OBJECT
private slots:
    void assetsRead();
    void assetsNotWritable();
    void assetsIterating();
};

void tst_Android::assetsRead()
{
    {
        QFile file("assets:/test.txt");
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("FooBar"));
    }

    {
        QFile file("assets:/test.txt");
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QCOMPARE(file.readAll(), QByteArray("FooBar"));
    }
}

void tst_Android::assetsNotWritable()
{
    QFile file("assets:/test.txt");
    QVERIFY(!file.open(QIODevice::WriteOnly));
    QVERIFY(!file.open(QIODevice::ReadWrite));
    QVERIFY(!file.open(QIODevice::Append));
}

void tst_Android::assetsIterating()
{
    QStringList assets = {"assets:/top_level_dir/file_in_top_dir.txt",
                          "assets:/top_level_dir/sub_dir",
                          "assets:/top_level_dir/sub_dir/file_in_sub_dir.txt",
                          "assets:/top_level_dir/sub_dir/sub_dir_2",
                          "assets:/top_level_dir/sub_dir/sub_dir_2/sub_dir_3",
                          "assets:/top_level_dir/sub_dir/sub_dir_2/sub_dir_3/file_in_sub_dir_3.txt"};
    // Note that we have an "assets:/top_level_dir/sub_dir/empty_sub_dir" in the test's
    // assets physical directory, but empty folders are not packaged in the built apk,
    // so it's expected to not have such folder be listed in the assets on runtime
    QDirIterator it("assets:/top_level_dir", QDirIterator::Subdirectories);
    QStringList iteratorAssets;
    while (it.hasNext())
        iteratorAssets.append(it.next());
    QVERIFY(assets == iteratorAssets);
}


QTEST_MAIN(tst_Android)
#include "tst_android.moc"

