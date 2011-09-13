
#include <private/qt_mac_p.h>
#include <QtCore/qpointer.h>
#include <QtWidgets/qmenu.h>
#include <QtWidgets/qmenubar.h>
#include <QtWidgets/qplatformmenu_qpa.h>

@class NSMenuItem;
class QCocoaMenuAction : public QPlatformMenuAction
{
public:
    QCocoaMenuAction();
    ~QCocoaMenuAction();

    NSMenuItem *menuItem;
    uchar ignore_accel : 1;
    uchar merged : 1;
    OSMenuRef menu;
    QPointer<QMenu> qtMenu;
};

struct QMenuMergeItem
{
    inline QMenuMergeItem(NSMenuItem *c, QCocoaMenuAction *a) : menuItem(c), action(a) { }
    NSMenuItem *menuItem;
    QCocoaMenuAction *action;
};
typedef QList<QMenuMergeItem> QMenuMergeList;

class QCocoaMenu : public QPlatformMenu
{
public:
    QCocoaMenu(QMenu *qtMenu);
    ~QCocoaMenu();

    OSMenuRef macMenu(OSMenuRef merge = 0);
    void syncSeparatorsCollapsible(bool collapse);
    void setMenuEnabled(bool enable);

    void addAction(QAction *action, QAction *before);
    void syncAction(QAction *action);
    void removeAction(QAction *action);

    void addAction(QCocoaMenuAction *action, QCocoaMenuAction *before);
    void syncAction(QCocoaMenuAction *action);
    void removeAction(QCocoaMenuAction *action);
    bool merged(const QAction *action) const;
    QCocoaMenuAction *findAction(QAction *action) const;

    OSMenuRef menu;
    static QHash<OSMenuRef, OSMenuRef> mergeMenuHash;
    static QHash<OSMenuRef, QMenuMergeList*> mergeMenuItemsHash;
    QList<QCocoaMenuAction*> actionItems;
    QMenu *qtMenu;
};

class QCocoaMenuBar : public QPlatformMenuBar
{
public:
    QCocoaMenuBar(QMenuBar *qtMenuBar);
    ~QCocoaMenuBar();

    void handleReparent(QWidget *newParent);

    void addAction(QAction *action, QAction *before);
    void syncAction(QAction *action);
    void removeAction(QAction *action);

    void addAction(QCocoaMenuAction *action, QCocoaMenuAction *before);
    void syncAction(QCocoaMenuAction *action);
    void removeAction(QCocoaMenuAction *action);

    bool macWidgetHasNativeMenubar(QWidget *widget);
    void macCreateMenuBar(QWidget *parent);
    void macDestroyMenuBar();
    OSMenuRef macMenu();
    static bool macUpdateMenuBarImmediatly();
    static void macUpdateMenuBar();
    QCocoaMenuAction *findAction(QAction *action) const;

    OSMenuRef menu;
    OSMenuRef apple_menu;
    QList<QCocoaMenuAction*> actionItems;
    QMenuBar *qtMenuBar;
};
