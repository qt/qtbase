/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Designer of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "quiloader.h"
#include "quiloader_p.h"
#include "customwidget.h"

#include <formbuilder.h>
#include <formbuilderextra_p.h>
#include <textbuilder_p.h>
#include <ui4_p.h>

#include <QtCore/qdebug.h>
#include <QtWidgets/QAction>
#include <QtWidgets/QActionGroup>
#include <QtWidgets/QApplication>
#include <QtCore/QDir>
#include <QtCore/QLibraryInfo>
#include <QtWidgets/QLayout>
#include <QtWidgets/QWidget>
#include <QtCore/QMap>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QToolBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFontComboBox>

QT_BEGIN_NAMESPACE

typedef QMap<QString, bool> widget_map;
Q_GLOBAL_STATIC(widget_map, g_widgets)

class QUiLoader;
class QUiLoaderPrivate;

#ifdef QFORMINTERNAL_NAMESPACE
namespace QFormInternal
{
#endif

class TranslatingTextBuilder : public QTextBuilder
{
public:
    TranslatingTextBuilder(bool trEnabled, const QByteArray &className) :
        m_trEnabled(trEnabled), m_className(className) {}

    virtual QVariant loadText(const DomProperty *icon) const;

    virtual QVariant toNativeValue(const QVariant &value) const;

private:
    bool m_trEnabled;
    QByteArray m_className;
};

QVariant TranslatingTextBuilder::loadText(const DomProperty *text) const
{
    const DomString *str = text->elementString();
    if (!str)
        return QVariant();
    if (str->hasAttributeNotr()) {
        const QString notr = str->attributeNotr();
        if (notr == QLatin1String("true") || notr == QLatin1String("yes"))
            return QVariant::fromValue(str->text());
    }
    QUiTranslatableStringValue strVal;
    strVal.setValue(str->text().toUtf8());
    if (str->hasAttributeComment())
        strVal.setComment(str->attributeComment().toUtf8());
    return QVariant::fromValue(strVal);
}

QVariant TranslatingTextBuilder::toNativeValue(const QVariant &value) const
{
    if (value.canConvert<QUiTranslatableStringValue>()) {
        QUiTranslatableStringValue tsv = qvariant_cast<QUiTranslatableStringValue>(value);
        if (!m_trEnabled)
            return QString::fromUtf8(tsv.value().data());
        return QVariant::fromValue(
            QApplication::translate(m_className, tsv.value(), tsv.comment(),
                                    QCoreApplication::UnicodeUTF8));
    }
    if (value.canConvert<QString>())
        return QVariant::fromValue(qvariant_cast<QString>(value));
    return value;
}

// This is "exported" to linguist
const QUiItemRolePair qUiItemRoles[] = {
    { Qt::DisplayRole, Qt::DisplayPropertyRole },
#ifndef QT_NO_TOOLTIP
    { Qt::ToolTipRole, Qt::ToolTipPropertyRole },
#endif
#ifndef QT_NO_STATUSTIP
    { Qt::StatusTipRole, Qt::StatusTipPropertyRole },
#endif
#ifndef QT_NO_WHATSTHIS
    { Qt::WhatsThisRole, Qt::WhatsThisPropertyRole },
#endif
    { -1 , -1 }
};

static void recursiveReTranslate(QTreeWidgetItem *item, const QByteArray &class_name)
{
    const QUiItemRolePair *irs = qUiItemRoles;

    int cnt = item->columnCount();
    for (int i = 0; i < cnt; ++i) {
        for (unsigned j = 0; irs[j].shadowRole >= 0; j++) {
            QVariant v = item->data(i, irs[j].shadowRole);
            if (v.isValid()) {
                QUiTranslatableStringValue tsv = qvariant_cast<QUiTranslatableStringValue>(v);
                const QString text = QApplication::translate(class_name,
                                                             tsv.value(), tsv.comment(),
                                                             QCoreApplication::UnicodeUTF8);
                item->setData(i, irs[j].realRole, text);
            }
        }
    }

    cnt = item->childCount();
    for (int i = 0; i < cnt; ++i)
        recursiveReTranslate(item->child(i), class_name);
}

template<typename T>
static void reTranslateWidgetItem(T *item, const QByteArray &class_name)
{
    const QUiItemRolePair *irs = qUiItemRoles;

    for (unsigned j = 0; irs[j].shadowRole >= 0; j++) {
        QVariant v = item->data(irs[j].shadowRole);
        if (v.isValid()) {
            QUiTranslatableStringValue tsv = qvariant_cast<QUiTranslatableStringValue>(v);
            const QString text = QApplication::translate(class_name,
                                                         tsv.value(), tsv.comment(),
                                                         QCoreApplication::UnicodeUTF8);
            item->setData(irs[j].realRole, text);
        }
    }
}

static void reTranslateTableItem(QTableWidgetItem *item, const QByteArray &class_name)
{
    if (item)
        reTranslateWidgetItem(item, class_name);
}

#define RETRANSLATE_SUBWIDGET_PROP(mainWidget, setter, propName) \
    do { \
        QVariant v = mainWidget->widget(i)->property(propName); \
        if (v.isValid()) { \
            QUiTranslatableStringValue tsv = qvariant_cast<QUiTranslatableStringValue>(v); \
            const QString text = QApplication::translate(m_className, \
                                                         tsv.value(), tsv.comment(), \
                                                         QCoreApplication::UnicodeUTF8); \
            mainWidget->setter(i, text); \
        } \
    } while (0)

class TranslationWatcher: public QObject
{
    Q_OBJECT

public:
    TranslationWatcher(QObject *parent, const QByteArray &className):
        QObject(parent),
        m_className(className)
    {
    }

