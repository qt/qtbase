/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the qmake application of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "property.h"
#include "option.h"

#include <qdir.h>
#include <qsettings.h>
#include <qlibraryinfo.h>
#include <qstringlist.h>
#include <stdio.h>

QT_BEGIN_NAMESPACE

static const struct {
    const char *name;
    QLibraryInfo::LibraryLocation loc;
    bool raw;
    bool singular;
} propList[] = {
    { "QT_SYSROOT", QLibraryInfo::SysrootPath, true, true },
    { "QT_INSTALL_PREFIX", QLibraryInfo::PrefixPath, false, false },
    { "QT_INSTALL_ARCHDATA", QLibraryInfo::ArchDataPath, false, false },
    { "QT_INSTALL_DATA", QLibraryInfo::DataPath, false, false },
    { "QT_INSTALL_DOCS", QLibraryInfo::DocumentationPath, false, false },
    { "QT_INSTALL_HEADERS", QLibraryInfo::HeadersPath, false, false },
    { "QT_INSTALL_LIBS", QLibraryInfo::LibrariesPath, false, false },
    { "QT_INSTALL_LIBEXECS", QLibraryInfo::LibraryExecutablesPath, false, false },
    { "QT_INSTALL_BINS", QLibraryInfo::BinariesPath, false, false },
    { "QT_INSTALL_TESTS", QLibraryInfo::TestsPath, false, false },
    { "QT_INSTALL_PLUGINS", QLibraryInfo::PluginsPath, false, false },
    { "QT_INSTALL_IMPORTS", QLibraryInfo::ImportsPath, false, false },
    { "QT_INSTALL_QML", QLibraryInfo::Qml2ImportsPath, false, false },
    { "QT_INSTALL_TRANSLATIONS", QLibraryInfo::TranslationsPath, false, false },
    { "QT_INSTALL_CONFIGURATION", QLibraryInfo::SettingsPath, false, false },
    { "QT_INSTALL_EXAMPLES", QLibraryInfo::ExamplesPath, false, false },
    { "QT_INSTALL_DEMOS", QLibraryInfo::ExamplesPath, false, false }, // Just backwards compat
    { "QT_HOST_PREFIX", QLibraryInfo::HostPrefixPath, true, false },
    { "QT_HOST_DATA", QLibraryInfo::HostDataPath, true, false },
    { "QT_HOST_BINS", QLibraryInfo::HostBinariesPath, true, false },
    { "QT_HOST_LIBS", QLibraryInfo::HostLibrariesPath, true, false },
    { "QMAKE_SPEC", QLibraryInfo::HostSpecPath, true, true },
    { "QMAKE_XSPEC", QLibraryInfo::TargetSpecPath, true, true },
};

QMakeProperty::QMakeProperty() : settings(0)
{
    reload();
}

void QMakeProperty::reload()
{
    QLibraryInfo::reload();
    for (unsigned i = 0; i < sizeof(propList)/sizeof(propList[0]); i++) {
        QString name = QString::fromLatin1(propList[i].name);
        if (!propList[i].singular) {
            m_values[ProKey(name + "/src")] = QLibraryInfo::rawLocation(propList[i].loc, QLibraryInfo::EffectiveSourcePaths);
            m_values[ProKey(name + "/get")] = QLibraryInfo::rawLocation(propList[i].loc, QLibraryInfo::EffectivePaths);
        }
        QString val = QLibraryInfo::rawLocation(propList[i].loc, QLibraryInfo::FinalPaths);
        if (!propList[i].raw) {
            m_values[ProKey(name + "/dev")] = QLibraryInfo::rawLocation(propList[i].loc, QLibraryInfo::DevicePaths);
            m_values[ProKey(name)] = QLibraryInfo::location(propList[i].loc);
            name += "/raw";
        }
        m_values[ProKey(name)] = val;
    }
    m_values["QMAKE_VERSION"] = ProString(QMAKE_VERSION_STR);
#ifdef QT_VERSION_STR
    m_values["QT_VERSION"] = ProString(QT_VERSION_STR);
#endif
}

