// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef VALIDATOR_H
#define VALIDATOR_H

#include "treewalker.h"

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

private:
    Driver *m_driver;
};

QT_END_NAMESPACE

#endif // VALIDATOR_H