    virtual bool eventFilter(QObject *o, QEvent *event)
    {
        if (event->type() == QEvent::LanguageChange) {
            foreach (const QByteArray &prop, o->dynamicPropertyNames()) {
                if (prop.startsWith(PROP_GENERIC_PREFIX)) {
                    const QByteArray propName = prop.mid(sizeof(PROP_GENERIC_PREFIX) - 1);
                    const QUiTranslatableStringValue tsv =
                                qvariant_cast<QUiTranslatableStringValue>(o->property(prop));
                    const QString text = QApplication::translate(m_className,
                                                                 tsv.value(), tsv.comment(),
                                                                 QCoreApplication::UnicodeUTF8);
                    o->setProperty(propName, text);
                }
            }
            if (0) {
#ifndef QT_NO_TABWIDGET
            } else if (QTabWidget *tabw = qobject_cast<QTabWidget*>(o)) {
                const int cnt = tabw->count();
                for (int i = 0; i < cnt; ++i) {
                    RETRANSLATE_SUBWIDGET_PROP(tabw, setTabText, PROP_TABPAGETEXT);
# ifndef QT_NO_TOOLTIP
                    RETRANSLATE_SUBWIDGET_PROP(tabw, setTabToolTip, PROP_TABPAGETOOLTIP);
# endif
# ifndef QT_NO_WHATSTHIS
                    RETRANSLATE_SUBWIDGET_PROP(tabw, setTabWhatsThis, PROP_TABPAGEWHATSTHIS);
# endif
                }
#endif
#ifndef QT_NO_LISTWIDGET
            } else if (QListWidget *listw = qobject_cast<QListWidget*>(o)) {
                const int cnt = listw->count();
                for (int i = 0; i < cnt; ++i)
                    reTranslateWidgetItem(listw->item(i), m_className);
#endif
#ifndef QT_NO_TREEWIDGET
            } else if (QTreeWidget *treew = qobject_cast<QTreeWidget*>(o)) {
                if (QTreeWidgetItem *item = treew->headerItem())
                    recursiveReTranslate(item, m_className);
                const int cnt = treew->topLevelItemCount();
                for (int i = 0; i < cnt; ++i) {
                    QTreeWidgetItem *item = treew->topLevelItem(i);
                    recursiveReTranslate(item, m_className);
                }
#endif
#ifndef QT_NO_TABLEWIDGET
            } else if (QTableWidget *tablew = qobject_cast<QTableWidget*>(o)) {
                const int row_cnt = tablew->rowCount();
                const int col_cnt = tablew->columnCount();
                for (int j = 0; j < col_cnt; ++j)
                    reTranslateTableItem(tablew->horizontalHeaderItem(j), m_className);
                for (int i = 0; i < row_cnt; ++i) {
                    reTranslateTableItem(tablew->verticalHeaderItem(i), m_className);
                    for (int j = 0; j < col_cnt; ++j)
                        reTranslateTableItem(tablew->item(i, j), m_className);
                }
#endif
#ifndef QT_NO_COMBOBOX
            } else if (QComboBox *combow = qobject_cast<QComboBox*>(o)) {
                if (!qobject_cast<QFontComboBox*>(o)) {
                    const int cnt = combow->count();
                    for (int i = 0; i < cnt; ++i) {
                        const QVariant v = combow->itemData(i, Qt::DisplayPropertyRole);
                        if (v.isValid()) {
                            QUiTranslatableStringValue tsv = qvariant_cast<QUiTranslatableStringValue>(v);
                            const QString text = QApplication::translate(m_className,
                                                                         tsv.value(), tsv.comment(),
                                                                         QCoreApplication::UnicodeUTF8);
                            combow->setItemText(i, text);
                        }
                    }
                }
#endif
#ifndef QT_NO_TOOLBOX
            } else if (QToolBox *toolw = qobject_cast<QToolBox*>(o)) {
                const int cnt = toolw->count();
                for (int i = 0; i < cnt; ++i) {
                    RETRANSLATE_SUBWIDGET_PROP(toolw, setItemText, PROP_TOOLITEMTEXT);
# ifndef QT_NO_TOOLTIP
                    RETRANSLATE_SUBWIDGET_PROP(toolw, setItemToolTip, PROP_TOOLITEMTOOLTIP);
# endif
                }
#endif
            }
        }
        return false;
    }

private:
    QByteArray m_className;
};

class FormBuilderPrivate: public QFormBuilder
{
    friend class QT_PREPEND_NAMESPACE(QUiLoader);
    friend class QT_PREPEND_NAMESPACE(QUiLoaderPrivate);
    typedef QFormBuilder ParentClass;

public:
    QUiLoader *loader;

