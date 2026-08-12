// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:insignificant reason:build-tool

#ifndef VALIDATOR_H
#define VALIDATOR_H

#include "treewalker.h"

#include <qstringlist.h>

QT_BEGIN_NAMESPACE

class QTextStream;
class Driver;
class Uic;

struct Option;

struct Validator : public TreeWalker
{
    Validator(Uic *uic);

    void acceptUI(DomUI *node) override;
    void acceptWidget(DomWidget *node) override;

    void acceptLayoutItem(DomLayoutItem *node) override;
    void acceptLayout(DomLayout *node) override;

    void acceptActionGroup(DomActionGroup *node) override;
    void acceptAction(DomAction *node) override;

    void acceptCustomWidget(DomCustomWidget *customWidget) override;
    void acceptInclude(DomInclude *incl) override;

    const QStringList &errors() const { return m_errors; }

private:
    QStringList m_errors;
    Driver *m_driver;
};

QT_END_NAMESPACE

#endif // VALIDATOR_H
