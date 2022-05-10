// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef FORWARD_DECLARED_PARAM_H
#define FORWARD_DECLARED_PARAM_H
#include <qobject.h>
#include <qmetatype.h>
Q_MOC_INCLUDE("forwarddeclaredparam.h")

// test support for const refs to forward-declared structs in parameters

struct ForwardDeclaredParam;
template <typename T> class ForwardDeclaredContainer;

struct FullyDefined {};
inline size_t qHash(const FullyDefined &, size_t seed = 0) { return seed; }
inline bool operator==(const FullyDefined &, const FullyDefined &) { return true; }
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
