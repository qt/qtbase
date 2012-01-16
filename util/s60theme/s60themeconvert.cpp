/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "s60themeconvert.h"

#include <QtGui>
#include <QtWebKit>

static const int pictureSize = 256;
static const char* const msgPartNotInTdf = "  Warning: The .tdf file does not have a part for ";
static const char* const msgSvgNotFound = "  Fatal: Could not find part .svg ";

void dumpPartPictures(const QHash<QString, QPicture> &partPictures) {
    foreach (const QString &partKey, partPictures.keys()) {
        QPicture partPicture = partPictures.value(partKey);
        qDebug() << partKey << partPicture.boundingRect();
        QImage image(partPicture.boundingRect().size(), QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        QPainter p(&image);
        partPicture.play(&p);
        image.save(partKey + QString::fromLatin1(".png"));
    }
}

void dumpColors(const QHash<QPair<QString, int>, QColor> &colors) {
    foreach (const QColor &color, colors.values()) {
        const QPair<QString, int> key = colors.key(color);
        qDebug() << key << color;
    }
}

class WebKitSVGRenderer : public QWebView
{
    Q_OBJECT

public:
    WebKitSVGRenderer(QWidget *parent = 0);
    QPicture svgToQPicture(const QString &svgFileName);

private slots:
    void loadFinishedSlot(bool ok);

private:
    QEventLoop m_loop;
    QPicture m_result;
};

WebKitSVGRenderer::WebKitSVGRenderer(QWidget *parent)
    : QWebView(parent)
{

    connect(this, SIGNAL(loadFinished(bool)), SLOT(loadFinishedSlot(bool)));
    setFixedSize(pictureSize, pictureSize);
    QPalette pal = palette();
    pal.setColor(QPalette::Base, Qt::transparent);
    setPalette(pal);
}

QPicture WebKitSVGRenderer::svgToQPicture(const QString &svgFileName)
{
    load(QUrl::fromLocalFile(svgFileName));
    m_loop.exec();
    return m_result;
}

void WebKitSVGRenderer::loadFinishedSlot(bool ok)
{
    // crude error-checking
    if (!ok)
        qDebug() << "Failed loading " << qPrintable(url().toString());

    page()->mainFrame()->evaluateJavaScript(QString::fromLatin1(
        "document.rootElement.preserveAspectRatio.baseVal.align = SVGPreserveAspectRatio.SVG_PRESERVEASPECTRATIO_NONE;"
        "document.rootElement.style.width = '100%';"
        "document.rootElement.style.height = '100%';"
        "document.rootElement.width.baseVal.newValueSpecifiedUnits(SVGLength.SVG_LENGTHTYPE_PERCENTAGE, 100);"
        "document.rootElement.height.baseVal.newValueSpecifiedUnits(SVGLength.SVG_LENGTHTYPE_PERCENTAGE, 100);"
    ));

    m_result = QPicture(); // "Clear"
    QPainter p(&m_result);
    page()->mainFrame()->render(&p);
    p.end();
    m_result.setBoundingRect(QRect(0, 0, pictureSize, pictureSize));

    m_loop.exit();
}

QPair<QString, int> colorIdPair(const QString &colorID)
{
    QPair<QString, int> result;
    QString idText = colorID;
    idText.remove(QRegExp(QString::fromLatin1("[0-9]")));
    if (QS60Style::colorListKeys().contains(idText)) {
        QString idNumber = colorID;
        idNumber.remove(QRegExp(QString::fromLatin1("[a-zA-Z]")));
        result.first = idText;
        result.second = idNumber.toInt();
    }
    return result;
}

bool parseTdfFile(const QString &tdfFile,
        QHash<QString, QString> &partSvgs,
        QHash<QPair<QString, int>, QColor> &colors)
{
    QFile file(tdfFile);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    const QLatin1String elementKey("element");
    const QLatin1String partKey("part");
    const QLatin1String elementIdKey("id");
    const QLatin1String layerKey("layer");
    const QLatin1String layerFileNameKey("filename");
    const QLatin1String layerColourrgbKey("colourrgb");
    const QString annoyingPrefix = QString::fromLatin1("S60_2_6%");

    QXmlStreamReader reader(&file);
    QString partId;
    QPair<QString, int> colorId;
    // Somebody with a sense of aesthetics may implement proper XML parsing, here.
    while (!reader.atEnd()) {
        const QXmlStreamReader::TokenType token = reader.readNext();
        switch (token) {
            case QXmlStreamReader::StartElement:
                if (reader.name() == elementKey || reader.name() == partKey) {
                    QString id = reader.attributes().value(elementIdKey).toString();
                    if (QS60Style::partKeys().contains(id))
                        partId = id;
                    else if (!id.isEmpty() && id.at(id.length()-1).isDigit())
                        colorId = colorIdPair(id);
                    else if (QS60Style::partKeys().contains(id.mid(annoyingPrefix.length())))
                        partId = id.mid(annoyingPrefix.length());
                } else if(reader.name() == layerKey) {
                    if (!partId.isEmpty()) {
                        const QString svgFile = reader.attributes().value(layerFileNameKey).toString();
                        partSvgs.insert(partId, svgFile);
                        partId.clear();
                    } else if (!colorId.first.isEmpty()) {
                        const QColor colorValue(reader.attributes().value(layerColourrgbKey).toString().toInt(NULL, 16));
                        colors.insert(colorId, colorValue);
                        colorId.first.clear();
                    }
                }
                break;
            case QXmlStreamReader::EndElement:
                if (reader.tokenString() == elementKey || reader.name() == partKey)
                    partId.clear();
                break;
            default:
                break;
        }
    }
    return true;
}

bool loadThemeFromTdf(const QString &tdfFile,
        QHash<QString, QPicture> &partPictures,
        QHash<QPair<QString, int>, QColor> &colors)
{
    QHash<QString, QString> parsedPartSvgs;
    QHash<QString, QPicture> parsedPartPictures;
    QHash<QPair<QString, int>, QColor> parsedColors;
    bool success = parseTdfFile(tdfFile, parsedPartSvgs, parsedColors);
    if (!success)
        return false;
    const QString tdfBasePath = QFileInfo(tdfFile).absolutePath();
    WebKitSVGRenderer renderer;
    foreach (const QString &partKey, QS60Style::partKeys()) {
        qDebug() << partKey;
        QString tdfFullName;
        if (parsedPartSvgs.contains(partKey)) {
            tdfFullName = tdfBasePath + QDir::separator() + parsedPartSvgs.value(partKey);
        } else {
            qWarning() << msgPartNotInTdf << partKey;
            tdfFullName = tdfBasePath + QDir::separator() + partKey + QLatin1String(".svg");
        }
        if (!QFile(tdfFullName).exists()) {
            qWarning() << msgSvgNotFound << QDir::toNativeSeparators(tdfFullName);
            return false;
        }
        const QPicture partPicture = renderer.svgToQPicture(tdfFullName);
        parsedPartPictures.insert(partKey, partPicture);
    }
//    dumpPartPictures(parsedPartPictures);
//    dumpColors(colors);
    partPictures = parsedPartPictures;
    colors = parsedColors;
    return true;
}

bool S60ThemeConvert::convertTdfToBlob(const QString &themeTdf, const QString &themeBlob)
{
    QHash<QString, QPicture> partPictures;
    QHash<QPair<QString, int>, QColor> colors;

    if (!::loadThemeFromTdf(themeTdf, partPictures, colors))
        return false;

    QS60Style style;
    style.setS60Theme(partPictures, colors);
    return style.saveS60ThemeToBlob(themeBlob);
}

bool parseDesignFile(const QString &designFile,
        QHash<QPair<QString, int>, QColor> &colors)
{
    const QLatin1String elementKey("element");
    const QLatin1String elementIdKey("id");
    const QLatin1String colorKey("defaultcolour_rgb");
    QFile file(designFile);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QXmlStreamReader reader(&file);
    QPair<QString, int> colorId;
    // Somebody with a sense of aesthetics may implement proper XML parsing, here.
    while (!reader.atEnd()) {
        const QXmlStreamReader::TokenType token = reader.readNext();
        switch (token) {
            case QXmlStreamReader::StartElement:
                if (reader.name() == elementKey) {
                    const QString colorString = reader.attributes().value(colorKey).toString();
                    if (!colorString.isEmpty()) {
                        const QString colorId = reader.attributes().value(elementIdKey).toString();
                        colors.insert(colorIdPair(colorId), colorString.toInt(NULL, 16));
                    }
                }
            default:
                break;
        }
    }
    return true;
}

bool loadDefaultTheme(const QString &themePath,
        QHash<QString, QPicture> &partPictures,
        QHash<QPair<QString, int>, QColor> &colors)
{
    const QDir dir(themePath);
    if (!dir.exists())
        return false;

    if (!parseDesignFile(themePath + QDir::separator() + QString::fromLatin1("defaultdesign.xml"), colors))
        return false;

    WebKitSVGRenderer renderer;
    foreach (const QString &partKey, QS60Style::partKeys()) {
        const QString partFileName = partKey + QLatin1String(".svg");		
        const QString partFile(dir.absolutePath() + QDir::separator() + partFileName);
        if (!QFile::exists(partFile)) {
            qWarning() << msgSvgNotFound << partFileName;
            return false;
        }
        const QPicture partPicture = renderer.svgToQPicture(partFile);
        partPictures.insert(partKey, partPicture);
    }
    return true;
}

bool S60ThemeConvert::convertDefaultThemeToBlob(const QString &themePath, const QString &themeBlob)
{
    QHash<QString, QPicture> partPictures;
    QHash<QPair<QString, int>, QColor> colors;

    if (!::loadDefaultTheme(themePath, partPictures, colors))
        return false;

    QS60Style style;
    style.setS60Theme(partPictures, colors);
    return style.saveS60ThemeToBlob(themeBlob);
}

#include "s60themeconvert.moc"
