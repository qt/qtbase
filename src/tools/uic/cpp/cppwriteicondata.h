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

#ifndef CPPWRITEICONDATA_H
#define CPPWRITEICONDATA_H

#include "treewalker.h"

QT_BEGIN_NAMESPACE

class QTextStream;
class QIODevice;
class Driver;
class Uic;

struct Option;

namespace CPP {

class WriteIconData : public TreeWalker
{
public:
    WriteIconData(Uic *uic);

    void acceptUI(DomUI *node) Q_DECL_OVERRIDE;
    void acceptImages(DomImages *images) Q_DECL_OVERRIDE;
    void acceptImage(DomImage *image) Q_DECL_OVERRIDE;

    static void writeImage(QTextStream &output, const QString &indent,
                           bool limitXPM_LineLength, const DomImage *image);
    static void writeImage(QIODevice &output, DomImage *image);

private:
    Driver *driver;
    QTextStream &output;
    const Option &option;
};

} // namespace CPP

QT_END_NAMESPACE

#endif // CPPWRITEICONDATA_H
