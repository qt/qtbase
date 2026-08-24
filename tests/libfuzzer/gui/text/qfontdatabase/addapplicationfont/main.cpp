// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QString>
#include <QStringList>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>

#ifdef QT_ASAN_ENABLED
#  include <sanitizer/lsan_interface.h>
#else
#  define __lsan_disable()
#  define __lsan_enable()
#endif

// Headers from the BrokenType ttf-otf-mutator tool. The include directory is
// wired up in CMakeLists.txt via the BROKENTYPE_PATH environment variable.
#include "common.h"
#include "random.h"
#include "sfnt_font.h"
#include "sfnt_mutator.h"

// BrokenType seeds its Mersenne-Twister from std::random_device the first time it
// is used, which makes mutations non-reproducible. globals::generator is defined
// in BrokenType's random.cpp.
namespace globals { extern std::mt19937 generator; }

#if !defined(_WIN32)
#  include <unistd.h>
#endif

// Silence warnings
static QtMessageHandler mh = qInstallMessageHandler([](QtMsgType, const QMessageLogContext &,
                                                       const QString &) {});

// Provided by libFuzzer; lets us fall back to the built-in mutator when the
// input is not a font we can mutate ourselves.
extern "C" size_t LLVMFuzzerMutate(uint8_t *Data, size_t Size, size_t MaxSize);

// Parse an in-memory SFNT (TrueType/OpenType) font into an SfntFont object.
// This mirrors SfntFont::LoadFromFile(), but reads from a memory buffer so we
// never touch the disk. It is deliberately defensive: any inconsistency makes
// it bail out so the caller can fall back to the default libFuzzer mutator.
static bool loadSfntFromMemory(const uint8_t *data, size_t size, SfntFont *font)
{
    font->sfnt_version_ = 0;
    font->sfnt_tables_.clear();

    if (size < sizeof(SfntHeader))
        return false;

    SfntHeader hdr;
    memcpy(&hdr, data, sizeof(SfntHeader));
    font->sfnt_version_ = hdr.version;

    const uint16_t numTables = SWAP16(hdr.num_tables);
    if (numTables == 0)
        return false;

    // The table directory must fit within the buffer.
    if (size < sizeof(SfntHeader) + static_cast<size_t>(numTables) * sizeof(SfntTableHeader))
        return false;

    font->sfnt_tables_.resize(numTables);
    for (uint16_t i = 0; i < numTables; ++i) {
        SfntTableHeader tableHdr;
        memcpy(&tableHdr,
               data + sizeof(SfntHeader) + static_cast<size_t>(i) * sizeof(SfntTableHeader),
               sizeof(SfntTableHeader));

        const uint32_t tableOffset = SWAP32(tableHdr.offset);
        const uint32_t tableLength = SWAP32(tableHdr.length);

        if (tableOffset > size
            || static_cast<size_t>(tableOffset) + tableLength > size) {
            font->sfnt_tables_.clear();
            return false;
        }

        font->sfnt_tables_[i].tag = SWAP32(tableHdr.tag);
        font->sfnt_tables_[i].data.assign(reinterpret_cast<const char *>(data + tableOffset),
                                          tableLength);
    }

    return true;
}

