/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qstringlist.h>

QT_BEGIN_NAMESPACE


enum Compiler {
    CC_UNKNOWN = 0,
    CC_MINGW   = 0x02,
    CC_INTEL   = 0x03,
    CC_MSVC2005 = 0x80,
    CC_MSVC2008 = 0x90,
    CC_MSVC2010 = 0xA0,
    CC_MSVC2012 = 0xB0,
    CC_MSVC2013 = 0xC0,
    CC_MSVC2015 = 0xD0,
    CC_MSVC2017 = 0xE0
};

struct CompilerInfo;
class Environment
{
public:
    static Compiler detectCompiler();
    static QString detectQMakeSpec();

    static int execute(QStringList arguments, const QStringList &additionalEnv, const QStringList &removeEnv);
    static QString execute(const QString &command, int *returnCode = 0);

private:
    static Compiler detectedCompiler;

    static CompilerInfo *compilerInfo(Compiler compiler);
};


QT_END_NAMESPACE
