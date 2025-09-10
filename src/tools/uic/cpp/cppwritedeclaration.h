// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CPPWRITEDECLARATION_H
#define CPPWRITEDECLARATION_H

#include "treewalker.h"

QT_BEGIN_NAMESPACE

class QTextStream;
class Driver;
class Uic;

struct Option;

namespace CPP {

struct WriteDeclaration : public TreeWalker
{
    WriteDeclaration(Uic *uic);

    void acceptUI(DomUI *node) override;
    void acceptWidget(DomWidget *node) override;
    void acceptSpacer(DomSpacer *node) override;
    void acceptLayout(DomLayout *node) override;
    void acceptActionGroup(DomActionGroup *node) override;
    void acceptAction(DomAction *node) override;
    void acceptButtonGroup(const DomButtonGroup *buttonGroup) override;

private:
    Uic *m_uic;
    Driver *m_driver;
    QTextStream &m_output;
    const Option &m_option;
};

} // namespace CPP

QT_END_NAMESPACE

#endif // CPPWRITEDECLARATION_H
