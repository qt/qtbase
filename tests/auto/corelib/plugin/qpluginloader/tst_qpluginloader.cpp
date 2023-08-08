// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2021 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QSignalSpy>
#include <QJsonArray>
#include <qdir.h>
#include <qendian.h>
#include <qpluginloader.h>
#include <qtemporaryfile.h>
#include <QScopeGuard>
#include "theplugin/plugininterface.h"

#if defined(QT_BUILD_INTERNAL) && defined(Q_OF_MACH_O)
#  include <QtCore/private/qmachparser_p.h>
#endif

// Helper macros to let us know if some suffixes are valid
#define bundle_VALID    false
#define dylib_VALID     false
#define sl_VALID        false
#define a_VALID         false
#define so_VALID        false
#define dll_VALID       false

#if defined(Q_OS_DARWIN)
# undef bundle_VALID
# undef dylib_VALID
# undef so_VALID
# define bundle_VALID   true
# define dylib_VALID    true
# define so_VALID       true
# ifdef QT_NO_DEBUG
#  define SUFFIX         ".dylib"
# else
#  define SUFFIX         "_debug.dylib"
# endif
# define PREFIX         "lib"

#elif defined(Q_OS_HPUX) && !defined(__ia64)
# undef sl_VALID
# define sl_VALID       true
# define SUFFIX         ".sl"
# define PREFIX         "lib"

#elif defined(Q_OS_AIX)
# undef a_VALID
# undef so_VALID
# define a_VALID        true
# define so_VALID       true
# define SUFFIX         ".so"
# define PREFIX         "lib"

#elif defined(Q_OS_WIN)
# undef dll_VALID
# define dll_VALID      true
//# ifdef QT_NO_DEBUG
#  define SUFFIX         ".dll"
//# else
//#  define SUFFIX         "d.dll"
//# endif
# define PREFIX         ""

#else  // all other Unix
# undef so_VALID
# define so_VALID       true
# define SUFFIX         ".so"
# define PREFIX         "lib"
#endif

#if defined(Q_OF_ELF)
#if __has_include(<elf.h>)
# include <elf.h>
#else
# include <sys/elf.h>
#endif
#  include <memory>
#  include <functional>

#  ifdef _LP64
using ElfHeader = Elf64_Ehdr;
using ElfPhdr = Elf64_Phdr;
using ElfNhdr = Elf64_Nhdr;
using ElfShdr = Elf64_Shdr;
#  else
using ElfHeader = Elf32_Ehdr;
using ElfPhdr = Elf32_Phdr;
using ElfNhdr = Elf32_Nhdr;
using ElfShdr = Elf32_Shdr;
#  endif

struct ElfPatcher
{
    using FullPatcher = void(ElfHeader *, QFile *);
    FullPatcher *f;

    ElfPatcher(FullPatcher *f = nullptr) : f(f) {}

    template <typename T> using IsSingleArg = std::is_invocable<T, ElfHeader *>;
    template <typename T> static std::enable_if_t<IsSingleArg<T>::value, ElfPatcher> fromLambda(T &&t)
    {
        using WithoutQFile = void(*)(ElfHeader *);
        static const WithoutQFile f = t;
        return { [](ElfHeader *h, QFile *) { f(h);} };
    }
    template <typename T> static std::enable_if_t<!IsSingleArg<T>::value, ElfPatcher> fromLambda(T &&t)
    {
        return { t };
    }
};

Q_DECLARE_METATYPE(ElfPatcher)

static std::unique_ptr<QTemporaryFile> patchElf(const QString &source, ElfPatcher patcher)
{
    std::unique_ptr<QTemporaryFile> tmplib;

    bool ok = false;
    [&]() {
        QFile srclib(source);
        QVERIFY2(srclib.open(QIODevice::ReadOnly), qPrintable(srclib.errorString()));
        qint64 srcsize = srclib.size();
        const uchar *srcdata = srclib.map(0, srcsize, QFile::MapPrivateOption);
        QVERIFY2(srcdata, qPrintable(srclib.errorString()));

        // copy our source plugin so we can modify it
        const char *basename = QTest::currentDataTag();
        if (!basename)
            basename = QTest::currentTestFunction();
        tmplib.reset(new QTemporaryFile(basename + QString(".XXXXXX" SUFFIX)));
        QVERIFY2(tmplib->open(), qPrintable(tmplib->errorString()));

        // sanity-check
        QByteArray magic = QByteArray::fromRawData(reinterpret_cast<const char *>(srcdata), SELFMAG);
        QCOMPARE(magic, QByteArray(ELFMAG));

        // copy everything via mmap()
        QVERIFY2(tmplib->resize(srcsize), qPrintable(tmplib->errorString()));
        uchar *dstdata = tmplib->map(0, srcsize);
        memcpy(dstdata, srcdata, srcsize);

        // now patch the file
        patcher.f(reinterpret_cast<ElfHeader *>(dstdata), tmplib.get());

        ok = true;
    }();
    if (!ok)
        tmplib.reset();
    return tmplib;
}

// All ELF systems are expected to support GCC expression statements
#define patchElf(source, patcher)   __extension__({     \
        auto r = patchElf(source, patcher);             \
        if (QTest::currentTestFailed()) return;         \
        std::move(r);                                   \
    })
#endif // Q_OF_ELF

