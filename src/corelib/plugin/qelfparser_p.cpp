// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2021 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qelfparser_p.h"

#ifdef Q_OF_ELF

#include "qlibrary_p.h"

#include <qloggingcategory.h>
#include <qnumeric.h>
#include <qsysinfo.h>

#if __has_include(<elf.h>)
#  include <elf.h>
#elif __has_include(<sys/elf.h>)
#  include <sys/elf.h>
#else
#  error "Need ELF header to parse plugins."
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// ### Qt7: propagate the constant and eliminate dead code
static constexpr bool ElfNotesAreMandatory = QT_VERSION >= QT_VERSION_CHECK(7,0,0);

// Whether we include some extra validity checks
// (checks to ensure we don't read out-of-bounds are always included)
static constexpr bool IncludeValidityChecks = true;

#ifdef QT_BUILD_INTERNAL
#  define QELFPARSER_DEBUG
#endif
#if defined(QELFPARSER_DEBUG)
static Q_LOGGING_CATEGORY(lcElfParser, "qt.core.plugin.elfparser")
#  define qEDebug       qCDebug(lcElfParser) << reinterpret_cast<const char16_t *>(error.errMsg->constData()) << ':'
#else
#  define qEDebug       if (false) {} else QNoDebug()
#endif

#ifndef PT_GNU_EH_FRAME
#  define PT_GNU_EH_FRAME   0x6474e550
#endif
#ifndef PT_GNU_STACK
#  define PT_GNU_STACK      0x6474e551
#endif
#ifndef PT_GNU_RELRO
#  define PT_GNU_RELRO      0x6474e552
#endif
#ifndef PT_GNU_PROPERTY
#  define PT_GNU_PROPERTY   0x6474e553
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wunused-const-variable")

namespace {
template <QSysInfo::Endian Order> struct ElfEndianTraits
{
    static constexpr unsigned char DataOrder = ELFDATA2LSB;
    template <typename T> static T fromEndian(T value) { return qFromLittleEndian(value); }
};
template <> struct ElfEndianTraits<QSysInfo::BigEndian>
{
    static constexpr unsigned char DataOrder = ELFDATA2MSB;
    template <typename T> static T fromEndian(T value) { return qFromBigEndian(value); }
};

template <typename EquivalentPointerType> struct ElfTypeTraits
{
    static constexpr unsigned char Class = ELFCLASS64;

    // integer types
    using Half = Elf64_Half;
    using Word = Elf64_Word;
    using Addr = Elf64_Addr;
    using Off = Elf64_Off;

    // structure types
    using Ehdr = Elf64_Ehdr;
    using Shdr = Elf64_Shdr;
    using Phdr = Elf64_Phdr;
    using Nhdr = Elf64_Nhdr;
};
template <> struct ElfTypeTraits<quint32>
{
    static constexpr unsigned char Class = ELFCLASS32;

    // integer types
    using Half = Elf32_Half;
    using Word = Elf32_Word;
    using Addr = Elf32_Addr;
    using Off = Elf32_Off;

    // structure types
    using Ehdr = Elf32_Ehdr;
    using Shdr = Elf32_Shdr;
    using Phdr = Elf32_Phdr;
    using Nhdr = Elf32_Nhdr;
};

struct ElfMachineCheck
{
    static const Elf32_Half ExpectedMachine =
#if 0
            // nothing
#elif defined(Q_PROCESSOR_ALPHA)
            EM_ALPHA
#elif defined(Q_PROCESSOR_ARM_32)
            EM_ARM
#elif defined(Q_PROCESSOR_ARM_64)
            EM_AARCH64
#elif defined(Q_PROCESSOR_BLACKFIN)
            EM_BLACKFIN
#elif defined(Q_PROCESSOR_HPPA)
            EM_PARISC
#elif defined(Q_PROCESSOR_IA64)
            EM_IA_64
#elif defined(Q_PROCESSOR_LOONGARCH)
            EM_LOONGARCH
#elif defined(Q_PROCESSOR_M68K)
            EM_68K
#elif defined(Q_PROCESSOR_MIPS)
            EM_MIPS
#elif defined(Q_PROCESSOR_POWER_32)
            EM_PPC
#elif defined(Q_PROCESSOR_POWER_64)
            EM_PPC64
#elif defined(Q_PROCESSOR_RISCV)
            EM_RISCV
#elif defined(Q_PROCESSOR_S390)
            EM_S390
#elif defined(Q_PROCESSOR_SH)
            EM_SH
#elif defined(Q_PROCESSOR_SPARC_V9)
            EM_SPARCV9
#elif defined(Q_PROCESSOR_SPARC_64)
            EM_SPARCV9
#elif defined(Q_PROCESSOR_SPARC)
            EM_SPARC
#elif defined(Q_PROCESSOR_WASM)
#elif defined(Q_PROCESSOR_X86_32)
            EM_386
#elif defined(Q_PROCESSOR_X86_64)
            EM_X86_64
#else
#  error "Unknown Q_PROCESSOR_xxx macro, please update."
            EM_NONE
#endif
            ;
};

struct ElfHeaderCommonCheck
{
    static_assert(std::is_same_v<decltype(Elf32_Ehdr::e_ident), decltype(Elf64_Ehdr::e_ident)>,
        "e_ident field is not the same in both Elf32_Ehdr and Elf64_Ehdr");

