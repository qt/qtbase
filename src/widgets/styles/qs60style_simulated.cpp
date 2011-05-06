/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qs60style.h"
#include "qs60style_p.h"
#include "qfile.h"
#include "qhash.h"
#include "qapplication.h"
#include "qpainter.h"
#include "qpicture.h"
#include "qstyleoption.h"
#include "qtransform.h"
#include "qlayout.h"
#include "qpixmapcache.h"
#include "qmetaobject.h"
#include "qdebug.h"
#include "qbuffer.h"
#include "qdesktopwidget.h"

#if !defined(QT_NO_STYLE_S60) || defined(QT_PLUGIN)

QT_BEGIN_NAMESPACE

static const quint32 blobVersion = 1;
static const int pictureSize = 256;

#if defined(Q_CC_GNU)
#if __GNUC__ >= 2
#define __FUNCTION__ __func__
#endif
#endif


bool saveThemeToBlob(const QString &themeBlob,
    const QHash<QString, QPicture> &partPictures,
    const QHash<QPair<QString, int>, QColor> &colors)
{
    QFile blob(themeBlob);
    if (!blob.open(QIODevice::WriteOnly)) {
        qWarning() << __FUNCTION__ << ": Could not create blob: " << themeBlob;
        return false;
    }

    QByteArray data;
    QBuffer dataBuffer(&data);
    dataBuffer.open(QIODevice::WriteOnly);
    QDataStream dataOut(&dataBuffer);

    const int colorsCount = colors.count();
    dataOut << colorsCount;
    const QList<QPair<QString, int> > colorKeys = colors.keys();
    for (int i = 0; i < colorsCount; ++i) {
        const QPair<QString, int> &key = colorKeys.at(i);
        dataOut << key;
        const QColor color = colors.value(key);
        dataOut << color;
    }

    dataOut << partPictures.count();
    QHashIterator<QString, QPicture> i(partPictures);
    while (i.hasNext()) {
        i.next();
        dataOut << i.key();
        dataOut << i.value(); // the QPicture
    }

    QDataStream blobOut(&blob);
    blobOut << blobVersion;
    blobOut << qCompress(data);
    return blobOut.status() == QDataStream::Ok;
}

bool loadThemeFromBlob(const QString &themeBlob,
    QHash<QString, QPicture> &partPictures,
    QHash<QPair<QString, int>, QColor> &colors)
{
    QFile blob(themeBlob);
    if (!blob.open(QIODevice::ReadOnly)) {
        qWarning() << __FUNCTION__ << ": Could not read blob: " << themeBlob;
        return false;
    }
    QDataStream blobIn(&blob);

    quint32 version;
    blobIn >> version;

    if (version != blobVersion) {
        qWarning() << __FUNCTION__ << ": Invalid blob version: " << version << " ...expected: " << blobVersion;
        return false;
    }

    QByteArray data;
    blobIn >> data;
    data = qUncompress(data);
    QBuffer dataBuffer(&data);
    dataBuffer.open(QIODevice::ReadOnly);
    QDataStream dataIn(&dataBuffer);

    int colorsCount;
    dataIn >> colorsCount;
    for (int i = 0; i < colorsCount; ++i) {
        QPair<QString, int> key;
        dataIn >> key;
        QColor value;
        dataIn >> value;
        colors.insert(key, value);
    }

    int picturesCount;
    dataIn >> picturesCount;
    for (int i = 0; i < picturesCount; ++i) {
        QString key;
        dataIn >> key;
        QPicture value;
        dataIn >> value;
        value.setBoundingRect(QRect(0, 0, pictureSize, pictureSize)); // Bug? The forced bounding rect was not deserialized.
        partPictures.insert(key, value);
    }

    if (dataIn.status() != QDataStream::Ok) {
        qWarning() << __FUNCTION__ << ": Invalid data blob: " << themeBlob;
        return false;
    }
    return true;
}

