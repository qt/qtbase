#include "qplatformmenu_qpa.h"

//
// QPlatformMenuAction
//

QPlatformMenuAction::~QPlatformMenuAction()
{

}

//
// QPlatformMenu
//
QPlatformMenu::QPlatformMenu()
{
}

QPlatformMenu::~QPlatformMenu()
{

}

void QPlatformMenu::setMenuEnabled(bool enable)
{
    Q_UNUSED(enable);
}

void QPlatformMenu::syncSeparatorsCollapsible(bool enable)
{
    Q_UNUSED(enable);
}

//
// QPlatformMenuBar
//
QPlatformMenuBar::QPlatformMenuBar()
{

}

QPlatformMenuBar::~QPlatformMenuBar()
{

}

void QPlatformMenuBar::handleReparent(QWidget *newParent)
{
    Q_UNUSED(newParent);
}