    bool dynamicTr;
    bool trEnabled;

    FormBuilderPrivate(): loader(0), dynamicTr(false), trEnabled(true), m_trwatch(0) {}

    QWidget *defaultCreateWidget(const QString &className, QWidget *parent, const QString &name)
    {
        return ParentClass::createWidget(className, parent, name);
    }

    QLayout *defaultCreateLayout(const QString &className, QObject *parent, const QString &name)
    {
        return ParentClass::createLayout(className, parent, name);
    }

    QAction *defaultCreateAction(QObject *parent, const QString &name)
    {
        return ParentClass::createAction(parent, name);
    }

    QActionGroup *defaultCreateActionGroup(QObject *parent, const QString &name)
    {
        return ParentClass::createActionGroup(parent, name);
    }

    virtual QWidget *createWidget(const QString &className, QWidget *parent, const QString &name)
    {
        if (QWidget *widget = loader->createWidget(className, parent, name)) {
            widget->setObjectName(name);
            return widget;
        }

        return 0;
    }

    virtual QLayout *createLayout(const QString &className, QObject *parent, const QString &name)
    {
        if (QLayout *layout = loader->createLayout(className, parent, name)) {
            layout->setObjectName(name);
            return layout;
        }

        return 0;
    }

    virtual QActionGroup *createActionGroup(QObject *parent, const QString &name)
    {
        if (QActionGroup *actionGroup = loader->createActionGroup(parent, name)) {
            actionGroup->setObjectName(name);
            return actionGroup;
        }

        return 0;
    }

    virtual QAction *createAction(QObject *parent, const QString &name)
    {
        if (QAction *action = loader->createAction(parent, name)) {
            action->setObjectName(name);
            return action;
        }

        return 0;
    }