class QS60StyleModeSpecifics
{
public:
    static QPixmap skinnedGraphics(QS60StyleEnums::SkinParts stylepart,
        const QSize &size, QS60StylePrivate::SkinElementFlags flags);
    static QHash<QString, QPicture> m_partPictures;
    static QHash<QPair<QString , int>, QColor> m_colors;
};
QHash<QString, QPicture> QS60StyleModeSpecifics::m_partPictures;
QHash<QPair<QString , int>, QColor> QS60StyleModeSpecifics::m_colors;

QS60StylePrivate::QS60StylePrivate()
{
    setCurrentLayout(0);
}

QColor QS60StylePrivate::s60Color(QS60StyleEnums::ColorLists list,
    int index, const QStyleOption *option)
{
    const QString listKey = QS60Style::colorListKeys().at(list);
    return QS60StylePrivate::stateColor(
        QS60StyleModeSpecifics::m_colors.value(QPair<QString, int>(listKey, index)),
        option
    );
}

QPixmap QS60StylePrivate::part(QS60StyleEnums::SkinParts part, const QSize &size,
                               QPainter *painter, QS60StylePrivate::SkinElementFlags flags)
{
    Q_UNUSED(painter);

    const QString partKey = QS60Style::partKeys().at(part);
    const QPicture partPicture = QS60StyleModeSpecifics::m_partPictures.value(partKey);
    QSize partSize(partPicture.boundingRect().size());
    if (flags & (SF_PointEast | SF_PointWest)) {
        const int temp = partSize.width();
        partSize.setWidth(partSize.height());
        partSize.setHeight(temp);
    }
    const qreal scaleX = size.width() / (qreal)partSize.width();
    const qreal scaleY = size.height() / (qreal)partSize.height();

    QImage partImage(size, QImage::Format_ARGB32);
    partImage.fill(Qt::transparent);
    QPainter resultPainter(&partImage);
    QTransform t;

    if (flags & SF_PointEast)
        t.translate(size.width(), 0);
    else if (flags & SF_PointSouth)
        t.translate(size.width(), size.height());
    else if (flags & SF_PointWest)
        t.translate(0, size.height());

    t.scale(scaleX, scaleY);

    if (flags & SF_PointEast)
        t.rotate(90);
    else if (flags & SF_PointSouth)
        t.rotate(180);
    else if (flags & SF_PointWest)
        t.rotate(270);

    resultPainter.setTransform(t, true);
    const_cast<QPicture *>(&partPicture)->play(&resultPainter);
    resultPainter.end();

    QPixmap result = QPixmap::fromImage(partImage);
    if (flags & SF_StateDisabled) {
        QStyleOption opt;
        QPalette *themePalette = QS60StylePrivate::themePalette();
        if (themePalette)
            opt.palette = *themePalette;
        result = QApplication::style()->generatedIconPixmap(QIcon::Disabled, result, &opt);
    }

    return result;
}