static QString sys_qualifiedLibraryName(const QString &fileName)
{
#ifdef Q_OS_ANDROID
    // On Android all the libraries must be located in the APK's libs subdir
    const QStringList paths = QCoreApplication::libraryPaths();
    if (!paths.isEmpty()) {
        return QLatin1String("%1/%2%3_%4%5").arg(paths.first(), PREFIX, fileName,
                                                 ANDROID_ARCH, SUFFIX);
    }
    return fileName;
#else
    QString name = QLatin1String("bin/") + QLatin1String(PREFIX) + fileName + QLatin1String(SUFFIX);
    const QString libname = QFINDTESTDATA(name);
    QFileInfo fi(libname);
    if (fi.exists())
        return fi.canonicalFilePath();
    return libname;
#endif
}

QT_FORWARD_DECLARE_CLASS(QPluginLoader)
class tst_QPluginLoader : public QObject
{
    Q_OBJECT
public slots:
    void cleanup();
private slots:
    void errorString();
    void loadHints();
    void deleteinstanceOnUnload();
#if defined (Q_OF_ELF)
    void loadDebugObj();
    void loadCorruptElf_data();
    void loadCorruptElf();
#  if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    void loadCorruptElfOldPlugin_data();
    void loadCorruptElfOldPlugin();
#  endif
#endif
    void loadMachO_data();
    void loadMachO();
    void relativePath();
    void absolutePath();
    void reloadPlugin();
    void loadSectionTableStrippedElf();
    void preloadedPlugin_data();
    void preloadedPlugin();
    void staticPlugins();
    void reregisteredStaticPlugins();
};

Q_IMPORT_PLUGIN(StaticPlugin)

void tst_QPluginLoader::cleanup()
{
    // check if the library/plugin was leaked
    // we can't use QPluginLoader::isLoaded here because on some platforms the plugin is always loaded by QPluginLoader.
    // Also, if this test fails once, it will keep on failing because we can't force the unload,
    // so we report it only once.
    static bool failedAlready = false;
    if (!failedAlready) {
        QLibrary lib(sys_qualifiedLibraryName("theplugin"));
        failedAlready = true;
        QVERIFY2(!lib.isLoaded(), "Plugin was leaked - will not check again");
        failedAlready = false;
    }
}

void tst_QPluginLoader::errorString()
{
#if !defined(QT_SHARED)
    QSKIP("This test requires Qt to create shared libraries.");
#endif

    const QString unknown(QLatin1String("Unknown error"));

    {
    QPluginLoader loader; // default constructed
    bool loaded = loader.load();
    QCOMPARE(loader.errorString(), unknown);
    QVERIFY(!loaded);

    QObject *obj = loader.instance();
    QCOMPARE(loader.errorString(), unknown);
    QCOMPARE(obj, static_cast<QObject*>(0));

    bool unloaded = loader.unload();
    QCOMPARE(loader.errorString(), unknown);
    QVERIFY(!unloaded);
    }
    {
    QPluginLoader loader( sys_qualifiedLibraryName("tst_qpluginloaderlib"));     //not a plugin
    bool loaded = loader.load();
    QVERIFY(loader.errorString() != unknown);
    QVERIFY(!loaded);

    QObject *obj = loader.instance();
    QVERIFY(loader.errorString() != unknown);
    QCOMPARE(obj, static_cast<QObject*>(0));

    bool unloaded = loader.unload();
    QVERIFY(loader.errorString() != unknown);
    QVERIFY(!unloaded);
    }

    {
    QPluginLoader loader( sys_qualifiedLibraryName("nosuchfile"));     //not a file
    bool loaded = loader.load();
    QVERIFY(loader.errorString() != unknown);
    QVERIFY(!loaded);

    QObject *obj = loader.instance();
    QVERIFY(loader.errorString() != unknown);
    QCOMPARE(obj, static_cast<QObject*>(0));

    bool unloaded = loader.unload();
    QVERIFY(loader.errorString() != unknown);
    QVERIFY(!unloaded);
    }

#if !defined(Q_OS_WIN) && !defined(Q_OS_DARWIN) && !defined(Q_OS_HPUX)
    {
    QPluginLoader loader( sys_qualifiedLibraryName("almostplugin"));     //a plugin with unresolved symbols
    loader.setLoadHints(QLibrary::ResolveAllSymbolsHint);
    bool loaded = loader.load();
    QVERIFY(loader.errorString() != unknown);
    QVERIFY(!loaded);

    QObject *obj = loader.instance();
    QVERIFY(loader.errorString() != unknown);
    QCOMPARE(obj, static_cast<QObject*>(0));

    bool unloaded = loader.unload();
    QVERIFY(loader.errorString() != unknown);
    QVERIFY(!unloaded);
    }
#endif

    static constexpr std::initializer_list<const char *> validplugins = {
        "theplugin",
#if defined(Q_OF_ELF) && QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
        "theoldplugin"
#endif
    };
    for (const char *basename : validplugins) {
        QPluginLoader loader( sys_qualifiedLibraryName(basename));     //a plugin

        // Check metadata
        const QJsonObject metaData = loader.metaData();
        QVERIFY2(!metaData.isEmpty(), "No metadata from " + loader.fileName().toLocal8Bit());
        QCOMPARE(metaData.value("IID").toString(), QStringLiteral("org.qt-project.Qt.autotests.plugininterface"));
        const QJsonObject kpluginObject = metaData.value("MetaData").toObject().value("KPlugin").toObject();
        QCOMPARE(kpluginObject.value("Name[mr]").toString(), QString::fromUtf8("चौकट भूमिती"));

        // Load
        QVERIFY2(loader.load(), qPrintable(loader.errorString()));
        QCOMPARE(loader.errorString(), unknown);

        QVERIFY(loader.instance() !=  static_cast<QObject*>(0));
        QCOMPARE(loader.errorString(), unknown);

        // Make sure that plugin really works
        PluginInterface* theplugin = qobject_cast<PluginInterface*>(loader.instance());
        QString pluginName = theplugin->pluginName();
        QCOMPARE(pluginName, QLatin1String("Plugin ok"));

        QCOMPARE(loader.unload(), true);
        QCOMPARE(loader.errorString(), unknown);
    }
}

