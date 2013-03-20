/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite module of the Qt Toolkit.
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

#include <QtTest/QTest>

#include <QDirIterator>
#include <QFile>
#include <QString>
#include <QStack>

#include "../../../../../shared/filesystem.h"

class bench_QDir_tree
    : public QObject
{
    Q_OBJECT

public:
    bench_QDir_tree()
        : prefix("./test-tree/"),
          musicprefix(QLatin1String("music")),
          photoprefix(QLatin1String("photos")),
          sourceprefix(QLatin1String("source")),
          musicsize(0),
          photosize(0),
          sourcesize(0)
    {
    }

private:
    QByteArray prefix;
    QString musicprefix;
    QString photoprefix;
    QString sourceprefix;
    qint64 musicsize;
    qint64 photosize;
    qint64 sourcesize;

private slots:
    void initTestCase()
    {
        QFile list(":/4.6.0-list.txt");
        QVERIFY(list.open(QIODevice::ReadOnly | QIODevice::Text));

        QVERIFY(fs.createDirectory(prefix));

        QStack<QByteArray> stack;
        QByteArray line;
        Q_FOREVER {
            char ch;
            if (!list.getChar(&ch))
                break;
            if (ch != ' ') {
                line.append(ch);
                continue;
            }

            int pop = 1;
            if (!line.isEmpty())
                pop = line.toInt();

            while (pop) {
                stack.pop();
                --pop;
            }

            line = list.readLine();
            line.chop(1);
            stack.push(line);

            line = prefix;
            Q_FOREACH(const QByteArray &pathElement, stack)
                line += pathElement;

            if (line.endsWith('/'))
                QVERIFY(fs.createDirectory(line));
            else
                QVERIFY(fs.createFile(line));

            line.clear();
        }

        //Use case: music collection - 10 files in 100 directories (albums)
        QVERIFY(fs.createDirectory(musicprefix));
        for (int i=0;i<1000;i++) {
            if ((i % 10) == 0)
                QVERIFY(fs.createDirectory(QString("%1/directory%2").arg(musicprefix).arg(i/10)));
            qint64 size = fs.createFileWithContent(QString("%1/directory%2/file%3").arg(musicprefix).arg(i/10).arg(i));
            QVERIFY(size > 0);
            musicsize += size;
        }
        //Use case: photos - 1000 files in 1 directory
        QVERIFY(fs.createDirectory(photoprefix));
        for (int i=0;i<1000;i++) {
            qint64 size = fs.createFileWithContent(QString("%1/file%2").arg(photoprefix).arg(i));
            QVERIFY(size > 0);
            photosize += size;
        }
        //Use case: source - 10 files in 10 subdirectories in 10 directories (1000 total)
        QVERIFY(fs.createDirectory(sourceprefix));
        for (int i=0;i<1000;i++) {
            if ((i % 100) == 0)
                QVERIFY(fs.createDirectory(QString("%1/directory%2").arg(sourceprefix).arg(i/100)));
            if ((i % 10) == 0)
                QVERIFY(fs.createDirectory(QString("%1/directory%2/subdirectory%3").arg(sourceprefix).arg(i/100).arg(i/10)));
            qint64 size = fs.createFileWithContent(QString("%1/directory%2/subdirectory%3/file%4").arg(sourceprefix).arg(i/100).arg(i/10).arg(i));
            QVERIFY(size > 0);
            sourcesize += size;
        }
    }

    void fileSearch_data() const
    {
        QTest::addColumn<QStringList>("nameFilters");
        QTest::addColumn<int>("filter");
        QTest::addColumn<int>("entryCount");

        QTest::newRow("*.cpp") << QStringList("*.cpp")
            << int(QDir::Files)
            << 3813;

        QTest::newRow("executables") << QStringList("*")
            << int(QDir::Executable | QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot)
            << 543;
    }

    void fileSearch() const
    {
        QFETCH(QStringList, nameFilters);
        QFETCH(int, filter);
        QFETCH(int, entryCount);

        int count = 0;
        QBENCHMARK {
            // Recursive directory iteration
            QDirIterator iterator(prefix, nameFilters, QDir::Filter(filter),
                QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);

            count = 0;
            while (iterator.hasNext()) {
                iterator.next();
                ++count;
            }

            QCOMPARE(count, entryCount);
        }

        QCOMPARE(count, entryCount);
    }

    void traverseDirectory() const
    {
        int count = 0;
        QBENCHMARK {
            QDirIterator iterator(prefix,
                    QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
                    QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);

            count = 0;
            while (iterator.hasNext()) {
                iterator.next();
                ++count;
            }

            QCOMPARE(count, 11963);
        }

        QCOMPARE(count, 11963);
    }

    void thousandFiles_data() const
    {
        QTest::addColumn<QString>("dirName");
        QTest::addColumn<qint64>("expectedSize");
        QTest::newRow("music") << musicprefix << musicsize;
        QTest::newRow("photos") << photoprefix << photosize;
        QTest::newRow("src") << sourceprefix << sourcesize;
    }

    void thousandFiles() const
    {
        QFETCH(QString, dirName);
        QFETCH(qint64, expectedSize);
        QBENCHMARK {
            qint64 totalsize = 0;
            int count = 0;
            QDirIterator iter(dirName, QDir::Files, QDirIterator::Subdirectories);
            while(iter.hasNext()) {
                iter.next();
                count++;
                totalsize += iter.fileInfo().size();
            }
            QCOMPARE(count, 1000);
            QCOMPARE(totalsize, expectedSize);
        }
    }
private:
    FileSystem fs;
};

QTEST_MAIN(bench_QDir_tree)
#include "bench_qdir_tree.moc"