QPixmap QS60StylePrivate::frame(SkinFrameElements frame, const QSize &size,
    SkinElementFlags flags)
{
    const QS60StyleEnums::SkinParts center =        m_frameElementsData[frame].center;
    const QS60StyleEnums::SkinParts topLeft =       QS60StyleEnums::SkinParts(center - 8);
    const QS60StyleEnums::SkinParts topRight =      QS60StyleEnums::SkinParts(center - 7);
    const QS60StyleEnums::SkinParts bottomLeft =    QS60StyleEnums::SkinParts(center - 6);
    const QS60StyleEnums::SkinParts bottomRight =   QS60StyleEnums::SkinParts(center - 5);
    const QS60StyleEnums::SkinParts top =           QS60StyleEnums::SkinParts(center - 4);
    const QS60StyleEnums::SkinParts bottom =        QS60StyleEnums::SkinParts(center - 3);
    const QS60StyleEnums::SkinParts left =          QS60StyleEnums::SkinParts(center - 2);
    const QS60StyleEnums::SkinParts right =         QS60StyleEnums::SkinParts(center - 1);

    // The size of topLeft defines all other sizes
    const QSize cornerSize(partSize(topLeft));
    // if frame is so small that corners would cover it completely, draw only center piece
    const bool drawOnlyCenter =
         2 * cornerSize.width() + 1 >= size.width() || 2 * cornerSize.height() + 1 >= size.height();

    const int cornerWidth = cornerSize.width();
    const int cornerHeight = cornerSize.height();
    const int rectWidth = size.width();
    const int rectHeight = size.height();
    const QRect rect(QPoint(), size);

    const QRect topLeftRect = QRect(rect.topLeft(), cornerSize);
    const QRect topRect = rect.adjusted(cornerWidth, 0, -cornerWidth, -(rectHeight - cornerHeight));
    const QRect topRightRect = topLeftRect.translated(rectWidth - cornerWidth, 0);
    const QRect rightRect = rect.adjusted(rectWidth - cornerWidth, cornerHeight, 0, -cornerHeight);
    const QRect bottomRightRect = topRightRect.translated(0, rectHeight - cornerHeight);
    const QRect bottomRect = topRect.translated(0, rectHeight - cornerHeight);
    const QRect bottomLeftRect = topLeftRect.translated(0, rectHeight - cornerHeight);
    const QRect leftRect = rightRect.translated(cornerWidth - rectWidth, 0);
    const QRect centerRect = drawOnlyCenter ? rect : rect.adjusted(cornerWidth, cornerWidth, -cornerWidth, -cornerWidth);

    QPixmap result(size);
    result.fill(Qt::transparent);
    QPainter painter(&result);

#if 0
    painter.save();
    painter.setOpacity(.3);
    painter.fillRect(topLeftRect, Qt::red);
    painter.fillRect(topRect, Qt::green);
    painter.fillRect(topRightRect, Qt::blue);
    painter.fillRect(rightRect, Qt::green);
    painter.fillRect(bottomRightRect, Qt::red);
    painter.fillRect(bottomRect, Qt::blue);
    painter.fillRect(bottomLeftRect, Qt::green);
    painter.fillRect(leftRect, Qt::blue);
    painter.fillRect(centerRect, Qt::red);
    painter.restore();
#else
    drawPart(topLeft, &painter, topLeftRect, flags);
    drawPart(top, &painter, topRect, flags);
    drawPart(topRight, &painter, topRightRect, flags);
    drawPart(right, &painter, rightRect, flags);
    drawPart(bottomRight, &painter, bottomRightRect, flags);
    drawPart(bottom, &painter, bottomRect, flags);
    drawPart(bottomLeft, &painter, bottomLeftRect, flags);
    drawPart(left, &painter, leftRect, flags);
    drawPart(center, &painter, centerRect, flags);
#endif

    return result;
}

QPixmap QS60StylePrivate::backgroundTexture(bool /*skipCreation*/)
{
    if (!m_background) {
        const QSize size = QApplication::desktop()->screen()->size();
        QPixmap background = part(QS60StyleEnums::SP_QsnBgScreen, size, 0);
        m_background = new QPixmap(background);
    }
    return *m_background;
}

bool QS60StylePrivate::isTouchSupported()
{
#ifdef QT_KEYPAD_NAVIGATION
    return !QApplication::keypadNavigationEnabled();
#else
    return true;
#endif
}

bool QS60StylePrivate::isToolBarBackground()
{
    return true;
}

bool QS60StylePrivate::hasSliderGrooveGraphic()
{
    return false;
}

bool QS60StylePrivate::isSingleClickUi()
{
    return false;
}

