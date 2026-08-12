// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:insignificant reason:build-tool

#include "validator.h"
#include "driver.h"
#include "ui4.h"
#include "uic.h"

#include <qstringview.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// An approximation of "Unicode Standard Annex #31" for checking property
// and enumeration identifiers to prevent code injection attacks.
// FIXME 6.9: Simplify according to QTBUG-126860
static bool isIdStart(QChar c)
{
    bool result = false;
    switch (c.category()) {
    case QChar::Letter_Uppercase:
    case QChar::Letter_Lowercase:
    case QChar::Letter_Titlecase:
    case QChar::Letter_Modifier:
    case QChar::Letter_Other:
    case QChar::Number_Letter:
        result = true;
        break;
    default:
        result = c == u'_';
        break;
    }
    return result;
}

static bool isIdContinuation(QChar c)
{
    bool result = false;
    switch (c.category()) {
    case QChar::Letter_Uppercase:
    case QChar::Letter_Lowercase:
    case QChar::Letter_Titlecase:
    case QChar::Letter_Modifier:
    case QChar::Letter_Other:
    case QChar::Number_Letter:
    case QChar::Mark_NonSpacing:
    case QChar::Mark_SpacingCombining:
    case QChar::Number_DecimalDigit:
    case QChar::Punctuation_Connector: // '_'
        result = true;
        break;
    default:
        break;
    }
    return result;
}

static bool isEnumIdContinuation(QChar c)
{
    return c == u':' || c == u'|' || c == u' ' || isIdContinuation(c);
}

static bool checkPropertyName(QStringView name)
{
    return !name.isEmpty() && isIdStart(name.at(0))
    && std::all_of(name.cbegin() + 1, name.cend(), isIdContinuation);
}

static bool checkEnumValue(QStringView name)
{
    return !name.isEmpty() && isIdStart(name.at(0))
    && std::all_of(name.cbegin() + 1, name.cend(), isEnumIdContinuation);
}

static bool isClassNameContinuation(QChar c)
{
    return c == u':' || c == u'.' || isIdContinuation(c);
}

static bool checkClassName(QStringView name)
{
    return !name.isEmpty() && isIdStart(name.at(0))
            && std::all_of(name.cbegin() + 1, name.cend(), isClassNameContinuation);
}

// Sanity check: A coarse check for any characters that would allow a code injection (C++/Python)
static bool isValidHeaderChar(QChar c)
{
    return !"<>\"'\n\t;#"_L1.contains(c);
}

static bool sanityCheck(QStringView name)
{
    return !name.isEmpty() && std::all_of(name.cbegin(), name.cend(), isValidHeaderChar);
}

static QString msgInvalidValue(const QString &name, const QString &value)
{
    return "Invalid property value: \""_L1 + name + "\": \""_L1 + value + u'"';
}

static QString msgInvalidPropertyName(const QString &name)
{
    return "Invalid property name: \""_L1 + name + u'"';
}

static QString msgInvalidClassName(const QString &name)
{
    return "Invalid class name: \""_L1 + name + u'"';
}

static QString msgInvalidPixmapFunction(const QString &name)
{
    return "Invalid pixmap function name: \""_L1 + name + u'"';
}

static QString msgInvalidAddPageMethod(const QString &name)
{
    return "Invalid add page method name: \""_L1 + name + u'"';
}

static QString msgInvalidHeader(const QString &name)
{
    return "Invalid header: \""_L1 + name + u'"';
}

static QString msgInvalidConnection(const DomConnection *connection)
{
    return "Invalid connection: \""_L1 + connection->elementSender() + "::"_L1
            + connection->elementSignal() + " -> "_L1 + connection->elementReceiver() + "::"_L1
            + connection->elementSlot() + u'"';
}

static void checkProperties(const QList<DomProperty *> &properties, QStringList *errors)
{
    for (const DomProperty *p : properties) {
        const bool isDynamicProperty = p->hasAttributeStdset() && p->attributeStdset() == 0;
        const QString &name = p->attributeName();
        if (!isDynamicProperty && !checkPropertyName(name))
            errors->append(msgInvalidPropertyName(name));
        switch (p->kind()) {
        case DomProperty::Set:
            if (!checkEnumValue(p->elementSet()))
                errors->append(msgInvalidValue(name, p->elementSet()));
            break;
        case DomProperty::Enum:
            if (!checkEnumValue(p->elementEnum()))
                errors->append(msgInvalidValue(name, p->elementEnum()));
        default:
            break;
        }
    }
}

Validator::Validator(Uic *uic)   :
    m_driver(uic->driver())
{
}

void Validator::acceptUI(DomUI *node)
{
    TreeWalker::acceptUI(node);

    if (!checkClassName(node->elementClass()))
        m_errors.append(msgInvalidClassName(node->elementClass()));

    if (node->hasElementPixmapFunction()) {
        const QString &pixmapFunction = node->elementPixmapFunction();
        // Accommodate for legacy forms with empty entries
        if (!pixmapFunction.isEmpty() && !checkClassName(pixmapFunction))
            m_errors.append(msgInvalidPixmapFunction(node->elementPixmapFunction()));
    }

    if (node->hasElementCustomWidgets())
        acceptCustomWidgets(node->elementCustomWidgets());
    if (node->hasElementIncludes())
        acceptIncludes(node->elementIncludes());
    if (node->hasElementConnections())
        acceptConnections(node->elementConnections());
}

void Validator::acceptWidget(DomWidget *node)
{
    (void) m_driver->findOrInsertWidget(node);

    checkProperties(node->elementProperty(), &m_errors);

    if (!checkClassName(node->attributeClass()))
        m_errors.append(msgInvalidClassName(node->attributeClass()));

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

    checkProperties(node->elementProperty(), &m_errors);

    TreeWalker::acceptLayout(node);
}

void Validator::acceptActionGroup(DomActionGroup *node)
{
    (void) m_driver->findOrInsertActionGroup(node);

    checkProperties(node->elementProperty(), &m_errors);

    TreeWalker::acceptActionGroup(node);
}

void Validator::acceptAction(DomAction *node)
{
    (void) m_driver->findOrInsertAction(node);

    checkProperties(node->elementProperty(), &m_errors);

    TreeWalker::acceptAction(node);
}

void Validator::acceptCustomWidget(DomCustomWidget *customWidget)
{
    if (customWidget->hasElementAddPageMethod()
        && !checkPropertyName(customWidget->elementAddPageMethod())) {
        m_errors.append(msgInvalidAddPageMethod(customWidget->elementAddPageMethod()));
    }
    if (customWidget->hasElementHeader())  {
        const QString &header = customWidget->elementHeader()->text();
        if (!sanityCheck(header))
            m_errors.append(msgInvalidHeader(header));
    }
    TreeWalker::acceptCustomWidget(customWidget);
}

void Validator::acceptInclude(DomInclude *incl)
{
    if (!sanityCheck(incl->text()))
        m_errors.append(msgInvalidHeader(incl->text()));
    TreeWalker::acceptInclude(incl);
}

void Validator::acceptConnection(DomConnection *connection)
{
    if (!sanityCheck(connection->elementSender()) || !sanityCheck(connection->elementSignal())
        || !sanityCheck(connection->elementReceiver()) || !sanityCheck(connection->elementSlot())) {
        m_errors.append(msgInvalidConnection(connection));
    }
    TreeWalker::acceptConnection(connection);
}

QT_END_NAMESPACE
