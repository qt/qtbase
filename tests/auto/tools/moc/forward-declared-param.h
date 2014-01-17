/****************************************************************************
**
** Copyright (C) 2012 Intel Corporation
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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