void tst_QPluginLoader::loadHints()
{
#if !defined(QT_SHARED)
    QSKIP("This test requires Qt to create shared libraries.");
#endif
    QPluginLoader loader;
    QCOMPARE(loader.loadHints(), QLibrary::PreventUnloadHint);   //Do not crash
    loader.setLoadHints(QLibrary::ResolveAllSymbolsHint);
    QCOMPARE(loader.loadHints(), QLibrary::ResolveAllSymbolsHint);
    loader.setFileName( sys_qualifiedLibraryName("theplugin"));     //a plugin
    QCOMPARE(loader.loadHints(), QLibrary::ResolveAllSymbolsHint);

    QPluginLoader loader2;
    QCOMPARE(loader2.loadHints(), QLibrary::PreventUnloadHint);
    loader2.setFileName(sys_qualifiedLibraryName("theplugin"));
    QCOMPARE(loader2.loadHints(), QLibrary::PreventUnloadHint);

    QPluginLoader loader3(sys_qualifiedLibraryName("theplugin"));
    QCOMPARE(loader3.loadHints(), QLibrary::PreventUnloadHint);
}

void tst_QPluginLoader::deleteinstanceOnUnload()
{
#if !defined(QT_SHARED)
    QSKIP("This test requires Qt to create shared libraries.");
#endif
    for (int pass = 0; pass < 2; ++pass) {
        QPluginLoader loader1;
        loader1.setFileName( sys_qualifiedLibraryName("theplugin"));     //a plugin
        if (pass == 0)
            loader1.load(); // not recommended, instance() should do the job.
        PluginInterface *instance1 = qobject_cast<PluginInterface*>(loader1.instance());
        QVERIFY(instance1);
        QCOMPARE(instance1->pluginName(), QLatin1String("Plugin ok"));

        QPluginLoader loader2;
        loader2.setFileName( sys_qualifiedLibraryName("theplugin"));     //a plugin
        if (pass == 0)
            loader2.load(); // not recommended, instance() should do the job.
        PluginInterface *instance2 = qobject_cast<PluginInterface*>(loader2.instance());
        QCOMPARE(instance2->pluginName(), QLatin1String("Plugin ok"));

        QSignalSpy spy1(loader1.instance(), &QObject::destroyed);
        QSignalSpy spy2(loader2.instance(), &QObject::destroyed);
        QVERIFY(spy1.isValid());
        QVERIFY(spy2.isValid());
        if (pass == 0) {
            QCOMPARE(loader2.unload(), false);  // refcount not reached 0, not really unloaded
            QCOMPARE(spy1.size(), 0);
            QCOMPARE(spy2.size(), 0);
        }
        QCOMPARE(instance1->pluginName(), QLatin1String("Plugin ok"));
        QCOMPARE(instance2->pluginName(), QLatin1String("Plugin ok"));
        QVERIFY(loader1.unload());   // refcount reached 0, did really unload
        QCOMPARE(spy1.size(), 1);
        QCOMPARE(spy2.size(), 1);
    }
}

#if defined(Q_OF_ELF)

void tst_QPluginLoader::loadDebugObj()
{
#if !defined(QT_SHARED)
    QSKIP("This test requires a shared build of Qt, as QPluginLoader::setFileName is a no-op in static builds");
#endif
    QVERIFY(QFile::exists(QFINDTESTDATA("elftest/debugobj.so")));
    QPluginLoader lib1(QFINDTESTDATA("elftest/debugobj.so"));
    QCOMPARE(lib1.load(), false);
}

template <typename Lambda>
static void newRow(const char *rowname, QString &&snippet, Lambda &&patcher)
{
    QTest::newRow(rowname)
            << std::move(snippet) << ElfPatcher::fromLambda(std::forward<Lambda>(patcher));
}

static ElfPhdr *getProgramEntry(ElfHeader *h, int index)
{
    auto phdr = reinterpret_cast<ElfPhdr *>(h->e_phoff + reinterpret_cast<uchar *>(h));
    return phdr + index;
}