QFont QS60StylePrivate::s60Font_specific(
        QS60StyleEnums::FontCategories fontCategory,
        int pointSize, bool resolveFontSize)
{
    QFont result;
    if (resolveFontSize)
        result.setPointSize(pointSize);
    switch (fontCategory) {
        case QS60StyleEnums::FC_Primary:
            result.setBold(true);
            break;
        case QS60StyleEnums::FC_Secondary:
        case QS60StyleEnums::FC_Title:
        case QS60StyleEnums::FC_PrimarySmall:
        case QS60StyleEnums::FC_Digital:
        case QS60StyleEnums::FC_Undefined:
        default:
            break;
    }
    return result;
}

int QS60StylePrivate::currentAnimationFrame(QS60StyleEnums::SkinParts part)
{
    return 0;
}

/*!
  Constructs a QS60Style object.
*/
QS60Style::QS60Style()
    : QCommonStyle(*new QS60StylePrivate)
{
    const QString defaultBlob = QString::fromLatin1(":/trolltech/styles/s60style/images/defaults60theme.blob");
    if (QFile::exists(defaultBlob))
        loadS60ThemeFromBlob(defaultBlob);
}

Q_GLOBAL_STATIC_WITH_INITIALIZER(QStringList, enumPartKeys, {
    const int enumIndex = QS60StyleEnums::staticMetaObject.indexOfEnumerator("SkinParts");
    Q_ASSERT(enumIndex >= 0);
    const QMetaEnum metaEnum = QS60StyleEnums::staticMetaObject.enumerator(enumIndex);
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        const QString enumKey = QString::fromLatin1(metaEnum.key(i));
        QString partKey;
        // Following loop does following conversions: "SP_QgnNoteInfo" to "qgn_note_info"...
        for (int charPosition = 3; charPosition < enumKey.length(); charPosition++) {
            if (charPosition > 3 && enumKey[charPosition].isUpper())
                partKey.append(QChar::fromLatin1('_'));
            partKey.append(enumKey[charPosition].toLower());
        }
        x->append(partKey);
    }
})

QStringList QS60Style::partKeys()
{
    return *enumPartKeys();
}

Q_GLOBAL_STATIC_WITH_INITIALIZER(QStringList, enumColorListKeys, {
    const int enumIndex = QS60StyleEnums::staticMetaObject.indexOfEnumerator("ColorLists");
    Q_ASSERT(enumIndex >= 0);
    const QMetaEnum metaEnum = QS60StyleEnums::staticMetaObject.enumerator(enumIndex);
    for (int i = 0; i < metaEnum.keyCount(); i++) {
        const QString enumKey = QString::fromLatin1(metaEnum.key(i));
        // Following line does following conversions: CL_QsnTextColors to "text"...
        x->append(enumKey.mid(6, enumKey.length() - 12).toLower());
    }
})

QStringList QS60Style::colorListKeys()
{
    return *enumColorListKeys();
}

void QS60Style::setS60Theme(const QHash<QString, QPicture> &parts,
    const QHash<QPair<QString , int>, QColor> &colors)
{
    Q_D(QS60Style);
    QS60StyleModeSpecifics::m_partPictures = parts;
    QS60StyleModeSpecifics::m_colors = colors;
    d->clearCaches(QS60StylePrivate::CC_ThemeChange);
    d->setBackgroundTexture(qApp);
    d->setThemePalette(qApp);
}

bool QS60Style::loadS60ThemeFromBlob(const QString &blobFile)
{
    QHash<QString, QPicture> partPictures;
    QHash<QPair<QString, int>, QColor> colors;

    if (!loadThemeFromBlob(blobFile, partPictures, colors))
        return false;
    setS60Theme(partPictures, colors);
    return true;
}

bool QS60Style::saveS60ThemeToBlob(const QString &blobFile) const
{
    return saveThemeToBlob(blobFile,
        QS60StyleModeSpecifics::m_partPictures, QS60StyleModeSpecifics::m_colors);
}

QPoint qt_s60_fill_background_offset(const QWidget *targetWidget)
{
    Q_UNUSED(targetWidget)
    return QPoint();
}

QT_END_NAMESPACE

#endif // QT_NO_STYLE_S60 || QT_PLUGIN
