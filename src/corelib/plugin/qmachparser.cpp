// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmachparser_p.h"

#include <qendian.h>

#include <mach-o/loader.h>
#include <mach-o/fat.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// Whether we include some extra validity checks
// (checks to ensure we don't read out-of-bounds are always included)
static constexpr bool IncludeValidityChecks = true;

#if defined(Q_PROCESSOR_X86_64)
#  define MACHO64
static const cpu_type_t my_cputype = CPU_TYPE_X86_64;
#elif defined(Q_PROCESSOR_X86_32)
static const cpu_type_t my_cputype = CPU_TYPE_X86;
#elif defined(Q_PROCESSOR_POWER_64)
#  define MACHO64
static const cpu_type_t my_cputype = CPU_TYPE_POWERPC64;
#elif defined(Q_PROCESSOR_POWER_32)
static const cpu_type_t my_cputype = CPU_TYPE_POWERPC;
#elif defined(Q_PROCESSOR_ARM_64)
#  define MACHO64
static const cpu_type_t my_cputype = CPU_TYPE_ARM64;
#elif defined(Q_PROCESSOR_ARM)
static const cpu_type_t my_cputype = CPU_TYPE_ARM;
#else
#  error "Unknown CPU type"
#endif

#ifdef MACHO64
#  undef MACHO64
typedef mach_header_64 my_mach_header;
typedef segment_command_64 my_segment_command;
typedef section_64 my_section;
static const uint32_t my_magic = MH_MAGIC_64;
#else
typedef mach_header my_mach_header;
typedef segment_command my_segment_command;
typedef section my_section;
static const uint32_t my_magic = MH_MAGIC;
#endif

Q_DECL_COLD_FUNCTION
static QLibraryScanResult notfound(const QString &reason, QString *errorString)
{
    *errorString = QLibrary::tr("'%1' is not a valid Mach-O binary (%2)")
            .arg(*errorString, reason.isEmpty() ? QLibrary::tr("file is corrupt") : reason);
    return {};
}

static bool isEncrypted(const my_mach_header *header)
{
    auto commandCursor = uintptr_t(header) + sizeof(my_mach_header);
    for (uint32_t i = 0; i < header->ncmds; ++i) {
        load_command *loadCommand = reinterpret_cast<load_command *>(commandCursor);
        if (loadCommand->cmd == LC_ENCRYPTION_INFO || loadCommand->cmd == LC_ENCRYPTION_INFO_64) {
            // The layout of encryption_info_command and encryption_info_command_64 is the same
            // up until and including cryptid, so we can treat it as encryption_info_command.
            auto encryptionInfoCommand = reinterpret_cast<encryption_info_command*>(loadCommand);
            return encryptionInfoCommand->cryptid != 0;
        }
        commandCursor += loadCommand->cmdsize;
    }

    return false;
}

