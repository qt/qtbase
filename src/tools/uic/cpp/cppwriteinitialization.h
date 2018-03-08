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

#ifndef CPPWRITEINITIALIZATION_H
#define CPPWRITEINITIALIZATION_H

#include "treewalker.h"
#include <qpair.h>
#include <qhash.h>
#include <qset.h>
#include <qmap.h>
#include <qstack.h>
#include <qtextstream.h>

QT_BEGIN_NAMESPACE

class Driver;
class Uic;
class DomBrush;
class DomFont;
class DomResourceIcon;
class DomSizePolicy;
class DomStringList;
struct Option;

namespace CPP {
    // Handle for a flat DOM font to get comparison functionality required for maps
    class FontHandle {
    public:
        FontHandle(const DomFont *domFont);
        int compare(const FontHandle &) const;
    private:
        const DomFont *m_domFont;
    };
    inline bool operator ==(const FontHandle &f1, const FontHandle &f2) { return f1.compare(f2) == 0; }
    inline bool operator  <(const FontHandle &f1, const FontHandle &f2) { return f1.compare(f2) < 0; }

    // Handle for a flat DOM icon to get comparison functionality required for maps
    class IconHandle {
    public:
        IconHandle(const DomResourceIcon *domIcon);
        int compare(const IconHandle &) const;
    private:
        const DomResourceIcon *m_domIcon;
    };
    inline bool operator ==(const IconHandle &i1, const IconHandle &i2) { return i1.compare(i2) == 0; }
    inline bool operator  <(const IconHandle &i1, const IconHandle &i2) { return i1.compare(i2) < 0; }

    // Handle for a flat DOM size policy to get comparison functionality required for maps
    class SizePolicyHandle {
    public:
        SizePolicyHandle(const DomSizePolicy *domSizePolicy);
        int compare(const SizePolicyHandle &) const;
    private:
        const DomSizePolicy *m_domSizePolicy;
    };
    inline bool operator ==(const SizePolicyHandle &f1, const SizePolicyHandle &f2) { return f1.compare(f2) == 0; }
    inline bool operator  <(const SizePolicyHandle &f1, const SizePolicyHandle &f2) { return f1.compare(f2) < 0; }


struct WriteInitialization : public TreeWalker
{
    typedef QList<DomProperty*> DomPropertyList;
    typedef QHash<QString, DomProperty*> DomPropertyMap;

    WriteInitialization(Uic *uic);

//
// widgets
//
    void acceptUI(DomUI *node) override;
    void acceptWidget(DomWidget *node) override;

    void acceptLayout(DomLayout *node) override;
    void acceptSpacer(DomSpacer *node) override;
    void acceptLayoutItem(DomLayoutItem *node) override;

//
// actions
//
    void acceptActionGroup(DomActionGroup *node) override;
    void acceptAction(DomAction *node) override;
    void acceptActionRef(DomActionRef *node) override;

//
// tab stops
//
    void acceptTabStops(DomTabStops *tabStops) override;

//
// custom widgets
//
    void acceptCustomWidgets(DomCustomWidgets *node) override;
    void acceptCustomWidget(DomCustomWidget *node) override;

//
// layout defaults/functions
//
    void acceptLayoutDefault(DomLayoutDefault *node) override   { m_LayoutDefaultHandler.acceptLayoutDefault(node); }
    void acceptLayoutFunction(DomLayoutFunction *node) override { m_LayoutDefaultHandler.acceptLayoutFunction(node); }

//
// signal/slot connections
//
    void acceptConnection(DomConnection *connection) override;

    enum {
        Use43UiFile = 0,
        TopLevelMargin,
        ChildMargin,
        SubLayoutMargin
    };

private:
    static QString domColor2QString(const DomColor *c);

    QString writeString(const QString &s, const QString &indent) const;

