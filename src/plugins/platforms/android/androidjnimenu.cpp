// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "androidjnimain.h"
#include "androidjnimenu.h"
#include "qandroidplatformmenu.h"
#include "qandroidplatformmenubar.h"
#include "qandroidplatformmenuitem.h"

#include <QMutex>
#include <QPoint>
#include <QQueue>
#include <QRect>
#include <QSet>
#include <QWindow>
#include <QtCore/private/qjnihelpers_p.h>
#include <QtCore/QJniObject>

QT_BEGIN_NAMESPACE

using namespace QtAndroid;

Q_DECLARE_JNI_CLASS(QtMenuInterface, "org/qtproject/qt/android/QtMenuInterface");

namespace QtAndroidMenu
{
    static QList<QAndroidPlatformMenu *> pendingContextMenus;
    static QAndroidPlatformMenu *visibleMenu = nullptr;
    Q_CONSTINIT static QRecursiveMutex visibleMenuMutex;

    static QSet<QAndroidPlatformMenuBar *> menuBars;
    static QAndroidPlatformMenuBar *visibleMenuBar = nullptr;
    static QWindow *activeTopLevelWindow = nullptr;
    Q_CONSTINIT static QRecursiveMutex menuBarMutex;

    static jmethodID clearMenuMethodID = 0;
    static jmethodID addMenuItemMethodID = 0;
    static int menuNoneValue = 0;
    static jmethodID setHeaderTitleContextMenuMethodID = 0;

    static jmethodID setCheckableMenuItemMethodID = 0;
    static jmethodID setCheckedMenuItemMethodID = 0;
    static jmethodID setEnabledMenuItemMethodID = 0;
    static jmethodID setIconMenuItemMethodID = 0;
    static jmethodID setVisibleMenuItemMethodID = 0;

    void resetMenuBar()
    {
        AndroidBackendRegister *reg = QtAndroid::backendRegister();
        reg->callInterface<QtJniTypes::QtMenuInterface, void>("resetOptionsMenu");
    }

    void openOptionsMenu()
    {
        AndroidBackendRegister *reg = QtAndroid::backendRegister();
        reg->callInterface<QtJniTypes::QtMenuInterface, void>("openOptionsMenu");
    }

    void showContextMenu(QAndroidPlatformMenu *menu, const QRect &anchorRect)
    {
        QMutexLocker lock(&visibleMenuMutex);
        if (visibleMenu)
            pendingContextMenus.append(visibleMenu);
        visibleMenu = menu;
        menu->aboutToShow();
        AndroidBackendRegister *reg = QtAndroid::backendRegister();
        reg->callInterface<QtJniTypes::QtMenuInterface, void>("openContextMenu", anchorRect.x(),
                                                              anchorRect.y(), anchorRect.width(),
                                                              anchorRect.height());
    }

    void hideContextMenu(QAndroidPlatformMenu *menu)
    {
        QMutexLocker lock(&visibleMenuMutex);
        if (visibleMenu == menu) {
            AndroidBackendRegister *reg = QtAndroid::backendRegister();
            reg->callInterface<QtJniTypes::QtMenuInterface, void>("closeContextMenu");
            pendingContextMenus.clear();
        } else {
            pendingContextMenus.removeOne(menu);
        }
    }

    // FIXME
    void syncMenu(QAndroidPlatformMenu */*menu*/)
    {
//        QMutexLocker lock(&visibleMenuMutex);
//        if (visibleMenu == menu)
//        {
//            hideContextMenu(menu);
//            showContextMenu(menu);
//        }
    }

    void androidPlatformMenuDestroyed(QAndroidPlatformMenu *menu)
    {
        QMutexLocker lock(&visibleMenuMutex);
        if (visibleMenu == menu)
            visibleMenu = 0;
    }

    void setMenuBar(QAndroidPlatformMenuBar *menuBar, QWindow *window)
    {
        if (activeTopLevelWindow == window && visibleMenuBar != menuBar) {
            visibleMenuBar = menuBar;
            resetMenuBar();
        }
    }

    void setActiveTopLevelWindow(QWindow *window)
    {
        Qt::WindowFlags flags = window ? window->flags() : Qt::WindowFlags();
        if (!window)
            return;

        bool isNonRegularWindow = flags & (Qt::Desktop | Qt::Popup | Qt::Dialog | Qt::Sheet) & ~Qt::Window;
        if (isNonRegularWindow)
            return;

        QMutexLocker lock(&menuBarMutex);
        if (activeTopLevelWindow == window)
            return;

        visibleMenuBar = 0;
        activeTopLevelWindow = window;
        for (QAndroidPlatformMenuBar *menuBar : std::as_const(menuBars)) {
            if (menuBar->parentWindow() == window) {
                visibleMenuBar = menuBar;
                resetMenuBar();
                break;
            }
        }

    }

