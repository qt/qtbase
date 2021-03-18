/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "driver.h"
#include "uic.h"
#include "ui4.h"

#include <language.h>

#include <qfileinfo.h>
#include <qdebug.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

Driver::Driver()
    : m_stdout(stdout, QFile::WriteOnly | QFile::Text)
{
    m_output = &m_stdout;
}

Driver::~Driver() = default;

static inline QString spacerItemClass()  { return QStringLiteral("QSpacerItem"); }
static inline QString actionGroupClass() { return QStringLiteral("QActionGroup"); }
static inline QString actionClass()      { return QStringLiteral("QAction"); }
static inline QString buttonGroupClass() { return QStringLiteral("QButtonGroup"); }

template <class DomClass>
Driver::DomObjectHashConstIt<DomClass>
    Driver::findByAttributeNameIt(const DomObjectHash<DomClass> &domHash,
                                  const QString &name) const
{
    const auto end = domHash.cend();
    for (auto it = domHash.cbegin(); it != end; ++it) {
        if (it.key()->attributeName() == name)
            return it;
    }
    return end;
}

template <class DomClass>
const DomClass *Driver::findByAttributeName(const DomObjectHash<DomClass> &domHash,
                                            const QString &name) const
{
    auto it = findByAttributeNameIt(domHash, name);
    return it != domHash.cend() ? it.key() : nullptr;
}

template <class DomClass>
QString Driver::findOrInsert(DomObjectHash<DomClass> *domHash, const DomClass *dom,
                             const QString &className, bool isMember)
{
    auto it = domHash->find(dom);
    if (it == domHash->end()) {
        const QString name = this->unique(dom->attributeName(), className);
        it = domHash->insert(dom, isMember ? language::self + name : name);
    }
    return it.value();
}

QString Driver::findOrInsertWidget(const DomWidget *ui_widget)
{
    // Top level is passed into setupUI(), everything else is a member variable
    const bool isMember = !m_widgets.isEmpty();
    return findOrInsert(&m_widgets, ui_widget, ui_widget->attributeClass(), isMember);
}

QString Driver::findOrInsertSpacer(const DomSpacer *ui_spacer)
{
    return findOrInsert(&m_spacers, ui_spacer, spacerItemClass());
}

QString Driver::findOrInsertLayout(const DomLayout *ui_layout)
{
    return findOrInsert(&m_layouts, ui_layout, ui_layout->attributeClass());
}

QString Driver::findOrInsertLayoutItem(const DomLayoutItem *ui_layoutItem)
{
    switch (ui_layoutItem->kind()) {
        case DomLayoutItem::Widget:
            return findOrInsertWidget(ui_layoutItem->elementWidget());
        case DomLayoutItem::Spacer:
            return findOrInsertSpacer(ui_layoutItem->elementSpacer());
        case DomLayoutItem::Layout:
            return findOrInsertLayout(ui_layoutItem->elementLayout());
        case DomLayoutItem::Unknown:
            break;
    }

    Q_ASSERT( 0 );

    return QString();
}

QString Driver::findOrInsertActionGroup(const DomActionGroup *ui_group)
{
    return findOrInsert(&m_actionGroups, ui_group, actionGroupClass());
}

QString Driver::findOrInsertAction(const DomAction *ui_action)
{
    return findOrInsert(&m_actions, ui_action, actionClass());
}

QString Driver::findOrInsertButtonGroup(const DomButtonGroup *ui_group)
{
    return findOrInsert(&m_buttonGroups, ui_group, buttonGroupClass());
}

// Find a group by its non-uniqified name
const DomButtonGroup *Driver::findButtonGroup(const QString &attributeName) const
{
    return findByAttributeName(m_buttonGroups, attributeName);
}


QString Driver::findOrInsertName(const QString &name)
{
    return unique(name);
}

QString Driver::normalizedName(const QString &name)
{
    QString result = name;
    std::replace_if(result.begin(), result.end(),
                    [] (QChar c) { return !c.isLetterOrNumber(); },
                    QLatin1Char('_'));
    return result;
}

QString Driver::unique(const QString &instanceName, const QString &className)
{
    QString name;
    bool alreadyUsed = false;

    if (!instanceName.isEmpty()) {
        name = normalizedName(instanceName);
        QString base = name;

        for (int id = 1; m_nameRepository.contains(name); ++id) {
            alreadyUsed = true;
            name = base + QString::number(id);
        }
    } else if (!className.isEmpty()) {
        name = unique(qtify(className));
    } else {
        name = unique(QLatin1String("var"));
    }

    if (alreadyUsed && !className.isEmpty()) {
        fprintf(stderr, "%s: Warning: The name '%s' (%s) is already in use, defaulting to '%s'.\n",
                qPrintable(m_option.messagePrefix()),
                qPrintable(instanceName), qPrintable(className),
                qPrintable(name));
    }

    m_nameRepository.insert(name, true);
    return name;
}

