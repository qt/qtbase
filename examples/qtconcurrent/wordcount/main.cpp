/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QList>
#include <QMap>
#include <QTextStream>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QElapsedTimer>
#include <QApplication>
#include <QDebug>

#include <qtconcurrentmap.h>

using namespace QtConcurrent;

/*
    Utility function that recursivily searches for files.
*/
QStringList findFiles(const QString &startDir, const QStringList &filters)
{
    QStringList names;
    QDir dir(startDir);

    const auto files = dir.entryList(filters, QDir::Files);
    for (const QString &file : files)
        names += startDir + '/' + file;

    const auto subdirs =  dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    for (const QString &subdir : subdirs)
        names += findFiles(startDir + '/' + subdir, filters);
    return names;
}

typedef QMap<QString, int> WordCount;

/*
    Single threaded word counter function.
*/
WordCount singleThreadedWordCount(const QStringList &files)
{
    WordCount wordCount;
    for (const QString &file : files) {
        QFile f(file);
        f.open(QIODevice::ReadOnly);
        QTextStream textStream(&f);
        while (!textStream.atEnd()) {
            const auto words =  textStream.readLine().split(' ');
            for (const QString &word : words)
                wordCount[word] += 1;
        }
    }
    return wordCount;
}


// countWords counts the words in a single file. This function is
// called in parallel by several threads and must be thread
// safe.
WordCount countWords(const QString &file)
{
    QFile f(file);
    f.open(QIODevice::ReadOnly);
    QTextStream textStream(&f);
    WordCount wordCount;

    while (!textStream.atEnd()) {
        const auto words =  textStream.readLine().split(' ');
        for (const QString &word : words)
            wordCount[word] += 1;
    }

    return wordCount;
}

// reduce adds the results from map to the final
// result. This functor will only be called by one thread
// at a time.
void reduce(WordCount &result, const WordCount &w)
{
    for (auto i = w.begin(), end = w.end(); i != end; ++i)
        result[i.key()] += i.value();
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    qDebug() << "finding files...";
    QStringList files = findFiles("../../", QStringList() << "*.cpp" << "*.h");
    qDebug() << files.count() << "files";

    qDebug() << "warmup";
    {
        WordCount total = singleThreadedWordCount(files);
    }

    qDebug() << "warmup done";

    int singleThreadTime = 0;
    {
        QElapsedTimer timer;
        timer.start();
        WordCount total = singleThreadedWordCount(files);
        singleThreadTime = timer.elapsed();
        qDebug() << "single thread" << singleThreadTime;
    }

    int mapReduceTime = 0;
    {
        QElapsedTimer timer;
        timer.start();
        WordCount total = mappedReduced(files, countWords, reduce);
        mapReduceTime = timer.elapsed();
        qDebug() << "MapReduce" << mapReduceTime;
    }
    qDebug() << "MapReduce speedup x" << ((double)singleThreadTime - (double)mapReduceTime) / (double)mapReduceTime + 1;
}