static void loadCorruptElfCommonRows()
{
    QTest::addColumn<QString>("snippet");
    QTest::addColumn<ElfPatcher>("patcher");

    using H = ElfHeader *;          // because I'm lazy
    newRow("not-elf", "invalid signature", [](H h) {
        h->e_ident[EI_MAG0] = 'Q';
        h->e_ident[EI_MAG1] = 't';
    });

    newRow("wrong-word-size", "file is for a different word size", [](H h) {
        h->e_ident[EI_CLASS] = sizeof(void *) == 8 ? ELFCLASS32 : ELFCLASS64;

        // unnecessary, but we're doing it anyway
#  ifdef _LP64
        Elf32_Ehdr o;
        o.e_phentsize = sizeof(Elf32_Phdr);
        o.e_shentsize = sizeof(Elf32_Shdr);
#  else
        Elf64_Ehdr o;
        o.e_phentsize = sizeof(Elf64_Phdr);
        o.e_shentsize = sizeof(Elf64_Shdr);
#  endif
        memcpy(o.e_ident, h->e_ident, EI_NIDENT);
        o.e_type = h->e_type;
        o.e_machine = h->e_machine;
        o.e_version = h->e_version;
        o.e_entry = h->e_entry;
        o.e_phoff = h->e_phoff;
        o.e_shoff = h->e_shoff;
        o.e_flags = h->e_flags;
        o.e_ehsize = sizeof(o);
        o.e_phnum = h->e_phnum;
        o.e_shnum = h->e_shnum;
        o.e_shstrndx = h->e_shstrndx;
        memcpy(h, &o, sizeof(o));
    });
    newRow("invalid-word-size", "file is for a different word size", [](H h) {
        h->e_ident[EI_CLASS] = ELFCLASSNONE;;
    });
    newRow("unknown-word-size", "file is for a different word size", [](H h) {
        h->e_ident[EI_CLASS] |= 0x40;
    });

    newRow("wrong-endian", "file is for the wrong endianness", [](H h) {
        h->e_ident[EI_DATA] = QSysInfo::ByteOrder == QSysInfo::LittleEndian ? ELFDATA2MSB : ELFDATA2LSB;

        // unnecessary, but we're doing it anyway
        h->e_type = qbswap(h->e_type);
        h->e_machine = qbswap(h->e_machine);
        h->e_version = qbswap(h->e_version);
        h->e_entry = qbswap(h->e_entry);
        h->e_phoff = qbswap(h->e_phoff);
        h->e_shoff = qbswap(h->e_shoff);
        h->e_flags = qbswap(h->e_flags);
        h->e_ehsize = qbswap(h->e_ehsize);
        h->e_phnum = qbswap(h->e_phnum);
        h->e_phentsize = qbswap(h->e_phentsize);
        h->e_shnum = qbswap(h->e_shnum);
        h->e_shentsize = qbswap(h->e_shentsize);
        h->e_shstrndx = qbswap(h->e_shstrndx);
    });
    newRow("invalid-endian", "file is for the wrong endianness", [](H h) {
        h->e_ident[EI_DATA] = ELFDATANONE;
    });
    newRow("unknown-endian", "file is for the wrong endianness", [](H h) {
        h->e_ident[EI_DATA] |= 0x40;
    });

    newRow("elf-version-0", "file has an unknown ELF version", [](H h) {
        --h->e_ident[EI_VERSION];
    });
    newRow("elf-version-2", "file has an unknown ELF version", [](H h) {
        ++h->e_ident[EI_VERSION];
    });

    newRow("executable", "file is not a shared object", [](H h) {
        h->e_type = ET_EXEC;
    });
    newRow("relocatable", "file is not a shared object", [](H h) {
        h->e_type = ET_REL;
    });
    newRow("core-file", "file is not a shared object", [](H h) {
        h->e_type = ET_CORE;
    });
    newRow("invalid-type", "file is not a shared object", [](H h) {
        h->e_type |= 0x100;
    });

    newRow("wrong-arch", "file is for a different processor", [](H h) {
        // could just ++h->e_machine...
#  if defined(Q_PROCESSOR_X86_64)
        h->e_machine = EM_AARCH64;
#  elif defined(Q_PROCESSOR_ARM_64)
        h->e_machine = EM_X86_64;
#  elif defined(Q_PROCESSOR_X86_32)
        h->e_machine = EM_ARM;
#  elif defined(Q_PROCESSOR_ARM)
        h->e_machine = EM_386;
#  elif defined(Q_PROCESSOR_MIPS_64)
        h->e_machine = EM_PPC64;
#  elif defined(Q_PROCESSOR_MIPS_32)
        h->e_machine = EM_PPC;
#  elif defined(Q_PROCESSOR_POWER_64)
        h->e_machine = EM_S390;
#  elif defined(Q_PROCESSOR_POWER_32)
        h->e_machine = EM_MIPS;
#  endif
    });

    newRow("file-version-0", "file has an unknown ELF version", [](H h) {
        --h->e_version;
    });
    newRow("file-version-2", "file has an unknown ELF version", [](H h) {
        ++h->e_version;
    });

    newRow("program-entry-size-zero", "unexpected program header entry size", [](H h) {
        h->e_phentsize = 0;
    });
    newRow("program-entry-small", "unexpected program header entry size", [](H h) {
        h->e_phentsize = alignof(ElfPhdr);
    });

    newRow("program-table-starts-past-eof", "program header table extends past the end of the file",
           [](H h, QFile *f) {
        h->e_phoff = f->size();
    });
    newRow("program-table-ends-past-eof", "program header table extends past the end of the file",
           [](H h, QFile *f) {
        h->e_phoff = f->size() + 1- h->e_phentsize * h->e_phnum;
    });

    newRow("segment-starts-past-eof", "a program header entry extends past the end of the file",
           [](H h, QFile *f) {
        for (int i = 0; i < h->e_phnum; ++i) {
            ElfPhdr *p = getProgramEntry(h, i);
            if (p->p_type != PT_LOAD)
                continue;
            p->p_offset = f->size();
            break;
        }
    });
    newRow("segment-ends-past-eof", "a program header entry extends past the end of the file",
           [](H h, QFile *f) {
        for (int i = 0; i < h->e_phnum; ++i) {
            ElfPhdr *p = getProgramEntry(h, i);
            if (p->p_type != PT_LOAD)
                continue;
            p->p_filesz = f->size() + 1 - p->p_offset;
            break;
        }
    });
    newRow("segment-bounds-overflow", "a program header entry extends past the end of the file",
           [](H h) {
        for (int i = 0; i < h->e_phnum; ++i) {
            ElfPhdr *p = getProgramEntry(h, i);
            if (p->p_type != PT_LOAD)
                continue;
            p->p_filesz = ~size_t(0);    // -1
            break;
        }
    });

    newRow("no-code", "file has no code", [](H h) {
        for (int i = 0; i < h->e_phnum; ++i) {
            ElfPhdr *p = getProgramEntry(h, i);
            if (p->p_type == PT_LOAD)
                p->p_flags &= ~PF_X;
        }
    });
}