    virtual void applyProperties(QObject *o, const QList<DomProperty*> &properties);
    virtual QWidget *create(DomUI *ui, QWidget *parentWidget);
    virtual QWidget *create(DomWidget *ui_widget, QWidget *parentWidget);
    virtual bool addItem(DomWidget *ui_widget, QWidget *widget, QWidget *parentWidget);

private:
    QByteArray m_class;
    TranslationWatcher *m_trwatch;
};

static QString convertTranslatable(const DomProperty *p, const QByteArray &className,
    QUiTranslatableStringValue *strVal)
{
    if (p->kind() != DomProperty::String)
        return QString();
    const DomString *dom_str = p->elementString();
    if (!dom_str)
        return QString();
    if (dom_str->hasAttributeNotr()) {
        const QString notr = dom_str->attributeNotr();
        if (notr == QLatin1String("yes") || notr == QLatin1String("true"))
            return QString();
    }
    strVal->setValue(dom_str->text().toUtf8());
    strVal->setComment(dom_str->attributeComment().toUtf8());
    if (strVal->value().isEmpty() && strVal->comment().isEmpty())
        return QString();
    return QApplication::translate(className,
                                   strVal->value(), strVal->comment(),
                                   QCoreApplication::UnicodeUTF8);
}

void FormBuilderPrivate::applyProperties(QObject *o, const QList<DomProperty*> &properties)
{
    typedef QList<DomProperty*> DomPropertyList;

    QFormBuilder::applyProperties(o, properties);

    if (!m_trwatch)
        m_trwatch = new TranslationWatcher(o, m_class);

    if (properties.empty())
        return;

    // Unlike string item roles, string properties are not loaded via the textBuilder
    // (as they are "shadowed" by the property sheets in designer). So do the initial
    // translation here.
    bool anyTrs = false;
    foreach (const DomProperty *p, properties) {
        QUiTranslatableStringValue strVal;
        const QString text = convertTranslatable(p, m_class, &strVal);
        if (text.isEmpty())
            continue;
        const QByteArray name = p->attributeName().toUtf8();
        if (dynamicTr) {
            o->setProperty(PROP_GENERIC_PREFIX + name, QVariant::fromValue(strVal));
            anyTrs = trEnabled;
        }
        o->setProperty(name, text);
    }
    if (anyTrs)
        o->installEventFilter(m_trwatch);
}

QWidget *FormBuilderPrivate::create(DomUI *ui, QWidget *parentWidget)
{
    m_class = ui->elementClass().toUtf8();
    m_trwatch = 0;
    setTextBuilder(new TranslatingTextBuilder(trEnabled, m_class));
    return QFormBuilder::create(ui, parentWidget);
}

QWidget *FormBuilderPrivate::create(DomWidget *ui_widget, QWidget *parentWidget)
{
    QWidget *w = QFormBuilder::create(ui_widget, parentWidget);
    if (w == 0)
        return 0;

    if (0) {
#ifndef QT_NO_TABWIDGET
    } else if (qobject_cast<QTabWidget*>(w)) {
#endif
#ifndef QT_NO_LISTWIDGET
    } else if (qobject_cast<QListWidget*>(w)) {
#endif
#ifndef QT_NO_TREEWIDGET
    } else if (qobject_cast<QTreeWidget*>(w)) {
#endif
#ifndef QT_NO_TABLEWIDGET
    } else if (qobject_cast<QTableWidget*>(w)) {
#endif
#ifndef QT_NO_COMBOBOX
    } else if (qobject_cast<QComboBox*>(w)) {
        if (qobject_cast<QFontComboBox*>(w))
            return w;
#endif
#ifndef QT_NO_TOOLBOX
    } else if (qobject_cast<QToolBox*>(w)) {
#endif
    } else {
        return w;
    }
    if (dynamicTr && trEnabled)
        w->installEventFilter(m_trwatch);
    return w;
}

#define TRANSLATE_SUBWIDGET_PROP(mainWidget, attribute, setter, propName) \
    do { \
        if (const DomProperty *p##attribute = attributes.value(strings.attribute)) { \
            QUiTranslatableStringValue strVal; \
            const QString text = convertTranslatable(p##attribute, m_class, &strVal); \
            if (!text.isEmpty()) { \
                if (dynamicTr) \
                    mainWidget->widget(i)->setProperty(propName, QVariant::fromValue(strVal)); \
                mainWidget->setter(i, text); \
            } \
        } \
    } while (0)

bool FormBuilderPrivate::addItem(DomWidget *ui_widget, QWidget *widget, QWidget *parentWidget)
{
    if (parentWidget == 0)
        return true;

    if (!ParentClass::addItem(ui_widget, widget, parentWidget))
        return false;

    // Check special cases. First: Custom container
    const QString className = QLatin1String(parentWidget->metaObject()->className());
    if (!d->customWidgetAddPageMethod(className).isEmpty())
        return true;

    const QFormBuilderStrings &strings = QFormBuilderStrings::instance();

    if (0) {
#ifndef QT_NO_TABWIDGET
    } else if (QTabWidget *tabWidget = qobject_cast<QTabWidget*>(parentWidget)) {
        const DomPropertyHash attributes = propertyMap(ui_widget->elementAttribute());
        const int i = tabWidget->count() - 1;
        TRANSLATE_SUBWIDGET_PROP(tabWidget, titleAttribute, setTabText, PROP_TABPAGETEXT);
# ifndef QT_NO_TOOLTIP
        TRANSLATE_SUBWIDGET_PROP(tabWidget, toolTipAttribute, setTabToolTip, PROP_TABPAGETOOLTIP);
# endif
# ifndef QT_NO_WHATSTHIS
        TRANSLATE_SUBWIDGET_PROP(tabWidget, whatsThisAttribute, setTabWhatsThis, PROP_TABPAGEWHATSTHIS);
# endif
#endif
#ifndef QT_NO_TOOLBOX
    } else if (QToolBox *toolBox = qobject_cast<QToolBox*>(parentWidget)) {
        const DomPropertyHash attributes = propertyMap(ui_widget->elementAttribute());
        const int i = toolBox->count() - 1;
        TRANSLATE_SUBWIDGET_PROP(toolBox, labelAttribute, setItemText, PROP_TOOLITEMTEXT);
# ifndef QT_NO_TOOLTIP
        TRANSLATE_SUBWIDGET_PROP(toolBox, toolTipAttribute, setItemToolTip, PROP_TOOLITEMTOOLTIP);
# endif
#endif
    }

    return true;
}

#ifdef QFORMINTERNAL_NAMESPACE
}
#endif

class QUiLoaderPrivate
{
public:
#ifdef QFORMINTERNAL_NAMESPACE
    QFormInternal::FormBuilderPrivate builder;
#else
    FormBuilderPrivate builder;
#endif

