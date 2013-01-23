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
#ifndef REPORT_H
#define REPORT_H

#include "baselineprotocol.h"
#include <QFile>
#include <QTextStream>
#include <QMap>
#include <QStringList>
#include <QSettings>

class BaselineHandler;

class Report
{
public:
    Report();
    ~Report();

    void init(const BaselineHandler *h, const QString &r, const PlatformInfo &p, const QSettings *s);
    void addItems(const ImageItemList& items);
    void addResult(const ImageItem& item);
    void end();

    bool reportProduced();

    int numberOfMismatches();
    QString summary();

    QString filePath();

    QString writeResultsXmlFiles();

    static void handleCGIQuery(const QString &query);

    static QString generateThumbnail(const QString &image, const QString &rootDir = QString());

private:
    void write();
    void writeFunctionResults(const ImageItemList &list);
    void writeItem(const QString &baseline, const QString &rendered, const ImageItem &item,
                   const QString &itemFile, const QString &ctx, const QString &misCtx, const QString &metadata);
    void writeHeader();
    void writeFooter();
    QString generateCompared(const QString &baseline, const QString &rendered, bool fuzzy = false);

    void updateLatestPointer();

    void computeStats();

    bool initialized;
    const BaselineHandler *handler;
    QString runId;
    PlatformInfo plat;
    QString rootDir;
    QString baseDir;
    QString path;
    QStringList testFunctions;
    QMap<QString, ImageItemList> itemLists;
    bool written;
    int numItems;
    int numMismatches;
    QTextStream out;
    bool hasOverride;
    const QSettings *settings;

    typedef QMap<ImageItem::ItemStatus, int> FuncStats;
    QMap<QString, FuncStats> stats;
    bool hasStats;
};

#endif // REPORT_H
