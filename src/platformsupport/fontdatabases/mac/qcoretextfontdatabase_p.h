/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCORETEXTFONTDATABASE_H
#define QCORETEXTFONTDATABASE_H

#include <qpa/qplatformfontdatabase.h>
#include <private/qcore_mac_p.h>

#ifndef Q_OS_IOS
#include <ApplicationServices/ApplicationServices.h>
#else
#include <CoreText/CoreText.h>
#include <CoreGraphics/CoreGraphics.h>
#endif

QT_BEGIN_NAMESPACE

class QCoreTextFontDatabase : public QPlatformFontDatabase
{
public:
    QCoreTextFontDatabase();
    ~QCoreTextFontDatabase();
    void populateFontDatabase();
    QFontEngine *fontEngine(const QFontDef &fontDef, QChar::Script script, void *handle);
    QFontEngine *fontEngine(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference);
    QStringList fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script) const;
#ifdef Q_OS_MACX
    QStringList addApplicationFont(const QByteArray &fontData, const QString &fileName);
#endif
    void releaseHandle(void *handle);
    QFont defaultFont() const;
    QList<int> standardSizes() const;

private:
    void populateFromDescriptor(CTFontDescriptorRef font);

    mutable QString defaultFontName;
    mutable QHash<QString, QString> psNameToFamily;
    mutable QHash<QString, QString> familyNameToPsName;

    void removeApplicationFonts();
#ifdef Q_OS_MACX
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_8
    QVector<QCFType<CGFontRef> > m_applicationGraphicsFonts;
    QVector<QCFType<CFURLRef> > m_applicationURLFonts;
#endif
    QVector<ATSFontContainerRef> m_applicationFonts;
#endif
};

QT_END_NAMESPACE

#endif // QCORETEXTFONTDATABASE_H
