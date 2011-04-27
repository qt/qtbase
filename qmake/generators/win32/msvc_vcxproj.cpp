/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the qmake application of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "msvc_vcxproj.h"
#include "msbuild_objectmodel.h"
#include <qdir.h>
#include <qdiriterator.h>
#include <quuid.h>


QT_BEGIN_NAMESPACE
// Filter GUIDs (Do NOT change these!) ------------------------------
const char _GUIDSourceFiles[]          = "{4FC737F1-C7A5-4376-A066-2A32D752A2FF}";
const char _GUIDHeaderFiles[]          = "{93995380-89BD-4b04-88EB-625FBE52EBFB}";
const char _GUIDGeneratedFiles[]       = "{71ED8ED8-ACB9-4CE9-BBE1-E00B30144E11}";
const char _GUIDResourceFiles[]        = "{D9D6E242-F8AF-46E4-B9FD-80ECBC20BA3E}";
const char _GUIDLexYaccFiles[]         = "{E12AE0D2-192F-4d59-BD23-7D3FA58D3183}";
const char _GUIDTranslationFiles[]     = "{639EADAA-A684-42e4-A9AD-28FC9BCB8F7C}";
const char _GUIDFormFiles[]            = "{99349809-55BA-4b9d-BF79-8FDBB0286EB3}";
const char _GUIDExtraCompilerFiles[]   = "{E0D8C965-CC5F-43d7-AD63-FAEF0BBC0F85}";
QT_END_NAMESPACE

QT_BEGIN_NAMESPACE


VcxprojGenerator::VcxprojGenerator() : VcprojGenerator()
{
}

VCProjectWriter *VcxprojGenerator::createProjectWriter()
{
    return new VCXProjectWriter;
}

QT_END_NAMESPACE

