// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