QMakeProperty::~QMakeProperty()
{
    delete settings;
    settings = 0;
}

void QMakeProperty::initSettings()
{
    if(!settings) {
        settings = new QSettings(QSettings::UserScope, "QtProject", "QMake");
        settings->setFallbacksEnabled(false);
    }
}

ProString
QMakeProperty::value(const ProKey &vk)
{
    ProString val = m_values.value(vk);
    if (!val.isNull())
        return val;

    initSettings();
    return settings->value(vk.toQString()).toString();
}

bool
QMakeProperty::hasValue(const ProKey &v)
{
    return !value(v).isNull();
}

void
QMakeProperty::setValue(QString var, const QString &val)
{
    initSettings();
    settings->setValue(var, val);
}

void
QMakeProperty::remove(const QString &var)
{
    initSettings();
    settings->remove(var);
}

bool
QMakeProperty::exec()
{
    bool ret = true;
    if(Option::qmake_mode == Option::QMAKE_QUERY_PROPERTY) {
        if(Option::prop::properties.isEmpty()) {
            initSettings();
            const auto keys = settings->childKeys();
            for (const QString &key : keys) {
                QString val = settings->value(key).toString();
                fprintf(stdout, "%s:%s\n", qPrintable(key), qPrintable(val));
            }
            QStringList specialProps;
            for (unsigned i = 0; i < sizeof(propList)/sizeof(propList[0]); i++)
                specialProps.append(QString::fromLatin1(propList[i].name));
            specialProps.append("QMAKE_VERSION");
#ifdef QT_VERSION_STR
            specialProps.append("QT_VERSION");
#endif
            for (const QString &prop : qAsConst(specialProps)) {
                ProString val = value(ProKey(prop));
                ProString pval = value(ProKey(prop + "/raw"));
                ProString gval = value(ProKey(prop + "/get"));
                ProString sval = value(ProKey(prop + "/src"));
                ProString dval = value(ProKey(prop + "/dev"));
                fprintf(stdout, "%s:%s\n", prop.toLatin1().constData(), val.toLatin1().constData());
                if (!pval.isEmpty() && pval != val)
                    fprintf(stdout, "%s/raw:%s\n", prop.toLatin1().constData(), pval.toLatin1().constData());
                if (!gval.isEmpty() && gval != (pval.isEmpty() ? val : pval))
                    fprintf(stdout, "%s/get:%s\n", prop.toLatin1().constData(), gval.toLatin1().constData());
                if (!sval.isEmpty() && sval != gval)
                    fprintf(stdout, "%s/src:%s\n", prop.toLatin1().constData(), sval.toLatin1().constData());
                if (!dval.isEmpty() && dval != pval)
                    fprintf(stdout, "%s/dev:%s\n", prop.toLatin1().constData(), dval.toLatin1().constData());
            }
            return true;
        }
        for(QStringList::ConstIterator it = Option::prop::properties.begin();
            it != Option::prop::properties.end(); it++) {
            if(Option::prop::properties.count() > 1)
                fprintf(stdout, "%s:", (*it).toLatin1().constData());
            const ProKey pkey(*it);
            if (!hasValue(pkey)) {
                ret = false;
                fprintf(stdout, "**Unknown**\n");
            } else {
                fprintf(stdout, "%s\n", value(pkey).toLatin1().constData());
            }
        }
    } else if(Option::qmake_mode == Option::QMAKE_SET_PROPERTY) {
        for(QStringList::ConstIterator it = Option::prop::properties.begin();
            it != Option::prop::properties.end(); it++) {
            QString var = (*it);
            it++;
            if(it == Option::prop::properties.end()) {
                ret = false;
                break;
            }
            if(!var.startsWith("."))
                setValue(var, (*it));
        }
    } else if(Option::qmake_mode == Option::QMAKE_UNSET_PROPERTY) {
        for(QStringList::ConstIterator it = Option::prop::properties.begin();
            it != Option::prop::properties.end(); it++) {
            QString var = (*it);
            if(!var.startsWith("."))
                remove(var);
        }
    }
    return ret;
}

QT_END_NAMESPACE
