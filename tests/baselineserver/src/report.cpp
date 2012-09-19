/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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
#include "report.h"
#include "baselineprotocol.h"
#include "baselineserver.h"
#include <QDir>
#include <QProcess>
#include <QUrl>

Report::Report()
    : written(false), numItems(0), numMismatches(0)
{
}

Report::~Report()
{
    end();
}

QString Report::filePath()
{
    return path;
}

void Report::init(const BaselineHandler *h, const QString &r, const PlatformInfo &p)
{
    handler = h;
    runId = r;
    plat = p;
    rootDir = BaselineServer::storagePath() + QLC('/');
    reportDir = plat.value(PI_TestCase) + QLC('/') + (plat.isAdHocRun() ? QLS("reports/adhoc/") : QLS("reports/pulse/"));
    QString dir = rootDir + reportDir;
    QDir cwd;
    if (!cwd.exists(dir))
        cwd.mkpath(dir);
    path = reportDir + QLS("Report_") + runId + QLS(".html");
    hasOverride = !plat.overrides().isEmpty();
}

void Report::addItems(const ImageItemList &items)
{
    if (items.isEmpty())
        return;
    numItems += items.size();
    QString func = items.at(0).testFunction;
    if (!testFunctions.contains(func))
        testFunctions.append(func);
    itemLists[func] += items;
}

void Report::addMismatch(const ImageItem &item)
{
    if (!testFunctions.contains(item.testFunction)) {
        qWarning() << "Report::addMismatch: unknown testfunction" << item.testFunction;
        return;
    }
    bool found = false;
    ImageItemList &list = itemLists[item.testFunction];
    for (ImageItemList::iterator it = list.begin(); it != list.end(); ++it) {
        if (it->itemName == item.itemName && it->itemChecksum == item.itemChecksum) {
            it->status = ImageItem::Mismatch;
            found = true;
            break;
        }
    }
    if (found)
        numMismatches++;
    else
        qWarning() << "Report::addMismatch: unknown item" << item.itemName << "in testfunction" << item.testFunction;
}

void Report::end()
{
    if (written || !numMismatches)
        return;
    write();
    written = true;
}


void Report::write()
{
    QFile file(rootDir + path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Failed to open report file" << file.fileName();
        return;
    }
    out.setDevice(&file);

    writeHeader();
    foreach(const QString &func, testFunctions) {
        writeFunctionResults(itemLists.value(func));
    }
    writeFooter();
    file.close();
}


void Report::writeHeader()
{
    QString title = plat.value(PI_TestCase) + QLS(" Qt Baseline Test Report");
    out << "<head><title>" << title << "</title></head>\n"
        << "<html><body><h1>" << title << "</h1>\n"
        << "<p>Note: This is a <i>static</i> page, generated at " << QDateTime::currentDateTime().toString()
        << " for the test run with id " << runId << "</p>\n"
        << "<p>Summary: <b><span style=\"color:red\">" << numMismatches << " of " << numItems << "</b></span> items reported mismatching</p>\n\n";
    out << "<h3>Testing Client Platform Info:</h3>\n"
        << "<table>\n";
    foreach (QString key, plat.keys())
        out << "<tr><td>" << key << ":</td><td>" << plat.value(key) << "</td></tr>\n";
    out << "</table>\n\n";
    if (hasOverride) {
        out << "<span style=\"color:red\"><h4>Note! Platform Override Info:</h4></span>\n"
            << "<p>The client's output has been compared to baselines created on a different platform. Differences:</p>\n"
            << "<table>\n";
        for (int i = 0; i < plat.overrides().size()-1; i+=2)
            out << "<tr><td>" << plat.overrides().at(i) << ":</td><td>" << plat.overrides().at(i+1) << "</td></tr>\n";
        out << "</table>\n\n";
    }
}


