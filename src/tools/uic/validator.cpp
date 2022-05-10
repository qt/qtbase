// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "validator.h"
#include "driver.h"
#include "ui4.h"
#include "uic.h"

QT_BEGIN_NAMESPACE

Validator::Validator(Uic *uic)   :
    m_driver(uic->driver())
{
}

void Validator::acceptUI(DomUI *node)
{
    TreeWalker::acceptUI(node);
}

void Validator::acceptWidget(DomWidget *node)
{
    (void) m_driver->findOrInsertWidget(node);

    TreeWalker::acceptWidget(node);
}

void Validator::acceptLayoutItem(DomLayoutItem *node)
{
    (void) m_driver->findOrInsertLayoutItem(node);

    TreeWalker::acceptLayoutItem(node);
}

void Validator::acceptLayout(DomLayout *node)
{
    (void) m_driver->findOrInsertLayout(node);

    TreeWalker::acceptLayout(node);
}

void Validator::acceptActionGroup(DomActionGroup *node)
{
    (void) m_driver->findOrInsertActionGroup(node);

    TreeWalker::acceptActionGroup(node);
}

void Validator::acceptAction(DomAction *node)
{
    (void) m_driver->findOrInsertAction(node);

    TreeWalker::acceptAction(node);
}

QT_END_NAMESPACE