    // bytes 0-3
    static bool checkElfMagic(const uchar *ident)
    {
        return memcmp(ident, ELFMAG, SELFMAG) == 0;
    }

    // byte 6
    static bool checkElfVersion(const uchar *ident)
    {
        uchar elfversion = ident[EI_VERSION];
        return elfversion == EV_CURRENT;
    }

    struct CommonHeader {
        Elf32_Half type;
        Elf32_Half machine;
        Elf32_Word version;
    };
};

template <typename EquivalentPointerType = quintptr, QSysInfo::Endian Order = QSysInfo::ByteOrder>
struct ElfHeaderCheck : public ElfHeaderCommonCheck
{
    using TypeTraits = ElfTypeTraits<EquivalentPointerType>;
    using EndianTraits = ElfEndianTraits<Order>;
    using Ehdr = typename TypeTraits::Ehdr;

    // byte 4
    static bool checkClass(const uchar *ident)
    {
        uchar klass = ident[EI_CLASS];
        return klass == TypeTraits::Class;
    }

    // byte 5
    static bool checkDataOrder(const uchar *ident)
    {
        uchar data = ident[EI_DATA];
        return data == EndianTraits::DataOrder;
    }

    // byte 7
    static bool checkOsAbi(const uchar *ident)
    {
        uchar osabi = ident[EI_OSABI];
        // we don't check
        Q_UNUSED(osabi);
        return true;
    }

    // byte 8
    static bool checkAbiVersion(const uchar *ident)
    {
        uchar abiversion = ident[EI_ABIVERSION];
        // we don't check (and I don't know anyone who uses this)
        Q_UNUSED(abiversion);
        return true;
    }

    // bytes 9-16
    static bool checkPadding(const uchar *ident)
    {
        // why would we check this?
        Q_UNUSED(ident);
        return true;
    }

    static bool checkIdent(const Ehdr &header)
    {
        return checkElfMagic(header.e_ident)
                && checkClass(header.e_ident)
                && checkDataOrder(header.e_ident)
                && checkElfVersion(header.e_ident)
                && checkOsAbi(header.e_ident)
                && checkAbiVersion(header.e_ident)
                && checkPadding(header.e_ident);
    }

    static bool checkType(const Ehdr &header)
    {
        return header.e_type == ET_DYN;
    }

    static bool checkMachine(const Ehdr &header)
    {
        return header.e_machine == ElfMachineCheck::ExpectedMachine;
    }

    static bool checkFileVersion(const Ehdr &header)
    {
        return header.e_version == EV_CURRENT;
    }

    static bool checkHeader(const Ehdr &header)
    {
        if (!checkIdent(header))
            return false;
        if (!IncludeValidityChecks)
            return true;
        return checkType(header)
                && checkMachine(header)
                && checkFileVersion(header);
    }

    Q_DECL_COLD_FUNCTION static QString explainCheckFailure(const Ehdr &header)
    {
        if (!checkElfMagic(header.e_ident))
            return QLibrary::tr("invalid signature");
        if (!checkClass(header.e_ident))
            return QLibrary::tr("file is for a different word size");
        if (!checkDataOrder(header.e_ident))
            return QLibrary::tr("file is for the wrong endianness");
        if (!checkElfVersion(header.e_ident) || !checkFileVersion(header))
            return QLibrary::tr("file has an unknown ELF version");
        if (!checkOsAbi(header.e_ident) || !checkAbiVersion(header.e_ident))
            return QLibrary::tr("file has an unexpected ABI");
        if (!checkType(header))
            return QLibrary::tr("file is not a shared object");
        if (!checkMachine(header))
            return QLibrary::tr("file is for a different processor");
        return QString();
    }