void Report::writeFunctionResults(const ImageItemList &list)
{
    QString testFunction = list.at(0).testFunction;
    QString pageUrl = BaselineServer::baseUrl() + path;
    QString ctx = handler->pathForItem(list.at(0), true, false).section(QLC('/'), 0, -2);
    QString misCtx = handler->pathForItem(list.at(0), false, false).section(QLC('/'), 0, -2);


    out << "\n<p>&nbsp;</p><h3>Test function: " << testFunction << "</h3>\n";
    if (!hasOverride) {
        out << "<p><a href=\"/cgi-bin/server.cgi?cmd=clearAllBaselines&context=" << ctx << "&url=" << pageUrl
            << "\"><b>Clear all baselines</b></a> for this testfunction (They will be recreated by the next run)</p>\n";
        out << "<p><a href=\"/cgi-bin/server.cgi?cmd=updateAllBaselines&context=" << ctx << "&mismatchContext=" << misCtx << "&url=" << pageUrl
            << "\"><b>Let these mismatching images be the new baselines</b></a> for this testfunction</p>\n\n";
    }

    out << "<table border=\"2\">\n"
           "<tr>\n"
           "<th width=123>Item</th>\n"
           "<th width=246>Baseline</th>\n"
           "<th width=246>Rendered</th>\n"
           "<th width=246>Comparison (diffs are <span style=\"color:red\">RED</span>)</th>\n"
           "<th width=246>Info/Action</th>\n"
           "</tr>\n\n";

    foreach (const ImageItem &item, list) {
        out << "<tr>\n";
        out << "<td>" << item.itemName << "</td>\n";
        QString prefix = handler->pathForItem(item, true, false);
        QString baseline = prefix + QLS(FileFormat);
        QString metadata = prefix + QLS(MetadataFileExt);
        if (item.status == ImageItem::Mismatch) {
            QString rendered = handler->pathForItem(item, false, false) + QLS(FileFormat);
            QString itemFile = prefix.section(QLC('/'), -1);
            writeItem(baseline, rendered, item, itemFile, ctx, misCtx, metadata);
        }
        else {
            out << "<td align=center><a href=\"/" << baseline << "\">image</a> <a href=\"/" << metadata << "\">info</a></td>\n"
                << "<td align=center colspan=2><small>n/a</small></td>\n"
                << "<td align=center>";
            switch (item.status) {
            case ImageItem::BaselineNotFound:
                out << "Baseline not found/regenerated";
                break;
            case ImageItem::IgnoreItem:
                out << "<span style=\"background-color:yellow\">Blacklisted</span> ";
                if (!hasOverride) {
                    out << "<a href=\"/cgi-bin/server.cgi?cmd=whitelist&context=" << ctx
                        << "&itemId=" << item.itemName << "&url=" << pageUrl
                        << "\">Whitelist this item</a>";
                }
                break;
            case ImageItem::Ok:
                out << "<span style=\"color:green\"><small>No mismatch reported</small></span>";
                break;
            default:
                out << "?";
                break;
            }
            out << "</td>\n";
        }
        out << "</tr>\n\n";
    }

    out << "</table>\n";
}

void Report::writeItem(const QString &baseline, const QString &rendered, const ImageItem &item,
                       const QString &itemFile, const QString &ctx, const QString &misCtx, const QString &metadata)
{
    QString compared = generateCompared(baseline, rendered);
    QString pageUrl = BaselineServer::baseUrl() + path;

    QStringList images = QStringList() << baseline << rendered << compared;
    foreach (const QString& img, images)
        out << "<td height=246 align=center><a href=\"/" << img << "\"><img src=\"/" << generateThumbnail(img) << "\"></a></td>\n";

    out << "<td align=center>\n"
        << "<p><span style=\"color:red\">Mismatch reported</span></p>\n"
        << "<p><a href=\"/" << metadata << "\">Baseline Info</a>\n";
    if (!hasOverride) {
        out << "<p><a href=\"/cgi-bin/server.cgi?cmd=updateSingleBaseline&context=" << ctx << "&mismatchContext=" << misCtx
            << "&itemFile=" << itemFile << "&url=" << pageUrl << "\">Let this be the new baseline</a></p>\n"
            << "<p><a href=\"/cgi-bin/server.cgi?cmd=blacklist&context=" << ctx
            << "&itemId=" << item.itemName << "&url=" << pageUrl << "\">Blacklist this item</a></p>\n";
    }
    out << "<p><a href=\"/cgi-bin/server.cgi?cmd=view&baseline=" << baseline << "&rendered=" << rendered
        << "&compared=" << compared << "&url=" << pageUrl << "\">Inspect</a></p>\n"
        << "</td>\n";
}