    void addMenuBar(QAndroidPlatformMenuBar *menuBar)
    {
        QMutexLocker lock(&menuBarMutex);
        menuBars.insert(menuBar);
    }

    void removeMenuBar(QAndroidPlatformMenuBar *menuBar)
    {
        QMutexLocker lock(&menuBarMutex);
        menuBars.remove(menuBar);
        if (visibleMenuBar == menuBar) {
            visibleMenuBar = 0;
            resetMenuBar();
        }
    }

    static QString removeAmpersandEscapes(QString s)
    {
        qsizetype i = 0;
        while (i < s.size()) {
            ++i;
            if (s.at(i - 1) != u'&')
                continue;
            if (i < s.size() && s.at(i) == u'&')
                ++i;
            s.remove(i-1,1);
        }
        return s.trimmed();
    }

    static void fillMenuItem(JNIEnv *env, jobject menuItem, bool checkable, bool checked, bool enabled, bool visible, const QIcon &icon=QIcon())
    {
        env->DeleteLocalRef(env->CallObjectMethod(menuItem, setCheckableMenuItemMethodID, checkable));
        env->DeleteLocalRef(env->CallObjectMethod(menuItem, setCheckedMenuItemMethodID, checked));
        env->DeleteLocalRef(env->CallObjectMethod(menuItem, setEnabledMenuItemMethodID, enabled));

        if (!icon.isNull()) { // isNull() only checks the d pointer, not the actual image data.
            int sz = qMax(36, qEnvironmentVariableIntValue("QT_ANDROID_APP_ICON_SIZE"));
            QImage img = icon.pixmap(QSize(sz,sz),
                                     enabled
                                        ? QIcon::Normal
                                        : QIcon::Disabled,
                                     QIcon::On).toImage();
            if (!img.isNull()) { // Make sure we have a valid image.
                env->DeleteLocalRef(env->CallObjectMethod(menuItem,
                                                          setIconMenuItemMethodID,
                                                          createBitmapDrawable(createBitmap(img, env), env)));
            }
        }

        env->DeleteLocalRef(env->CallObjectMethod(menuItem, setVisibleMenuItemMethodID, visible));
    }

    static int addAllMenuItemsToMenu(JNIEnv *env, jobject menu, QAndroidPlatformMenu *platformMenu) {
         int order = 0;
         QMutexLocker lock(platformMenu->menuItemsMutex());
         const auto items = platformMenu->menuItems();
         for (QAndroidPlatformMenuItem *item : items) {
             if (item->isSeparator())
                 continue;
             QString itemText = removeAmpersandEscapes(item->text());
             jstring jtext = env->NewString(reinterpret_cast<const jchar *>(itemText.data()),
                                           itemText.length());
             jint menuId = platformMenu->menuId(item);
             jobject menuItem = env->CallObjectMethod(menu,
                                                      addMenuItemMethodID,
                                                      menuNoneValue,
                                                      menuId,
                                                      order++,
                                                      jtext);
             env->DeleteLocalRef(jtext);
             fillMenuItem(env,
                          menuItem,
                          item->isCheckable(),
                          item->isChecked(),
                          item->isEnabled(),
                          item->isVisible(),
                          item->icon());
             env->DeleteLocalRef(menuItem);
         }

         return order;
    }

    static jboolean onPrepareOptionsMenu(JNIEnv *env, jobject thiz, jobject menu)
    {
        Q_UNUSED(thiz)

        env->CallVoidMethod(menu, clearMenuMethodID);
        QMutexLocker lock(&menuBarMutex);
        if (!visibleMenuBar)
            return JNI_FALSE;

        const QAndroidPlatformMenuBar::PlatformMenusType &menus = visibleMenuBar->menus();
        int order = 0;
        QMutexLocker lockMenuBarMutex(visibleMenuBar->menusListMutex());
        if (menus.size() == 1) { // Expand the menu
            order = addAllMenuItemsToMenu(env, menu, static_cast<QAndroidPlatformMenu *>(menus.front()));
        } else {
            for (QAndroidPlatformMenu *item : menus) {
                QString itemText = removeAmpersandEscapes(item->text());
                jstring jtext = env->NewString(reinterpret_cast<const jchar *>(itemText.data()),
                                               itemText.length());
                jint menuId = visibleMenuBar->menuId(item);
                jobject menuItem = env->CallObjectMethod(menu,
                                                         addMenuItemMethodID,
                                                         menuNoneValue,
                                                         menuId,
                                                         order++,
                                                         jtext);
                env->DeleteLocalRef(jtext);

                fillMenuItem(env,
                             menuItem,
                             false,
                             false,
                             item->isEnabled(),
                             item->isVisible(),
                             item->icon());
            }
        }
        return order ? JNI_TRUE : JNI_FALSE;
    }

