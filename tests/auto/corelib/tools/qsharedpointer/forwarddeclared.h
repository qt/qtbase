// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef FORWARDDECLARED_H
#define FORWARDDECLARED_H

extern int forwardDeclaredDestructorRunCount;
class ForwardDeclared;

#ifdef QT_NAMESPACE
namespace QT_NAMESPACE {
#endif
template <typename T> class QSharedPointer;
#ifdef QT_NAMESPACE
}
using namespace QT_NAMESPACE;
#endif

QSharedPointer<ForwardDeclared> *forwardPointer();

#endif // FORWARDDECLARED_H
