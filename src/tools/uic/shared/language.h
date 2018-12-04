/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef LANGUAGE_H
#define LANGUAGE_H

#include <QtCore/qstringview.h>

QT_FORWARD_DECLARE_CLASS(QTextStream)

namespace language {

// Base class for streamable objects with one QStringView parameter
class StringViewStreamable
{
public:
    StringViewStreamable(QStringView parameter) : m_parameter(parameter) {}

    QStringView parameter() const { return m_parameter; }

private:
    QStringView m_parameter;
};

class qtConfig : public StringViewStreamable
{
public:
    qtConfig(QStringView name) : StringViewStreamable(name) {}
};

QTextStream &operator<<(QTextStream &str, const qtConfig &c);

class openQtConfig : public StringViewStreamable
{
public:
    openQtConfig(QStringView name) : StringViewStreamable(name) {}
};

QTextStream &operator<<(QTextStream &str, const openQtConfig &c);

class closeQtConfig : public StringViewStreamable
{
public:
    closeQtConfig(QStringView name) : StringViewStreamable(name) {}
};

QTextStream &operator<<(QTextStream &, const closeQtConfig &c);

const char *toolbarArea(int v);
const char *sizePolicy(int v);
const char *dockWidgetArea(int v);
const char *paletteColorRole(int v);

} // namespace language

#endif // LANGUAGE_H
