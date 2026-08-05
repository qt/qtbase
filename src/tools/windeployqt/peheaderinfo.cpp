// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "peheaderinfo.h"

#include <QtCore/private/qsystemerror_p.h>
#include <QString>
#include <QMap>

#if defined(Q_OS_WIN)
#  include <QtCore/qt_windows.h>
#  include <QtCore/private/qsystemerror_p.h>
#  include <shlwapi.h>
#  include <delayimp.h>
#endif  // Q_OS_WIN

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

PeHeaderInfo::PeHeaderInfo(const QString fileName)
{
    mFileHandle = CreateFile(reinterpret_cast<const WCHAR*>(fileName.utf16()), GENERIC_READ, FILE_SHARE_READ, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (mFileHandle == INVALID_HANDLE_VALUE || mFileHandle == NULL) {
        mErrorMessage = QString::fromLatin1("Cannot open '%1': %2")
        .arg(fileName, QSystemError::windowsString());
        return;
    }

    mFileMapHandle = CreateFileMapping(mFileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
    if (mFileMapHandle == NULL) {
        mErrorMessage = QString::fromLatin1("Cannot create file mapping of '%1': %2")
        .arg(fileName, QSystemError::windowsString());
        return;
    }

    mFileMemory = MapViewOfFile(mFileMapHandle, FILE_MAP_READ, 0, 0, 0);
    if (!mFileMemory) {
        mErrorMessage = QString::fromLatin1("Cannot map '%1': %2")
        .arg(fileName, QSystemError::windowsString());
        return;
    }

    mNtHeaders = getNtHeader();
    if (!mNtHeaders)
        return;

    // Extract all the information immediately, and then release the handles to avoid file
    // locking.
    mWordSize = ntHeaderWordSize(mNtHeaders);
    mMachineArch = mNtHeaders->FileHeader.Machine;
    if (mWordSize == 32) {
        mDependentLibs =
            determineDependentLibs(reinterpret_cast<const IMAGE_NT_HEADERS32 *>(mNtHeaders));
        mIsDebug = determineDebug(reinterpret_cast<const IMAGE_NT_HEADERS32 *>(mNtHeaders));
    } else {
        mDependentLibs =
            determineDependentLibs(reinterpret_cast<const IMAGE_NT_HEADERS64 *>(mNtHeaders));
        mIsDebug = determineDebug(reinterpret_cast<const IMAGE_NT_HEADERS64 *>(mNtHeaders));
    }

    mValid = true;

    releaseFileResources();
}

PeHeaderInfo::~PeHeaderInfo()
{
    releaseFileResources();
}

void PeHeaderInfo::releaseFileResources()
{
    if (mFileMemory) {
        UnmapViewOfFile(mFileMemory);
        mFileMemory = nullptr;
    }

    if (mFileMapHandle != NULL) {
        CloseHandle(mFileMapHandle);
        mFileMapHandle = NULL;
    }

    if (mFileHandle != NULL && mFileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(mFileHandle);
        mFileHandle = NULL;
    }

    mNtHeaders = nullptr;
}

bool PeHeaderInfo::isValid()
{
    return mValid;
}

QString PeHeaderInfo::errorMessage()
{
    return mErrorMessage;
}

unsigned int PeHeaderInfo::wordSize()
{
    return mWordSize;
}

unsigned int PeHeaderInfo::machineArch()
{
    return mMachineArch;
}

QStringList PeHeaderInfo::dependentLibs()
{
    return mDependentLibs;
}

bool PeHeaderInfo::isDebug()
{
    return mIsDebug.value_or(false);
}

IMAGE_NT_HEADERS *PeHeaderInfo::getNtHeader()
{
    IMAGE_DOS_HEADER *dosHeader = static_cast<PIMAGE_DOS_HEADER>(mFileMemory);
    // Check DOS header consistency
    if (IsBadReadPtr(dosHeader, sizeof(IMAGE_DOS_HEADER))
        || dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        mErrorMessage = QString::fromLatin1("DOS header check failed.");
        return 0;
    }
    // Retrieve NT header
    char *ntHeaderC = static_cast<char *>(mFileMemory) + dosHeader->e_lfanew;
    IMAGE_NT_HEADERS *ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS *>(ntHeaderC);
    // check NT header consistency
    if (IsBadReadPtr(ntHeaders, sizeof(ntHeaders->Signature))
        || ntHeaders->Signature != IMAGE_NT_SIGNATURE
        || IsBadReadPtr(&ntHeaders->FileHeader, sizeof(IMAGE_FILE_HEADER))) {
        mErrorMessage = QString::fromLatin1("NT header check failed.");
        return 0;
    }
    // Check magic
    if (!ntHeaderWordSize(ntHeaders)) {
        mErrorMessage = QString::fromLatin1("NT header check failed; magic %1 is invalid.").arg(ntHeaders->OptionalHeader.Magic);
        return 0;
    }
    // Check section headers
    IMAGE_SECTION_HEADER *sectionHeaders = IMAGE_FIRST_SECTION(ntHeaders);
    if (IsBadReadPtr(sectionHeaders,
                     ntHeaders->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER))) {
        mErrorMessage = QString::fromLatin1("NT header section header check failed.");
        return 0;
    }
    return ntHeaders;
}

QString PeHeaderInfo::stringFromRvaPtr(const void *rvaPtr)
{
    return QString::fromLocal8Bit(static_cast<const char *>(rvaPtr));
}

PeHeaderInfo::MsvcDebugRuntimeResult PeHeaderInfo::checkMsvcDebugRuntime()
{
    for (const QString &lib : mDependentLibs) {
        qsizetype pos = 0;
        if (lib.startsWith("MSVCR"_L1, Qt::CaseInsensitive)
            || lib.startsWith("MSVCP"_L1, Qt::CaseInsensitive)
            || lib.startsWith("VCRUNTIME"_L1, Qt::CaseInsensitive)
            || lib.startsWith("VCCORLIB"_L1, Qt::CaseInsensitive)
            || lib.startsWith("CONCRT"_L1, Qt::CaseInsensitive)
            || lib.startsWith("UCRTBASE"_L1, Qt::CaseInsensitive)) {
            qsizetype lastDotPos = lib.lastIndexOf(u'.');
            pos = -1 == lastDotPos ? 0 : lastDotPos - 1;
        }

        if (pos > 0) {
            const auto removeExtraSuffix = [&lib, &pos](const QString &suffix) -> void {
                if (lib.contains(suffix, Qt::CaseInsensitive))
                    pos -= suffix.size();
            };
            removeExtraSuffix("_app"_L1);
            removeExtraSuffix("_atomic_wait"_L1);
            removeExtraSuffix("_codecvt_ids"_L1);
        }

        if (pos)
            return lib.at(pos).toLower() == u'd' ? MsvcDebugRuntime : MsvcReleaseRuntime;
    }
    return NoMsvcRuntime;
}

template <class ImageNtHeader>
QStringList PeHeaderInfo::readImportSections(const ImageNtHeader *ntHeaders)
{
    // Get import directory entry RVA and read out
    const DWORD importsStartRVA = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    if (!importsStartRVA) {
        mErrorMessage = QString::fromLatin1("Failed to find IMAGE_DIRECTORY_ENTRY_IMPORT entry.");
        return QStringList();
    }
    const IMAGE_IMPORT_DESCRIPTOR *importDesc = static_cast<const IMAGE_IMPORT_DESCRIPTOR *>(rvaToPtr(importsStartRVA, ntHeaders, mFileMemory));
    if (!importDesc) {
        mErrorMessage = QString::fromLatin1("Failed to find IMAGE_IMPORT_DESCRIPTOR entry.");
        return QStringList();
    }
    QStringList result;
    for ( ; importDesc->Name; ++importDesc)
        result.push_back(stringFromRvaPtr(rvaToPtr(importDesc->Name, ntHeaders, mFileMemory)));

    // Read delay-loaded DLLs, see http://msdn.microsoft.com/en-us/magazine/cc301808.aspx .
    // Check on grAttr bit 1 whether this is the format using RVA's > VS 6
    if (const DWORD delayedImportsStartRVA = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress) {
        const ImgDelayDescr *delayedImportDesc = static_cast<const ImgDelayDescr *>(rvaToPtr(delayedImportsStartRVA, ntHeaders, mFileMemory));
        for ( ; delayedImportDesc->rvaDLLName && (delayedImportDesc->grAttrs & 1); ++delayedImportDesc)
            result.push_back(stringFromRvaPtr(rvaToPtr(delayedImportDesc->rvaDLLName, ntHeaders, mFileMemory)));
    }

    return result;
}

template <class ImageNtHeader>
QStringList PeHeaderInfo::determineDependentLibs(const ImageNtHeader *nth)
{
    return readImportSections(nth);
}

template <class ImageNtHeader>
bool PeHeaderInfo::determineDebug(const ImageNtHeader *nth)
{
    if (nth->FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED)
        return false;

    if (mDependentLibs.isEmpty())
        mDependentLibs = determineDependentLibs(nth);

    const bool hasDebugEntry = nth->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
    // When an MSVC debug entry is present, check whether the debug runtime
    // is actually used to detect -release / -force-debug-info builds.
    const MsvcDebugRuntimeResult msvcrt = checkMsvcDebugRuntime();
    if (msvcrt == NoMsvcRuntime)
        return hasDebugEntry;
    else
        return hasDebugEntry && msvcrt == MsvcDebugRuntime;
}

template <class ImageNtHeader>
const IMAGE_SECTION_HEADER *PeHeaderInfo::findSectionHeader(DWORD rva, const ImageNtHeader *nTHeader)
{
    const IMAGE_SECTION_HEADER *section = IMAGE_FIRST_SECTION(nTHeader);
    const IMAGE_SECTION_HEADER *sectionEnd = section + nTHeader->FileHeader.NumberOfSections;
    for ( ; section < sectionEnd; ++section)
        if (rva >= section->VirtualAddress && rva < (section->VirtualAddress + section->Misc.VirtualSize))
            return section;
    return 0;
}

template <class ImageNtHeader>
const void *PeHeaderInfo::rvaToPtr(DWORD rva, const ImageNtHeader *nTHeader, const void *imageBase)
{
    const IMAGE_SECTION_HEADER *sectionHdr = findSectionHeader(rva, nTHeader);
    if (!sectionHdr)
        return 0;
    const DWORD delta = sectionHdr->VirtualAddress - sectionHdr->PointerToRawData;
    return static_cast<const char *>(imageBase) + rva - delta;
}

template <class ImageNtHeader>
unsigned int PeHeaderInfo::ntHeaderWordSize(const ImageNtHeader *header)
{
    // defines IMAGE_NT_OPTIONAL_HDR32_MAGIC, IMAGE_NT_OPTIONAL_HDR64_MAGIC
    enum { imageNtOptionlHeader32Magic = 0x10b, imageNtOptionlHeader64Magic = 0x20b };
    if (header->OptionalHeader.Magic == imageNtOptionlHeader32Magic)
        return 32;
    if (header->OptionalHeader.Magic == imageNtOptionlHeader64Magic)
        return 64;
    return 0;
}

typedef QMap<QString, PeHeaderInfo *> PeHeaderInfoMap;
Q_GLOBAL_STATIC(PeHeaderInfoMap, peCache);

PeHeaderInfo *PeHeaderInfoCache::peHeaderInfo(const QString &fileName)
{
    if (peCache->contains(fileName))
        return peCache->value(fileName);

    auto *peHeaderInfo = new PeHeaderInfo(fileName);
    peCache->insert(fileName, peHeaderInfo);
    return peHeaderInfo;
}

QT_END_NAMESPACE
