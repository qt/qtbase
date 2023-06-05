// Copyright (C) 2018 The Qt Company Ltd.
// Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author James Turner <james.turner@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <AppKit/AppKit.h>

#include <qpa/qplatformtheme.h>

#include "qcocoamenuitem.h"

#include "qcocoansmenu.h"
#include "qcocoamenu.h"
#include "qcocoamenubar.h"
#include "qcocoahelpers.h"
#include "qcocoaapplication.h" // for custom application category
#include "qcocoamenuloader.h"
#include <QtGui/private/qcoregraphics_p.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/private/qcore_mac_p.h>
#include <QtGui/private/qapplekeymapper_p.h>

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static const char *application_menu_strings[] =
{
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU","About %1"),
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU","Preferences..."),
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU","Services"),
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU","Hide %1"),
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU","Hide Others"),
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU","Show All"),
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU","Quit %1")
};

QString qt_mac_applicationmenu_string(int type)
{
    QString menuString = QString::fromLatin1(application_menu_strings[type]);
    const QString translated = QCoreApplication::translate("QMenuBar", application_menu_strings[type]);
    if (translated != menuString) {
        return translated;
    } else {
        return QCoreApplication::translate("MAC_APPLICATION_MENU", application_menu_strings[type]);
    }
}

QCocoaMenuItem::QCocoaMenuItem() :
    m_native(nil),
    m_itemView(nil),
    m_menu(nullptr),
    m_role(NoRole),
    m_iconSize(16),
    m_textSynced(false),
    m_isVisible(true),
    m_enabled(true),
    m_parentEnabled(true),
    m_isSeparator(false),
    m_checked(false),
    m_merged(false)
{
}

QCocoaMenuItem::~QCocoaMenuItem()
{
    QMacAutoReleasePool pool;

    if (m_menu && m_menu->menuParent() == this)
        m_menu->setMenuParent(nullptr);
    if (m_merged) {
        m_native.hidden = YES;
    } else {
        if (m_menu && m_menu->attachedItem() == m_native)
            m_menu->setAttachedItem(nil);
        [m_native release];
    }

    [m_itemView release];
}

void QCocoaMenuItem::setText(const QString &text)
{
    m_text = text;
}

void QCocoaMenuItem::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

void QCocoaMenuItem::setMenu(QPlatformMenu *menu)
{
    if (menu == m_menu)
        return;

    bool setAttached = false;
    if ([m_native.menu isKindOfClass:[QCocoaNSMenu class]]) {
        auto parentMenu = static_cast<QCocoaNSMenu *>(m_native.menu);
        setAttached = parentMenu.platformMenu && parentMenu.platformMenu->isAboutToShow();
    }

    if (m_menu && m_menu->menuParent() == this) {
        m_menu->setMenuParent(nullptr);
        // Free the menu from its parent's influence
        m_menu->propagateEnabledState(true);
        if (m_native && m_menu->attachedItem() == m_native)
            m_menu->setAttachedItem(nil);
    }

    QMacAutoReleasePool pool;
    m_menu = static_cast<QCocoaMenu *>(menu);
    if (m_menu) {
        m_menu->setMenuParent(this);
        m_menu->propagateEnabledState(isEnabled());
        if (setAttached)
            m_menu->setAttachedItem(m_native);
    } else {
        // we previously had a menu, but no longer
        // clear out our item so the nexy sync() call builds a new one
        [m_native release];
        m_native = nil;
    }
}

void QCocoaMenuItem::setVisible(bool isVisible)
{
    m_isVisible = isVisible;
}

void QCocoaMenuItem::setIsSeparator(bool isSeparator)
{
    m_isSeparator = isSeparator;
}

void QCocoaMenuItem::setFont(const QFont &font)
{
    Q_UNUSED(font);
}

void QCocoaMenuItem::setRole(MenuRole role)
{
    if (role != m_role)
        m_textSynced = false; // Changing role deserves a second chance.
    m_role = role;
}

#ifndef QT_NO_SHORTCUT
void QCocoaMenuItem::setShortcut(const QKeySequence& shortcut)
{
    m_shortcut = shortcut;
}
#endif

void QCocoaMenuItem::setChecked(bool isChecked)
{
    m_checked = isChecked;
}

