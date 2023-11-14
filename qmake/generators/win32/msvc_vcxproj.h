// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef MSVC_VCXPROJ_H
#define MSVC_VCXPROJ_H

#include "msvc_vcproj.h"

QT_BEGIN_NAMESPACE

class VcxprojGenerator : public VcprojGenerator
{
public:
    VcxprojGenerator();

protected:
    VCProjectWriter *createProjectWriter() override;
};

QT_END_NAMESPACE

#endif // MSVC_VCXPROJ_H
