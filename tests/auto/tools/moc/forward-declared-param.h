/****************************************************************************
**
** Copyright (C) 2016 Intel Corporation.
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

#ifndef FORWARD_DECLARED_PARAM_H
#define FORWARD_DECLARED_PARAM_H
#include <qobject.h>
#include <qmetatype.h>

// test support for const refs to forward-declared structs in parameters

struct ForwardDeclaredParam;
template <typename T> class ForwardDeclaredContainer;

struct FullyDefined {};
Q_DECLARE_METATYPE(FullyDefined)

class ForwardDeclaredParamClass : public QObject
{
    Q_OBJECT
public slots:
    void slotNaked(const ForwardDeclaredParam &) {}
    void slotFDC(const ForwardDeclaredContainer<ForwardDeclaredParam> &) {}
    void slotFDC(const ForwardDeclaredContainer<int> &) {}
    void slotFDC(const ForwardDeclaredContainer<QString> &) {}
    void slotFDC(const ForwardDeclaredContainer<FullyDefined> &) {}
    void slotQSet(const QSet<ForwardDeclaredParam> &) {}
    void slotQSet(const QSet<int> &) {}
    void slotQSet(const QSet<QString> &) {}
    void slotQSet(const QSet<FullyDefined> &) {}

signals:
    void signalNaked(const ForwardDeclaredParam &);
    void signalFDC(const ForwardDeclaredContainer<ForwardDeclaredParam> &);
    void signalFDC(const ForwardDeclaredContainer<int> &);
    void signalFDC(const ForwardDeclaredContainer<QString> &);
    void signalFDC(const ForwardDeclaredContainer<FullyDefined> &);
    void signalQSet(const QSet<ForwardDeclaredParam> &);
    void signalQSet(const QSet<int> &);
    void signalQSet(const QSet<QString> &);
    void signalQSet(const QSet<FullyDefined> &);
};
#endif // FORWARD_DECLARED_PARAM_H
