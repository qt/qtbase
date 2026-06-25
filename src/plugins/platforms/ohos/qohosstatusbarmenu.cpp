// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosstatusbarmenu.h"
#include "qohosjsutils.h"
#include <QtCore/qobject.h>
#include <QtCore/qlist.h>
#include <QtGui/qicon.h>
#include <QtGui/QWindow>
#include <atomic>
#include <cstdint>
#include <qpa/qplatformtheme.h>
#include <string>

QT_BEGIN_NAMESPACE

namespace {

std::shared_ptr<void> registerOhosRightMenuClickListener(
    QtOhos::JsState &jsState, QOhosConsumer<std::string> clickedMenuCodeConsumer)
{
   return QtOhos::registerOnOffMethodsBasedEventHandler(
       jsState.eval<QNapi::Object>("@kit.StatusBarExtensionKit.statusBarManager"),
       "rightMenuClick",
       [clickedMenuCodeConsumer = std::move(clickedMenuCodeConsumer)](const QtOhos::CallbackInfo &info) {
            auto eventData = info.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
            auto optMenuCode = QNapi::getOptionalPropOrEmpty<QNapi::String>(
                QNapi::getOptionalPropOrEmpty<QNapi::Object>(eventData, "data"),
                "menuCode");
            if (!optMenuCode.IsEmpty()) {
                clickedMenuCodeConsumer(optMenuCode);
            } else {
                qOhosPrintfDebug(
                    "%s: no 'menuCode' in the event (%s), ignoring it",
                    Q_FUNC_INFO, QNapi::toJsonString(eventData).c_str());
            }
       });
}

QNapi::Object makeNotifyOnlyJsStatusBarMenuAction(
    QtOhos::JsState &jsState, const std::string &menuCode)
{
    return QNapi::makeObject(
        jsState.env(),
        {
            {"abilityName", "invalidAbility"},
            {"notifyOnly", true},
            {"menuCode", menuCode},
        });
}

QNapi::Object makeJsStatusBarMenuItemWithAction(
    QtOhos::JsState &jsState, const std::string &title, const std::string &menuCode)
{
    return QNapi::makeObject(
        jsState.env(),
        {
            {"title", title},
            {"menuAction", makeNotifyOnlyJsStatusBarMenuAction(jsState, menuCode)},
        });
}

QNapi::Object makeJsStatusBarMenuItemWithSubMenu(
    QtOhos::JsState &jsState, const std::string &title, QNapi::Array jsStatusBarSubMenuItems)
{
    return QNapi::makeObject(
        jsState.env(),
        {
            {"title", title},
            {"subMenu", jsStatusBarSubMenuItems},
        });
}

QNapi::Object makeJsStatusBarSubMenuItem(
    QtOhos::JsState &jsState, const std::string &subTitle, const std::string &menuCode)
{
    return QNapi::makeObject(
        jsState.env(),
        {
            {"subTitle", subTitle},
            {"menuAction", makeNotifyOnlyJsStatusBarMenuAction(jsState, menuCode)},
        });
}

class QOhosStatusBarMenuImpl;

class QOhosStatusBarMenuItem : public QPlatformMenuItem
{
    Q_OBJECT

public:
    QOhosStatusBarMenuItem();

    void setText(const QString &text) override;
    QString text() const;
    void setIcon(const QIcon &icon) override;

    void setMenu(QPlatformMenu *menu) override;

    void setVisible(bool isVisible) override;
    void setIsSeparator(bool isSeparator) override;
    void setFont(const QFont &font) override;
    void setRole(MenuRole role) override;
    void setCheckable(bool checkable) override;
    void setChecked(bool isChecked) override;
    void setShortcut(const QKeySequence &shortcut) override;
    void setEnabled(bool enabled) override;
    void setIconSize(int size) override;

    std::string menuCode() const;

    std::function<QOhosOptional<QNapi::Object>(QtOhos::JsState &)> makeJsStatusBarMenuItemFactory() const;
    std::function<QOhosOptional<QNapi::Object>(QtOhos::JsState &)> makeJsStatusBarSubMenuItemFactory() const;

private:
    QString m_text;
    std::string m_menuCode;
    bool m_isSeparator = false;
    QOhosStatusBarMenuImpl *m_menu = nullptr;

    static std::string generateNextMenuCode();
};

class QOhosStatusBarMenuImpl : public QOhosStatusBarMenu
{
    Q_OBJECT

public:
    QOhosStatusBarMenuImpl();