    void setupWidgetMap() const;
};

void QUiLoaderPrivate::setupWidgetMap() const
{
    if (!g_widgets()->isEmpty())
        return;

#define DECLARE_WIDGET(a, b) g_widgets()->insert(QLatin1String(#a), true);
#define DECLARE_LAYOUT(a, b)

#include "widgets.table"

#undef DECLARE_WIDGET
#undef DECLARE_WIDGET_1
#undef DECLARE_LAYOUT
}

/*!
    \class QUiLoader
    \inmodule QtUiTools

    \brief The QUiLoader class enables standalone applications to
    dynamically create user interfaces at run-time using the
    information stored in UI files or specified in plugin paths.

    In addition, you can customize or create your own user interface by
    deriving your own loader class.

    If you have a custom component or an application that embeds \QD, you can
    also use the QFormBuilder class provided by the QtDesigner module to create
    user interfaces from UI files.

    The QUiLoader class provides a collection of functions allowing you to
    create widgets based on the information stored in UI files (created
    with \QD) or available in the specified plugin paths. The specified plugin
    paths can be retrieved using the pluginPaths() function. Similarly, the
    contents of a UI file can be retrieved using the load() function. For
    example:

    \snippet doc/src/snippets/quiloader/mywidget.cpp 0

    By including the user interface in the form's resources (\c myform.qrc), we
    ensure that it will be present at run-time:

    \quotefile doc/src/snippets/quiloader/mywidget.qrc

    The availableWidgets() function returns a QStringList with the class names
    of the widgets available in the specified plugin paths. To create these
    widgets, simply use the createWidget() function. For example:

    \snippet doc/src/snippets/quiloader/main.cpp 0

    To make a custom widget available to the loader, you can use the
    addPluginPath() function; to remove all available widgets, you can call
    the clearPluginPaths() function.

    The createAction(), createActionGroup(), createLayout(), and createWidget()
    functions are used internally by the QUiLoader class whenever it has to
    create an action, action group, layout, or widget respectively. For that
    reason, you can subclass the QUiLoader class and reimplement these
    functions to intervene the process of constructing a user interface. For
    example, you might want to have a list of the actions created when loading
    a form or creating a custom widget.

    For a complete example using the QUiLoader class, see the
    \l{Calculator Builder Example}.

    \sa QtUiTools, QFormBuilder
*/

/*!
    Creates a form loader with the given \a parent.
*/
QUiLoader::QUiLoader(QObject *parent)
    : QObject(parent), d_ptr(new QUiLoaderPrivate)
{
    Q_D(QUiLoader);

    d->builder.loader = this;

    QStringList paths;
    foreach (const QString &path, QApplication::libraryPaths()) {
        QString libPath = path;
        libPath  += QDir::separator();
        libPath  += QLatin1String("designer");
        paths.append(libPath);
    }

    d->builder.setPluginPath(paths);
}

/*!
    Destroys the loader.
*/
QUiLoader::~QUiLoader()
{
}

/*!
    Loads a form from the given \a device and creates a new widget with the
    given \a parentWidget to hold its contents.

    \sa createWidget()
*/
QWidget *QUiLoader::load(QIODevice *device, QWidget *parentWidget)
{
    Q_D(QUiLoader);
    // QXmlStreamReader will report errors on open failure.
    if (!device->isOpen())
        device->open(QIODevice::ReadOnly|QIODevice::Text);
    return d->builder.load(device, parentWidget);
}

/*!
    Returns a list naming the paths in which the loader will search when
    locating custom widget plugins.

    \sa addPluginPath(), clearPluginPaths()
*/
QStringList QUiLoader::pluginPaths() const
{
    Q_D(const QUiLoader);
    return d->builder.pluginPaths();
}

/*!
    Clears the list of paths in which the loader will search when locating
    plugins.

    \sa addPluginPath(), pluginPaths()
*/
void QUiLoader::clearPluginPaths()
{
    Q_D(QUiLoader);
    d->builder.clearPluginPaths();
}

/*!
    Adds the given \a path to the list of paths in which the loader will search
    when locating plugins.

    \sa pluginPaths(), clearPluginPaths()
*/
void QUiLoader::addPluginPath(const QString &path)
{
    Q_D(QUiLoader);
    d->builder.addPluginPath(path);
}

/*!
    Creates a new widget with the given \a parent and \a name using the class
    specified by \a className. You can use this function to create any of the
    widgets returned by the availableWidgets() function.

    The function is also used internally by the QUiLoader class whenever it
    creates a widget. Hence, you can subclass QUiLoader and reimplement this
    function to intervene process of constructing a user interface or widget.
    However, in your implementation, ensure that you call QUiLoader's version
    first.

    \sa availableWidgets(), load()
*/
QWidget *QUiLoader::createWidget(const QString &className, QWidget *parent, const QString &name)
{
    Q_D(QUiLoader);
    return d->builder.defaultCreateWidget(className, parent, name);
}

/*!
    Creates a new layout with the given \a parent and \a name using the class
    specified by \a className.

    The function is also used internally by the QUiLoader class whenever it
    creates a widget. Hence, you can subclass QUiLoader and reimplement this
    function to intervene process of constructing a user interface or widget.
    However, in your implementation, ensure that you call QUiLoader's version
    first.

    \sa createWidget(), load()
*/
QLayout *QUiLoader::createLayout(const QString &className, QObject *parent, const QString &name)
{
    Q_D(QUiLoader);
    return d->builder.defaultCreateLayout(className, parent, name);
}

/*!
    Creates a new action group with the given \a parent and \a name.

    The function is also used internally by the QUiLoader class whenever it
    creates a widget. Hence, you can subclass QUiLoader and reimplement this
    function to intervene process of constructing a user interface or widget.
    However, in your implementation, ensure that you call QUiLoader's version
    first.

    \sa createAction(), createWidget(), load()
 */
QActionGroup *QUiLoader::createActionGroup(QObject *parent, const QString &name)
{
    Q_D(QUiLoader);
    return d->builder.defaultCreateActionGroup(parent, name);
}

/*!
    Creates a new action with the given \a parent and \a name.

    The function is also used internally by the QUiLoader class whenever it
    creates a widget. Hence, you can subclass QUiLoader and reimplement this
    function to intervene process of constructing a user interface or widget.
    However, in your implementation, ensure that you call QUiLoader's version
    first.

    \sa createActionGroup(), createWidget(), load()
*/
QAction *QUiLoader::createAction(QObject *parent, const QString &name)
{
    Q_D(QUiLoader);
    return d->builder.defaultCreateAction(parent, name);
}

/*!
    Returns a list naming all available widgets that can be built using the
    createWidget() function, i.e all the widgets specified within the given
    plugin paths.

    \sa pluginPaths(), createWidget()

*/
QStringList QUiLoader::availableWidgets() const
{
    Q_D(const QUiLoader);

    d->setupWidgetMap();
    widget_map available = *g_widgets();

    foreach (QDesignerCustomWidgetInterface *plugin, d->builder.customWidgets()) {
        available.insert(plugin->name(), true);
    }

    return available.keys();
}


/*!
    \since 4.5
    Returns a list naming all available layouts that can be built using the
    createLayout() function

    \sa createLayout()
*/

QStringList QUiLoader::availableLayouts() const
{
    QStringList rc;
#define DECLARE_WIDGET(a, b)
#define DECLARE_LAYOUT(a, b) rc.push_back(QLatin1String(#a));

#include "widgets.table"

#undef DECLARE_WIDGET
#undef DECLARE_LAYOUT
    return rc;
}

/*!
    Sets the working directory of the loader to \a dir. The loader will look
    for other resources, such as icons and resource files, in paths relative to
    this directory.

    \sa workingDirectory()
*/

void QUiLoader::setWorkingDirectory(const QDir &dir)
{
    Q_D(QUiLoader);
    d->builder.setWorkingDirectory(dir);
}

/*!
    Returns the working directory of the loader.

    \sa setWorkingDirectory()
*/

QDir QUiLoader::workingDirectory() const
{
    Q_D(const QUiLoader);
    return d->builder.workingDirectory();
}

/*!
    \internal
    \since 4.3

    If \a enabled is true, the loader will be able to execute scripts.
    Otherwise, execution of scripts will be disabled.

    \sa isScriptingEnabled()
*/

void QUiLoader::setScriptingEnabled(bool enabled)
{
    Q_D(QUiLoader);
    d->builder.setScriptingEnabled(enabled);
}

/*!
    \internal
    \since 4.3

    Returns true if execution of scripts is enabled; returns false otherwise.

    \sa setScriptingEnabled()
*/

bool QUiLoader::isScriptingEnabled() const
{
    Q_D(const QUiLoader);
    return d->builder.isScriptingEnabled();
}

/*!
    \since 4.5

    If \a enabled is true, user interfaces loaded by this loader will
    automatically retranslate themselves upon receiving a language change
    event. Otherwise, the user interfaces will not be retranslated.

    \sa isLanguageChangeEnabled()
*/

void QUiLoader::setLanguageChangeEnabled(bool enabled)
{
    Q_D(QUiLoader);
    d->builder.dynamicTr = enabled;
}

/*!
    \since 4.5

    Returns true if dynamic retranslation on language change is enabled;
    returns false otherwise.

    \sa setLanguageChangeEnabled()
*/

bool QUiLoader::isLanguageChangeEnabled() const
{
    Q_D(const QUiLoader);
    return d->builder.dynamicTr;
}

/*!
    \internal
    \since 4.5

    If \a enabled is true, user interfaces loaded by this loader will be
    translated. Otherwise, the user interfaces will not be translated.

    \note This is orthogonal to languageChangeEnabled.

    \sa isLanguageChangeEnabled(), setLanguageChangeEnabled()
*/

void QUiLoader::setTranslationEnabled(bool enabled)
{
    Q_D(QUiLoader);
    d->builder.trEnabled = enabled;
}

/*!
    \internal
    \since 4.5

    Returns true if translation is enabled; returns false otherwise.

    \sa setTranslationEnabled()
*/

bool QUiLoader::isTranslationEnabled() const
{
    Q_D(const QUiLoader);
    return d->builder.trEnabled;
}

QT_END_NAMESPACE

#include "quiloader.moc"
