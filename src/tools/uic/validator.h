/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
****************************************************************************/

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