QLibraryScanResult  QMachOParser::parse(const char *m_s, ulong fdlen, QString *errorString)
{
    // The minimum size of a Mach-O binary we're interested in.
    // It must have a full Mach header, at least one segment and at least one
    // section. It's probably useless with just the "qtmetadata" section, but
    // it's valid nonetheless.
    // A fat binary must have this plus the fat header, of course.
    static const size_t MinFileSize = sizeof(my_mach_header) + sizeof(my_segment_command) + sizeof(my_section);
    static const size_t MinFatHeaderSize = sizeof(fat_header) + 2 * sizeof(fat_arch);

    if (Q_UNLIKELY(fdlen < MinFileSize))
        return notfound(QLibrary::tr("file too small"), errorString);

    // find out if this is a fat Mach-O binary first
    const my_mach_header *header = nullptr;
    const fat_header *fat = reinterpret_cast<const fat_header *>(m_s);
    if (fat->magic == qToBigEndian(FAT_MAGIC)) {
        // find our architecture in the binary
        const fat_arch *arch = reinterpret_cast<const fat_arch *>(fat + 1);
        if (Q_UNLIKELY(fdlen < MinFatHeaderSize)) {
            return notfound(QLibrary::tr("file too small"), errorString);
        }

        int count = qFromBigEndian(fat->nfat_arch);
        if (Q_UNLIKELY(fdlen < sizeof(*fat) + sizeof(*arch) * count))
            return notfound(QString(), errorString);

        for (int i = 0; i < count; ++i) {
            if (arch[i].cputype == qToBigEndian(my_cputype)) {
                // ### should we check the CPU subtype? Maybe on ARM?
                uint32_t size = qFromBigEndian(arch[i].size);
                uint32_t offset = qFromBigEndian(arch[i].offset);
                if (Q_UNLIKELY(size > fdlen) || Q_UNLIKELY(offset > fdlen)
                        || Q_UNLIKELY(size + offset > fdlen) || Q_UNLIKELY(size < MinFileSize))
                    return notfound(QString(), errorString);

                header = reinterpret_cast<const my_mach_header *>(m_s + offset);
                fdlen = size;
                break;
            }
        }
        if (!header)
            return notfound(QLibrary::tr("no suitable architecture in fat binary"), errorString);

        // check the magic again
        if (Q_UNLIKELY(header->magic != my_magic))
            return notfound(QString(), errorString);
    } else {
        header = reinterpret_cast<const my_mach_header *>(m_s);
        fat = 0;

        // check magic
        if (header->magic != my_magic)
            return notfound(QLibrary::tr("invalid magic %1").arg(qFromBigEndian(header->magic),
                                                                 8, 16, '0'_L1),
                      errorString);
    }

    // from this point on, everything is in host byte order

    // (re-)check the CPU type
    // ### should we check the CPU subtype? Maybe on ARM?
    if (header->cputype != my_cputype) {
        if (fat)
            return notfound(QString(), errorString);
        return notfound(QLibrary::tr("wrong architecture"), errorString);
    }

    // check the file type
    if (Q_UNLIKELY(header->filetype != MH_BUNDLE && header->filetype != MH_DYLIB))
        return notfound(QLibrary::tr("not a dynamic library"), errorString);

    // find the __TEXT segment, "qtmetadata" section
    const my_segment_command *seg = reinterpret_cast<const my_segment_command *>(header + 1);
    ulong minsize = sizeof(*header);

    for (uint i = 0; i < header->ncmds; ++i,
         seg = reinterpret_cast<const my_segment_command *>(reinterpret_cast<const char *>(seg) + seg->cmdsize)) {
        // We're sure that the file size includes at least one load command
        // but we have to check anyway if we're past the first
        if (Q_UNLIKELY(fdlen < minsize + sizeof(load_command)))
            return notfound(QString(), errorString);

        // cmdsize can't be trusted until validated
        // so check it against fdlen anyway
        // (these are unsigned operations, with overflow behavior specified in the standard)
        minsize += seg->cmdsize;
        if (Q_UNLIKELY(fdlen < minsize) || Q_UNLIKELY(fdlen < seg->cmdsize))
            return notfound(QString(), errorString);

        const uint32_t MyLoadCommand = sizeof(void *) > 4 ? LC_SEGMENT_64 : LC_SEGMENT;
        if (seg->cmd != MyLoadCommand)
            continue;

        // is this the __TEXT segment?
        if (strcmp(seg->segname, "__TEXT") == 0) {
            const my_section *sect = reinterpret_cast<const my_section *>(seg + 1);
            for (uint j = 0; j < seg->nsects; ++j) {
                // is this the "qtmetadata" section?
                if (strcmp(sect[j].sectname, "qtmetadata") != 0)
                    continue;

                // found it!
                if (Q_UNLIKELY(fdlen < sect[j].offset) || Q_UNLIKELY(fdlen < sect[j].size)
                        || Q_UNLIKELY(fdlen < sect[j].offset + sect[j].size))
                    return notfound(QString(), errorString);

                if (sect[j].size < sizeof(QPluginMetaData::MagicHeader))
                    return notfound(QLibrary::tr(".qtmetadata section is too small"), errorString);

                const bool binaryIsEncrypted = isEncrypted(header);
                qsizetype pos = reinterpret_cast<const char *>(header) - m_s + sect[j].offset;

                // We can not read the section data of encrypted libraries until they
                // have been dlopened(), so skip validity check if that's the case.
                if (IncludeValidityChecks && !binaryIsEncrypted) {
                    QByteArrayView expectedMagic = QByteArrayView::fromArray(QPluginMetaData::MagicString);
                    QByteArrayView actualMagic = QByteArrayView(m_s + pos, expectedMagic.size());
                    if (expectedMagic != actualMagic)
                        return notfound(QLibrary::tr(".qtmetadata section has incorrect magic"), errorString);
                }

                pos += sizeof(QPluginMetaData::MagicString);
                return { pos, qsizetype(sect[j].size - sizeof(QPluginMetaData::MagicString)), binaryIsEncrypted };
            }
        }

        // other type of segment
        seg = reinterpret_cast<const my_segment_command *>(reinterpret_cast<const char *>(seg) + seg->cmdsize);
    }

    // No .qtmetadata section was found
    *errorString = QLibrary::tr("'%1' is not a Qt plugin").arg(*errorString);
    return {};
}

QT_END_NAMESPACE
