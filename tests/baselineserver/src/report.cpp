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
#include "report.h"
#include "baselineprotocol.h"
#include "baselineserver.h"
#include <QDir>
#include <QProcess>
#include <QUrl>
#include <QXmlStreamWriter>

Report::Report()
    : initialized(false), handler(0), written(false), numItems(0), numMismatches(0), settings(0),
      hasStats(false)
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

int Report::numberOfMismatches()
{
    return numMismatches;
}

bool Report::reportProduced()
{
    return written;
}

void Report::init(const BaselineHandler *h, const QString &r, const PlatformInfo &p, const QSettings *s)
{
    handler = h;
    runId = r;
    plat = p;
    settings = s;
    rootDir = BaselineServer::storagePath() + QLC('/');
    baseDir = handler->pathForItem(ImageItem(), true, false).remove(QRegExp("/baselines/.*$"));
    QString dir = baseDir + (plat.isAdHocRun() ? QLS("/adhoc-reports") : QLS("/auto-reports"));
    QDir cwd;
    if (!cwd.exists(rootDir + dir))
        cwd.mkpath(rootDir + dir);
    path = dir + QLS("/Report_") + runId + QLS(".html");
    hasOverride = !plat.overrides().isEmpty();
    initialized = true;
}

void Report::addItems(const ImageItemList &items)
{
    if (items.isEmpty())
        return;
    numItems += items.size();
    QString func = items.at(0).testFunction;
    if (!testFunctions.contains(func))
        testFunctions.append(func);
    ImageItemList list = items;
    if (settings->value("ReportMissingResults").toBool()) {
        for (ImageItemList::iterator it = list.begin(); it != list.end(); ++it) {
            if (it->status == ImageItem::Ok)
                it->status = ImageItem::Error;   // Status should be set by report from client, else report as error
        }
    }
    itemLists[func] += list;
}

void Report::addResult(const ImageItem &item)
{
    if (!testFunctions.contains(item.testFunction)) {
        qWarning() << "Report::addResult: unknown testfunction" << item.testFunction;
        return;
    }
    bool found = false;
    ImageItemList &list = itemLists[item.testFunction];
    for (ImageItemList::iterator it = list.begin(); it != list.end(); ++it) {
        if (it->itemName == item.itemName && it->itemChecksum == item.itemChecksum) {
            it->status = item.status;
            found = true;
            break;
        }
    }
    if (found) {
        if (item.status == ImageItem::Mismatch)
            numMismatches++;
    } else {
        qWarning() << "Report::addResult: unknown item" << item.itemName << "in testfunction" << item.testFunction;
    }
}

void Report::end()
{
    if (!initialized || written)
        return;
    // Make report iff (#mismatches>0) || (#fuzzymatches>0) || (#errors>0 && settings say report errors)
    bool doReport = (numMismatches > 0);
    if (!doReport) {
        bool reportErrors = settings->value("ReportMissingResults").toBool();
        computeStats();
        foreach (const QString &func, itemLists.keys()) {
            FuncStats stat = stats.value(func);
            if (stat.value(ImageItem::FuzzyMatch) > 0) {
                doReport = true;
                break;
            }
            foreach (const ImageItem &item, itemLists.value(func)) {
                if (reportErrors && item.status == ImageItem::Error) {
                    doReport = true;
                    break;
                }
            }
            if (doReport)
                break;
        }
    }
    if (!doReport)
        return;
    write();
    written = true;
}

void Report::computeStats()
{
    if (hasStats)
        return;
    foreach (const QString &func, itemLists.keys()) {
        FuncStats funcStat;
        funcStat[ImageItem::Ok] = 0;
        funcStat[ImageItem::BaselineNotFound] = 0;
        funcStat[ImageItem::IgnoreItem] = 0;
        funcStat[ImageItem::Mismatch] = 0;
        funcStat[ImageItem::FuzzyMatch] = 0;
        funcStat[ImageItem::Error] = 0;
        foreach (const ImageItem &item, itemLists.value(func)) {
            funcStat[item.status]++;
        }
        stats[func] = funcStat;
    }
    hasStats = true;
}

QString Report::summary()
{
    computeStats();
    QString res;
    foreach (const QString &func, itemLists.keys()) {
        FuncStats stat = stats.value(func);
        QString s = QString("%1 %3 mismatch(es), %4 error(s), %5 fuzzy match(es)\n");
        s = s.arg(QString("%1() [%2 items]:").arg(func).arg(itemLists.value(func).size()).leftJustified(40));
        s = s.arg(stat.value(ImageItem::Mismatch));
        s = s.arg(stat.value(ImageItem::Error));
        s = s.arg(stat.value(ImageItem::FuzzyMatch));
        res += s;
    }
#if 0
    qDebug() << "***************************** Summary *************************";
    qDebug() << res;
    qDebug() << "***************************************************************";
#endif
    return res;
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
    updateLatestPointer();
}