    static CommonHeader extractCommonHeader(const uchar *data)
    {
        auto header = reinterpret_cast<const Ehdr *>(data);
        CommonHeader r;
        r.type = EndianTraits::fromEndian(header->e_type);
        r.machine = EndianTraits::fromEndian(header->e_machine);
        r.version = EndianTraits::fromEndian(header->e_version);
        return r;
    }
};

struct ElfHeaderDebug { const uchar *e_ident; };
Q_DECL_UNUSED Q_DECL_COLD_FUNCTION static QDebug &operator<<(QDebug &d, ElfHeaderDebug h)
{
    const uchar *e_ident = h.e_ident;
    if (!ElfHeaderCommonCheck::checkElfMagic(e_ident)) {
        d << "Not an ELF file (invalid signature)";
        return d;
    }

    QDebugStateSaver saver(d);
    d.nospace();
    quint8 elfclass = e_ident[EI_CLASS];
    switch (elfclass) {
    case ELFCLASSNONE:
    default:
        d << "Invalid ELF file (class " << e_ident[EI_CLASS] << "), ";
        break;
    case ELFCLASS32:
        d << "ELF 32-bit ";
        break;
    case ELFCLASS64:
        d << "ELF 64-bit ";
        break;
    }

    quint8 dataorder = e_ident[EI_DATA];
    switch (dataorder) {
    case ELFDATANONE:
    default:
        d << "invalid endianness (" << e_ident[EI_DATA] << ')';
        break;
    case ELFDATA2LSB:
        d << "LSB";
        break;
    case ELFDATA2MSB:
        d << "MSB";
        break;
    }

    switch (e_ident[EI_OSABI]) {
    case ELFOSABI_SYSV:     d << " (SYSV"; break;
    case ELFOSABI_HPUX:     d << " (HP-UX"; break;
    case ELFOSABI_NETBSD:   d << " (NetBSD"; break;
    case ELFOSABI_LINUX:    d << " (GNU/Linux"; break;
    case ELFOSABI_SOLARIS:  d << " (Solaris"; break;
    case ELFOSABI_AIX:      d << " (AIX"; break;
    case ELFOSABI_IRIX:     d << " (IRIX"; break;
    case ELFOSABI_FREEBSD:  d << " (FreeBSD"; break;
    case ELFOSABI_OPENBSD:  d << " (OpenBSD"; break;
    default:                d << " (OS ABI " << e_ident[EI_VERSION]; break;
    }

    if (e_ident[EI_ABIVERSION])
        d << " v" << e_ident[EI_ABIVERSION];
    d << ')';

    if (e_ident[EI_VERSION] != 1) {
        d << ", file version " << e_ident[EI_VERSION];
        return d;
    }

    ElfHeaderCommonCheck::CommonHeader r;
    if (elfclass == ELFCLASS64 && dataorder == ELFDATA2LSB)
        r = ElfHeaderCheck<quint64, QSysInfo::LittleEndian>::extractCommonHeader(e_ident);
    else if (elfclass == ELFCLASS32 && dataorder == ELFDATA2LSB)
        r = ElfHeaderCheck<quint32, QSysInfo::LittleEndian>::extractCommonHeader(e_ident);
    else if (elfclass == ELFCLASS64 && dataorder == ELFDATA2MSB)
        r = ElfHeaderCheck<quint64, QSysInfo::BigEndian>::extractCommonHeader(e_ident);
    else if (elfclass == ELFCLASS32 && dataorder == ELFDATA2MSB)
        r = ElfHeaderCheck<quint32, QSysInfo::BigEndian>::extractCommonHeader(e_ident);
    else
        return d;

    d << ", version " << r.version;

    switch (r.type) {
    case ET_NONE:   d << ", no type"; break;
    case ET_REL:    d << ", relocatable"; break;
    case ET_EXEC:   d << ", executable"; break;
    case ET_DYN:    d << ", shared library or PIC executable"; break;
    case ET_CORE:   d << ", core dump"; break;
    default:        d << ", unknown type " << r.type; break;
    }

    switch (r.machine) {
    // list definitely not exhaustive!
    case EM_NONE:       d << ", no machine"; break;
    case EM_ALPHA:      d << ", Alpha"; break;
    case EM_68K:        d << ", MC68000"; break;
    case EM_ARM:        d << ", ARM"; break;
    case EM_AARCH64:    d << ", AArch64"; break;
#ifdef EM_BLACKFIN
    case EM_BLACKFIN:   d << ", Blackfin"; break;
#endif
    case EM_IA_64:      d << ", IA-64"; break;
#ifdef EM_LOONGARCH
    case EM_LOONGARCH:  d << ", LoongArch"; break;
#endif
    case EM_MIPS:       d << ", MIPS"; break;
    case EM_PARISC:     d << ", HPPA"; break;
    case EM_PPC:        d << ", PowerPC"; break;
    case EM_PPC64:      d << ", PowerPC 64-bit"; break;
#ifdef EM_RISCV
    case EM_RISCV:      d << ", RISC-V"; break;
#endif
#ifdef EM_S390
    case EM_S390:       d << ", S/390"; break;
#endif
    case EM_SH:         d << ", SuperH"; break;
    case EM_SPARC:      d << ", SPARC"; break;
    case EM_SPARCV9:    d << ", SPARCv9"; break;
    case EM_386:        d << ", i386"; break;
    case EM_X86_64:     d << ", x86-64"; break;
    default:            d << ", other machine type " << r.machine; break;
    }

    return d;
}

struct ElfSectionDebug { const ElfHeaderCheck<>::TypeTraits::Shdr *shdr; };
Q_DECL_UNUSED static QDebug &operator<<(QDebug &d, ElfSectionDebug s)
{
    // not exhaustive, just a few common things
    QDebugStateSaver saver(d);
    d << Qt::hex << Qt::showbase;
    d << "type";
    switch (s.shdr->sh_type) {
    case SHT_NULL:          d << "NULL"; break;
    case SHT_PROGBITS:      d << "PROGBITS"; break;
    case SHT_SYMTAB:        d << "SYMTAB"; break;
    case SHT_STRTAB:        d << "STRTAB"; break;
    case SHT_RELA:          d << "RELA"; break;
    case SHT_HASH:          d << "HASH"; break;
    case SHT_DYNAMIC:       d << "DYNAMIC"; break;
    case SHT_NOTE:          d << "NOTE"; break;
    case SHT_NOBITS:        d << "NOBITS"; break;
    case SHT_DYNSYM:        d << "DYNSYM"; break;
    case SHT_INIT_ARRAY:    d << "INIT_ARRAY"; break;
    case SHT_FINI_ARRAY:    d << "FINI_ARRAY"; break;
    default:                d << s.shdr->sh_type;
    }

    d << "flags";
    d.nospace();
    if (s.shdr->sh_flags & SHF_WRITE)
        d << 'W';
    if (s.shdr->sh_flags & SHF_ALLOC)
        d << 'A';
    if (s.shdr->sh_flags & SHF_EXECINSTR)
        d << 'X';
    if (s.shdr->sh_flags & SHF_STRINGS)
        d << 'S';
    if (s.shdr->sh_flags & SHF_TLS)
        d << 'T';

    d.space() << "offset" << s.shdr->sh_offset << "size" << s.shdr->sh_size;
    return d;
}

struct ElfProgramDebug { const ElfHeaderCheck<>::TypeTraits::Phdr *phdr; };
Q_DECL_UNUSED static QDebug &operator<<(QDebug &d, ElfProgramDebug p)
{
    QDebugStateSaver saved(d);
    d << Qt::hex << Qt::showbase << "program";
    switch (p.phdr->p_type) {
    case PT_NULL:       d << "NULL"; break;
    case PT_LOAD:       d << "LOAD"; break;
    case PT_DYNAMIC:    d << "DYNAMIC"; break;
    case PT_INTERP:     d << "INTERP"; break;
    case PT_NOTE:       d << "NOTE"; break;
    case PT_PHDR:       d << "PHDR"; break;
    case PT_TLS:        d << "TLS"; break;
    case PT_GNU_EH_FRAME:   d << "GNU_EH_FRAME"; break;
    case PT_GNU_STACK:      d << "GNU_STACK"; break;
    case PT_GNU_RELRO:      d << "GNU_RELRO"; break;
    case PT_GNU_PROPERTY:   d << "GNU_PROPERTY"; break;
    default:            d << "type" << p.phdr->p_type; break;
    }

    d << "offset" << p.phdr->p_offset
      << "virtaddr" << p.phdr->p_vaddr
      << "filesz" << p.phdr->p_filesz
      << "memsz" << p.phdr->p_memsz
      << "align" << p.phdr->p_align
      << "flags";

    d.nospace();
    if (p.phdr->p_flags & PF_R)
        d << 'R';
    if (p.phdr->p_flags & PF_W)
        d << 'W';
    if (p.phdr->p_flags & PF_X)
        d << 'X';

    return d;
}

struct ErrorMaker
{
    QString *errMsg;
    constexpr ErrorMaker(QString *errMsg) : errMsg(errMsg) {}