void tst_QPluginLoader::loadCorruptElf_data()
{
#if !defined(QT_SHARED)
    QSKIP("This test requires a shared build of Qt, as QPluginLoader::setFileName is a no-op in static builds");
#endif
    loadCorruptElfCommonRows();
    using H = ElfHeader *;          // because I'm lazy

    // PT_NOTE tests
    // general validity is tested in the common rows, for all segments

    newRow("misaligned-note-segment", "note segment start is not properly aligned", [](H h) {
        for (int i = 0; i < h->e_phnum; ++i) {
            ElfPhdr *p = getProgramEntry(h, i);
            if (p->p_type == PT_NOTE)
                ++p->p_offset;
        }
    });

    static const auto getFirstNote = [](void *header, ElfPhdr *phdr) {
        return reinterpret_cast<ElfNhdr *>(static_cast<uchar *>(header) + phdr->p_offset);
    };
    static const auto getNextNote = [](void *header, ElfPhdr *phdr, ElfNhdr *n) {
        // how far into the segment are we?
        size_t offset = reinterpret_cast<uchar *>(n) - static_cast<uchar *>(header) - phdr->p_offset;

        size_t delta = sizeof(*n) + n->n_namesz + phdr->p_align - 1;
        delta &= -phdr->p_align;
        delta += n->n_descsz + phdr->p_align - 1;
        delta &= -phdr->p_align;

        offset += delta;
        if (offset < phdr->p_filesz)
            n = reinterpret_cast<ElfNhdr *>(reinterpret_cast<uchar *>(n) + delta);
        else
            n = nullptr;
        return n;
    };

    // all the intra-note errors cause the notes simply to be skipped
    auto newNoteRow = [](const char *rowname, auto &&lambda) {
        newRow(rowname, "is not a Qt plugin (metadata not found)", std::move(lambda));
    };
    newNoteRow("no-notes", [](H h) {
        for (int i = 0; i < h->e_phnum; ++i) {
            ElfPhdr *p = getProgramEntry(h, i);
            if (p->p_type == PT_NOTE)
                p->p_type = PT_NULL;
        }
    });

    newNoteRow("note-larger-than-segment-nonqt", [](H h) {
        for (int i = 0; i < h->e_phnum; ++i) {
            ElfPhdr *p = getProgramEntry(h, i);
            if (p->p_type != PT_NOTE)
                continue;
            ElfNhdr *n = getFirstNote(h, p);
            n->n_descsz = p->p_filesz;
            n->n_type = 0;          // ensure it's not the Qt note
        }
    });
    newNoteRow("note-larger-than-segment-qt", [](H h) {
        for (int i = 0; i < h->e_phnum; ++i) {
            ElfPhdr *p = getProgramEntry(h, i);
            if (p->p_type != PT_NOTE || p->p_align != alignof(QPluginMetaData::ElfNoteHeader))
                continue;

            // find the Qt metadata note
            constexpr QPluginMetaData::ElfNoteHeader header(0);
            ElfNhdr *n = getFirstNote(h, p);
            for ( ; n; n = getNextNote(h, p, n)) {
                if (n->n_type == header.n_type && n->n_namesz == header.n_namesz) {
                    if (memcmp(n + 1, header.name, sizeof(header.name)) == 0)
                        break;
                }
            }

            if (!n)
                break;
            n->n_descsz = p->p_filesz;
            return;
        }
        qWarning("Could not find the Qt metadata note in this file. Test will fail.");
    });
    newNoteRow("note-size-overflow1", [](H h) {
        // due to limited range, this will not overflow on 64-bit
        for (int i = 0; i < h->e_phnum; ++i) {
            ElfPhdr *p = getProgramEntry(h, i);
            if (p->p_type != PT_NOTE)
                continue;
            ElfNhdr *n = getFirstNote(h, p);
            n->n_namesz = ~decltype(n->n_namesz)(0);
        }
    });
    newNoteRow("note-size-overflow2", [](H h) {
        // due to limited range, this will not overflow on 64-bit
        for (int i = 0; i < h->e_phnum; ++i) {
            ElfPhdr *p = getProgramEntry(h, i);
            if (p->p_type != PT_NOTE)
                continue;
            ElfNhdr *n = getFirstNote(h, p);
            n->n_namesz = ~decltype(n->n_namesz)(0) / 2;
            n->n_descsz = ~decltype(n->n_descsz)(0) / 2;
        }
    });
}

static void loadCorruptElf_helper(const QString &origLibrary)
{
    QFETCH(QString, snippet);
    QFETCH(ElfPatcher, patcher);

    std::unique_ptr<QTemporaryFile> tmplib = patchElf(origLibrary, patcher);

    QPluginLoader lib(tmplib->fileName());
    QVERIFY(!lib.load());
    QVERIFY2(lib.errorString().contains(snippet), qPrintable(lib.errorString()));
}

void tst_QPluginLoader::loadCorruptElf()
{
    loadCorruptElf_helper(sys_qualifiedLibraryName("theplugin"));
}

