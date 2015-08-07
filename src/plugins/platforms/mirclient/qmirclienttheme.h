/*
 * Copyright (C) 2014 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef QMIRCLIENTTHEME_H
#define QMIRCLIENTTHEME_H

#include <QtPlatformSupport/private/qgenericunixthemes_p.h>

class QMirClientTheme : public QGenericUnixTheme
{
public:
    static const char* name;
    QMirClientTheme();
    virtual ~QMirClientTheme();

    // From QPlatformTheme
    QVariant themeHint(ThemeHint hint) const override;
};

#endif // QMIRCLIENTTHEME_H
