// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef PYTHONWRITEIMPORTS_H
#define PYTHONWRITEIMPORTS_H

#include <writeincludesbase.h>

#include <QtCore/qhash.h>
#include <QtCore/qmap.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

namespace Python {

class WriteImports : public WriteIncludesBase
{
public:
    using ClassesPerModule = QMap<QString, QStringList>;

    explicit WriteImports(Uic *uic);

    void acceptUI(DomUI *node) override;
    void acceptProperty(DomProperty *node) override;

protected:
    void doAdd(const QString &className, const DomCustomWidget *dcw = nullptr) override;

private:
    void addPythonCustomWidget(const QString &className, const DomCustomWidget *dcw);
    bool addQtClass(const QString &className);
    void addEnumBaseClass(const QString &v);
    void writeResourceImport(const QString &module);
    QString resourceAbsolutePath(QString resource) const;

    QHash<QString, QString> m_classToModule;
    // Module->class (modules sorted)

    ClassesPerModule m_qtClasses;
    ClassesPerModule m_customWidgets;
    QStringList m_plainCustomWidgets; // Custom widgets without any module
};

} // namespace Python

QT_END_NAMESPACE

#endif // PYTHONWRITEIMPORTS_H
