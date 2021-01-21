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

#ifndef PYTHONWRITEIMPORTS_H
#define PYTHONWRITEIMPORTS_H

#include <treewalker.h>

QT_BEGIN_NAMESPACE

class Uic;

namespace Python {

struct WriteImports : public TreeWalker
{
public:
    explicit WriteImports(Uic *uic);

    void acceptUI(DomUI *node) override;
    void acceptCustomWidget(DomCustomWidget *node) override;

private:
    void writeImport(const QString &module);
    QString qtModuleOf(const DomCustomWidget *node) const;

    Uic *const m_uic;
};

} // namespace Python

QT_END_NAMESPACE

#endif // PYTHONWRITEIMPORTS_H
