#ifndef QPEPPERTHEME_H
#define QPEPPERTHEME_H

#include <QtCore/QHash>
#include <qpa/qplatformtheme.h>

QT_BEGIN_NAMESPACE

class QPepperTheme : public QPlatformTheme
{
public:
    QPepperTheme();
    ~QPepperTheme();

    QVariant themeHint(ThemeHint hint) const;
};

QT_END_NAMESPACE

#endif
