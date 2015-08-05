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

#include "plugin.h"
#include "integration.h"

QStringList UbuntuMirClientIntegrationPlugin::keys() const
{
    QStringList list;
    list << "ubuntumirclient";
    return list;
}

QPlatformIntegration* UbuntuMirClientIntegrationPlugin::create(const QString &system,
                                                               const QStringList &)
{
    if (system.toLower() == "ubuntumirclient") {
#ifdef PLATFORM_API_TOUCH
        setenv("UBUNTU_PLATFORM_API_BACKEND", "touch_mirclient", 1);
#else
        setenv("UBUNTU_PLATFORM_API_BACKEND", "desktop_mirclient", 1);
#endif
        return new UbuntuClientIntegration;
    } else {
        return 0;
    }
}
