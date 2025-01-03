// Copyright (C) 2021 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcoffpeparser_p.h"

#include <qendian.h>
#include <qloggingcategory.h>
#include <qnumeric.h>

#include <optional>

#include <qt_windows.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// Whether we include some extra validity checks
// (checks to ensure we don't read out-of-bounds are always included)
static constexpr bool IncludeValidityChecks = true;

static constexpr inline auto metadataSectionName() noexcept { return ".qtmetadata"_L1; }
static constexpr QLatin1StringView truncatedSectionName =
        metadataSectionName().left(sizeof(IMAGE_SECTION_HEADER::Name));

#ifdef QT_BUILD_INTERNAL
#  define QCOFFPEPARSER_DEBUG
#endif
#if defined(QCOFFPEPARSER_DEBUG)
static Q_LOGGING_CATEGORY(lcCoffPeParser, "qt.core.plugin.coffpeparser")
#  define peDebug       qCDebug(lcCoffPeParser) << reinterpret_cast<const char16_t *>(error.errMsg->constData()) << ':'
#else
#  define peDebug       if (false) {} else QNoDebug()
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wunused-const-variable")

static const WORD ExpectedMachine =
#if 0
        // nothing, just so everything is #elf
#elif defined(Q_PROCESSOR_ARM_32)
            IMAGE_FILE_MACHINE_ARMNT
#elif defined(Q_PROCESSOR_ARM_64)
            IMAGE_FILE_MACHINE_ARM64
#elif defined(Q_PROCESSOR_IA64)
            IMAGE_FILE_MACHINE_IA64
#elif defined(Q_PROCESSOR_RISCV_32)
            0x5032          // IMAGE_FILE_MACHINE_RISCV32
#elif defined(Q_PROCESSOR_RISCV_64)
            0x5064          // IMAGE_FILE_MACHINE_RISCV64
#elif defined(Q_PROCESSOR_X86_32)
            IMAGE_FILE_MACHINE_I386
#elif defined(Q_PROCESSOR_X86_64)
            IMAGE_FILE_MACHINE_AMD64
#else
#  error "Unknown Q_PROCESSOR_xxx macro, please update."
            IMAGE_FILE_MACHINE_UNKNOWN
#endif
        ;

static const WORD ExpectedOptionalHeaderSignature =
        sizeof(void*) == sizeof(quint64) ? IMAGE_NT_OPTIONAL_HDR64_MAGIC :
            IMAGE_NT_OPTIONAL_HDR32_MAGIC;

namespace {
struct ErrorMaker
{
    QString *errMsg;
    constexpr ErrorMaker(QString *errMsg) : errMsg(errMsg) {}

    Q_DECL_COLD_FUNCTION QLibraryScanResult operator()(QString &&text) const
    {
        *errMsg = QLibrary::tr("'%1' is not a valid Windows DLL (%2)").arg(*errMsg, std::move(text));
        return {};
    }

    Q_DECL_COLD_FUNCTION QLibraryScanResult toosmall() const
    {
        *errMsg = QLibrary::tr("'%1' is too small").arg(*errMsg);
        return {};
    }

    Q_DECL_COLD_FUNCTION QLibraryScanResult notplugin(QString &&explanation) const
    {
        *errMsg = QLibrary::tr("'%1' is not a Qt plugin (%2)").arg(*errMsg, explanation);
        return {};
    }

