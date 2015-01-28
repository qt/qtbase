/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef FAKEDIRMODEL_H
#define FAKEDIRMODEL_H

#include <QtGui/QStandardItemModel>
#include <QtGui/QStandardItem>
#include <QtGui/QIcon>
#include <QtGui/QPixmap>
#include <QtGui/QImage>
#include <QtCore/QStringList>

typedef QList<QStandardItem *> StandardItemList;

static inline QIcon coloredIcon(Qt::GlobalColor color)
{
    QImage image(22, 22, QImage::Format_ARGB32);
    image.fill(color);
    return QPixmap::fromImage(image);
}

static void addFileEntry(const StandardItemList &directory, const QString &name, const QString &size)
{
    static const QIcon fileIcon = coloredIcon(Qt::blue);
    directory.front()->appendRow(StandardItemList() << new QStandardItem(fileIcon, name) << new QStandardItem(size));
}

static StandardItemList createDirEntry(const QString &name)
{
    static const QIcon dirIcon = coloredIcon(Qt::red);
    StandardItemList result;
    result << new QStandardItem(dirIcon, name) << new QStandardItem;
    return result;
}

static inline StandardItemList addDirEntry(const StandardItemList &directory, const QString &name)
{
    const StandardItemList entry = createDirEntry(name);
    directory.front()->appendRow(entry);
    return entry;
}

static QStandardItem *populateFakeDirModel(QStandardItemModel *model)
{
    enum Columns { NameColumn, SizeColumn, ColumnCount };

    model->setColumnCount(ColumnCount);
    model->setHorizontalHeaderLabels(QStringList() << QStringLiteral("Name") << QStringLiteral("Size"));

    const StandardItemList root = createDirEntry(QStringLiteral("/"));
    model->appendRow(root);

    const StandardItemList binDir = addDirEntry(root, QStringLiteral("bin"));
    addFileEntry(binDir, QStringLiteral("ls"), QStringLiteral("100 KB"));
    addFileEntry(binDir, QStringLiteral("bash"), QStringLiteral("200 KB"));

    const StandardItemList devDir = addDirEntry(root, QStringLiteral("dev"));
    addFileEntry(devDir, QStringLiteral("tty1"), QStringLiteral("0 B"));
    addDirEntry(devDir, QStringLiteral("proc"));

    const StandardItemList etcDir = addDirEntry(root, QStringLiteral("etc"));
    addFileEntry(etcDir, QStringLiteral("foo1.config"), QStringLiteral("1 KB"));
    addFileEntry(etcDir, QStringLiteral("foo2.conf"), QStringLiteral("654 B"));

    const StandardItemList homeDir = addDirEntry(root, QStringLiteral("home"));
    addFileEntry(homeDir, QStringLiteral("file1"), QStringLiteral("1 KB"));

    const StandardItemList documentsDir = addDirEntry(homeDir, QStringLiteral("Documents"));
    addFileEntry(documentsDir, QStringLiteral("txt1.odt"), QStringLiteral("2 MB"));
    addFileEntry(documentsDir, QStringLiteral("sheet1.xls"), QStringLiteral("32 KB"));
    addFileEntry(documentsDir, QStringLiteral("foo.doc"), QStringLiteral("214 KB"));

    const StandardItemList downloadsDir = addDirEntry(homeDir, QStringLiteral("Downloads"));
    addFileEntry(downloadsDir, QStringLiteral("package1.zip"), QStringLiteral("34 MB"));
    addFileEntry(downloadsDir, QStringLiteral("package2.zip"), QStringLiteral("623 KB"));

    const StandardItemList picturesDir = addDirEntry(homeDir, QStringLiteral("Pictures"));
    addFileEntry(picturesDir, QStringLiteral("img0001.jpg"), QStringLiteral("4 MB"));
    addFileEntry(picturesDir, QStringLiteral("img0002.png"), QStringLiteral("10 MB"));

    // qcolumnview::moveCursor() requires an empty directory followed by another one.
    addDirEntry(root, QStringLiteral("lost+found"));

    const StandardItemList tmpDir = addDirEntry(root, QStringLiteral("tmp"));
    addFileEntry(tmpDir, "asdujhsdjys", "435 B");
    addFileEntry(tmpDir, "krtbldfhd", "5557 B");

    return homeDir.front();
}

#endif // FAKEDIRMODEL_H