#  if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
void tst_QPluginLoader::loadCorruptElfOldPlugin_data()
{
#if !defined(QT_SHARED)
    QSKIP("This test requires a shared build of Qt, as QPluginLoader::setFileName is a no-op in static builds");
#endif
    loadCorruptElfCommonRows();
    using H = ElfHeader *;          // because I'm lazy

    newRow("section-entry-size-zero", "unexpected section entry size", [](H h) {
        h->e_shentsize = 0;
    });
    newRow("section-entry-small", "unexpected section entry size", [](H h) {
        h->e_shentsize = alignof(ElfShdr);
    });
    newRow("section-entry-misaligned", "unexpected section entry size", [](H h) {
        ++h->e_shentsize;
    });
    newRow("no-sections", "is not a Qt plugin (metadata not found)", [](H h){
        h->e_shnum = h->e_shoff = h->e_shstrndx = 0;
    });

    // section table tests
    newRow("section-table-starts-past-eof", "section table extends past the end of the file",
           [](H h, QFile *f) {
        h->e_shoff = f->size();
    });
    newRow("section-table-ends-past-eof", "section table extends past the end of the file",
           [](H h, QFile *f) {
        h->e_shoff = f->size() + 1 - h->e_shentsize * h->e_shnum;
    });

    static auto getSection = +[](H h, int index) {
        auto sections = reinterpret_cast<ElfShdr *>(h->e_shoff + reinterpret_cast<uchar *>(h));
        return sections + index;
    };

    // arbitrary section bounds checks
    // section index = 0 is usually a NULL section, so we try 1
    newRow("section1-starts-past-eof", "section contents extend past the end of the file",
           [](H h, QFile *f) {
        ElfShdr *s = getSection(h, 1);
        s->sh_offset = f->size();
    });
    newRow("section1-ends-past-eof", "section contents extend past the end of the file",
           [](H h, QFile *f) {
        ElfShdr *s = getSection(h, 1);
        s->sh_size = f->size() + 1 - s->sh_offset;
    });
    newRow("section1-bounds-overflow", "section contents extend past the end of the file", [](H h) {
        ElfShdr *s = getSection(h, 1);
        s->sh_size = -sizeof(*s);
    });

    // section header string table tests
    newRow("shstrndx-invalid", "e_shstrndx greater than the number of sections", [](H h) {
        h->e_shstrndx = h->e_shnum;
    });
    newRow("shstrtab-starts-past-eof", "section header string table extends past the end of the file",
           [](H h, QFile *f) {
        ElfShdr *s = getSection(h, h->e_shstrndx);
        s->sh_offset = f->size();
    });
    newRow("shstrtab-ends-past-eof", "section header string table extends past the end of the file",
           [](H h, QFile *f) {
        ElfShdr *s = getSection(h, h->e_shstrndx);
        s->sh_size = f->size() + 1 - s->sh_offset;
    });
    newRow("shstrtab-bounds-overflow", "section header string table extends past the end of the file", [](H h) {
        ElfShdr *s = getSection(h, h->e_shstrndx);
        s->sh_size = -sizeof(*s);
    });
    newRow("section-name-past-eof", "section name extends past the end of the file", [](H h, QFile *f) {
        ElfShdr *section1 = getSection(h, 1);
        ElfShdr *shstrtab = getSection(h, h->e_shstrndx);
        section1->sh_name = f->size() - shstrtab->sh_offset;
    });
    newRow("section-name-past-end-of-shstrtab", "section name extends past the end of the file", [](H h) {
        ElfShdr *section1 = getSection(h, 1);
        ElfShdr *shstrtab = getSection(h, h->e_shstrndx);
        section1->sh_name = shstrtab->sh_size;
    });

    newRow("debug-symbols", "metadata not found", [](H h) {
        // attempt to make it look like extracted debug info
        for (int i = 1; i < h->e_shnum; ++i) {
            ElfShdr *s = getSection(h, i);
            if (s->sh_type == SHT_NOBITS)
                break;
            if (s->sh_type != SHT_NOTE && s->sh_flags & SHF_ALLOC)
                s->sh_type = SHT_NOBITS;
        }
    });

    // we don't know which section is .qtmetadata, so we just apply to all of them
    static auto applyToAllSectionFlags = +[](H h, int flag) {
        for (int i = 0; i < h->e_shnum; ++i)
            getSection(h, i)->sh_flags |= flag;
    };
    newRow("qtmetadata-executable", ".qtmetadata section is executable", [](H h) {
        applyToAllSectionFlags(h, SHF_EXECINSTR);
    });
    newRow("qtmetadata-writable", ".qtmetadata section is writable", [](H h) {
        applyToAllSectionFlags(h, SHF_WRITE);
    });
}

void tst_QPluginLoader::loadCorruptElfOldPlugin()
{
    // ### Qt7: don't forget to remove theoldplugin from the build
    loadCorruptElf_helper(sys_qualifiedLibraryName("theoldplugin"));
}
#  endif // Qt 7
#endif // Q_OF_ELF

void tst_QPluginLoader::loadMachO_data()
{
#if defined(QT_BUILD_INTERNAL) && defined(Q_OF_MACH_O)
    QTest::addColumn<bool>("success");

    QTest::newRow("/dev/null") << false;
    QTest::newRow("elftest/debugobj.so") << false;
    QTest::newRow("tst_qpluginloader.cpp") << false;
    QTest::newRow("tst_qpluginloader") << false;

#  ifdef Q_PROCESSOR_X86_64
    QTest::newRow("machtest/good.x86_64.dylib") << true;
    QTest::newRow("machtest/good.i386.dylib") << false;
    QTest::newRow("machtest/good.fat.no-x86_64.dylib") << false;
    QTest::newRow("machtest/good.fat.no-i386.dylib") << true;
#  elif defined(Q_PROCESSOR_X86_32)
    QTest::newRow("machtest/good.i386.dylib") << true;
    QTest::newRow("machtest/good.x86_64.dylib") << false;
    QTest::newRow("machtest/good.fat.no-i386.dylib") << false;
    QTest::newRow("machtest/good.fat.no-x86_64.dylib") << true;
#  endif
#  ifndef Q_PROCESSOR_POWER_64
    QTest::newRow("machtest/good.ppc64.dylib") << false;
#  endif

    QTest::newRow("machtest/good.fat.all.dylib") << true;
    QTest::newRow("machtest/good.fat.stub-x86_64.dylib") << false;
    QTest::newRow("machtest/good.fat.stub-i386.dylib") << false;

    QDir d(QFINDTESTDATA("machtest"));
    const QStringList badlist = d.entryList(QStringList() << "bad*.dylib");
    for (const QString &bad : badlist)
        QTest::newRow(qPrintable("machtest/" + bad)) << false;
#endif
}