    Q_DECL_COLD_FUNCTION QLibraryScanResult notfound() const
    {
        return notplugin(QLibrary::tr("metadata not found"));
    }
};

struct HeaderDebug  { const IMAGE_NT_HEADERS *h; };
Q_DECL_UNUSED static QDebug &operator<<(QDebug &d, HeaderDebug h)
{
    switch (h.h->Signature & 0xffff) {
    case IMAGE_OS2_SIGNATURE:       return d << "NE executable";
    case IMAGE_VXD_SIGNATURE:       return d << "LE executable";
    default:                        return d << "Unknown file type";
    case IMAGE_NT_SIGNATURE:        break;
    }

    // the FileHeader and the starting portion of OptionalHeader are the same
    // for 32- and 64-bit
    switch (h.h->OptionalHeader.Magic) {
    case IMAGE_NT_OPTIONAL_HDR32_MAGIC: d << "COFF PE"; break;
    case IMAGE_NT_OPTIONAL_HDR64_MAGIC: d << "COFF PE+"; break;
    default:                            return d << "Unknown COFF PE type";
    }

    QDebugStateSaver saver(d);
    d.nospace() << Qt::hex << Qt::showbase;

    switch (h.h->FileHeader.Machine) {
    case IMAGE_FILE_MACHINE_I386:       d << "i386"; break;
    case IMAGE_FILE_MACHINE_ARM:        d << "ARM"; break;
    case IMAGE_FILE_MACHINE_ARMNT:      d << "ARM Thumb-2"; break;;
    case IMAGE_FILE_MACHINE_THUMB:      d << "Thumb"; break;
    case IMAGE_FILE_MACHINE_IA64:       d << "IA-64"; break;
    case IMAGE_FILE_MACHINE_MIPS16:     d << "MIPS16"; break;
    case IMAGE_FILE_MACHINE_MIPSFPU:    d << "MIPS with FPU"; break;
    case IMAGE_FILE_MACHINE_MIPSFPU16:  d << "MIPS16 with FPU"; break;
    case IMAGE_FILE_MACHINE_AMD64:      d << "x86-64"; break;
    case 0xaa64:                        d << "AArch64"; break;
    default:                            d << h.h->FileHeader.Machine; break;
    }

    // this usually prints "executable DLL"
    if (h.h->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)
        d << " executable";
    if (h.h->FileHeader.Characteristics & IMAGE_FILE_DLL)
        d << " DLL";
    if (h.h->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE)
        d << " large-address aware";

    d << ", " << Qt::dec << h.h->FileHeader.NumberOfSections << " sections, ";
    if (h.h->FileHeader.SizeOfOptionalHeader < sizeof(IMAGE_OPTIONAL_HEADER32))
        return d;

    auto optDebug = [&d](const auto *hdr) {
        d << "(Windows " << hdr->MajorSubsystemVersion
          << '.' << hdr->MinorSubsystemVersion;
        switch (hdr->Subsystem) {
        case IMAGE_SUBSYSTEM_NATIVE:        d << " native)"; break;
        case IMAGE_SUBSYSTEM_WINDOWS_GUI:   d << " GUI)"; break;
        case IMAGE_SUBSYSTEM_WINDOWS_CUI:   d << " CUI)"; break;
        default:                            d << " subsystem " << hdr->Subsystem << ')'; break;
        }

        d.space() << Qt::hex << hdr->SizeOfHeaders << "header bytes,"
                  << Qt::dec << hdr->NumberOfRvaAndSizes << "RVA entries";
    };

    if (h.h->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        optDebug(reinterpret_cast<const IMAGE_OPTIONAL_HEADER64 *>(&h.h->OptionalHeader));
    else
        optDebug(reinterpret_cast<const IMAGE_OPTIONAL_HEADER32 *>(&h.h->OptionalHeader));
    return d;
}

struct SectionDebug { const IMAGE_SECTION_HEADER *s; };
Q_DECL_UNUSED static QDebug &operator<<(QDebug &d, SectionDebug s)
{
    QDebugStateSaver saver(d);
    d << Qt::hex << Qt::showbase;
    d << "contains";
    if (s.s->Characteristics & IMAGE_SCN_CNT_CODE)
        d << "CODE";
    if (s.s->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
        d << "DATA";
    if (s.s->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
        d << "BSS";

    d << "flags";
    d.nospace();
    if (s.s->Characteristics & IMAGE_SCN_MEM_READ)
        d << 'R';
    if (s.s->Characteristics & IMAGE_SCN_MEM_WRITE)
        d << 'W';
    if (s.s->Characteristics & IMAGE_SCN_MEM_EXECUTE)
        d << 'X';
    if (s.s->Characteristics & IMAGE_SCN_MEM_SHARED)
        d << 'S';
    if (s.s->Characteristics & IMAGE_SCN_MEM_DISCARDABLE)
        d << 'D';

    d.space() << "offset" << s.s->PointerToRawData << "size" << s.s->SizeOfRawData;
    return d;
}
} // unnamed namespace

QT_WARNING_POP

const IMAGE_NT_HEADERS *checkNtHeaders(QByteArrayView data, const ErrorMaker &error)
{
    if (size_t(data.size()) < qMax(sizeof(IMAGE_DOS_HEADER), sizeof(IMAGE_NT_HEADERS))) {
        peDebug << "file too small:" << size_t(data.size());
        return error.toosmall(), nullptr;
    }

    // check if there's a DOS image header (almost everything does)
    size_t off = 0;
    auto dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER *>(data.data());
    if (dosHeader->e_magic == IMAGE_DOS_SIGNATURE) {
        off = dosHeader->e_lfanew;
        // peDebug << "DOS file header redirects to offset" << Qt::hex << Qt::showbase << off;
        if (size_t end; qAddOverflow<sizeof(IMAGE_NT_HEADERS)>(off, &end)
            || end > size_t(data.size())) {
            peDebug << "file too small:" << size_t(data.size());
            return error.toosmall(), nullptr;
        }
    }

    // now check the NT headers
    auto ntHeader = reinterpret_cast<const IMAGE_NT_HEADERS *>(data.data() + off);
    peDebug << HeaderDebug{ntHeader};
    if (ntHeader->Signature != IMAGE_NT_SIGNATURE)      // "PE\0\0"
        return error(QLibrary::tr("invalid signature")), nullptr;
    if (ntHeader->FileHeader.Machine != ExpectedMachine)
        return error(QLibrary::tr("file is for a different processor")), nullptr;
    if (ntHeader->FileHeader.NumberOfSections == 0)
        return error(QLibrary::tr("file has no sections")), nullptr;

    WORD requiredCharacteristics =
            IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_DLL;
    if ((ntHeader->FileHeader.Characteristics & requiredCharacteristics) != requiredCharacteristics)
        return error(QLibrary::tr("wrong characteristics")), nullptr;

    // the optional header is not optional
    if (ntHeader->OptionalHeader.Magic != ExpectedOptionalHeaderSignature)
        return error(QLibrary::tr("file is for a different word size")), nullptr;
    if (ntHeader->OptionalHeader.SizeOfCode == 0)
        return error.notplugin(QLibrary::tr("file has no code")), nullptr;

    return ntHeader;
}

static const IMAGE_SECTION_HEADER *
findSectionTable(QByteArrayView data, const IMAGE_NT_HEADERS *ntHeader, const ErrorMaker &error)
{
    // macro IMAGE_FIRST_SECTION can't overflow due to limited range
    // of type, but adding to the offset from the DOS header could
    // overflow on 32-bit
    static_assert(sizeof(ntHeader->FileHeader.SizeOfOptionalHeader) < sizeof(size_t));
    static_assert(sizeof(ntHeader->FileHeader.NumberOfSections) < sizeof(size_t));

    size_t off = offsetof(IMAGE_NT_HEADERS, OptionalHeader);
    off += ntHeader->FileHeader.SizeOfOptionalHeader;
    if (qAddOverflow<size_t>(off, reinterpret_cast<const char *>(ntHeader) - data.data(), &off))
        return error.toosmall(), nullptr;

    size_t end = ntHeader->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);

    // validate that the file is big enough for all sections we're
    // supposed to have
    if (qAddOverflow(end, off, &end) || end > size_t(data.size()))
        return error.toosmall(), nullptr;

    peDebug << "contains" << ntHeader->FileHeader.NumberOfSections << "sections at offset" << off;
    return reinterpret_cast<const IMAGE_SECTION_HEADER *>(data.data() + off);
}

static std::optional<QByteArrayView>
findStringTable(QByteArrayView data, const IMAGE_NT_HEADERS *ntHeader, const ErrorMaker &error)
{
    // first, find the symbol table
    size_t off = ntHeader->FileHeader.PointerToSymbolTable;
    if (off == 0)
        return QByteArrayView();

    // skip the symbol table to find the string table after it
    constexpr size_t SymbolEntrySize = 18;
    size_t size = ntHeader->FileHeader.NumberOfSymbols;
    if (qMulOverflow<SymbolEntrySize>(size, &size)
            || qAddOverflow(off, size, &off)
            || qAddOverflow(off, sizeof(DWORD), &off)
            || off > size_t(data.size()))
        return error.toosmall(), std::nullopt;

    off -= sizeof(DWORD);

    // we've found the string table, ensure it's big enough
    size = qFromUnaligned<DWORD>(data.data() + off);
    if (size_t end; qAddOverflow(off, size, &end) || end > size_t(data.size()))
        return error.toosmall(), std::nullopt;

    // the conversion to signed is fine because we checked above it wasn't
    // bigger than data.size()
    return data.sliced(off, size);
}

static QLatin1StringView findSectionName(const IMAGE_SECTION_HEADER *section, QByteArrayView stringTable)
{
    auto ptr = reinterpret_cast<const char *>(section->Name);
    qsizetype n = qstrnlen(ptr, sizeof(section->Name));
    if (ptr[0] == '/' && !stringTable.isEmpty()) {
        // long section name
        // Microsoft's link.exe does not use these and will truncate the
        // section name to fit section->Name. GNU binutils' ld does use long
        // section names on executable image files by default (can be disabled
        // using --disable-long-section-names). LLVM's lld does generate a
        // string table in MinGW mode, but does not use it for our section.

        static_assert(sizeof(section->Name) - 1 < std::numeric_limits<uint>::digits10);
        bool ok;
        qsizetype offset = QByteArrayView(ptr + 1, n - 1).toUInt(&ok);
        if (!ok || offset >= stringTable.size())
            return {};

        ptr = stringTable.data() + offset;
        n = qstrnlen(ptr, stringTable.size() - offset);
    }

    return {ptr, n};
}

QLibraryScanResult QCoffPeParser::parse(QByteArrayView data, QString *errMsg)
{
    ErrorMaker error(errMsg);
    auto ntHeaders = checkNtHeaders(data, error);
    if (!ntHeaders)
        return {};

    auto section = findSectionTable(data, ntHeaders, error);
    if (!ntHeaders)
        return {};

    QByteArrayView stringTable;
    if (auto optional = findStringTable(data, ntHeaders, error))
        stringTable = *optional;
    else
        return {};

    // scan the sections now
    const auto sectionTableEnd = section + ntHeaders->FileHeader.NumberOfSections;
    for ( ; section < sectionTableEnd; ++section) {
        QLatin1StringView sectionName = findSectionName(section, stringTable);
        peDebug << "section" << sectionName << SectionDebug{section};
        if (IncludeValidityChecks && sectionName.isEmpty())
            return error(QLibrary::tr("a section name is empty or extends past the end of the file"));

        size_t offset = section->PointerToRawData;
        if (size_t end; qAddOverflow<size_t>(offset, section->SizeOfRawData, &end)
                || end > size_t(data.size()))
            return error(QLibrary::tr("section contents extend past the end of the file"));

        DWORD type = section->Characteristics
                & (IMAGE_SCN_CNT_CODE | IMAGE_SCN_CNT_INITIALIZED_DATA
                   | IMAGE_SCN_CNT_UNINITIALIZED_DATA);
        if (type != IMAGE_SCN_CNT_INITIALIZED_DATA)
            continue;

        // if we do have a string table, the name may be complete
        if (sectionName != truncatedSectionName && sectionName != metadataSectionName())
            continue;
        peDebug << "found .qtmetadata section";

        size_t size = qMin(section->SizeOfRawData, section->Misc.VirtualSize);
        if (size < sizeof(QPluginMetaData::MagicHeader))
            return error(QLibrary::tr(".qtmetadata section is too small"));
        if (IncludeValidityChecks) {
            QByteArrayView expectedMagic = QByteArrayView::fromArray(QPluginMetaData::MagicString);
            QByteArrayView actualMagic = data.sliced(offset, expectedMagic.size());
            if (expectedMagic != actualMagic)
                return error(QLibrary::tr(".qtmetadata section has incorrect magic"));

            if (section->Characteristics & IMAGE_SCN_MEM_WRITE)
                return error(QLibrary::tr(".qtmetadata section is writable"));
            if (section->Characteristics & IMAGE_SCN_MEM_EXECUTE)
                return error(QLibrary::tr(".qtmetadata section is executable"));
        }

        return { qsizetype(offset + sizeof(QPluginMetaData::MagicString)),
                 qsizetype(size - sizeof(QPluginMetaData::MagicString)) };
    }

    return error.notfound();
}

QT_END_NAMESPACE
