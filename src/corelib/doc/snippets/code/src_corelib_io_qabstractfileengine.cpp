// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
class ZipEngineHandler : public QAbstractFileEngineHandler
{
public:
    QAbstractFileEngine *create(const QString &fileName) const override;
};

QAbstractFileEngine *ZipEngineHandler::create(const QString &fileName) const
{
    // ZipEngineHandler returns a ZipEngine for all .zip files
    return fileName.toLower().endsWith(".zip") ? new ZipEngine(fileName) : 0;
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    ZipEngineHandler engine;

    MainWindow window;
    window.show();

    return app.exec();
}
//! [0]


//! [1]
QAbstractSocketEngine *ZipEngineHandler::create(const QString &fileName) const
{
    // ZipEngineHandler returns a ZipEngine for all .zip files
    return fileName.toLower().endsWith(".zip") ? new ZipEngine(fileName) : 0;
}
//! [1]


//! [2]
QAbstractFileEngineIterator *
CustomFileEngine::beginEntryList(QDir::Filters filters, const QStringList &filterNames)
{
    return new CustomFileEngineIterator(filters, filterNames);
}
//! [2]


//! [3]
class CustomIterator : public QAbstractFileEngineIterator
{
public:
    CustomIterator(const QStringList &nameFilters, QDir::Filters filters)
        : QAbstractFileEngineIterator(nameFilters, filters), index(0)
    {
        // In a real iterator, these entries are fetched from the
        // file system based on the value of path().
        entries << "entry1" << "entry2" << "entry3";
    }

    bool hasNext() const override
    {
        return index < entries.size() - 1;
    }

    QString next() override
    {
       if (!hasNext())
           return QString();
       ++index;
       return currentFilePath();
    }

    QString currentFileName() override
    {
        return entries.at(index);
    }

private:
    QStringList entries;
    int index;
};
//! [3]
