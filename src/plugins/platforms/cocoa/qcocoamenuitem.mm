/****************************************************************************
**
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author James Turner <james.turner@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qpa/qplatformtheme.h>

#include "qcocoamenuitem.h"

#include "qcocoamenu.h"
#include "qcocoamenubar.h"
#include "messages.h"
#include "qcocoahelpers.h"
#include "qt_mac_p.h"
#include "qcocoaapplication.h" // for custom application category
#include "qcocoamenuloader.h"

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

static inline QCocoaMenuLoader *getMenuLoader()
{
    return [NSApp QT_MANGLE_NAMESPACE(qt_qcocoamenuLoader)];
}


static quint32 constructModifierMask(quint32 accel_key)
{
    quint32 ret = 0;
    const bool dontSwap = qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta);
    if ((accel_key & Qt::CTRL) == Qt::CTRL)
        ret |= (dontSwap ? NSControlKeyMask : NSCommandKeyMask);
    if ((accel_key & Qt::META) == Qt::META)
        ret |= (dontSwap ? NSCommandKeyMask : NSControlKeyMask);
    if ((accel_key & Qt::ALT) == Qt::ALT)
        ret |= NSAlternateKeyMask;
    if ((accel_key & Qt::SHIFT) == Qt::SHIFT)
        ret |= NSShiftKeyMask;
    return ret;
}

// return an autoreleased string given a QKeySequence (currently only looks at the first one).
NSString *keySequenceToKeyEqivalent(const QKeySequence &accel)
{
    quint32 accel_key = (accel[0] & ~(Qt::MODIFIER_MASK | Qt::UNICODE_ACCEL));
    QChar cocoa_key = qt_mac_qtKey2CocoaKey(Qt::Key(accel_key));
    if (cocoa_key.isNull())
        cocoa_key = QChar(accel_key).toLower().unicode();
    // Similar to qt_mac_removePrivateUnicode change the delete key so the symbol is correctly seen in native menubar
    if (cocoa_key.unicode() == NSDeleteFunctionKey)
        cocoa_key = NSDeleteCharacter;
    return [NSString stringWithCharacters:&cocoa_key.unicode() length:1];
}

// return the cocoa modifier mask for the QKeySequence (currently only looks at the first one).
NSUInteger keySequenceModifierMask(const QKeySequence &accel)
{
    return constructModifierMask(accel[0]);
}

QCocoaMenuItem::QCocoaMenuItem() :
    m_native(NULL),
    m_itemView(nil),
    m_textSynced(false),
    m_menu(NULL),
    m_isVisible(true),
    m_enabled(true),
    m_isSeparator(false),
    m_role(NoRole),
    m_checked(false),
    m_merged(false),
    m_tag(0),
    m_iconSize(16)
{
}

QCocoaMenuItem::~QCocoaMenuItem()
{
    QMacAutoReleasePool pool;

    if (m_menu && m_menu->menuParent() == this)
        m_menu->setMenuParent(0);
    if (m_merged) {
        [m_native setHidden:YES];
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

    if (m_menu) {
        if (m_menu->menuParent() == this)
            m_menu->setMenuParent(0);
    }

    QMacAutoReleasePool pool;
    m_menu = static_cast<QCocoaMenu *>(menu);
    if (m_menu) {
        m_menu->setMenuParent(this);
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
    m_font = font;
}

void QCocoaMenuItem::setRole(MenuRole role)
{
    if (role != m_role)
        m_textSynced = false; // Changing role deserves a second chance.
    m_role = role;
}

void QCocoaMenuItem::setShortcut(const QKeySequence& shortcut)
{
    m_shortcut = shortcut;
}

void QCocoaMenuItem::setChecked(bool isChecked)
{
    m_checked = isChecked;
}

void QCocoaMenuItem::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

void QCocoaMenuItem::setNativeContents(WId item)
{
    NSView *itemView = (NSView *)item;
    if (m_itemView == itemView)
        return;
    [m_itemView release];
    m_itemView = [itemView retain];
    [m_itemView setAutoresizesSubviews:YES];
    [m_itemView setAutoresizingMask:NSViewWidthSizable];
    [m_itemView setHidden:NO];
    [m_itemView setNeedsDisplay:YES];
}

NSMenuItem *QCocoaMenuItem::sync()
{
    if (m_isSeparator != [m_native isSeparatorItem]) {
        [m_native release];
        if (m_isSeparator) {
            m_native = [[NSMenuItem separatorItem] retain];
            [m_native setTag:reinterpret_cast<NSInteger>(this)];
        } else
            m_native = nil;
    }

    if ((m_role != NoRole && !m_textSynced) || m_merged) {
        NSMenuItem *mergeItem = nil;
        QCocoaMenuLoader *loader = getMenuLoader();
        switch (m_role) {
        case ApplicationSpecificRole:
            mergeItem = [loader appSpecificMenuItem:reinterpret_cast<NSInteger>(this)];
            break;
        case AboutRole:
            mergeItem = [loader aboutMenuItem];
            break;
        case AboutQtRole:
            mergeItem = [loader aboutQtMenuItem];
            break;
        case QuitRole:
            mergeItem = [loader quitMenuItem];
            break;
        case PreferencesRole:
            mergeItem = [loader preferencesMenuItem];
            break;
        case TextHeuristicRole: {
            QObject *p = menuParent();
            int depth = 1;
            QCocoaMenuBar *menubar = 0;
            while (depth < 3 && p && !(menubar = qobject_cast<QCocoaMenuBar *>(p))) {
                ++depth;
                QCocoaMenuObject *menuObject = dynamic_cast<QCocoaMenuObject *>(p);
                Q_ASSERT(menuObject);
                p = menuObject->menuParent();
            }
            if (depth == 3 || !menubar)
                break; // Menu item too deep in the hierarchy, or not connected to any menubar

            m_detectedRole = detectMenuRole(m_text);
            switch (m_detectedRole) {
            case QPlatformMenuItem::AboutRole:
                if (m_text.indexOf(QRegExp(QString::fromLatin1("qt$"), Qt::CaseInsensitive)) == -1)
                    mergeItem = [loader aboutMenuItem];
                else
                    mergeItem = [loader aboutQtMenuItem];
                break;
            case QPlatformMenuItem::PreferencesRole:
                mergeItem = [loader preferencesMenuItem];
                break;
            case QPlatformMenuItem::QuitRole:
                mergeItem = [loader quitMenuItem];
                break;
            default:
                if (m_detectedRole >= CutRole && m_detectedRole < RoleCount && menubar)
                    mergeItem = menubar->itemForRole(m_detectedRole);
                if (!m_text.isEmpty())
                    m_textSynced = true;
                break;
            }
            break;
        }

        default:
            qWarning() << "menu item" << m_text << "has unsupported role" << (int)m_role;
        }

        if (mergeItem) {
            m_textSynced = true;
            m_merged = true;
            [mergeItem retain];
            [m_native release];
            m_native = mergeItem;
            [m_native setTag:reinterpret_cast<NSInteger>(this)];
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
        m_native = [[NSMenuItem alloc] initWithTitle:QCFString::toNSString(m_text)
                                       action:nil
                                       keyEquivalent:@""];
        [m_native setTag:reinterpret_cast<NSInteger>(this)];
    }

    [m_native setHidden: !m_isVisible];
    [m_native setView:m_itemView];

    QString text = mergeText();
    QKeySequence accel = mergeAccel();

    // Show multiple key sequences as part of the menu text.
    if (accel.count() > 1)
        text += QLatin1String(" (") + accel.toString(QKeySequence::NativeText) + QLatin1String(")");

    QString finalString = QPlatformTheme::removeMnemonics(text);
    bool useAttributedTitle = false;
    // Cocoa Font and title
    if (m_font.resolve()) {
        NSFont *customMenuFont = [NSFont fontWithName:QCFString::toNSString(m_font.family())
                                  size:m_font.pointSize()];
        if (customMenuFont) {
            NSArray *keys = [NSArray arrayWithObjects:NSFontAttributeName, nil];
            NSArray *objects = [NSArray arrayWithObjects:customMenuFont, nil];
            NSDictionary *attributes = [NSDictionary dictionaryWithObjects:objects forKeys:keys];
            NSAttributedString *str = [[[NSAttributedString alloc] initWithString:QCFString::toNSString(finalString)
                                     attributes:attributes] autorelease];
            [m_native setAttributedTitle: str];
            useAttributedTitle = true;
        }
    }
    if (!useAttributedTitle) {
       [m_native setTitle: QCFString::toNSString(finalString)];
    }

    if (accel.count() == 1) {
        [m_native setKeyEquivalent:keySequenceToKeyEqivalent(accel)];
        [m_native setKeyEquivalentModifierMask:keySequenceModifierMask(accel)];
    } else {
        [m_native setKeyEquivalent:@""];
        [m_native setKeyEquivalentModifierMask:NSCommandKeyMask];
    }

    NSImage *img = nil;
    if (!m_icon.isNull()) {
        img = qt_mac_create_nsimage(m_icon);
        [img setSize:NSMakeSize(m_iconSize, m_iconSize)];
    }
    [m_native setImage:img];
    [img release];

    [m_native setState:m_checked ?  NSOnState : NSOffState];
    return m_native;
}

QT_BEGIN_NAMESPACE
extern QString qt_mac_applicationmenu_string(int type);
QT_END_NAMESPACE

QString QCocoaMenuItem::mergeText()
{
    QCocoaMenuLoader *loader = getMenuLoader();
    if (m_native == [loader aboutMenuItem]) {
        return qt_mac_applicationmenu_string(6).arg(qt_mac_applicationName());
    } else if (m_native== [loader aboutQtMenuItem]) {
        if (m_text == QString("About Qt"))
            return msgAboutQt();
        else
            return m_text;
    } else if (m_native == [loader preferencesMenuItem]) {
        return qt_mac_applicationmenu_string(4);
    } else if (m_native == [loader quitMenuItem]) {
        return qt_mac_applicationmenu_string(5).arg(qt_mac_applicationName());
    } else if (m_text.contains('\t')) {
        return m_text.left(m_text.indexOf('\t'));
    }
    return m_text;
}

QKeySequence QCocoaMenuItem::mergeAccel()
{
    QCocoaMenuLoader *loader = getMenuLoader();
    if (m_native == [loader preferencesMenuItem])
        return QKeySequence(QKeySequence::Preferences);
    else if (m_native == [loader quitMenuItem])
        return QKeySequence(QKeySequence::Quit);
    else if (m_text.contains('\t'))
        return QKeySequence(m_text.mid(m_text.indexOf('\t') + 1), QKeySequence::NativeText);

    return m_shortcut;
}

void QCocoaMenuItem::syncMerged()
{
    if (!m_merged) {
        qWarning("Trying to sync a non-merged item");
        return;
    }
    [m_native setTag:reinterpret_cast<NSInteger>(this)];
    [m_native setHidden: !m_isVisible];
}

void QCocoaMenuItem::syncModalState(bool modal)
{
    if (modal)
        [m_native setEnabled:NO];
    else
        [m_native setEnabled:YES];
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

QT_END_NAMESPACE
