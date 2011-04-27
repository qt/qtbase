/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QDebug>
#include <QPainter>
#include <QPixmapCache>
#include <QSvgRenderer>

#include "theme.h"

Q_DECLARE_METATYPE(Theme::Themes*)

Theme::Theme(QObject *parent)
  : QObject(parent)
  , m_currentTheme()
  , m_availableThemes()
  , m_fonts()
  , m_pixmapPath()
  , m_listItemBackgroundBrushEven()
  , m_listItemBackgroundOpacityEven()
  , m_listItemBackgroundBrushOdd()
  , m_listItemBackgroundOpacityOdd()
  , m_listItemBorderPen(QPen())
  , m_listItemRounding()
#if (QT_VERSION >= 0x040600)
  , m_iconOpacityEffectEnabled()
#endif
  , m_iconRotation()
  , m_iconSmoothTransformation()
{
    m_availableThemes << "Blue" << "Lime";

    // Set blue theme as a default theme without emiting themeChanged() signal
    setBlueTheme();
}

Theme::~Theme()
{
}

Theme* Theme::p()
{
    static Theme t;
    return &t;
}

void Theme::setTheme(const QString theme)
{
    if (theme.compare("blue", Qt::CaseInsensitive) == 0)
    {
        setTheme(Theme::Blue);
    }
    else if (theme.compare("lime", Qt::CaseInsensitive) == 0)
    {
        setTheme(Theme::Lime);
    }
    else
    {
        qDebug() << "Unknown theme";
    }
}

void Theme::setTheme(const Themes theme)
{
    if (m_currentTheme == theme)
        return;

    switch (theme)
    {
        case Theme::Blue:
            setBlueTheme();
            emit themeChanged();
            break;

        case Theme::Lime:
            setLimeTheme();
            emit themeChanged();
            break;
    }
}

void Theme::setBlueTheme()
{
    m_currentTheme = Theme::Blue;

    m_fonts[ContactName].setFamily("Arial");
    m_fonts[ContactName].setPixelSize(16);
    m_fonts[ContactName].setStyle(QFont::StyleNormal);
    m_fonts[ContactName].setWeight(QFont::Normal);

    m_fonts[ContactNumber].setFamily("Arial");
    m_fonts[ContactNumber].setPixelSize(14);
    m_fonts[ContactNumber].setStyle(QFont::StyleNormal);

    m_fonts[ContactEmail].setFamily("Arial");
    m_fonts[ContactEmail].setPixelSize(14);
    m_fonts[ContactEmail].setStyle(QFont::StyleNormal);

    m_fonts[TitleBar].setFamily("Arial");
    m_fonts[TitleBar].setPixelSize(36);
    m_fonts[TitleBar].setStyle(QFont::StyleNormal);

    m_fonts[StatusBar].setFamily("Arial");
    m_fonts[StatusBar].setPixelSize(16);
    m_fonts[StatusBar].setStyle(QFont::StyleNormal);

    m_fonts[MenuItem].setFamily("Arial");
    m_fonts[MenuItem].setPixelSize(14);
    m_fonts[MenuItem].setStyle(QFont::StyleNormal);

    m_pixmapPath = ":/themes/blue/";

    m_listItemBackgroundBrushEven = QBrush(Qt::NoBrush);
    m_listItemBackgroundOpacityEven = 1.0;
    m_listItemBackgroundBrushOdd = QBrush(QColor(44,214,250), Qt::SolidPattern);
    m_listItemBackgroundOpacityOdd = 1.0;

    m_listItemBorderPen = QPen(Qt::NoPen);
    m_listItemRounding = QSize(0.0, 0.0);

#if (QT_VERSION >= 0x040600)
    m_iconOpacityEffectEnabled[ListItem::LeftIcon] = false;
    m_iconOpacityEffectEnabled[ListItem::RightIcon] = false;
#endif
    m_iconRotation[ListItem::LeftIcon] =  0.0;
    m_iconRotation[ListItem::RightIcon] = 0.0;

    m_iconSmoothTransformation[ListItem::LeftIcon] = false;
    m_iconSmoothTransformation[ListItem::RightIcon] = false;
}

void Theme::setLimeTheme()
{
    m_currentTheme = Theme::Lime;

    m_fonts[ContactName].setFamily("Arial");
    m_fonts[ContactName].setPixelSize(16);
    m_fonts[ContactName].setStyle(QFont::StyleItalic);
    m_fonts[ContactName].setWeight(QFont::Bold);

    m_fonts[ContactNumber].setFamily("Arial");
    m_fonts[ContactNumber].setPixelSize(14);
    m_fonts[ContactNumber].setStyle(QFont::StyleItalic);

    m_fonts[ContactEmail].setFamily("Arial");
    m_fonts[ContactEmail].setPixelSize(14);
    m_fonts[ContactEmail].setStyle(QFont::StyleItalic);

    m_fonts[TitleBar].setFamily("Arial");
    m_fonts[TitleBar].setPixelSize(36);
    m_fonts[TitleBar].setStyle(QFont::StyleItalic);

    m_fonts[StatusBar].setFamily("Arial");
    m_fonts[StatusBar].setPixelSize(16);
    m_fonts[StatusBar].setStyle(QFont::StyleItalic);

    m_fonts[MenuItem].setFamily("Arial");
    m_fonts[MenuItem].setPixelSize(14);
    m_fonts[MenuItem].setStyle(QFont::StyleItalic);

    m_pixmapPath = ":/themes/lime/";

    m_listItemBackgroundBrushEven = QBrush(QPixmap(":/avatars/avatar_014.png"));
    m_listItemBackgroundOpacityEven = 0.05;

    m_listItemBackgroundBrushOdd = QBrush(QPixmap(":/avatars/avatar_012.png"));
    m_listItemBackgroundOpacityOdd = 0.15;

    m_listItemBorderPen = QPen(QColor(0,0,0,55), 3, Qt::SolidLine);
    m_listItemRounding = QSize(12.0, 12.0);

#if (QT_VERSION >= 0x040600)
    m_iconOpacityEffectEnabled[ListItem::LeftIcon] = true;
    m_iconOpacityEffectEnabled[ListItem::RightIcon] = false;
#endif
    m_iconRotation[ListItem::LeftIcon] = -4.0;
    m_iconRotation[ListItem::RightIcon] = 0.0;

    m_iconSmoothTransformation[ListItem::LeftIcon] = true;
    m_iconSmoothTransformation[ListItem::RightIcon] = false;
}

QPixmap Theme::pixmap(const QString filename, QSize size)
{
    if (filename.endsWith(".svg", Qt::CaseInsensitive))
    {
        QSvgRenderer doc(m_pixmapPath+filename);
        if (size == QSize(0,0))
            size = doc.defaultSize();
        QPixmap pix(size.width(),size.height());
        pix.fill(Qt::transparent);
        QPainter painter(&pix);
        painter.setViewport(0, 0, size.width(), size.height());
        doc.render(&painter);
        return pix;
    }
    else
    {
        QPixmap pix(m_pixmapPath+filename);
        return pix.scaled(size);
    }
}