void tst_QPluginLoader::loadMachO()
{
#if defined(QT_BUILD_INTERNAL) && defined(Q_OF_MACH_O)
    QFile f(QFINDTESTDATA(QTest::currentDataTag()));
    QVERIFY(f.open(QIODevice::ReadOnly));
    QByteArray data = f.readAll();

    QString errorString = f.fileName();
    QLibraryScanResult r = QMachOParser::parse(data.constData(), data.size(), &errorString);

    QFETCH(bool, success);
    if (success) {
        QVERIFY(r.length != 0);
    } else {
        QCOMPARE(r.length, 0);
        return;
    }

    QVERIFY(r.pos > 0);
    QVERIFY(size_t(r.length) >= sizeof(void*));
    QVERIFY(r.pos + r.length < data.size());
    QCOMPARE(r.pos & (sizeof(void*) - 1), 0UL);

    void *value = *(void**)(data.constData() + r.pos);
    QCOMPARE(value, sizeof(void*) > 4 ? (void*)(0xc0ffeec0ffeeL) : (void*)0xc0ffee);

    // now that we know it's valid, let's try to make it invalid
    ulong offeredlen = r.pos;
    do {
        --offeredlen;
        errorString = f.fileName();
        r = QMachOParser::parse(data.constData(), offeredlen, &errorString);
        QVERIFY2(r.length == 0, qPrintable(QString("Failed at size 0x%1").arg(offeredlen, 0, 16)));
    } while (offeredlen);
#endif
}

void tst_QPluginLoader::relativePath()
{
#if !defined(QT_SHARED)
    QSKIP("This test requires Qt to create shared libraries.");
#endif
#ifdef Q_OS_ANDROID
    // On Android we do not need to explicitly set library paths, as they are
    // already set.
    // But we need to use ARCH suffix in pulgin name
    const QString pluginName("theplugin_" ANDROID_ARCH SUFFIX);
#else
    // Windows binaries run from release and debug subdirs, so we can't rely on the current dir.
    const QString binDir = QFINDTESTDATA("bin");
    QVERIFY(!binDir.isEmpty());
    QCoreApplication::addLibraryPath(binDir);
    const QString pluginName("theplugin" SUFFIX);
#endif
    QPluginLoader loader(pluginName);
    loader.load(); // not recommended, instance() should do the job.
    PluginInterface *instance = qobject_cast<PluginInterface*>(loader.instance());
    QVERIFY(instance);
    QCOMPARE(instance->pluginName(), QLatin1String("Plugin ok"));
    QVERIFY(loader.unload());
}

void tst_QPluginLoader::absolutePath()
{
#if !defined(QT_SHARED)
    QSKIP("This test requires Qt to create shared libraries.");
#endif
#ifdef Q_OS_ANDROID
    // On Android we need to clear library paths to make sure that the absolute
    // path works
    const QStringList libraryPaths = QCoreApplication::libraryPaths();
    QVERIFY(!libraryPaths.isEmpty());
    QCoreApplication::setLibraryPaths(QStringList());
    const QString pluginPath(libraryPaths.first() + "/" PREFIX "theplugin_" ANDROID_ARCH SUFFIX);
#else
    // Windows binaries run from release and debug subdirs, so we can't rely on the current dir.
    const QString binDir = QFINDTESTDATA("bin");
    QVERIFY(!binDir.isEmpty());
    QVERIFY(QDir::isAbsolutePath(binDir));
    const QString pluginPath(binDir + "/" PREFIX "theplugin" SUFFIX);
#endif
    QPluginLoader loader(pluginPath);
    loader.load(); // not recommended, instance() should do the job.
    PluginInterface *instance = qobject_cast<PluginInterface*>(loader.instance());
#ifdef Q_OS_ANDROID
    // Restore library paths
    QCoreApplication::setLibraryPaths(libraryPaths);
#endif
    QVERIFY(instance);
    QCOMPARE(instance->pluginName(), QLatin1String("Plugin ok"));
    QVERIFY(loader.unload());
}

void tst_QPluginLoader::reloadPlugin()
{
#if !defined(QT_SHARED)
    QSKIP("This test requires Qt to create shared libraries.");
#endif
    QPluginLoader loader;
    loader.setFileName( sys_qualifiedLibraryName("theplugin"));     //a plugin
    loader.load(); // not recommended, instance() should do the job.
    PluginInterface *instance = qobject_cast<PluginInterface*>(loader.instance());
    QVERIFY(instance);
    QCOMPARE(instance->pluginName(), QLatin1String("Plugin ok"));

    QSignalSpy spy(loader.instance(), &QObject::destroyed);
    QVERIFY(spy.isValid());
    QVERIFY(loader.unload());   // refcount reached 0, did really unload
    QCOMPARE(spy.size(), 1);

    // reload plugin
    QVERIFY(loader.load());
    QVERIFY(loader.isLoaded());

    PluginInterface *instance2 = qobject_cast<PluginInterface*>(loader.instance());
    QVERIFY(instance2);
    QCOMPARE(instance2->pluginName(), QLatin1String("Plugin ok"));

    QVERIFY(loader.unload());
}

