/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#ifndef EXTRAQUALIFICATION_H
#define EXTRAQUALIFICATION_H

#include <QObject>

class Test : public QObject
{
    Q_OBJECT
public slots:
    // this is invalid code that does not compile, the extra qualification
    // is bad. However for example older gccs silently accept it, so customers
    // can write the code and moc generates bad metadata. So instead moc should
    // now write out a warning and /not/ generate any code, because the code is
    // bad and with a decent compiler it won't compile anyway.
    void Test::badFunctionDeclaration() {}

public:
    Q_SLOT void Test::anotherOne() {}
};
#endif // EXTRAQUALIFICATION_H