    static jboolean onOptionsItemSelected(JNIEnv *env, jobject thiz, jint menuId, jboolean checked)
    {
        Q_UNUSED(env)
        Q_UNUSED(thiz)

        QMutexLocker lock(&menuBarMutex);
        if (!visibleMenuBar)
            return JNI_FALSE;

        const QAndroidPlatformMenuBar::PlatformMenusType &menus = visibleMenuBar->menus();
        if (menus.size() == 1) { // Expanded menu
            QAndroidPlatformMenuItem *item = static_cast<QAndroidPlatformMenuItem *>(menus.front()->menuItemForId(menuId));
            if (item) {
                if (item->menu()) {
                    showContextMenu(item->menu(), QRect());
                } else {
                    if (item->isCheckable())
                        item->setChecked(checked);
                    item->activated();
                }
            }
        } else {
            QAndroidPlatformMenu *menu = static_cast<QAndroidPlatformMenu *>(visibleMenuBar->menuForId(menuId));
            if (menu)
                showContextMenu(menu, QRect());
        }

        return JNI_TRUE;
    }

    static void onOptionsMenuClosed(JNIEnv *env, jobject thiz, jobject menu)
    {
        Q_UNUSED(env)
        Q_UNUSED(thiz)
        Q_UNUSED(menu)
    }

    static void onCreateContextMenu(JNIEnv *env, jobject thiz, jobject menu)
    {
        Q_UNUSED(thiz)

        env->CallVoidMethod(menu, clearMenuMethodID);
        QMutexLocker lock(&visibleMenuMutex);
        if (!visibleMenu)
            return;

        QString menuText = removeAmpersandEscapes(visibleMenu->text());
        jstring jtext = env->NewString(reinterpret_cast<const jchar*>(menuText.data()),
                                       menuText.length());
        env->CallObjectMethod(menu, setHeaderTitleContextMenuMethodID, jtext);
        env->DeleteLocalRef(jtext);
        addAllMenuItemsToMenu(env, menu, visibleMenu);
    }

    static void fillContextMenu(JNIEnv *env, jobject thiz, jobject menu)
    {
        Q_UNUSED(thiz)
        env->CallVoidMethod(menu, clearMenuMethodID);
        QMutexLocker lock(&visibleMenuMutex);
        if (!visibleMenu)
            return;

        addAllMenuItemsToMenu(env, menu, visibleMenu);
    }

    static jboolean onContextItemSelected(JNIEnv *env, jobject thiz, jint menuId, jboolean checked)
    {
        Q_UNUSED(env)
        Q_UNUSED(thiz)

        QMutexLocker lock(&visibleMenuMutex);
        QAndroidPlatformMenuItem * item = static_cast<QAndroidPlatformMenuItem *>(visibleMenu->menuItemForId(menuId));
        if (item) {
            if (item->menu()) {
                showContextMenu(item->menu(), QRect());
            } else {
                if (item->isCheckable())
                    item->setChecked(checked);
                item->activated();
                visibleMenu->aboutToHide();
                visibleMenu = 0;
                for (QAndroidPlatformMenu *menu : std::as_const(pendingContextMenus)) {
                    if (menu->isVisible())
                        menu->aboutToHide();
                }
                pendingContextMenus.clear();
            }
        }
        return JNI_TRUE;
    }

    static void onContextMenuClosed(JNIEnv *env, jobject thiz, jobject menu)
    {
        Q_UNUSED(env)
        Q_UNUSED(thiz)
        Q_UNUSED(menu)

        QMutexLocker lock(&visibleMenuMutex);
        if (!visibleMenu)
            return;

        visibleMenu->aboutToHide();
        visibleMenu = 0;
        if (!pendingContextMenus.empty())
            showContextMenu(pendingContextMenus.takeLast(), QRect());
    }

