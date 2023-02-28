// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets/qfiledialog.h>
#include <QtWidgets/qapplication.h>
#include <QtCore/qmimedatabase.h>
#include <QtCore/qelapsedtimer.h>
#include <QtConcurrent/qtconcurrentmap.h>

typedef QMap<QString, int> WordCount;

void printHighestResult(const WordCount &, qsizetype);
QStringList findFiles(const QString &);

// Single threaded word counter function.
WordCount singleThreadedWordCount(const QStringList &files)
{
    WordCount wordCount;
    for (const QString &file : files) {
        QFile f(file);
        f.open(QIODevice::ReadOnly);
        QTextStream textStream(&f);
        while (!textStream.atEnd()) {
            const auto words =  textStream.readLine().split(' ', Qt::SkipEmptyParts);
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
        const auto words =  textStream.readLine().split(' ', Qt::SkipEmptyParts);
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
    app.setOrganizationName("QtProject");
    app.setApplicationName(QCoreApplication::translate("main", "Word Count"));

    QFileDialog fileDialog;
    fileDialog.setOption(QFileDialog::ReadOnly);
    // Grab the directory path from the dialog
    auto dirPath = QFileDialog::getExistingDirectory(nullptr,
                                QCoreApplication::translate("main","Select a Folder"),
                                QDir::currentPath());

    QStringList files = findFiles(dirPath);
    qDebug() << QCoreApplication::translate("main", "Indexing %1 files in %2")
        .arg(files.size()).arg(dirPath);

    // Start the single threaded operation
    qint64 singleThreadTime;
    {
        QElapsedTimer timer;
        timer.start();
    //! [1]
        WordCount total = singleThreadedWordCount(files);
    //! [1]
        singleThreadTime = timer.elapsed();
        qDebug() << QCoreApplication::translate("main", "Single threaded scanning took %1 ms")
            .arg(singleThreadTime);
    }
    // Start the multithreaded mappedReduced operation.
    qint64 mapReduceTime;
    {
        QElapsedTimer timer;
        timer.start();
    //! [2]
        WordCount total = QtConcurrent::mappedReduced(files, countWords, reduce).result();
    //! [2]
        mapReduceTime = timer.elapsed();
        qDebug() << QCoreApplication::translate("main", "MapReduce scanning took %1 ms")
            .arg(mapReduceTime);
        qDebug() << QCoreApplication::translate("main", "MapReduce speedup: %1")
            .arg(((double)singleThreadTime - (double)mapReduceTime) / (double)mapReduceTime + 1);
        printHighestResult(total, 10);
    }
}

// Utility function that recursively searches for text files.
QStringList findFiles(const QString &startDir)
{
    QStringList names;
    QDir dir(startDir);
    static const QMimeDatabase db;

    const auto files = dir.entryList(QDir::Files);
    for (const QString &file : files) {
        const auto path = startDir + QDir::separator() + file;
        const QMimeType mime = db.mimeTypeForFile(QFileInfo(path));
        const auto mimeTypesForFile = mime.parentMimeTypes();

        for (const auto &i : mimeTypesForFile) {
            if (i.contains("text", Qt::CaseInsensitive)
                || mime.comment().contains("text", Qt::CaseInsensitive)) {
                names += startDir + QDir::separator() + file;
            }
        }
    }

    const auto subdirs =  dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    for (const QString &subdir : subdirs) {
        if (names.length() >= 20000) {
            qDebug() << QCoreApplication::translate("main", "Too many files! Aborting ...");
            exit(-1);
        }
        names += findFiles(startDir + QDir::separator() + subdir);
    }
    return names;
}

// Utility function that prints the results of the map in decreasing order based on the value.
void printHighestResult(const WordCount &countedWords, qsizetype nResults)
{
    using pair = QPair<QString, int>;
    QList<pair> vec;

    std::copy(countedWords.keyValueBegin(), countedWords.keyValueEnd(),
              std::back_inserter<QList<pair>>(vec));
    std::sort(vec.begin(), vec.end(),
              [](const pair &l, const pair &r) { return l.second > r.second; });

    qDebug() << QCoreApplication::translate("main", "Most occurring words are:");
    for (qsizetype i = 0; i < qMin(vec.size(), nResults); ++i)
        qDebug() << vec[i].first << " : " << vec[i].second;
}