void Report::writeFooter()
{
    out << "\n</body></html>\n";
}


QString Report::generateCompared(const QString &baseline, const QString &rendered, bool fuzzy)
{
    QString res = rendered;
    QFileInfo fi(res);
    res.chop(fi.suffix().length() + 1);
    res += QLS(fuzzy ? "_fuzzycompared.png" : "_compared.png");
    QStringList args;
    if (fuzzy)
        args << QLS("-fuzz") << QLS("5%");
    args << rootDir+baseline << rootDir+rendered << rootDir+res;
    QProcess::execute(QLS("compare"), args);
    return res;
}


QString Report::generateThumbnail(const QString &image)
{
    QString res = image;
    QFileInfo imgFI(rootDir+image);
    res.chop(imgFI.suffix().length() + 1);
    res += QLS("_thumbnail.jpg");
    QFileInfo resFI(rootDir+res);
    if (resFI.exists() && resFI.lastModified() > imgFI.lastModified())
        return res;
    QStringList args;
    args << rootDir+image << QLS("-resize") << QLS("240x240>") << QLS("-quality") << QLS("50") << rootDir+res;
    QProcess::execute(QLS("convert"), args);
    return res;
}


void Report::handleCGIQuery(const QString &query)
{
    QUrl cgiUrl(QLS("http://dummy/cgi-bin/dummy.cgi?") + query);
    QTextStream s(stdout);
    s << "Content-Type: text/html\r\n\r\n"
      << "<HTML>";

    QString command(cgiUrl.queryItemValue("cmd"));

    if (command == QLS("view")) {
        s << BaselineHandler::view(cgiUrl.queryItemValue(QLS("baseline")),
                                   cgiUrl.queryItemValue(QLS("rendered")),
                                   cgiUrl.queryItemValue(QLS("compared")));
    }
    else if (command == QLS("updateSingleBaseline")) {
        s << BaselineHandler::updateBaselines(cgiUrl.queryItemValue(QLS("context")),
                                              cgiUrl.queryItemValue(QLS("mismatchContext")),
                                              cgiUrl.queryItemValue(QLS("itemFile")));
    } else if (command == QLS("updateAllBaselines")) {
        s << BaselineHandler::updateBaselines(cgiUrl.queryItemValue(QLS("context")),
                                              cgiUrl.queryItemValue(QLS("mismatchContext")),
                                              QString());
    } else if (command == QLS("clearAllBaselines")) {
        s << BaselineHandler::clearAllBaselines(cgiUrl.queryItemValue(QLS("context")));
    } else if (command == QLS("blacklist")) {
        // blacklist a test
        s << BaselineHandler::blacklistTest(cgiUrl.queryItemValue(QLS("context")),
                                            cgiUrl.queryItemValue(QLS("itemId")));
    } else if (command == QLS("whitelist")) {
        // whitelist a test
        s << BaselineHandler::blacklistTest(cgiUrl.queryItemValue(QLS("context")),
                                            cgiUrl.queryItemValue(QLS("itemId")), true);
    } else {
        s << "Unknown query:<br>" << query << "<br>";
    }
    s << "<p><a href=\"" << cgiUrl.queryItemValue(QLS("url")) << "\">Back to report</a>";
    s << "</HTML>";
}