    static JNINativeMethod methods[] = {
        {"onPrepareOptionsMenu", "(Landroid/view/Menu;)Z", (void *)onPrepareOptionsMenu},
        {"onOptionsItemSelected", "(IZ)Z", (void *)onOptionsItemSelected},
        {"onOptionsMenuClosed", "(Landroid/view/Menu;)V", (void*)onOptionsMenuClosed},
        {"onCreateContextMenu", "(Landroid/view/ContextMenu;)V", (void *)onCreateContextMenu},
        {"fillContextMenu", "(Landroid/view/Menu;)V", (void *)fillContextMenu},
        {"onContextItemSelected", "(IZ)Z", (void *)onContextItemSelected},
        {"onContextMenuClosed", "(Landroid/view/Menu;)V", (void*)onContextMenuClosed},
    };

#define FIND_AND_CHECK_CLASS(CLASS_NAME) \
    clazz = env->FindClass(CLASS_NAME); \
    if (!clazz) { \
        __android_log_print(ANDROID_LOG_FATAL, qtTagText(), classErrorMsgFmt(), CLASS_NAME); \
        return false; \
    }

#define GET_AND_CHECK_METHOD(VAR, CLASS, METHOD_NAME, METHOD_SIGNATURE) \
    VAR = env->GetMethodID(CLASS, METHOD_NAME, METHOD_SIGNATURE); \
    if (!VAR) { \
        __android_log_print(ANDROID_LOG_FATAL, qtTagText(), methodErrorMsgFmt(), METHOD_NAME, METHOD_SIGNATURE); \
        return false; \
    }

#define GET_AND_CHECK_STATIC_METHOD(VAR, CLASS, METHOD_NAME, METHOD_SIGNATURE) \
    VAR = env->GetStaticMethodID(CLASS, METHOD_NAME, METHOD_SIGNATURE); \
    if (!VAR) { \
        __android_log_print(ANDROID_LOG_FATAL, qtTagText(), methodErrorMsgFmt(), METHOD_NAME, METHOD_SIGNATURE); \
        return false; \
    }

#define GET_AND_CHECK_STATIC_FIELD(VAR, CLASS, FIELD_NAME, FIELD_SIGNATURE) \
    VAR = env->GetStaticFieldID(CLASS, FIELD_NAME, FIELD_SIGNATURE); \
    if (!VAR) { \
        __android_log_print(ANDROID_LOG_FATAL, qtTagText(), methodErrorMsgFmt(), FIELD_NAME, FIELD_SIGNATURE); \
        return false; \
    }

    bool registerNatives(QJniEnvironment &env)
    {
        jclass appClass = applicationClass();

        if (!env.registerNativeMethods(appClass, methods,  sizeof(methods) / sizeof(methods[0]))) {
            __android_log_print(ANDROID_LOG_FATAL,"Qt", "RegisterNatives failed");
            return false;
        }

        jclass clazz;
        FIND_AND_CHECK_CLASS("android/view/Menu");
        GET_AND_CHECK_METHOD(clearMenuMethodID, clazz, "clear", "()V");
        GET_AND_CHECK_METHOD(addMenuItemMethodID, clazz, "add", "(IIILjava/lang/CharSequence;)Landroid/view/MenuItem;");
        jfieldID menuNoneFiledId;
        GET_AND_CHECK_STATIC_FIELD(menuNoneFiledId, clazz, "NONE", "I");
        menuNoneValue = env->GetStaticIntField(clazz, menuNoneFiledId);

        FIND_AND_CHECK_CLASS("android/view/ContextMenu");
        GET_AND_CHECK_METHOD(setHeaderTitleContextMenuMethodID, clazz, "setHeaderTitle","(Ljava/lang/CharSequence;)Landroid/view/ContextMenu;");

        FIND_AND_CHECK_CLASS("android/view/MenuItem");
        GET_AND_CHECK_METHOD(setCheckableMenuItemMethodID, clazz, "setCheckable", "(Z)Landroid/view/MenuItem;");
        GET_AND_CHECK_METHOD(setCheckedMenuItemMethodID, clazz, "setChecked", "(Z)Landroid/view/MenuItem;");
        GET_AND_CHECK_METHOD(setEnabledMenuItemMethodID, clazz, "setEnabled", "(Z)Landroid/view/MenuItem;");
        GET_AND_CHECK_METHOD(setIconMenuItemMethodID, clazz, "setIcon", "(Landroid/graphics/drawable/Drawable;)Landroid/view/MenuItem;");
        GET_AND_CHECK_METHOD(setVisibleMenuItemMethodID, clazz, "setVisible", "(Z)Landroid/view/MenuItem;");
        return true;
    }
}

QT_END_NAMESPACE
