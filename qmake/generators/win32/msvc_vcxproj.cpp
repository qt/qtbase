// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "msvc_vcxproj.h"
#include "msbuild_objectmodel.h"

QT_BEGIN_NAMESPACE

VcxprojGenerator::VcxprojGenerator() : VcprojGenerator()
{
}

VCProjectWriter *VcxprojGenerator::createProjectWriter()
{
    return new VCXProjectWriter;
}

QT_END_NAMESPACE