QString Driver::qtify(const QString &name)
{
    QString qname = name;

    if (qname.at(0) == QLatin1Char('Q') || qname.at(0) == QLatin1Char('K'))
        qname.remove(0, 1);

    for (int i = 0, size = qname.size(); i < size && qname.at(i).isUpper(); ++i)
        qname[i] = qname.at(i).toLower();

    return qname;
}

static bool isAnsiCCharacter(QChar c)
{
    return (c.toUpper() >= QLatin1Char('A') && c.toUpper() <= QLatin1Char('Z'))
           || c.isDigit() || c == QLatin1Char('_');
}

QString Driver::headerFileName() const
{
    QString name = m_option.outputFile;

    if (name.isEmpty()) {
        name = QLatin1String("ui_"); // ### use ui_ as prefix.
        name.append(m_option.inputFile);
    }

    return headerFileName(name);
}

QString Driver::headerFileName(const QString &fileName)
{
    if (fileName.isEmpty())
        return headerFileName(QLatin1String("noname"));

    QFileInfo info(fileName);
    QString baseName = info.baseName();
    // Transform into a valid C++ identifier
    if (!baseName.isEmpty() && baseName.at(0).isDigit())
        baseName.prepend(QLatin1Char('_'));
    for (int i = 0; i < baseName.size(); ++i) {
        QChar c = baseName.at(i);
        if (!isAnsiCCharacter(c)) {
            // Replace character by its unicode value
            QString hex = QString::number(c.unicode(), 16);
            baseName.replace(i, 1, QLatin1Char('_') + hex + QLatin1Char('_'));
            i += hex.size() + 1;
        }
    }
    return baseName.toUpper() + QLatin1String("_H");
}

bool Driver::printDependencies(const QString &fileName)
{
    Q_ASSERT(m_option.dependencies == true);

    m_option.inputFile = fileName;

    Uic tool(this);
    return tool.printDependencies();
}

bool Driver::uic(const QString &fileName, DomUI *ui, QTextStream *out)
{
    m_option.inputFile = fileName;
    setUseIdBasedTranslations(ui->attributeIdbasedtr());

    QTextStream *oldOutput = m_output;

    m_output = out != nullptr ? out : &m_stdout;

    Uic tool(this);
    const bool result = tool.write(ui);

    m_output = oldOutput;

    return result;
}

bool Driver::uic(const QString &fileName, QTextStream *out)
{
    QFile f;
    if (fileName.isEmpty())
        f.open(stdin, QIODevice::ReadOnly);
    else {
        f.setFileName(fileName);
        if (!f.open(QIODevice::ReadOnly))
            return false;
    }

    m_option.inputFile = fileName;

    QTextStream *oldOutput = m_output;
    bool deleteOutput = false;

    if (out) {
        m_output = out;
    } else {
#ifdef Q_OS_WIN
        // As one might also redirect the output to a file on win,
        // we should not create the textstream with QFile::Text flag.
        // The redirected file is opened in TextMode and this will
        // result in broken line endings as writing will replace \n again.
        m_output = new QTextStream(stdout, QIODevice::WriteOnly);
#else
        m_output = new QTextStream(stdout, QIODevice::WriteOnly | QFile::Text);
#endif
        deleteOutput = true;
    }

    Uic tool(this);
    bool rtn = tool.write(&f);
    f.close();

    if (deleteOutput)
        delete m_output;

    m_output = oldOutput;

    return rtn;
}

const DomWidget *Driver::widgetByName(const QString &attributeName) const
{
    return findByAttributeName(m_widgets, attributeName);
}

QString Driver::widgetVariableName(const QString &attributeName) const
{
    auto it = findByAttributeNameIt(m_widgets, attributeName);
    return it != m_widgets.cend() ? it.value() : QString();
}

const DomActionGroup *Driver::actionGroupByName(const QString &attributeName) const
{
    return findByAttributeName(m_actionGroups, attributeName);
}

const DomAction *Driver::actionByName(const QString &attributeName) const
{
    return findByAttributeName(m_actions, attributeName);
}

QT_END_NAMESPACE