void QCocoaMenuItem::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        if (m_menu)
            m_menu->propagateEnabledState(isEnabled());
    }
}

void QCocoaMenuItem::setNativeContents(WId item)
{
    NSView *itemView = (NSView *)item;
    if (m_itemView == itemView)
        return;
    [m_itemView release];
    m_itemView = [itemView retain];
    m_itemView.autoresizesSubviews = YES;
    m_itemView.autoresizingMask = NSViewWidthSizable;
    m_itemView.hidden = NO;
    m_itemView.needsDisplay = YES;
}

static QPlatformMenuItem::MenuRole detectMenuRole(const QString &captionWithPossibleMnemonic)
{
    QString itemCaption(captionWithPossibleMnemonic);
    itemCaption.remove(u'&');

    static const std::tuple<QPlatformMenuItem::MenuRole, std::vector<std::tuple<Qt::MatchFlags, const char *>>> roleMap[] = {
        { QPlatformMenuItem::AboutRole, {
            { Qt::MatchStartsWith | Qt::MatchEndsWith, QT_TRANSLATE_NOOP("QCocoaMenuItem", "About") }
        }},
        { QPlatformMenuItem::PreferencesRole, {
            { Qt::MatchStartsWith, QT_TRANSLATE_NOOP("QCocoaMenuItem", "Config") },
            { Qt::MatchStartsWith, QT_TRANSLATE_NOOP("QCocoaMenuItem", "Preference") },
            { Qt::MatchStartsWith, QT_TRANSLATE_NOOP("QCocoaMenuItem", "Options") },
            { Qt::MatchStartsWith, QT_TRANSLATE_NOOP("QCocoaMenuItem", "Setting") },
            { Qt::MatchStartsWith, QT_TRANSLATE_NOOP("QCocoaMenuItem", "Setup") },
        }},
        { QPlatformMenuItem::QuitRole, {
            { Qt::MatchStartsWith, QT_TRANSLATE_NOOP("QCocoaMenuItem", "Quit") },
            { Qt::MatchStartsWith, QT_TRANSLATE_NOOP("QCocoaMenuItem", "Exit") },
        }},
        { QPlatformMenuItem::CutRole, {
            { Qt::MatchExactly, QT_TRANSLATE_NOOP("QCocoaMenuItem", "Cut") }
        }},
        { QPlatformMenuItem::CopyRole, {
            { Qt::MatchExactly, QT_TRANSLATE_NOOP("QCocoaMenuItem", "Copy") }
        }},
        { QPlatformMenuItem::PasteRole, {
            { Qt::MatchExactly, QT_TRANSLATE_NOOP("QCocoaMenuItem", "Paste") }
        }},
        { QPlatformMenuItem::SelectAllRole, {
            { Qt::MatchExactly, QT_TRANSLATE_NOOP("QCocoaMenuItem", "Select All") }
        }},
    };

    auto match = [](const QString &caption, const QString &itemCaption, Qt::MatchFlags matchFlags) {
        if (matchFlags.testFlag(Qt::MatchExactly))
            return !itemCaption.compare(caption, Qt::CaseInsensitive);
        if (matchFlags.testFlag(Qt::MatchStartsWith) && itemCaption.startsWith(caption, Qt::CaseInsensitive))
            return true;
        if (matchFlags.testFlag(Qt::MatchEndsWith) && itemCaption.endsWith(caption, Qt::CaseInsensitive))
            return true;
        return false;
    };

    QPlatformMenuItem::MenuRole detectedRole = [&]{
        for (const auto &[role, captions] : roleMap) {
            for (const auto &[matchFlags, caption] : captions) {
                // Check for untranslated match
                if (match(caption, itemCaption, matchFlags))
                    return role;
                // Then translated with the current Qt translation
                if (match(QCoreApplication::translate("QCocoaMenuItem", caption), itemCaption, matchFlags))
                    return role;
            }
        }
        return QPlatformMenuItem::NoRole;
    }();

    if (detectedRole == QPlatformMenuItem::AboutRole) {
        static const QRegularExpression qtRegExp("qt$"_L1, QRegularExpression::CaseInsensitiveOption);
        if (itemCaption.contains(qtRegExp))
            detectedRole = QPlatformMenuItem::AboutQtRole;
    }

    return detectedRole;
}