// Custom libFuzzer mutator. libFuzzer keeps ownership of coverage measurement
// and corpus management; we only replace the step that turns one input into the
// next candidate. By running the input through BrokenType's SFNT-aware mutation
// engine we get structurally valid fonts (correct table directory, fixed-up
// checksums) that reach deep into the font parsing/rasterization code instead of
// being rejected early as garbage.
extern "C" size_t LLVMFuzzerCustomMutator(uint8_t *Data, size_t Size, size_t MaxSize,
                                          unsigned int Seed)
{
    SfntFont font;
    if (!loadSfntFromMemory(Data, Size, &font)) {
        // Not something we recognise as a font (e.g. an empty or random initial
        // input): let libFuzzer mutate it the usual way so it can still make
        // progress towards a valid font.
        return LLVMFuzzerMutate(Data, Size, MaxSize);
    }

    SfntStrategies strategies;
    InitSfntMutationStrategies(&strategies);

    // Make the mutation a deterministic function of (input, Seed) so libFuzzer can
    // reproduce a crash from a saved artifact.
    std::srand(Seed);
    globals::generator.seed(Seed);

    // MutateSfntFile() prints a line per table to stdout. Silence it around the
    // call so fuzzing stays fast and the output readable.
#if !defined(_WIN32)
    fflush(stdout);
    const int savedStdout = dup(fileno(stdout));
    FILE *const nullOut = freopen("/dev/null", "w", stdout);
    (void)nullOut;
#endif

    MutateSfntFile(&strategies, &font);

#if !defined(_WIN32)
    fflush(stdout);
    if (savedStdout != -1) {
        dup2(savedStdout, fileno(stdout));
        close(savedStdout);
    }
#endif

    std::string output;
    if (!font.SaveToString(&output) || output.empty() || output.size() > MaxSize) {
        // Could not serialize a usable font within the size limit; fall back.
        return LLVMFuzzerMutate(Data, Size, MaxSize);
    }

    memcpy(Data, output.data(), output.size());
    return output.size();
}

Q_GLOBAL_STATIC(QImage, image, 400, 100, QImage::Format_ARGB32)
Q_GLOBAL_STATIC(QPainter, painter)

// Avoid populating system fonts every iteration
static void useMinimalFontConfig()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    if (qEnvironmentVariableIsSet("FONTCONFIG_FILE"))
        return;

    const QString base = QDir::tempPath()
            + QStringLiteral("/qfontdatabase_fuzz_fc_%1").arg(getpid());
    if (!QDir().mkpath(base + QStringLiteral("/fonts"))
        || !QDir().mkpath(base + QStringLiteral("/cache"))) {
        return;
    }

    const QString confPath = base + QStringLiteral("/fonts.conf");
    QFile conf(confPath);
    if (!conf.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;

    conf.write("<?xml version=\"1.0\"?>\n"
               "<!DOCTYPE fontconfig SYSTEM \"fonts.dtd\">\n"
               "<fontconfig>\n"
               "  <dir>" + (base + QStringLiteral("/fonts")).toUtf8() + "</dir>\n"
               "  <cachedir>" + (base + QStringLiteral("/cache")).toUtf8() + "</cachedir>\n"
               "  <config></config>\n"
               "</fontconfig>\n");
    conf.close();

    qputenv("FONTCONFIG_FILE", confPath.toLocal8Bit());
#endif
}

extern "C" int LLVMFuzzerInitialize(int *_argc, char ***_argv)
{
    Q_UNUSED(_argc);
    Q_UNUSED(_argv);

    __lsan_disable();

    // Must happen before the platform font database is initialized.
    useMinimalFontConfig();

    static int argc = 1;
    static char arg1[] = "fuzzer";
    static char *argv[] = {arg1, nullptr};

    QHashSeed::setDeterministicGlobalSeed();

    // Trigger BrokenType's one-time random_device seeding now, so that the
    // per-mutation reseeding in LLVMFuzzerCustomMutator is not overwritten the
    // first time the generator is used.
    (void)Random32();

    // We leak this to avoid global static QGuiApp, but lsan is disabled so it is ok
    static QGuiApplication *app = new QGuiApplication(argc, argv);

    // Initialize global statics
    image()->fill(Qt::white);
    painter()->begin(image());
    painter()->end();

    __lsan_enable();

    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size)
{
    const int id = QFontDatabase::addApplicationFontFromData(QByteArray::fromRawData(Data, Size));
    if (id != -1) {
        const QStringList families = QFontDatabase::applicationFontFamilies(id);
        if (!families.isEmpty()) {
            QFont font(families.first());
            font.setPixelSize(24);

            painter->begin(image());
            painter()->setFont(font);
            painter()->drawText(image()->rect(),
                                Qt::AlignCenter,
                                QStringLiteral("The quick brown fox jumps over the lazy dog"));
            painter()->end();
        }
        QFontDatabase::removeApplicationFont(id);
    }

    return 0;
}
