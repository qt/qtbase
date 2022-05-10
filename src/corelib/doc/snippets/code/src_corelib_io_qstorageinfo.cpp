// Copyright (C) 2014 Ivan Komissarov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
    QStorageInfo storage(qApp->applicationDirPath());
    if (storage.isValid() && storage.isReady()) {
        // ...
    }
//! [0]

//! [1]
    foreach (const QStorageInfo &storage, QStorageInfo::mountedVolumes()) {
        if (storage.isValid() && storage.isReady()) {
            if (!storage.isReadOnly()) {
                // ...
            }
        }
    }
//! [1]

//! [2]
    QStorageInfo storage = QStorageInfo::root();

    qDebug() << storage.rootPath();
    if (storage.isReadOnly())
        qDebug() << "isReadOnly:" << storage.isReadOnly();

    qDebug() << "name:" << storage.name();
    qDebug() << "fileSystemType:" << storage.fileSystemType();
    qDebug() << "size:" << storage.bytesTotal()/1000/1000 << "MB";
    qDebug() << "availableSize:" << storage.bytesAvailable()/1000/1000 << "MB";
//! [2]
