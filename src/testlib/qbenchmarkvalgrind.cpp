// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTest/private/qbenchmark_p.h>

#include <QtTest/private/qbenchmarkvalgrind_p.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qprocess.h>
#include <QtCore/qdir.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qset.h>
#include <QtTest/private/callgrind_p.h>

#include <charconv>
#include <optional>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// Returns \c true if valgrind is available.
bool QBenchmarkValgrindUtils::haveValgrind()
{
#ifdef NVALGRIND
    return false;
#else
    QProcess process;
    process.start(u"valgrind"_s, QStringList(u"--version"_s));
    return process.waitForStarted() && process.waitForFinished(-1);
#endif
}

// Reruns this program through callgrind.
// Returns \c true upon success, otherwise false.
bool QBenchmarkValgrindUtils::rerunThroughCallgrind(const QStringList &origAppArgs, int &exitCode)
{
    if (!QBenchmarkValgrindUtils::runCallgrindSubProcess(origAppArgs, exitCode)) {
        qWarning("failed to run callgrind subprocess");
        return false;
    }
    return true;
}

static void dumpOutput(const QByteArray &data, FILE *fh)
{
    QFile file;
    file.open(fh, QIODevice::WriteOnly);
    file.write(data);
}

qint64 QBenchmarkValgrindUtils::extractResult(const QString &fileName)
{
    QFile file(fileName);
    const bool openOk = file.open(QIODevice::ReadOnly | QIODevice::Text);
    Q_ASSERT(openOk);
    Q_UNUSED(openOk);

    std::optional<qint64> val = std::nullopt;
    while (!file.atEnd()) {
        const QByteArray line = file.readLine();
        constexpr QByteArrayView tag = "summary: ";
        if (line.startsWith(tag)) {
            const auto maybeNumber = line.data() + tag.size();
            const auto end = line.data() + line.size();
            qint64 v;
            const auto r = std::from_chars(maybeNumber, end, v);
            if (r.ec == std::errc{}) {
                val = v;
                break;
            }
        }
    }
    if (Q_UNLIKELY(!val))
        qFatal("Failed to extract result");
    return *val;
}

// Gets the newest file name (i.e. the one with the highest integer suffix).
QString QBenchmarkValgrindUtils::getNewestFileName()
{
    QStringList nameFilters;
    QString base = QBenchmarkGlobalData::current->callgrindOutFileBase;
    Q_ASSERT(!base.isEmpty());

    nameFilters << QString::fromLatin1("%1.*").arg(base);
    const QFileInfoList fiList = QDir().entryInfoList(nameFilters, QDir::Files | QDir::Readable);
    Q_ASSERT(!fiList.empty());
    int hiSuffix = -1;
    QFileInfo lastFileInfo;
    const QString pattern = QString::fromLatin1("%1.(\\d+)").arg(base);
    QRegularExpression rx(pattern);
    for (const QFileInfo &fileInfo : fiList) {
        QRegularExpressionMatch match = rx.match(fileInfo.fileName());
        Q_ASSERT(match.hasMatch());
        bool ok;
        const int suffix = match.captured(1).toInt(&ok);
        Q_ASSERT(ok);
        Q_ASSERT(suffix >= 0);
        if (suffix > hiSuffix) {
            lastFileInfo = fileInfo;
            hiSuffix = suffix;
        }
    }

    return lastFileInfo.fileName();
}

qint64 QBenchmarkValgrindUtils::extractLastResult()
{
    return extractResult(getNewestFileName());
}

void QBenchmarkValgrindUtils::cleanup()
{
    QStringList nameFilters;
    QString base = QBenchmarkGlobalData::current->callgrindOutFileBase;
    Q_ASSERT(!base.isEmpty());
    nameFilters
        << base // overall summary
        << QString::fromLatin1("%1.*").arg(base); // individual dumps
    const QFileInfoList fiList = QDir().entryInfoList(nameFilters, QDir::Files | QDir::Readable);
    for (const QFileInfo &fileInfo : fiList) {
        const bool removeOk = QFile::remove(fileInfo.fileName());
        Q_ASSERT(removeOk);
        Q_UNUSED(removeOk);
    }
}

QString QBenchmarkValgrindUtils::outFileBase(qint64 pid)
{
    return QString::fromLatin1("callgrind.out.%1").arg(
        pid != -1 ? pid : QCoreApplication::applicationPid());
}

// Reruns this program through callgrind, storing callgrind result files in the
// current directory.
// Returns \c true upon success, otherwise false.
bool QBenchmarkValgrindUtils::runCallgrindSubProcess(const QStringList &origAppArgs, int &exitCode)
{
    const QString &execFile = origAppArgs.at(0);
    QStringList args{ u"--tool=callgrind"_s, u"--instr-atstart=yes"_s,
                      u"--quiet"_s, execFile, u"-callgrindchild"_s };

    // pass on original arguments that make sense (e.g. avoid wasting time producing output
    // that will be ignored anyway) ...
    for (int i = 1; i < origAppArgs.size(); ++i) {
        const QString &arg = origAppArgs.at(i);
        if (arg == "-callgrind"_L1)
            continue;
        args << arg; // ok to pass on
    }

    QProcess process;
    process.start(u"valgrind"_s, args);
    process.waitForStarted(-1);
    QBenchmarkGlobalData::current->callgrindOutFileBase =
        QBenchmarkValgrindUtils::outFileBase(process.processId());
    const bool finishedOk = process.waitForFinished(-1);
    exitCode = process.exitCode();

    dumpOutput(process.readAllStandardOutput(), stdout);
    dumpOutput(process.readAllStandardError(), stderr);

    return finishedOk;
}

void QBenchmarkCallgrindMeasurer::start()
{
    CALLGRIND_ZERO_STATS;
}

QList<QBenchmarkMeasurerBase::Measurement> QBenchmarkCallgrindMeasurer::stop()
{
    CALLGRIND_DUMP_STATS;
    const qint64 result = QBenchmarkValgrindUtils::extractLastResult();
    return { { qreal(result), QTest::InstructionReads } };
}

bool QBenchmarkCallgrindMeasurer::isMeasurementAccepted(Measurement measurement)
{
    Q_UNUSED(measurement);
    return true;
}

int QBenchmarkCallgrindMeasurer::adjustIterationCount(int)
{
    return 1;
}

int QBenchmarkCallgrindMeasurer::adjustMedianCount(int)
{
    return 1;
}

bool QBenchmarkCallgrindMeasurer::needsWarmupIteration()
{
    return true;
}

QT_END_NAMESPACE
