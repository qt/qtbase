/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef REPORT_H
#define REPORT_H

#include "baselineprotocol.h"
#include <QFile>
#include <QTextStream>
#include <QMap>
#include <QStringList>

class BaselineHandler;

class Report
{
public:
    Report();
    ~Report();

    void init(const BaselineHandler *h, const QString &r, const PlatformInfo &p);
    void addItems(const ImageItemList& items);
    void addMismatch(const ImageItem& item);
    void end();

    QString filePath();

    static void handleCGIQuery(const QString &query);

private:
    void write();
    void writeFunctionResults(const ImageItemList &list);
    void writeItem(const QString &baseline, const QString &rendered, const ImageItem &item,
                   const QString &itemFile, const QString &ctx, const QString &misCtx, const QString &metadata);
    void writeHeader();
    void writeFooter();
    QString generateCompared(const QString &baseline, const QString &rendered, bool fuzzy = false);
    QString generateThumbnail(const QString &image);

    const BaselineHandler *handler;
    QString runId;
    PlatformInfo plat;
    QString rootDir;
    QString reportDir;
    QString path;
    QStringList testFunctions;
    QMap<QString, ImageItemList> itemLists;
    bool written;
    int numItems;
    int numMismatches;
    QTextStream out;
    bool hasOverride;
};

#endif // REPORT_H