    QString iconCall(const DomProperty *prop);
    QString pixCall(const DomProperty *prop) const;
    QString pixCall(const QString &type, const QString &text) const;
    QString trCall(const QString &str, const QString &comment = QString(), const QString &id = QString()) const;
    QString trCall(DomString *str, const QString &defaultString = QString()) const;
    QString noTrCall(DomString *str, const QString &defaultString = QString()) const;
    QString autoTrCall(DomString *str, const QString &defaultString = QString()) const;
    inline QTextStream &autoTrOutput(const DomProperty *str);
    QTextStream &autoTrOutput(const DomString *str, const QString &defaultString = QString());
    // Apply a comma-separated list of values using a function "setSomething(int idx, value)"
    void writePropertyList(const QString &varName, const QString &setFunction, const QString &value, const QString &defaultValue);

    enum { WritePropertyIgnoreMargin = 1, WritePropertyIgnoreSpacing = 2, WritePropertyIgnoreObjectName = 4 };
    QString writeStringListProperty(const DomStringList *list) const;
    void writeProperties(const QString &varName, const QString &className, const DomPropertyList &lst, unsigned flags = 0);
    void writeColorGroup(DomColorGroup *colorGroup, const QString &group, const QString &paletteName);
    void writeBrush(const DomBrush *brush, const QString &brushName);

//
// special initialization
//
    class Item {
    public:
        Item(const QString &itemClassName, const QString &indent, QTextStream &setupUiStream, QTextStream &retranslateUiStream, Driver *driver);
        ~Item();
        enum EmptyItemPolicy {
            DontConstruct,
            ConstructItemOnly,
            ConstructItemAndVariable
        };
        QString writeSetupUi(const QString &parent, EmptyItemPolicy emptyItemPolicy = ConstructItemOnly);
        void writeRetranslateUi(const QString &parentPath);
        void addSetter(const QString &setter, const QString &directive = QString(), bool translatable = false); // don't call it if you already added *this as a child of another Item
        void addChild(Item *child); // all setters should already been added
        int setupUiCount() const { return m_setupUiData.setters.count(); }
        int retranslateUiCount() const { return m_retranslateUiData.setters.count(); }
    private:
        struct ItemData {
            ItemData() : policy(DontGenerate) {}
            QMultiMap<QString, QString> setters; // directive to setter
            QSet<QString> directives;
            enum TemporaryVariableGeneratorPolicy { // policies with priority, number describes the priority
                DontGenerate = 1,
                GenerateWithMultiDirective = 2,
                Generate = 3
            } policy;
        };
        ItemData m_setupUiData;
        ItemData m_retranslateUiData;
        QList<Item *> m_children;
        Item *m_parent;

        const QString m_itemClassName;
        const QString m_indent;
        QTextStream &m_setupUiStream;
        QTextStream &m_retranslateUiStream;
        Driver *m_driver;
    };

    void addInitializer(Item *item,
            const QString &name, int column, const QString &value, const QString &directive = QString(), bool translatable = false) const;
    void addQtFlagsInitializer(Item *item, const DomPropertyMap &properties,
            const QString &name, int column = -1) const;
    void addQtEnumInitializer(Item *item,
                    const DomPropertyMap &properties, const QString &name, int column = -1) const;
    void addBrushInitializer(Item *item,
                    const DomPropertyMap &properties, const QString &name, int column = -1);
    void addStringInitializer(Item *item,
            const DomPropertyMap &properties, const QString &name, int column = -1, const QString &directive = QString()) const;
    void addCommonInitializers(Item *item,
                    const DomPropertyMap &properties, int column = -1);

    void initializeMenu(DomWidget *w, const QString &parentWidget);
    void initializeComboBox(DomWidget *w);
    void initializeListWidget(DomWidget *w);
    void initializeTreeWidget(DomWidget *w);
    QList<Item *> initializeTreeWidgetItems(const QVector<DomItem *> &domItems);
    void initializeTableWidget(DomWidget *w);

