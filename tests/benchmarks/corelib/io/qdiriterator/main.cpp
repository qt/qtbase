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
#include <QDebug>
#include <QDirIterator>
#include <QString>

#ifdef Q_OS_WIN
#   include <qt_windows.h>
#else
#   include <sys/stat.h>
#   include <sys/types.h>
#   include <dirent.h>
#   include <errno.h>
#   include <string.h>
#endif

#include <qtest.h>

#include "qfilesystemiterator.h"

class tst_qdiriterator : public QObject
{
    Q_OBJECT
private slots:
    void posix();
    void posix_data() { data(); }
    void diriterator();
    void diriterator_data() { data(); }
    void fsiterator();
    void fsiterator_data() { data(); }
    void data();
};


void tst_qdiriterator::data()
{
#if defined(Q_OS_WINCE)
    QByteArray qtdir = qPrintable(QCoreApplication::applicationDirPath());
    qtdir += "/depot";
#else
#if defined(Q_OS_WIN)
    const char *qtdir = "C:\\depot\\qt\\main";
#else
    const char *qtdir = ::getenv("QTDIR");
#endif
    if (!qtdir) {
        fprintf(stderr, "QTDIR not set\n");
        exit(1);
    }
#endif

    QTest::addColumn<QByteArray>("dirpath");
    QByteArray ba = QByteArray(qtdir) + "/src/corelib";
    QByteArray ba1 = ba + "/io";
    QTest::newRow(ba) << ba;
    //QTest::newRow(ba1) << ba1;
}

#ifdef Q_OS_WIN
static int posix_helper(const wchar_t *dirpath)
{
    int count = 0;
    HANDLE hSearch;
    WIN32_FIND_DATA fd;

    const size_t origDirPathLength = wcslen(dirpath);

    wchar_t appendedPath[MAX_PATH];
    wcscpy(appendedPath, dirpath);
    wcscat(appendedPath, L"\\*");
    hSearch = FindFirstFile(appendedPath, &fd);
    appendedPath[origDirPathLength] = 0;

    if (hSearch == INVALID_HANDLE_VALUE) {
        qWarning("FindFirstFile failed");
        return count;
    }

    do {
        if (!(fd.cFileName[0] == L'.' && fd.cFileName[1] == 0) &&
            !(fd.cFileName[0] == L'.' && fd.cFileName[1] == L'.' && fd.cFileName[2] == 0))
        {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                wcscat(appendedPath, L"\\");
                wcscat(appendedPath, fd.cFileName);
                count += posix_helper(appendedPath);
                appendedPath[origDirPathLength] = 0;
            }
            else {
                ++count;
            }
        }
    } while (FindNextFile(hSearch, &fd));
    FindClose(hSearch);

    return count;
}

#else

static int posix_helper(const char *dirpath)
{
    //qDebug() << "DIR" << dirpath;
    DIR *dir = ::opendir(dirpath);
    if (!dir)
        return 0;

    dirent *entry = 0;

    int count = 0;
    while ((entry = ::readdir(dir))) {
        if (qstrcmp(entry->d_name, ".") == 0)
            continue;
        if (qstrcmp(entry->d_name, "..") == 0)
            continue;
        ++count;
        QByteArray ba = dirpath;
        ba += '/';
        ba += entry->d_name;
        struct stat st;
        lstat(ba.constData(), &st);
        if (S_ISDIR(st.st_mode))
            count += posix_helper(ba.constData());
    }

    ::closedir(dir);
    return count;
}
#endif


void tst_qdiriterator::posix()
{
    QFETCH(QByteArray, dirpath);

    int count = 0;
    QString path(dirpath);
    QBENCHMARK {
#ifdef Q_OS_WIN
        wchar_t wPath[MAX_PATH];
        path.toWCharArray(wPath);
        count = posix_helper(wPath);
#else
        count = posix_helper(dirpath.constData());
#endif
    }
    qDebug() << count;
}

void tst_qdiriterator::diriterator()
{
    QFETCH(QByteArray, dirpath);

    int count = 0;

    QBENCHMARK {
        int c = 0;

        QDirIterator dir(dirpath,
            //QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot,
            //QDir::AllEntries | QDir::Hidden,
            QDir::Files,
            QDirIterator::Subdirectories);

        while (dir.hasNext()) {
            dir.next();
            //printf("%s\n", qPrintable(dir.fileName()));
            0 && printf("%d %s\n",
                dir.fileInfo().isDir(),
                //qPrintable(dir.fileInfo().absoluteFilePath()),
                //qPrintable(dir.path()),
                qPrintable(dir.filePath()));
            ++c;
        }
        count = c;
    }
    qDebug() << count;
}

void tst_qdiriterator::fsiterator()
{
    QFETCH(QByteArray, dirpath);

    int count = 0;
    int dump = 0;

    QBENCHMARK {
        int c = 0;

        dump && printf("\n\n\n\n");
        QFileSystemIterator dir(dirpath,
            //QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot,
            //QDir::AllEntries | QDir::Hidden,
            //QDir::Files | QDir::NoDotAndDotDot,
            QDir::Files,
            QFileSystemIterator::Subdirectories);

        for (; !dir.atEnd(); dir.next()) {
            dump && printf("%d %s\n",
                dir.fileInfo().isDir(),
                //qPrintable(dir.fileInfo().absoluteFilePath()),
                //qPrintable(dir.path()),
                qPrintable(dir.filePath())
            );
            ++c;
        }
        count = c;
    }
    qDebug() << count;
}

QTEST_MAIN(tst_qdiriterator)

#include "main.moc"