void Report::writeHeader()
{
    QString title = plat.value(PI_Project) + QLC(':') + plat.value(PI_TestCase) + QLS(" Lancelot Test Report");
    out << "<!DOCTYPE html>\n"
        << "<html><head><title>" << title << "</title></head>\n"
        << "<body bgcolor=""#ddeeff""><h1>" << title << "</h1>\n"
        << "<p>Note: This is a <i>static</i> page, generated at " << QDateTime::currentDateTime().toString()
        << " for the test run with id " << runId << "</p>\n"
        << "<p>Summary: <b><span style=\"color:red\">" << numMismatches << " of " << numItems << "</span></b> items reported mismatching</p>\n";
    out << "<pre>\n" << summary() << "</pre>\n\n";
    out << "<h3>Testing Client Platform Info:</h3>\n"
        << "<table>\n";
    foreach (QString key, plat.keys())
        out << "<tr><td>" << key << ":</td><td>" << plat.value(key) << "</td></tr>\n";
    out << "</table>\n\n";
    if (hasOverride) {
        out << "<span style=\"color:red\"><h4>Note! Override Platform Info:</h4></span>\n"
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
        QString mmPrefix = handler->pathForItem(item, false, false);
        QString blPrefix = handler->pathForItem(item, true, false);

        // Make hard links to the current baseline, so that the report is static even if the baseline changes
        generateThumbnail(blPrefix + QLS(FileFormat), rootDir);   // Make sure baseline thumbnail is up to date
        QString lnPrefix = mmPrefix + QLS("baseline.");
        QByteArray blPrefixBa = (rootDir + blPrefix).toLatin1();
        QByteArray lnPrefixBa = (rootDir + lnPrefix).toLatin1();
        ::link((blPrefixBa + FileFormat).constData(), (lnPrefixBa + FileFormat).constData());
        ::link((blPrefixBa + MetadataFileExt).constData(), (lnPrefixBa + MetadataFileExt).constData());
        ::link((blPrefixBa + ThumbnailExt).constData(), (lnPrefixBa + ThumbnailExt).constData());

        QString baseline = lnPrefix + QLS(FileFormat);
        QString metadata = lnPrefix + QLS(MetadataFileExt);
        out << "<tr>\n";
        out << "<td>" << item.itemName << "</td>\n";
        if (item.status == ImageItem::Mismatch || item.status == ImageItem::FuzzyMatch) {
            QString rendered = mmPrefix + QLS(FileFormat);
            QString itemFile = mmPrefix.section(QLC('/'), -1);
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
            case ImageItem::Error:
                out << "<span style=\"background-color:red\">Error: No result reported!</span>";
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
        out << "<td height=246 align=center><a href=\"/" << img << "\"><img src=\"/" << generateThumbnail(img, rootDir) << "\"></a></td>\n";

    out << "<td align=center>\n";
    if (item.status == ImageItem::FuzzyMatch)
        out << "<p><span style=\"color:orange\">Fuzzy match</span></p>\n";
    else
        out << "<p><span style=\"color:red\">Mismatch reported</span></p>\n";
    out << "<p><a href=\"/" << metadata << "\">Baseline Info</a>\n";
    if (!hasOverride) {
        out << "<p><a href=\"/cgi-bin/server.cgi?cmd=updateSingleBaseline&context=" << ctx << "&mismatchContext=" << misCtx
            << "&itemFile=" << itemFile << "&url=" << pageUrl << "\">Let this be the new baseline</a></p>\n"
            << "<p><a href=\"/cgi-bin/server.cgi?cmd=blacklist&context=" << ctx
            << "&itemId=" << item.itemName << "&url=" << pageUrl << "\">Blacklist this item</a></p>\n";
    }
    out << "<p><a href=\"/cgi-bin/server.cgi?cmd=view&baseline=" << baseline << "&rendered=" << rendered
        << "&compared=" << compared << "&url=" << pageUrl << "\">Inspect</a></p>\n";

#if 0
    out << "<p><a href=\"/cgi-bin/server.cgi?cmd=diffstats&baseline=" << baseline << "&rendered=" << rendered
        << "&url=" << pageUrl << "\">Diffstats</a></p>\n";
#endif

    out << "</td>\n";
}

void Report::writeFooter()
{
    out << "\n</body></html>\n";
}


QString Report::generateCompared(const QString &baseline, const QString &rendered, bool fuzzy)
{
    QString res = rendered;
    QFileInfo fi(res);
    res.chop(fi.suffix().length());
    res += QLS(fuzzy ? "fuzzycompared.png" : "compared.png");
    QStringList args;
    if (fuzzy)
        args << QLS("-fuzz") << QLS("5%");
    args << rootDir+baseline << rootDir+rendered << rootDir+res;
    QProcess::execute(QLS("compare"), args);
    return res;
}


QString Report::generateThumbnail(const QString &image, const QString &rootDir)
{
    QString res = image;
    QFileInfo imgFI(rootDir+image);
    if (!imgFI.exists())
        return res;
    res.chop(imgFI.suffix().length());
    res += ThumbnailExt;
    QFileInfo resFI(rootDir+res);
    if (resFI.exists() && resFI.lastModified() > imgFI.lastModified())
        return res;
    QStringList args;
    args << rootDir+image << QLS("-resize") << QLS("240x240>") << QLS("-quality") << QLS("50") << rootDir+res;
    QProcess::execute(QLS("convert"), args);
    return res;
}


QString Report::writeResultsXmlFiles()
{
    if (!itemLists.size())
        return QString();
    QString dir = rootDir + baseDir + QLS("/xml-reports/") + runId;
    QDir cwd;
    if (!cwd.exists(dir))
        cwd.mkpath(dir);
    foreach (const QString &func, itemLists.keys()) {
        QFile f(dir + "/" + func + "-results.xml");
        if (!f.open(QIODevice::WriteOnly))
            continue;
        QXmlStreamWriter s(&f);
        s.setAutoFormatting(true);
        s.writeStartDocument();
        foreach (QString key, plat.keys()) {
            QString cmt = " " + key + "=\"" + plat.value(key) +"\" ";
            s.writeComment(cmt.replace("--", "[-]"));
        }
        s.writeStartElement("testsuite");
        s.writeAttribute("name", func);
        foreach (const ImageItem &item, itemLists.value(func)) {
            QString res;
            switch (item.status) {
            case ImageItem::Ok:
            case ImageItem::FuzzyMatch:
                res = "pass";
                break;
            case ImageItem::Mismatch:
            case ImageItem::Error:
                res = "fail";
                break;
            case ImageItem::BaselineNotFound:
            case ImageItem::IgnoreItem:
            default:
                res = "skip";
            }
            s.writeStartElement("testcase");
            s.writeAttribute("name", item.itemName);
            s.writeAttribute("result", res);
            s.writeEndElement();
        }
        s.writeEndElement();
        s.writeEndDocument();
    }
    return dir;
}



void Report::updateLatestPointer()
{
    QString linkPath = rootDir + baseDir + QLS("/latest_report.html");
    QString reportPath = path.mid(baseDir.size()+1);
    QFile::remove(linkPath);             // possible race with another thread, yada yada yada
    QFile::link(reportPath, linkPath);

#if 0
    QByteArray fwd = "<!DOCTYPE html><html><head><meta HTTP-EQUIV=\"refresh\" CONTENT=\"0;URL=%1\"></meta></head><body></body></html>\n";
    fwd.replace("%1", filePath().prepend(QLC('/')).toLatin1());

    QFile file(rootDir + baseDir + "/latest_report.html");
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        file.write(fwd);
#endif
}


void Report::handleCGIQuery(const QString &query)
{
    QUrl cgiUrl(QLS("http://dummy/cgi-bin/dummy.cgi?") + query);
    QTextStream s(stdout);
    s << "Content-Type: text/html\r\n\r\n"
      << "<!DOCTYPE html>\n<HTML>\n<body bgcolor=""#ddeeff"">\n";          // Lancelot blue

    QString command(cgiUrl.queryItemValue("cmd"));

    if (command == QLS("view")) {
        s << BaselineHandler::view(cgiUrl.queryItemValue(QLS("baseline")),
                                   cgiUrl.queryItemValue(QLS("rendered")),
                                   cgiUrl.queryItemValue(QLS("compared")));
    }
#if 0
    else if (command == QLS("diffstats")) {
        s << BaselineHandler::diffstats(cgiUrl.queryItemValue(QLS("baseline")),
                                        cgiUrl.queryItemValue(QLS("rendered")));
    }
#endif
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
    s << "<p><a href=\"" << cgiUrl.queryItemValue(QLS("url")) << "\">Back to report</a>\n";
    s << "</body>\n</HTML>";
}