void tst_QPluginLoader::loadSectionTableStrippedElf()
{
#ifdef Q_OS_ANDROID
    if (QNativeInterface::QAndroidApplication::sdkVersion() >= 24)
        QSKIP("Android 7+ (API 24+) linker doesn't allow missing or bad section header");
#endif
#if !defined(QT_SHARED)
    QSKIP("This test requires a shared build of Qt, as QPluginLoader::setFileName is a no-op in static builds");
#elif !defined(Q_OF_ELF)
    QSKIP("Test specific to the ELF file format");
#else
    ElfPatcher patcher { [](ElfHeader *header, QFile *f) {
        // modify the header to make it look like the section table was stripped
        header->e_shoff = header->e_shnum = header->e_shstrndx = 0;

        // and append a bad header at the end
        QPluginMetaData::MagicHeader badHeader = {};
        --badHeader.header.qt_major_version;
        f->seek(f->size());
        f->write(reinterpret_cast<const char *>(&badHeader), sizeof(badHeader));
    } };

    QString tmpLibName;
    {
        std::unique_ptr<QTemporaryFile> tmplib =
                patchElf(sys_qualifiedLibraryName("theplugin"), patcher);

        tmpLibName = tmplib->fileName();
        tmplib->setAutoRemove(false);
    }
#if defined(Q_OS_QNX)
    // On QNX plugin access is still too early, even when QTemporaryFile is closed
    QTest::qSleep(1000);
#endif
    auto removeTmpLib = qScopeGuard([=]{
        QFile::remove(tmpLibName);
    });

    // now attempt to load it
    QPluginLoader loader(tmpLibName);
    QVERIFY2(loader.load(), qPrintable(loader.errorString()));
    PluginInterface *instance = qobject_cast<PluginInterface*>(loader.instance());
    QVERIFY(instance);
    QCOMPARE(instance->pluginName(), QLatin1String("Plugin ok"));
    QVERIFY(loader.unload());
#endif
}

void tst_QPluginLoader::preloadedPlugin_data()
{
    QTest::addColumn<bool>("doLoad");
    QTest::addColumn<QString>("libname");
    QTest::newRow("create-plugin") << false << sys_qualifiedLibraryName("theplugin");
    QTest::newRow("load-plugin") << true << sys_qualifiedLibraryName("theplugin");
    QTest::newRow("create-non-plugin") << false << sys_qualifiedLibraryName("tst_qpluginloaderlib");
    QTest::newRow("load-non-plugin") << true << sys_qualifiedLibraryName("tst_qpluginloaderlib");
}

void tst_QPluginLoader::preloadedPlugin()
{
#if !defined(QT_SHARED)
    QSKIP("This test requires Qt to create shared libraries.");
#endif
    // check that using QPluginLoader does not interfere with QLibrary
    QFETCH(QString, libname);
    QLibrary lib(libname);
    QVERIFY(lib.load());

    typedef int *(*pf_t)();
    pf_t pf = (pf_t)lib.resolve("pointerAddress");
    QVERIFY(pf);

    int *pluginVariable = pf();
    QVERIFY(pluginVariable);
    QCOMPARE(*pluginVariable, 0xc0ffee);

    {
        // load the plugin
        QPluginLoader loader(libname);
        QFETCH(bool, doLoad);
        if (doLoad && loader.load()) {
            // unload() returns false because QLibrary has it loaded
            QVERIFY(!loader.unload());
        }
    }

    QVERIFY(lib.isLoaded());

    // if the library was unloaded behind our backs, the following will crash:
    QCOMPARE(*pluginVariable, 0xc0ffee);
    QVERIFY(lib.unload());
}

void tst_QPluginLoader::staticPlugins()
{
    const QObjectList instances = QPluginLoader::staticInstances();
    QVERIFY(instances.size());

    // ensure the our plugin only shows up once
    int foundCount = std::count_if(instances.begin(), instances.end(), [](QObject *obj) {
            return obj->metaObject()->className() == QLatin1String("StaticPlugin");
    });
    QCOMPARE(foundCount, 1);

    const auto plugins = QPluginLoader::staticPlugins();
    QCOMPARE(plugins.size(), instances.size());

    // find the metadata
    QJsonObject metaData;
    bool found = false;
    for (const auto &p : plugins) {
        metaData = p.metaData();
        found = metaData.value("className").toString() == QLatin1String("StaticPlugin");
        if (found)
            break;
    }
    QVERIFY(found);

    // We don't store the patch release version anymore (since 5.13)
    QCOMPARE(metaData.value("version").toInt() / 0x100, QT_VERSION / 0x100);
    QCOMPARE(metaData.value("IID").toString(), "SomeIID");
    QCOMPARE(metaData.value("ExtraMetaData"), QJsonArray({ "StaticPlugin", "foo" }));
    QCOMPARE(metaData.value("URI").toString(), "qt.test.pluginloader.staticplugin");
}

void tst_QPluginLoader::reregisteredStaticPlugins()
{
    // the Q_IMPORT_PLUGIN macro will have already done this
    qRegisterStaticPluginFunction(qt_static_plugin_StaticPlugin());
    staticPlugins();
    if (QTest::currentTestFailed())
        return;

    qRegisterStaticPluginFunction(qt_static_plugin_StaticPlugin());
    staticPlugins();
}


QTEST_MAIN(tst_QPluginLoader)
#include "tst_qpluginloader.moc"