    QString disableSorting(DomWidget *w, const QString &varName);
    void enableSorting(DomWidget *w, const QString &varName, const QString &tempName);

    QString findDeclaration(const QString &name);
    DomWidget *findWidget(QLatin1String widgetClass);

    bool isValidObject(const QString &name) const;

private:
    QString writeFontProperties(const DomFont *f);
    void writeResourceIcon(QTextStream &output, const QString &iconName, const QString &indent, const DomResourceIcon *i) const;
    QString writeIconProperties(const DomResourceIcon *i);
    QString writeSizePolicy(const DomSizePolicy *sp);
    QString writeBrushInitialization(const DomBrush *brush);
    void addButtonGroup(const DomWidget *node, const QString &varName);
    void addWizardPage(const QString &pageVarName, const DomWidget *page, const QString &parentWidget);

    const Uic *m_uic;
    Driver *m_driver;
    QTextStream &m_output;
    const Option &m_option;
    QString m_indent;
    QString m_dindent;
    bool m_stdsetdef;

    struct Buddy
    {
        QString objName;
        QString buddy;
    };
    friend class QTypeInfo<Buddy>;

    QStack<DomWidget*> m_widgetChain;
    QStack<DomLayout*> m_layoutChain;
    QStack<DomActionGroup*> m_actionGroupChain;
    QVector<Buddy> m_buddies;

    QSet<QString> m_buttonGroups;
    QHash<QString, DomWidget*> m_registeredWidgets;
    QHash<QString, DomAction*> m_registeredActions;
    typedef QHash<uint, QString> ColorBrushHash;
    ColorBrushHash m_colorBrushHash;
    // Map from font properties to  font variable name for reuse
    // Map from size policy to  variable for reuse
    typedef QMap<FontHandle, QString> FontPropertiesNameMap;
    typedef QMap<IconHandle, QString> IconPropertiesNameMap;
    typedef QMap<SizePolicyHandle, QString> SizePolicyNameMap;
    FontPropertiesNameMap m_fontPropertiesNameMap;
    IconPropertiesNameMap m_iconPropertiesNameMap;
    SizePolicyNameMap     m_sizePolicyNameMap;

    class LayoutDefaultHandler {
    public:
        LayoutDefaultHandler();
        void acceptLayoutDefault(DomLayoutDefault *node);
        void acceptLayoutFunction(DomLayoutFunction *node);

        // Write out the layout margin and spacing properties applying the defaults.
        void writeProperties(const QString &indent, const QString &varName,
                             const DomPropertyMap &pm, int marginType,
                             bool suppressMarginDefault, QTextStream &str) const;
    private:
        void writeProperty(int p, const QString &indent, const QString &objectName, const DomPropertyMap &pm,
                           const QString &propertyName, const QString &setter, int defaultStyleValue,
                           bool suppressDefault, QTextStream &str) const;

        enum Properties { Margin, Spacing, NumProperties };
        enum StateFlags { HasDefaultValue = 1, HasDefaultFunction = 2};
        unsigned m_state[NumProperties];
        int m_defaultValues[NumProperties];
        QString m_functions[NumProperties];
    };

    // layout defaults
    LayoutDefaultHandler m_LayoutDefaultHandler;
    int m_layoutMarginType;

    QString m_generatedClass;
    QString m_mainFormVarName;
    bool m_mainFormUsedInRetranslateUi;

    QString m_delayedInitialization;
    QTextStream m_delayedOut;

    QString m_refreshInitialization;
    QTextStream m_refreshOut;

    QString m_delayedActionInitialization;
    QTextStream m_actionOut;

    bool m_layoutWidget;
    bool m_firstThemeIcon;
};

} // namespace CPP

Q_DECLARE_TYPEINFO(CPP::WriteInitialization::Buddy, Q_MOVABLE_TYPE);

QT_END_NAMESPACE

#endif // CPPWRITEINITIALIZATION_H
