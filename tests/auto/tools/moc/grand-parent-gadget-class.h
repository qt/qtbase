// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef GRANDPARENTGADGETCLASS_H
#define GRANDPARENTGADGETCLASS_H

#include <QtCore/qobjectdefs.h>

namespace GrandParentGadget {

struct BaseGadget { Q_GADGET };
struct Derived : BaseGadget {};
struct DerivedGadget : Derived { Q_GADGET };
template<typename T> struct CRTP : BaseGadget {};
struct CRTPDerivedGadget : CRTP<CRTPDerivedGadget> { Q_GADGET };
}

#endif // GRANDPARENTGADGETCLASS_H