NSMenuItem *QCocoaMenuItem::sync()
{
    if (m_isSeparator != m_native.separatorItem) {
        [m_native release];
        if (m_isSeparator)
            m_native = [[QCocoaNSMenuItem separatorItemWithPlatformMenuItem:this] retain];
        else
            m_native = nil;
    }

    if ((m_role != NoRole && !m_textSynced) || m_merged) {
        QCocoaMenuBar *menubar = nullptr;
        if (m_role == TextHeuristicRole) {
            // Recognized menu roles are only found in the first menus below the menubar
            QObject *p = menuParent();
            int depth = 1;
            while (depth < 3 && p && !(menubar = qobject_cast<QCocoaMenuBar *>(p))) {
                ++depth;
                QCocoaMenuObject *menuObject = dynamic_cast<QCocoaMenuObject *>(p);
                p = menuObject ? menuObject->menuParent() : nullptr;
            }

            if (menubar && depth < 3)
                m_detectedRole = detectMenuRole(m_text);
            else
                m_detectedRole = NoRole;
        }

        QCocoaMenuLoader *loader = [QCocoaMenuLoader sharedMenuLoader];
        NSMenuItem *mergeItem = nil;
        const auto role = effectiveRole();
        switch (role) {
        case AboutRole:
            mergeItem = [loader aboutMenuItem];
            break;
        case AboutQtRole:
            mergeItem = [loader aboutQtMenuItem];
            break;
        case PreferencesRole:
            mergeItem = [loader preferencesMenuItem];
            break;
        case ApplicationSpecificRole:
            mergeItem = [loader appSpecificMenuItem:this];
            break;
        case QuitRole:
            mergeItem = [loader quitMenuItem];
            break;
        case CutRole:
        case CopyRole:
        case PasteRole:
        case SelectAllRole:
            if (menubar)
                mergeItem = menubar->itemForRole(role);
            break;
        case NoRole:
            // The heuristic couldn't resolve the menu role
            m_textSynced = false;
            break;
        default:
            if (!m_text.isEmpty())
                m_textSynced = true;
            break;
        }

        if (mergeItem) {
            m_textSynced = true;
            m_merged = true;
            [mergeItem retain];
            [m_native release];
            m_native = mergeItem;
            if (auto *nativeItem = qt_objc_cast<QCocoaNSMenuItem *>(m_native))
                nativeItem.platformMenuItem = this;
        } else if (m_merged) {
            // was previously merged, but no longer
            [m_native release];
            m_native = nil; // create item below
            m_merged = false;
        }
    } else if (!m_text.isEmpty()) {
        m_textSynced = true; // NoRole, and that was set explicitly. So, nothing to do anymore.
    }

    if (!m_native) {
        m_native = [[QCocoaNSMenuItem alloc] initWithPlatformMenuItem:this];
        m_native.title = m_text.toNSString();
    }

    resolveTargetAction();

    m_native.hidden = !m_isVisible;
    m_native.view = m_itemView;

    QString text = mergeText();
#ifndef QT_NO_SHORTCUT
    QKeySequence accel = mergeAccel();

    // Show multiple key sequences as part of the menu text.
    if (accel.count() > 1)
        text += " ("_L1 + accel.toString(QKeySequence::NativeText) + ")"_L1;
#endif

    m_native.title = QPlatformTheme::removeMnemonics(text).toNSString();

#ifndef QT_NO_SHORTCUT
    if (accel.count() == 1) {
        auto key = accel[0].key();
        auto modifiers = accel[0].keyboardModifiers();

        QChar cocoaKey = QAppleKeyMapper::toCocoaKey(key);
        if (cocoaKey.isNull())
            cocoaKey = QChar(key).toLower().unicode();
        // Similar to qt_mac_removePrivateUnicode change the delete key,
        // so the symbol is correctly seen in native menu bar.
        if (cocoaKey.unicode() == NSDeleteFunctionKey)
            cocoaKey = QChar(NSDeleteCharacter);

        m_native.keyEquivalent = QStringView(&cocoaKey, 1).toNSString();
        m_native.keyEquivalentModifierMask = QAppleKeyMapper::toCocoaModifiers(modifiers);
    } else
#endif
    {
        m_native.keyEquivalent = @"";
        m_native.keyEquivalentModifierMask = NSEventModifierFlagCommand;
    }

    m_native.image = [NSImage imageFromQIcon:m_icon withSize:m_iconSize];

    m_native.state = m_checked ?  NSControlStateValueOn : NSControlStateValueOff;
    return m_native;
}

