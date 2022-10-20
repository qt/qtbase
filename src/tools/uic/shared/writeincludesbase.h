/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef WRITEINCLUDES_BASE_H
#define WRITEINCLUDES_BASE_H

#include <treewalker.h>

#include <QtCore/qset.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class DomCustomWidget;
class Uic;

struct ClassInfoEntry
{
    const char *klass;
    const char *module;
    const char *header;
};

struct ClassInfoEntries
{
    const ClassInfoEntry *begin() const { return m_begin; }
    const ClassInfoEntry *end() const { return m_end; }

    const ClassInfoEntry *m_begin;
    const ClassInfoEntry *m_end;
};

ClassInfoEntries classInfoEntries();

class WriteIncludesBase : public TreeWalker
{
public:
    explicit WriteIncludesBase(Uic *uic);

    void acceptUI(DomUI *node) override;
    void acceptWidget(DomWidget *node) override;
    void acceptLayout(DomLayout *node) override;
    void acceptSpacer(DomSpacer *node) override;
    void acceptProperty(DomProperty *node) override;

    //
    // actions
    //
    void acceptActionGroup(DomActionGroup *node) override;
    void acceptAction(DomAction *node) override;
    void acceptActionRef(DomActionRef *node) override;

    //
    // custom widgets
    //
    void acceptCustomWidget(DomCustomWidget *node) override;

protected:
    void add(const QString &className, const DomCustomWidget *dcw = nullptr);

    virtual void doAdd(const QString &className, const DomCustomWidget *dcw = nullptr) = 0;

    const Uic *uic() const { return m_uic; }
    Uic *uic() { return m_uic; }

private:
    QSet<QString> m_knownClasses;
    Uic *m_uic;
    bool m_laidOut = false;
};

QT_END_NAMESPACE

#endif // WRITEINCLUDES_BASE_H
