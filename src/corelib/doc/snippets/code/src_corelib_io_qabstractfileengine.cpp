// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
using namespace Qt::StringLiterals;
// ...
class ZipEngineHandler : public QAbstractFileEngineHandler
{
public:
    std::unique_ptr<QAbstractFileEngine> create(const QString &fileName) const override;
};

std::unique_ptr<QAbstractFileEngine> ZipEngineHandler::create(const QString &fileName) const
{
    // ZipEngineHandler returns a ZipEngine for all .zip files
    if (fileName.toLower().endsWith(".zip"_L1))
        return std::make_unique<ZipEngine>(fileName);
    return {};
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
std::unique_ptr<QAbstractFileEngine> ZipEngineHandler::create(const QString &fileName) const
{
    // ZipEngineHandler returns a ZipEngine for all .zip files
    if (fileName.toLower().endsWith(".zip"_L1))
        return std::make_unique<ZipEngine>(fileName);
    else
        return {};
}
//! [1]


//! [2]
QAbstractFileEngine::IteratorUniquePtr
CustomFileEngine::beginEntryList(const QString &path, QDir::Filters filters,
                                 const QStringList &filterNames)
{
    return std::make_unique<CustomFileEngineIterator>(path, filters, filterNames);
}
//! [2]


//! [3]
class CustomIterator : public QAbstractFileEngineIterator
{
public:
    CustomIterator(const QString &path, const QStringList &nameFilters, QDir::Filters filters)
        : QAbstractFileEngineIterator(path, nameFilters, filters), index(0)
    {
        // In a real iterator, these entries are fetched from the
        // file system based on the value of path().
        entries << "entry1" << "entry2" << "entry3";
    }

    bool advance() override
    {
        if (entries.isEmpty())
            return false;
        if (index < entries.size() - 1) {
            ++index;
            return true;
        }
        return false;
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
