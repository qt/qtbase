/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

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
    void writeImport(const QString &module);

    QHash<QString, QString> m_classToModule;
    // Module->class (modules sorted)

    ClassesPerModule m_qtClasses;
    ClassesPerModule m_customWidgets;
    QStringList m_plainCustomWidgets; // Custom widgets without any module
};

} // namespace Python

QT_END_NAMESPACE

#endif // PYTHONWRITEIMPORTS_H
