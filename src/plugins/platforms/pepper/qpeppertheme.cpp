#include "qpeppertheme.h"

#include <QtCore/QStringList>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE


QPepperTheme::QPepperTheme()
{

}

QPepperTheme::~QPepperTheme()
{

}
QVariant QPepperTheme::themeHint(ThemeHint hint) const
{
    switch (hint) {
    case QPlatformTheme::StyleNames:
        return QStringList(QStringLiteral("cleanlooks"));
    default:
        break;
    }
    return QPlatformTheme::themeHint(hint);
}


QT_END_NAMESPACE
