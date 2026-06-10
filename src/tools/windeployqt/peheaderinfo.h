// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef PEHEADERINFO_H
#define PEHEADERINFO_H

#include <qt_windows.h>

#include <QString>
#include <QStringList>

QT_BEGIN_NAMESPACE

class PeHeaderInfo {
public:
    PeHeaderInfo(const QString fileName);

    ~PeHeaderInfo();

    bool isValid();

    QString errorMessage();

    unsigned wordSize();

    unsigned int machineArch();

    QStringList dependentLibs();

    bool isDebug();

private:
    bool mValid = false;
    QString mErrorMessage;

    unsigned int mWordSize = 0;
    unsigned int mMachineArch = 0;
    QStringList mDependentLibs;
    std::optional<bool> mIsDebug;

    HANDLE mFileHandle = NULL;
    HANDLE mFileMapHandle = NULL;
    void *mFileMemory = nullptr;
    IMAGE_NT_HEADERS *mNtHeaders = nullptr;

    void releaseFileResources();

    IMAGE_NT_HEADERS *getNtHeader();
    template <class ImageNtHeader>
    QStringList readImportSections(const ImageNtHeader *ntHeaders);
    template <class ImageNtHeader>
    QStringList determineDependentLibs(const ImageNtHeader *nth);
    template <class ImageNtHeader>
    bool determineDebug(const ImageNtHeader *nth);

    static QString stringFromRvaPtr(const void *rvaPtr);

    // Helper for reading out PE executable files: Find a section header for an RVA
    // (IMAGE_NT_HEADERS64, IMAGE_NT_HEADERS32).
    template <class ImageNtHeader>
    const IMAGE_SECTION_HEADER *findSectionHeader(DWORD rva, const ImageNtHeader *nTHeader);

    // Helper for reading out PE executable files: convert RVA to pointer (IMAGE_NT_HEADERS64, IMAGE_NT_HEADERS32).
    template <class ImageNtHeader>
    const void *rvaToPtr(DWORD rva, const ImageNtHeader *nTHeader, const void *imageBase);

    template <class ImageNtHeader>
    unsigned ntHeaderWordSize(const ImageNtHeader *header);

    // Check for MSCV runtime (MSVCP90D.dll/MSVCP90.dll, MSVCP120D.dll/MSVCP120.dll,
    // VCRUNTIME140D.DLL/VCRUNTIME140.DLL (VS2015) or msvcp120d_app.dll/msvcp120_app.dll).
    enum MsvcDebugRuntimeResult { MsvcDebugRuntime, MsvcReleaseRuntime, NoMsvcRuntime };

    MsvcDebugRuntimeResult checkMsvcDebugRuntime();
};

class PeHeaderInfoCache {
public:
    PeHeaderInfoCache() = delete;

    static PeHeaderInfo *peHeaderInfo(const QString &fileName);
};

QT_END_NAMESPACE

#endif // PEHEADERINFO_H