    Q_DECL_COLD_FUNCTION QLibraryScanResult operator()(QString &&text) const
    {
        *errMsg = QLibrary::tr("'%1' is not a valid ELF object (%2)").arg(*errMsg, std::move(text));
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
} // unnamed namespace

QT_WARNING_POP

using T = ElfHeaderCheck<>::TypeTraits;

template <typename F>
static bool scanProgramHeaders(QByteArrayView data, const ErrorMaker &error, F f)
{
    auto header = reinterpret_cast<const T::Ehdr *>(data.data());
    Q_UNUSED(error);

    auto phdr = reinterpret_cast<const T::Phdr *>(data.data() + header->e_phoff);
    auto phdr_end = phdr + header->e_phnum;
    for ( ; phdr != phdr_end; ++phdr) {
        if (!f(phdr))
            return false;
    }
    return true;
}

static bool preScanProgramHeaders(QByteArrayView data, const ErrorMaker &error)
{
    auto header = reinterpret_cast<const T::Ehdr *>(data.data());

    // first, validate the extent of the full program header table
    T::Word e_phnum = header->e_phnum;
    T::Off offset = e_phnum * sizeof(T::Phdr);  // can't overflow due to size of T::Half
    if (qAddOverflow(offset, header->e_phoff, &offset) || offset > size_t(data.size()))
        return error(QLibrary::tr("program header table extends past the end of the file")), false;

    // confirm validity
    bool hasCode = false;
    auto checker = [&](const T::Phdr *phdr) {
        qEDebug << ElfProgramDebug{phdr};

        if (T::Off end; qAddOverflow(phdr->p_offset, phdr->p_filesz, &end)
                || end > size_t(data.size()))
            return error(QLibrary::tr("a program header entry extends past the end of the file")), false;

        // this is not a validity check, it's to exclude debug symbol files
        if (phdr->p_type == PT_LOAD && phdr->p_filesz != 0 && (phdr->p_flags & PF_X))
            hasCode = true;

        // this probably applies to all segments, but we'll only apply it to notes
        if (phdr->p_type == PT_NOTE && qPopulationCount(phdr->p_align) == 1
                && phdr->p_offset & (phdr->p_align - 1)) {
            return error(QLibrary::tr("a note segment start is not properly aligned "
                                      "(offset 0x%1, alignment %2)")
                         .arg(phdr->p_offset, 6, 16, QChar(u'0'))
                         .arg(phdr->p_align)), false;
        }

        return true;
    };
    if (!scanProgramHeaders(data, error, checker))
        return false;
    if (!hasCode)
        return error.notplugin(QLibrary::tr("file has no code")), false;
    return true;
}

static QLibraryScanResult scanProgramHeadersForNotes(QByteArrayView data, const ErrorMaker &error)
{
    // minimum metadata payload is 2 bytes
    constexpr size_t MinPayloadSize = sizeof(QPluginMetaData::Header) + 2;
    constexpr qptrdiff MinNoteSize = sizeof(QPluginMetaData::ElfNoteHeader) + 2;
    constexpr size_t NoteNameSize = sizeof(QPluginMetaData::ElfNoteHeader::name);
    constexpr size_t NoteAlignment = alignof(QPluginMetaData::ElfNoteHeader);
    constexpr qptrdiff PayloadStartDelta = offsetof(QPluginMetaData::ElfNoteHeader, header);
    static_assert(MinNoteSize > PayloadStartDelta);
    static_assert((PayloadStartDelta & (NoteAlignment - 1)) == 0);

    QLibraryScanResult r = {};
    auto noteFinder = [&](const T::Phdr *phdr) {
        if (phdr->p_type != PT_NOTE || phdr->p_align != NoteAlignment)
            return true;

        // check for signed integer overflows, to avoid issues with the
        // arithmetic below
        if (qptrdiff(phdr->p_filesz) < 0) {
            auto h = reinterpret_cast<const T::Ehdr *>(data.data());
            auto segments = reinterpret_cast<const T::Phdr *>(data.data() + h->e_phoff);
            qEDebug << "segment" << (phdr - segments) << "contains a note with size"
                    << Qt::hex << Qt::showbase << phdr->p_filesz
                    << "which is larger than half the virtual memory space";
            return true;
        }

        // iterate over the notes in this segment
        T::Off offset = phdr->p_offset;
        const T::Off end_offset = offset + phdr->p_filesz;
        while (qptrdiff(end_offset - offset) >= MinNoteSize) {
            auto nhdr = reinterpret_cast<const T::Nhdr *>(data.data() + offset);
            T::Word n_namesz = nhdr->n_namesz;
            T::Word n_descsz = nhdr->n_descsz;
            T::Word n_type = nhdr->n_type;

            // overflow check: calculate where the next note will be, if it exists
            T::Off next_offset = offset;
            next_offset += sizeof(T::Nhdr);          // can't overflow (we checked above)
            next_offset += NoteAlignment - 3;        // offset is aligned, this can't overflow
            if (qAddOverflow<T::Off>(next_offset, n_namesz, &next_offset))
                break;
            next_offset &= -NoteAlignment;

            next_offset += NoteAlignment - 3;        // offset is aligned, this can't overflow
            if (qAddOverflow<T::Off>(next_offset, n_descsz, &next_offset))
                break;
            next_offset &= -NoteAlignment;
            if (next_offset > end_offset)
                break;

            if (n_namesz == NoteNameSize && n_descsz >= MinPayloadSize
                    && n_type == QPluginMetaData::ElfNoteHeader::NoteType
                    && memcmp(nhdr + 1, QPluginMetaData::ElfNoteHeader::NoteName, NoteNameSize) == 0) {
                // yes, it's our note
                r.pos = offset + PayloadStartDelta;
                r.length = nhdr->n_descsz;
                return false;
            }
            offset = next_offset;
        }
        return true;
    };
    scanProgramHeaders(data, error, noteFinder);

    if (!r.length)
        return r;

    qEDebug << "found Qt metadata in ELF note at"
                << Qt::hex << Qt::showbase << r.pos << "size" << Qt::reset << r.length;
    return r;
}

static QLibraryScanResult scanSections(QByteArrayView data, const ErrorMaker &error)
{
    auto header = reinterpret_cast<const T::Ehdr *>(data.data());

    // in order to find the .qtmetadata section, we need to:
    // a) find the section table
    //    it's located at offset header->e_shoff
    // validate it
    T::Word e_shnum = header->e_shnum;
    T::Off offset = e_shnum * sizeof(T::Shdr);          // can't overflow due to size of T::Half
    if (qAddOverflow(offset, header->e_shoff, &offset) || offset > size_t(data.size()))
        return error(QLibrary::tr("section table extends past the end of the file"));

    // b) find the section entry for the section header string table (shstrab)
    //    it's a section whose entry is pointed by e_shstrndx
    auto sections = reinterpret_cast<const T::Shdr *>(data.data() + header->e_shoff);
    auto sections_end = sections + e_shnum;
    auto shdr = sections + header->e_shstrndx;

    // validate the shstrtab
    offset = shdr->sh_offset;
    T::Off shstrtab_size = shdr->sh_size;
    qEDebug << "shstrtab section is located at offset" << offset << "size" << shstrtab_size;
    if (T::Off end; qAddOverflow<T::Off>(offset, shstrtab_size, &end)
            || end > size_t(data.size()))
        return error(QLibrary::tr("section header string table extends past the end of the file"));

    // c) iterate over the sections to find .qtmetadata
    const char *shstrtab_start = data.data() + offset;
    shdr = sections;
    for (int section = 0; shdr != sections_end; ++section, ++shdr) {
        QLatin1StringView name;
        if (shdr->sh_name < shstrtab_size) {
            const char *namestart = shstrtab_start + shdr->sh_name;
            size_t len = qstrnlen(namestart, shstrtab_size - shdr->sh_name);
            name = QLatin1StringView(namestart, len);
        }
        qEDebug << "section" << section << "name" << name << ElfSectionDebug{shdr};

        // sanity check the section
        if (name.isNull())
            return error(QLibrary::tr("a section name extends past the end of the file"));

        // sections aren't allowed to extend past the end of the file, unless
        // they are NOBITS sections
        if (shdr->sh_type == SHT_NOBITS)
            continue;;
        if (T::Off end; qAddOverflow(shdr->sh_offset, shdr->sh_size, &end)
                || end > size_t(data.size())) {
            return error(QLibrary::tr("section contents extend past the end of the file"));
        }

        if (name != ".qtmetadata"_L1)
            continue;
        qEDebug << "found .qtmetadata section";
        if (shdr->sh_size < sizeof(QPluginMetaData::MagicHeader))
            return error(QLibrary::tr(".qtmetadata section is too small"));

        if (IncludeValidityChecks) {
            QByteArrayView expectedMagic = QByteArrayView::fromArray(QPluginMetaData::MagicString);
            QByteArrayView actualMagic = data.sliced(shdr->sh_offset, expectedMagic.size());
            if (expectedMagic != actualMagic)
                return error(QLibrary::tr(".qtmetadata section has incorrect magic"));

            if (shdr->sh_flags & SHF_WRITE)
                return error(QLibrary::tr(".qtmetadata section is writable"));
            if (shdr->sh_flags & SHF_EXECINSTR)
                return error(QLibrary::tr(".qtmetadata section is executable"));
        }

        return { qsizetype(shdr->sh_offset + sizeof(QPluginMetaData::MagicString)),
                 qsizetype(shdr->sh_size - sizeof(QPluginMetaData::MagicString)) };
    }

    // section .qtmetadata not found
    return error.notfound();
}

QLibraryScanResult QElfParser::parse(QByteArrayView data, QString *errMsg)
{
    ErrorMaker error(errMsg);
    if (size_t(data.size()) < sizeof(T::Ehdr)) {
        qEDebug << "file too small:" << size_t(data.size());
        return error(QLibrary::tr("file too small"));
    }

    qEDebug << ElfHeaderDebug{ reinterpret_cast<const uchar *>(data.data()) };

    auto header = reinterpret_cast<const T::Ehdr *>(data.data());
    if (!ElfHeaderCheck<>::checkHeader(*header))
        return error(ElfHeaderCheck<>::explainCheckFailure(*header));

    qEDebug << "contains" << header->e_phnum << "program headers of"
            << header->e_phentsize << "bytes at offset" << header->e_phoff;
    qEDebug << "contains" << header->e_shnum << "sections of" << header->e_shentsize
            << "bytes at offset" << header->e_shoff
            << "; section header string table (shstrtab) is entry" << header->e_shstrndx;

    // some sanity checks
    if constexpr (IncludeValidityChecks) {
        if (header->e_phentsize != sizeof(T::Phdr))
            return error(QLibrary::tr("unexpected program header entry size (%1)")
                         .arg(header->e_phentsize));
    }

    if (!preScanProgramHeaders(data, error))
        return {};

    if (QLibraryScanResult r = scanProgramHeadersForNotes(data, error); r.length)
        return r;

    if (!ElfNotesAreMandatory) {
        if constexpr (IncludeValidityChecks) {
            if (header->e_shentsize != sizeof(T::Shdr))
                return error(QLibrary::tr("unexpected section entry size (%1)")
                             .arg(header->e_shentsize));
        }
        if (header->e_shoff == 0 || header->e_shnum == 0) {
            // this is still a valid ELF file but we don't have a section table
            qEDebug << "no section table present, not able to find Qt metadata";
            return error.notfound();
        }

        if (header->e_shnum && header->e_shstrndx >= header->e_shnum)
            return error(QLibrary::tr("e_shstrndx greater than the number of sections e_shnum (%1 >= %2)")
                         .arg(header->e_shstrndx).arg(header->e_shnum));
        return scanSections(data, error);
    }
    return error.notfound();
}

QT_END_NAMESPACE

#endif // Q_OF_ELF