QString QCocoaMenuItem::mergeText()
{
    QCocoaMenuLoader *loader = [QCocoaMenuLoader sharedMenuLoader];
    if (m_native == [loader aboutMenuItem]) {
        return qt_mac_applicationmenu_string(AboutAppMenuItem).arg(qt_mac_applicationName());
    } else if (m_native== [loader aboutQtMenuItem]) {
        if (m_text == QString("About Qt"))
            return QCoreApplication::translate("QCocoaMenuItem", "About Qt");
        else
            return m_text;
    } else if (m_native == [loader preferencesMenuItem]) {
        return qt_mac_applicationmenu_string(PreferencesAppMenuItem);
    } else if (m_native == [loader quitMenuItem]) {
        return qt_mac_applicationmenu_string(QuitAppMenuItem).arg(qt_mac_applicationName());
    } else if (m_text.contains('\t')) {
        return m_text.left(m_text.indexOf('\t'));
    }
    return m_text;
}

#ifndef QT_NO_SHORTCUT
QKeySequence QCocoaMenuItem::mergeAccel()
{
    QCocoaMenuLoader *loader = [QCocoaMenuLoader sharedMenuLoader];
    if (m_native == [loader preferencesMenuItem])
        return QKeySequence(QKeySequence::Preferences);
    else if (m_native == [loader quitMenuItem])
        return QKeySequence(QKeySequence::Quit);
    else if (m_text.contains('\t'))
        return QKeySequence(m_text.mid(m_text.indexOf('\t') + 1), QKeySequence::NativeText);

    return m_shortcut;
}
#endif

void QCocoaMenuItem::syncMerged()
{
    if (!m_merged) {
        qCWarning(lcQpaMenus) << "Trying to sync non-merged" << this;
        return;
    }

    m_native.hidden = !m_isVisible;
}

void QCocoaMenuItem::setParentEnabled(bool enabled)
{
    if (m_parentEnabled != enabled) {
        m_parentEnabled = enabled;
        if (m_menu)
            m_menu->propagateEnabledState(isEnabled());
    }
}

QPlatformMenuItem::MenuRole QCocoaMenuItem::effectiveRole() const
{
    if (m_role > TextHeuristicRole)
        return m_role;
    else
        return m_detectedRole;
}

void QCocoaMenuItem::setIconSize(int size)
{
    m_iconSize = size;
}

void QCocoaMenuItem::resolveTargetAction()
{
    if (m_native.separatorItem)
        return;

    // Some items created by QCocoaMenuLoader are not
    // instances of QCocoaNSMenuItem and have their
    // target/action set as Interface Builder would.
    if (![m_native isMemberOfClass:[QCocoaNSMenuItem class]])
        return;

    // Use the responder chain and ensure native modal dialogs
    // continue receiving cut/copy/paste/etc. key equivalents.
    SEL roleAction;
    switch (effectiveRole()) {
    case CutRole:
        roleAction = @selector(cut:);
        break;
    case CopyRole:
        roleAction = @selector(copy:);
        break;
    case PasteRole:
        roleAction = @selector(paste:);
        break;
    case SelectAllRole:
        roleAction = @selector(selectAll:);
        break;
    default:
        if (m_menu) {
            // Menu items that represent sub menus should have submenuAction: as their
            // action, so that clicking the menu item opens the sub menu without closing
            // the entire menu hierarchy. A menu item with this action and a valid submenu
            // will disable NSMenuValidation for the item, which is normally not an issue
            // as NSMenuItems are enabled by default. But in our case, we haven't attached
            // the submenu yet, which results in AppKit concluding that there's no validator
            // for the item (the target is nil, and nothing responds to submenuAction:), and
            // will in response disable the menu item. To work around this we explicitly
            // enable the menu item in QCocoaMenu::setAttachedItem() once we have a submenu.
            roleAction = @selector(submenuAction:);
        } else {
            roleAction = @selector(qt_itemFired:);
        }
    }

    m_native.action = roleAction;
    m_native.target = nil;
}

QT_END_NAMESPACE