    void insertMenuItem(QPlatformMenuItem *menuItem, QPlatformMenuItem *before) override;
    void removeMenuItem(QPlatformMenuItem *menuItem) override;
    void syncMenuItem(QPlatformMenuItem *menuItem) override;
    void syncSeparatorsCollapsible(bool enable) override;

    void setText(const QString &text) override;
    void setIcon(const QIcon &icon) override;
    void setEnabled(bool enabled) override;
    void setVisible(bool visible) override;

    void showPopup(const QWindow *parentWindow, const QRect &targetRect, const QPlatformMenuItem *item) override;
    void dismiss() override;

    QPlatformMenuItem *menuItemAt(int position) const override;
    QPlatformMenuItem *menuItemForTag(quintptr tag) const override;

    QPlatformMenuItem *createMenuItem() const override;
    QPlatformMenu *createSubMenu() const override;

    std::function<QNapi::Array(QtOhos::JsState &)> makeJsStatusBarGroupMenusFactory() const override;

    std::function<QNapi::Array(QtOhos::JsState &)> makeJsStatusBarSubMenuItemsFactory() const;

private:
    QPlatformMenuItem *findItemByMenuCode(const std::string &menuCode) const;
    void handleRightClickEvent(const std::string &menuCode);

    QString m_text;
    QList<QPlatformMenuItem *> m_menuItems;

    struct JsScopeData
    {
        std::shared_ptr<void> m_rightMenuClickListenerHandle;
    };

