// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef PYTHONWRITEDECLARATION_H
#define PYTHONWRITEDECLARATION_H

#include <treewalker.h>

QT_BEGIN_NAMESPACE

class QTextStream;
class Driver;
class Uic;

struct Option;

namespace Python {

struct WriteDeclaration : public TreeWalker
{
    WriteDeclaration(Uic *uic);

    void acceptUI(DomUI *node) override;
    void acceptButtonGroup(const DomButtonGroup *buttonGroup) override;

private:
    Uic *m_uic;
    Driver *m_driver;
    QTextStream &m_output;
    const Option &m_option;
};

} // namespace Python

QT_END_NAMESPACE

#endif // PYTHONWRITEDECLARATION_H
