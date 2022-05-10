// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
        WordCount total = mappedReduced(files, countWords, reduce).result();
        mapReduceTime = timer.elapsed();
        qDebug() << "MapReduce" << mapReduceTime;
    }
    qDebug() << "MapReduce speedup x" << ((double)singleThreadTime - (double)mapReduceTime) / (double)mapReduceTime + 1;
}