    std::shared_ptr<JsScopeData> m_jsScopeData;
};

QOhosStatusBarMenuItem::QOhosStatusBarMenuItem()
    : QPlatformMenuItem()
    , m_menuCode(generateNextMenuCode())
{
}

void QOhosStatusBarMenuItem::setText(const QString &text)
{
    m_text = text;
}

QString QOhosStatusBarMenuItem::text() const
{
    return m_text;
}

void QOhosStatusBarMenuItem::setIcon(const QIcon &icon)
{
    Q_UNUSED(icon);
}

void QOhosStatusBarMenuItem::setMenu(QPlatformMenu *menu)
{
    auto *ohosMenu = qobject_cast<QOhosStatusBarMenuImpl *>(menu);
    if (menu != nullptr && ohosMenu == nullptr)
        qOhosPrintfWarning("%s: got menu object of incompatible type", Q_FUNC_INFO);

    m_menu = ohosMenu;
}

void QOhosStatusBarMenuItem::setVisible(bool isVisible)
{
    Q_UNUSED(isVisible);
}

void QOhosStatusBarMenuItem::setIsSeparator(bool isSeparator)
{
    m_isSeparator = isSeparator;
}

void QOhosStatusBarMenuItem::setFont(const QFont &font)
{
    Q_UNUSED(font);
}

void QOhosStatusBarMenuItem::setRole(MenuRole role)
{
    Q_UNUSED(role);
}

void QOhosStatusBarMenuItem::setCheckable(bool checkable)
{
    Q_UNUSED(checkable);
}

void QOhosStatusBarMenuItem::setChecked(bool isChecked)
{
    Q_UNUSED(isChecked);
}

void QOhosStatusBarMenuItem::setShortcut(const QKeySequence &shortcut)
{
    Q_UNUSED(shortcut);
}

void QOhosStatusBarMenuItem::setEnabled(bool enabled)
{
    Q_UNUSED(enabled);
}

void QOhosStatusBarMenuItem::setIconSize(int size)
{
    Q_UNUSED(size);
}

std::string QOhosStatusBarMenuItem::menuCode() const
{
    return m_menuCode;
}

std::function<QOhosOptional<QNapi::Object>(QtOhos::JsState &)> QOhosStatusBarMenuItem::makeJsStatusBarMenuItemFactory() const
{
    if (m_isSeparator) {
        return [](QtOhos::JsState &) {
            return QOhosOptional<QNapi::Object>();
        };
    }

    auto title = QPlatformTheme::removeMnemonics(m_text).toStdString();

    if (m_menu == nullptr) {
        return [title, menuCode = m_menuCode](QtOhos::JsState &jsState) {
            return makeQOhosOptional(
                makeJsStatusBarMenuItemWithAction(jsState, title, menuCode));
        };
    } else {
        return [title, jsSubMenuItemsFactory = m_menu->makeJsStatusBarSubMenuItemsFactory()](QtOhos::JsState &jsState) {
            return makeQOhosOptional(
                makeJsStatusBarMenuItemWithSubMenu(jsState, title, jsSubMenuItemsFactory(jsState)));
        };
    }
}

std::function<QOhosOptional<QNapi::Object>(QtOhos::JsState &)> QOhosStatusBarMenuItem::makeJsStatusBarSubMenuItemFactory() const
{
    if (m_isSeparator) {
        qOhosPrintfWarning(
            "%s: separator item %p used in sub-menu, which is unsupported on OHOS, ignoring",
            Q_FUNC_INFO, this);
        return [](QtOhos::JsState &) {
            return QOhosOptional<QNapi::Object>();
        };
    }

    if (m_menu != nullptr) {
        qOhosPrintfWarning(
            "%s: nested-menu item %p used in sub-menu, which is unsupported on OHOS, ignoring",
            Q_FUNC_INFO, this);
        return [](QtOhos::JsState &) {
            return QOhosOptional<QNapi::Object>();
        };
    }

    auto subTitle = QPlatformTheme::removeMnemonics(m_text).toStdString();

    return [subTitle, menuCode = m_menuCode](QtOhos::JsState &jsState) {
        return makeQOhosOptional(
            makeJsStatusBarSubMenuItem(jsState, subTitle, menuCode));
    };
}

std::string QOhosStatusBarMenuItem::generateNextMenuCode()
{
    static std::atomic<std::uint64_t> menuCodeCounter(0);

    auto counterValue = ++menuCodeCounter;
    return std::to_string(counterValue);
}

QOhosStatusBarMenuImpl::QOhosStatusBarMenuImpl()
    : QOhosStatusBarMenu()
{
    auto selfRef = QtOhos::makeQThreadSafeRef(this);
    m_jsScopeData = QtOhos::evalInJsThread(
        [&](QtOhos::JsState &jsState) {
            return QtOhos::makeProxyWithJsThreadDeleter(
                QtOhos::moveToSharedPtr(
                    JsScopeData {
                        .m_rightMenuClickListenerHandle = registerOhosRightMenuClickListener(
                            jsState,
                            [selfRef](std::string menuCode) {
                                selfRef.visitInQtThreadIfAlive(
                                    [menuCode](auto &self) {
                                        self.handleRightClickEvent(menuCode);
                                    });
                            }),
                    }));
        },
        Q_FUNC_INFO);
}

void QOhosStatusBarMenuImpl::insertMenuItem(QPlatformMenuItem *menuItem, QPlatformMenuItem *before)
{
    if (menuItem == nullptr)
        return;

    if (before == nullptr) {
        m_menuItems.append(menuItem);
        return;
    }

    const int idx = m_menuItems.indexOf(before);
    if (idx >= 0)
        m_menuItems.insert(idx, menuItem);
    else
        m_menuItems.append(menuItem);
}

void QOhosStatusBarMenuImpl::removeMenuItem(QPlatformMenuItem *menuItem)
{
    if (menuItem == nullptr)
        return;
    m_menuItems.removeAll(menuItem);
}

void QOhosStatusBarMenuImpl::syncMenuItem(QPlatformMenuItem *menuItem)
{
    Q_UNUSED(menuItem);
}

void QOhosStatusBarMenuImpl::syncSeparatorsCollapsible(bool enable)
{
    Q_UNUSED(enable);
}

void QOhosStatusBarMenuImpl::setText(const QString &text)
{
    m_text = text;
}

void QOhosStatusBarMenuImpl::setIcon(const QIcon &icon)
{
    Q_UNUSED(icon);
}

void QOhosStatusBarMenuImpl::setEnabled(bool enabled)
{
    Q_UNUSED(enabled);
}

void QOhosStatusBarMenuImpl::setVisible(bool visible)
{
    Q_UNUSED(visible);
}

void QOhosStatusBarMenuImpl::showPopup(const QWindow *parentWindow, const QRect &targetRect, const QPlatformMenuItem *item)
{
    Q_UNUSED(parentWindow);
    Q_UNUSED(targetRect);
    Q_UNUSED(item);
}

void QOhosStatusBarMenuImpl::dismiss()
{
}

QPlatformMenuItem *QOhosStatusBarMenuImpl::menuItemAt(int position) const
{
    if (position < 0 || position >= m_menuItems.size())
        return nullptr;
    return m_menuItems.at(position);
}

QPlatformMenuItem *QOhosStatusBarMenuImpl::menuItemForTag(quintptr tag) const
{
    for (auto *item : m_menuItems) {
        if (item && item->tag() == tag)
            return item;
    }
    return nullptr;
}

QPlatformMenuItem *QOhosStatusBarMenuImpl::createMenuItem() const
{
    return new QOhosStatusBarMenuItem();
}

QPlatformMenu *QOhosStatusBarMenuImpl::createSubMenu() const
{
    return new QOhosStatusBarMenuImpl();
}

std::function<QNapi::Array(QtOhos::JsState &)> QOhosStatusBarMenuImpl::makeJsStatusBarGroupMenusFactory() const
{
    std::vector<std::function<QOhosOptional<QNapi::Object>(QtOhos::JsState &)>> jsMenuItemsFactories;
    for (auto *item : m_menuItems) {
        auto *ohosItem = qobject_cast<QOhosStatusBarMenuItem *>(item);
        jsMenuItemsFactories.push_back(
            ohosItem != nullptr
                ? ohosItem->makeJsStatusBarMenuItemFactory()
                : [](QtOhos::JsState &) {
                    return QOhosOptional<QNapi::Object>();
                });
    }

    return [jsMenuItemsFactories = std::move(jsMenuItemsFactories)](QtOhos::JsState &jsState) {
        std::vector<QNapi::ValueWrapper> jsGroupMenusArray;

        std::vector<QNapi::ValueWrapper> currentJsMenuItemsArray;

        for (const auto &jsMenuItemFactory : jsMenuItemsFactories) {
            QOhosOptional<QNapi::Object> optJsMenuItem = jsMenuItemFactory(jsState);
            if (optJsMenuItem.has_value()) {
                currentJsMenuItemsArray.push_back(optJsMenuItem.value());
            } else {
                jsGroupMenusArray.push_back(QNapi::makeArray(jsState.env(), currentJsMenuItemsArray));
                currentJsMenuItemsArray.clear();
            }
        }

        jsGroupMenusArray.push_back(QNapi::makeArray(jsState.env(), currentJsMenuItemsArray));
        currentJsMenuItemsArray.clear();

        return QNapi::makeArray(jsState.env(), jsGroupMenusArray);
    };
}

std::function<QNapi::Array(QtOhos::JsState &)> QOhosStatusBarMenuImpl::makeJsStatusBarSubMenuItemsFactory() const
{
    std::vector<std::function<QOhosOptional<QNapi::Object>(QtOhos::JsState &)>> jsSubMenuItemsFactories;
    for (auto *item : m_menuItems) {
        auto *ohosItem = qobject_cast<QOhosStatusBarMenuItem *>(item);
        if (ohosItem != nullptr)
            jsSubMenuItemsFactories.push_back(ohosItem->makeJsStatusBarSubMenuItemFactory());
    }

    return [jsSubMenuItemsFactories = std::move(jsSubMenuItemsFactories)](QtOhos::JsState &jsState) {
        std::vector<QNapi::ValueWrapper> jsSubMenuItems;
        for (const auto &jsSubMenuItemsFactory : jsSubMenuItemsFactories) {
            QOhosOptional<QNapi::Object> optJsSubMenuItem = jsSubMenuItemsFactory(jsState);
            if (optJsSubMenuItem.has_value())
                jsSubMenuItems.push_back(optJsSubMenuItem.value());
        }
        return QNapi::makeArray(jsState.env(), jsSubMenuItems);
    };
}

QPlatformMenuItem *QOhosStatusBarMenuImpl::findItemByMenuCode(const std::string &menuCode) const
{
    for (auto *item : m_menuItems) {
        auto *ohosItem = qobject_cast<QOhosStatusBarMenuItem *>(item);
        if (ohosItem != nullptr && ohosItem->menuCode() == menuCode)
            return item;
    }
    return nullptr;
}

void QOhosStatusBarMenuImpl::handleRightClickEvent(const std::string &menuCode)
{
    QPlatformMenuItem *item = findItemByMenuCode(menuCode);
    if (item == nullptr) {
        qOhosPrintfWarning("%s: Menu item with code '%s' not found", Q_FUNC_INFO, menuCode.c_str());
        return;
    }

    QOhosStatusBarMenuItem *ohosItem = qobject_cast<QOhosStatusBarMenuItem *>(item);
    if (ohosItem != nullptr)
        Q_EMIT ohosItem->activated();
}

}

QOhosStatusBarMenu::QOhosStatusBarMenu() = default;

std::unique_ptr<QOhosStatusBarMenu> makeQOhosStatusBarMenu()
{
    return std::make_unique<QOhosStatusBarMenuImpl>();
}

QT_END_NAMESPACE

#include "qohosstatusbarmenu.moc"
