/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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


#include <QtTest/QtTest>

#include <limits.h>

#include <qcolor.h>
#include <qdebug.h>
#include <private/qcolortrclut_p.h>
#include <private/qdrawingprimitive_sse2_p.h>
#include <qrgba64.h>

class tst_QColor : public QObject
{
    Q_OBJECT

public:
    tst_QColor();

private slots:
    void getSetCheck();
    void isValid_data();
    void isValid();

    void name_data();
    void name();
    void namehex_data();
    void namehex();
    void setNamedColor_data();
    void setNamedColor();

    void constructNamedColorWithSpace();

    void colorNames();

    void spec();

    void globalColors_data();
    void globalColors();
#if defined(Q_COMPILER_CONSTEXPR) & defined(Q_COMPILER_UNIFORM_INIT)
    void colorConstants_data();
    void colorConstants();
#endif

    void setRed();
    void setGreen();
    void setBlue();

    void setRgb();
    void setRgbF();
    void setRgba();
    void setHsv();
    void setCmyk();
    void setHsl();

    void toRgb_data();
    void toRgb();
    void toRgbNonDestructive();

    void toHsv_data();
    void toHsv();
    void toHsvNonDestructive();

    void toCmyk_data();
    void toCmyk();
    void toCmykNonDestructive();

    void toHsl_data();
    void toHsl();;
    void toHslNonDestructive();

    void convertTo();

    void lighter();
    void darker();

    void specConstructor_data();
    void specConstructor();

    void achromaticHslHue();

    void equality();

    void premultiply();
    void unpremultiply_sse4();
    void qrgba64();
    void qrgba64MemoryLayout();
    void qrgba64Premultiply();
    void qrgba64Equivalence();

    void qcolorprofile_data();
    void qcolorprofile();
};

// Testing get/set functions
void tst_QColor::getSetCheck()
{
    QColor obj1;
    // int QColor::alpha()
    // void QColor::setAlpha(int)
    obj1.setAlpha(0);
    QCOMPARE(obj1.alpha(), 0);
    obj1.setAlpha(-1);
    QCOMPARE(obj1.alpha(), 0); // range<0, 255>
    obj1.setAlpha(INT_MIN);
    QCOMPARE(obj1.alpha(), 0); // range<0, 255>
    obj1.setAlpha(255);
    QCOMPARE(obj1.alpha(), 255); // range<0, 255>
    obj1.setAlpha(INT_MAX);
    QCOMPARE(obj1.alpha(), 255); // range<0, 255>

    // qreal QColor::alphaF()
    // void QColor::setAlphaF(qreal)
    obj1.setAlphaF(0.0);
    QCOMPARE(obj1.alphaF(), qreal(0.0)); // range<0.0, 1.0>
    obj1.setAlphaF(-0.2);
    QCOMPARE(obj1.alphaF(), qreal(0.0)); // range<0.0, 1.0>
    obj1.setAlphaF(1.0);
    QCOMPARE(obj1.alphaF(), qreal(1.0)); // range<0.0, 1.0>
    obj1.setAlphaF(1.1);
    QCOMPARE(obj1.alphaF(), qreal(1.0)); // range<0.0, 1.0>

    // int QColor::red()
    // void QColor::setRed(int)
    obj1.setRed(0);
    QCOMPARE(obj1.red(), 0);
    obj1.setRed(-1);
    QCOMPARE(obj1.red(), 0); // range<0, 255>
    obj1.setRed(INT_MIN);
    QCOMPARE(obj1.red(), 0); // range<0, 255>
    obj1.setRed(255);
    QCOMPARE(obj1.red(), 255); // range<0, 255>
    obj1.setRed(INT_MAX);
    QCOMPARE(obj1.red(), 255); // range<0, 255>

    // int QColor::green()
    // void QColor::setGreen(int)
    obj1.setGreen(0);
    QCOMPARE(obj1.green(), 0);
    obj1.setGreen(-1);
    QCOMPARE(obj1.green(), 0); // range<0, 255>
    obj1.setGreen(INT_MIN);
    QCOMPARE(obj1.green(), 0); // range<0, 255>
    obj1.setGreen(255);
    QCOMPARE(obj1.green(), 255); // range<0, 255>
    obj1.setGreen(INT_MAX);
    QCOMPARE(obj1.green(), 255); // range<0, 255>

    // int QColor::blue()
    // void QColor::setBlue(int)
    obj1.setBlue(0);
    QCOMPARE(obj1.blue(), 0);
    obj1.setBlue(-1);
    QCOMPARE(obj1.blue(), 0); // range<0, 255>
    obj1.setBlue(INT_MIN);
    QCOMPARE(obj1.blue(), 0); // range<0, 255>
    obj1.setBlue(255);
    QCOMPARE(obj1.blue(), 255); // range<0, 255>
    obj1.setBlue(INT_MAX);
    QCOMPARE(obj1.blue(), 255); // range<0, 255>

    // qreal QColor::redF()
    // void QColor::setRedF(qreal)
    obj1.setRedF(0.0);
    QCOMPARE(obj1.redF(), qreal(0.0));
    obj1.setRedF(-0.25);
    QCOMPARE(obj1.redF(), qreal(-0.25));
    obj1.setRedF(1.25);
    QCOMPARE(obj1.redF(), qreal(1.25));

    // qreal QColor::greenF()
    // void QColor::setGreenF(qreal)
    obj1.setGreenF(0.0);
    QCOMPARE(obj1.greenF(), qreal(0.0));
    obj1.setGreenF(-0.25);
    QCOMPARE(obj1.greenF(), qreal(-0.25));
    obj1.setGreenF(1.5);
    QCOMPARE(obj1.greenF(), qreal(1.5));

    // qreal QColor::blueF()
    // void QColor::setBlueF(qreal)
    obj1.setBlueF(0.0);
    QCOMPARE(obj1.blueF(), qreal(0.0));
    obj1.setBlueF(-0.5);
    QCOMPARE(obj1.blueF(), qreal(-0.5));
    obj1.setBlueF(2.0);
    QCOMPARE(obj1.blueF(), qreal(2.0));

    // QRgb QColor::rgba()
    // void QColor::setRgba(QRgb)
    QRgb var9(qRgba(10, 20, 30, 40));
    obj1.setRgba(var9);
    QCOMPARE(obj1.rgba(), var9);
    obj1.setRgba(QRgb(0));
    QCOMPARE(obj1.rgba(), QRgb(0));

    // QRgb QColor::rgb()
    // void QColor::setRgb(QRgb)
    QRgb var10(qRgb(10, 20, 30));
    obj1.setRgb(var10);
    QCOMPARE(obj1.rgb(), var10);
    obj1.setRgb(QRgb(0));
    QCOMPARE(obj1.rgb(), qRgb(0, 0, 0));
}



tst_QColor::tst_QColor()

{ }

void tst_QColor::isValid_data()
{
    QTest::addColumn<QColor>("color");
    QTest::addColumn<bool>("isValid");

    QTest::newRow("defaultConstructor") << QColor() << false;
    QTest::newRow("rgbConstructor-valid") << QColor(2,5,7) << true;
    QTest::newRow("rgbConstructor-invalid") << QColor(2,5,999) << false;
    QTest::newRow("nameQStringConstructor-valid") << QColor(QString("#ffffff")) << true;
    QTest::newRow("nameQStringConstructor-invalid") << QColor(QString("#ffffgg")) << false;
    QTest::newRow("nameQStringConstructor-empty") << QColor(QString("")) << false;
    QTest::newRow("nameQStringConstructor-named") << QColor(QString("red")) << true;
    QTest::newRow("nameCharConstructor-valid") << QColor("#ffffff") << true;
    QTest::newRow("nameCharConstructor-invalid") << QColor("#ffffgg") << false;
    QTest::newRow("nameCharConstructor-invalid-2") << QColor("#fffffg") << false;
}

void tst_QColor::isValid()
{
    QFETCH(QColor, color);
    QFETCH(bool, isValid);
    QCOMPARE(color.isValid(), isValid);
}

Q_DECLARE_METATYPE(QColor::NameFormat);

void tst_QColor::name_data()
{
    QTest::addColumn<QColor>("color");
    QTest::addColumn<QString>("name");
    QTest::addColumn<QColor::NameFormat>("nameFormat");

    QTest::newRow("invalid") << QColor() << "#000000" << QColor::HexRgb;
    QTest::newRow("global color black") << QColor(Qt::black) << "#000000" << QColor::HexRgb;
    QTest::newRow("global color white") << QColor(Qt::white) << "#ffffff" << QColor::HexRgb;
    QTest::newRow("global color darkGray") << QColor(Qt::darkGray) << "#808080" << QColor::HexRgb;
    QTest::newRow("global color gray") << QColor(Qt::gray) << "#a0a0a4" << QColor::HexRgb;
    QTest::newRow("global color lightGray") << QColor(Qt::lightGray) << "#c0c0c0" << QColor::HexRgb;
    QTest::newRow("global color red") << QColor(Qt::red) << "#ff0000" << QColor::HexRgb;
    QTest::newRow("global color green") << QColor(Qt::green) << "#00ff00" << QColor::HexRgb;
    QTest::newRow("global color blue") << QColor(Qt::blue) << "#0000ff" << QColor::HexRgb;
    QTest::newRow("global color cyan") << QColor(Qt::cyan) << "#00ffff" << QColor::HexRgb;
    QTest::newRow("global color magenta") << QColor(Qt::magenta) << "#ff00ff" << QColor::HexRgb;
    QTest::newRow("global color yellow") << QColor(Qt::yellow) << "#ffff00" << QColor::HexRgb;
    QTest::newRow("global color darkRed") << QColor(Qt::darkRed) << "#800000" << QColor::HexRgb;
    QTest::newRow("global color darkGreen") << QColor(Qt::darkGreen) << "#008000" << QColor::HexRgb;
    QTest::newRow("global color darkBlue") << QColor(Qt::darkBlue) << "#000080" << QColor::HexRgb;
    QTest::newRow("global color darkCyan") << QColor(Qt::darkCyan) << "#008080" << QColor::HexRgb;
    QTest::newRow("global color darkMagenta") << QColor(Qt::darkMagenta) << "#800080" << QColor::HexRgb;
    QTest::newRow("global color darkYellow") << QColor(Qt::darkYellow) << "#808000" << QColor::HexRgb;
    QTest::newRow("transparent red") << QColor(255, 0, 0, 102) << "#66ff0000" << QColor::HexArgb;
    QTest::newRow("fully_transparent_green_rgb") << QColor(0, 0, 255, 0) << "#0000ff" << QColor::HexRgb;
    QTest::newRow("fully_transparent_green_argb") << QColor(0, 0, 255, 0) << "#000000ff" << QColor::HexArgb;
}

void tst_QColor::name()
{
    QFETCH(QColor, color);
    QFETCH(QString, name);
    QFETCH(QColor::NameFormat, nameFormat);
    QCOMPARE(color.name(nameFormat), name);
}

void tst_QColor::namehex_data()
{
    QTest::addColumn<QString>("hexcolor");
    QTest::addColumn<QColor>("color");

    QTest::newRow("global color black") << "#000000" << QColor(Qt::black);
    QTest::newRow("global color white") << "#ffffff" << QColor(Qt::white);
    QTest::newRow("global color darkGray") << "#808080" << QColor(Qt::darkGray);
    QTest::newRow("global color gray") << "#a0a0a4" << QColor(Qt::gray);
    QTest::newRow("global color lightGray") << "#c0c0c0" << QColor(Qt::lightGray);
    QTest::newRow("global color red") << "#ff0000" << QColor(Qt::red);
    QTest::newRow("global color green") << "#00ff00" << QColor(Qt::green);
    QTest::newRow("global color blue") << "#0000ff" << QColor(Qt::blue);
    QTest::newRow("global color cyan") << "#00ffff" << QColor(Qt::cyan);
    QTest::newRow("global color magenta") << "#ff00ff" << QColor(Qt::magenta);
    QTest::newRow("global color yellow") << "#ffff00" << QColor(Qt::yellow);
    QTest::newRow("global color darkRed") << "#800000" << QColor(Qt::darkRed);
    QTest::newRow("global color darkGreen") << "#008000" << QColor(Qt::darkGreen);
    QTest::newRow("global color darkBlue") << "#000080" << QColor(Qt::darkBlue);
    QTest::newRow("global color darkCyan") << "#008080" << QColor(Qt::darkCyan);
    QTest::newRow("global color darkMagenta") << "#800080" << QColor(Qt::darkMagenta);
    QTest::newRow("global color darkYellow") << "#808000" << QColor(Qt::darkYellow);
    QTest::newRow("#RGB") << "#888" << QColor(0x88, 0x88, 0x88);
    QTest::newRow("#RRRGGGBBB") << "#80F80F80F" << QColor(qRgba64(0x80f8, 0x80f8, 0x80f8, 0xffff));
    QTest::newRow("#RRRRGGGGBBBB") << "#808180818081" << QColor(qRgba64(0x8081, 0x8081, 0x8081, 0xffff));
    QTest::newRow("transparent red") << "#66ff0000" << QColor(255, 0, 0, 102);
    QTest::newRow("invalid red") << "#gg0000" << QColor();
    QTest::newRow("invalid transparent") << "#gg00ff00" << QColor();
    // when configured with "-sanitize undefined", this resulted in:
    // "runtime error: left shift of negative value -1"
    QTest::newRow("oss-fuzz 23968") << "#ÿÿÿÿÿÿÿÿÿ" << QColor();
}

void tst_QColor::namehex()
{
    QFETCH(QString, hexcolor);
    QFETCH(QColor, color);
    QCOMPARE(QColor(hexcolor), color);
}

void tst_QColor::globalColors_data()
{
    QTest::addColumn<QColor>("color");
    QTest::addColumn<uint>("argb");

    QTest::newRow("invalid") << QColor() << 0xff000000;
    QTest::newRow("global color black") << QColor(Qt::black) << 0xff000000;
    QTest::newRow("global color white") << QColor(Qt::white) << 0xffffffff;
    QTest::newRow("global color darkGray") << QColor(Qt::darkGray) << 0xff808080;
    QTest::newRow("global color gray") << QColor(Qt::gray) << 0xffa0a0a4;
    QTest::newRow("global color lightGray") << QColor(Qt::lightGray) << 0xffc0c0c0;
    QTest::newRow("global color red") << QColor(Qt::red) << 0xffff0000;
    QTest::newRow("global color green") << QColor(Qt::green) << 0xff00ff00;
    QTest::newRow("global color blue") << QColor(Qt::blue) << 0xff0000ff;
    QTest::newRow("global color cyan") << QColor(Qt::cyan) << 0xff00ffff;
    QTest::newRow("global color magenta") << QColor(Qt::magenta) << 0xffff00ff;
    QTest::newRow("global color yellow") << QColor(Qt::yellow) << 0xffffff00;
    QTest::newRow("global color darkRed") << QColor(Qt::darkRed) << 0xff800000;
    QTest::newRow("global color darkGreen") << QColor(Qt::darkGreen) << 0xff008000;
    QTest::newRow("global color darkBlue") << QColor(Qt::darkBlue) << 0xff000080;
    QTest::newRow("global color darkCyan") << QColor(Qt::darkCyan) << 0xff008080;
    QTest::newRow("global color darkMagenta") << QColor(Qt::darkMagenta) << 0xff800080;
    QTest::newRow("global color darkYellow") << QColor(Qt::darkYellow) << 0xff808000;
    QTest::newRow("global color transparent") << QColor(Qt::transparent) << 0x00000000u;
}


void tst_QColor::globalColors()
{
    QFETCH(QColor, color);
    QFETCH(uint, argb);
    QCOMPARE(color.rgba(), argb);
}

#if defined(Q_COMPILER_CONSTEXPR) & defined(Q_COMPILER_UNIFORM_INIT)
void tst_QColor::colorConstants_data()
{
    QTest::addColumn<QColor>("color");
    QTest::addColumn<QRgb>("argb");

    QTest::newRow("invalid") << QColor() << 0xff000000;
    QTest::newRow("global color color0") << QColorConstants::Color0 << 0xff000000u;
    QTest::newRow("global color color1") << QColorConstants::Color1 << 0xffffffffu;
    QTest::newRow("global color black") << QColorConstants::Black << 0xff000000u;
    QTest::newRow("global color white") << QColorConstants::White << 0xffffffffu;
    QTest::newRow("global color darkGray") << QColorConstants::DarkGray << 0xff808080u;
    QTest::newRow("global color gray") << QColorConstants::Gray << 0xffa0a0a4u;
    QTest::newRow("global color lightGray") << QColorConstants::LightGray << 0xffc0c0c0u;
    QTest::newRow("global color red") << QColorConstants::Red << 0xffff0000u;
    QTest::newRow("global color green") << QColorConstants::Green << 0xff00ff00u;
    QTest::newRow("global color blue") << QColorConstants::Blue << 0xff0000ffu;
    QTest::newRow("global color cyan") << QColorConstants::Cyan << 0xff00ffffu;
    QTest::newRow("global color magenta") << QColorConstants::Magenta << 0xffff00ffu;
    QTest::newRow("global color yellow") << QColorConstants::Yellow << 0xffffff00u;
    QTest::newRow("global color darkRed") << QColorConstants::DarkRed << 0xff800000u;
    QTest::newRow("global color darkGreen") << QColorConstants::DarkGreen << 0xff008000u;
    QTest::newRow("global color darkBlue") << QColorConstants::DarkBlue << 0xff000080u;
    QTest::newRow("global color darkCyan") << QColorConstants::DarkCyan << 0xff008080u;
    QTest::newRow("global color darkMagenta") << QColorConstants::DarkMagenta << 0xff800080u;
    QTest::newRow("global color darkYellow") << QColorConstants::DarkYellow << 0xff808000u;
    QTest::newRow("global color transparent") << QColorConstants::Transparent << 0x00000000u;

    QTest::newRow("SVG aliceblue") << QColorConstants::Svg::aliceblue << 0xfff0f8ffu;
    QTest::newRow("SVG antiquewhite") << QColorConstants::Svg::antiquewhite << 0xfffaebd7u;
    QTest::newRow("SVG aqua") << QColorConstants::Svg::aqua << 0xff00ffffu;
    QTest::newRow("SVG aquamarine") << QColorConstants::Svg::aquamarine << 0xff7fffd4u;
    QTest::newRow("SVG azure") << QColorConstants::Svg::azure << 0xfff0ffffu;
    QTest::newRow("SVG beige") << QColorConstants::Svg::beige << 0xfff5f5dcu;
    QTest::newRow("SVG bisque") << QColorConstants::Svg::bisque << 0xffffe4c4u;
    QTest::newRow("SVG black") << QColorConstants::Svg::black << 0xff000000u;
    QTest::newRow("SVG blanchedalmond") << QColorConstants::Svg::blanchedalmond << 0xffffebcdu;
    QTest::newRow("SVG blue") << QColorConstants::Svg::blue << 0xff0000ffu;
    QTest::newRow("SVG blueviolet") << QColorConstants::Svg::blueviolet << 0xff8a2be2u;
    QTest::newRow("SVG brown") << QColorConstants::Svg::brown << 0xffa52a2au;
    QTest::newRow("SVG burlywood") << QColorConstants::Svg::burlywood << 0xffdeb887u;
    QTest::newRow("SVG cadetblue") << QColorConstants::Svg::cadetblue << 0xff5f9ea0u;
    QTest::newRow("SVG chartreuse") << QColorConstants::Svg::chartreuse << 0xff7fff00u;
    QTest::newRow("SVG chocolate") << QColorConstants::Svg::chocolate << 0xffd2691eu;
    QTest::newRow("SVG coral") << QColorConstants::Svg::coral << 0xffff7f50u;
    QTest::newRow("SVG cornflowerblue") << QColorConstants::Svg::cornflowerblue << 0xff6495edu;
    QTest::newRow("SVG cornsilk") << QColorConstants::Svg::cornsilk << 0xfffff8dcu;
    QTest::newRow("SVG crimson") << QColorConstants::Svg::crimson << 0xffdc143cu;
    QTest::newRow("SVG cyan") << QColorConstants::Svg::cyan << 0xff00ffffu;
    QTest::newRow("SVG darkblue") << QColorConstants::Svg::darkblue << 0xff00008bu;
    QTest::newRow("SVG darkcyan") << QColorConstants::Svg::darkcyan << 0xff008b8bu;
    QTest::newRow("SVG darkgoldenrod") << QColorConstants::Svg::darkgoldenrod << 0xffb8860bu;
    QTest::newRow("SVG darkgray") << QColorConstants::Svg::darkgray << 0xffa9a9a9u;
    QTest::newRow("SVG darkgreen") << QColorConstants::Svg::darkgreen << 0xff006400u;
    QTest::newRow("SVG darkgrey") << QColorConstants::Svg::darkgrey << 0xffa9a9a9u;
    QTest::newRow("SVG darkkhaki") << QColorConstants::Svg::darkkhaki << 0xffbdb76bu;
    QTest::newRow("SVG darkmagenta") << QColorConstants::Svg::darkmagenta << 0xff8b008bu;
    QTest::newRow("SVG darkolivegreen") << QColorConstants::Svg::darkolivegreen << 0xff556b2fu;
    QTest::newRow("SVG darkorange") << QColorConstants::Svg::darkorange << 0xffff8c00u;
    QTest::newRow("SVG darkorchid") << QColorConstants::Svg::darkorchid << 0xff9932ccu;
    QTest::newRow("SVG darkred") << QColorConstants::Svg::darkred << 0xff8b0000u;
    QTest::newRow("SVG darksalmon") << QColorConstants::Svg::darksalmon << 0xffe9967au;
    QTest::newRow("SVG darkseagreen") << QColorConstants::Svg::darkseagreen << 0xff8fbc8fu;
    QTest::newRow("SVG darkslateblue") << QColorConstants::Svg::darkslateblue << 0xff483d8bu;
    QTest::newRow("SVG darkslategray") << QColorConstants::Svg::darkslategray << 0xff2f4f4fu;
    QTest::newRow("SVG darkslategrey") << QColorConstants::Svg::darkslategrey << 0xff2f4f4fu;
    QTest::newRow("SVG darkturquoise") << QColorConstants::Svg::darkturquoise << 0xff00ced1u;
    QTest::newRow("SVG darkviolet") << QColorConstants::Svg::darkviolet << 0xff9400d3u;
    QTest::newRow("SVG deeppink") << QColorConstants::Svg::deeppink << 0xffff1493u;
    QTest::newRow("SVG deepskyblue") << QColorConstants::Svg::deepskyblue << 0xff00bfffu;
    QTest::newRow("SVG dimgray") << QColorConstants::Svg::dimgray << 0xff696969u;
    QTest::newRow("SVG dimgrey") << QColorConstants::Svg::dimgrey << 0xff696969u;
    QTest::newRow("SVG dodgerblue") << QColorConstants::Svg::dodgerblue << 0xff1e90ffu;
    QTest::newRow("SVG firebrick") << QColorConstants::Svg::firebrick << 0xffb22222u;
    QTest::newRow("SVG floralwhite") << QColorConstants::Svg::floralwhite << 0xfffffaf0u;
    QTest::newRow("SVG forestgreen") << QColorConstants::Svg::forestgreen << 0xff228b22u;
    QTest::newRow("SVG fuchsia") << QColorConstants::Svg::fuchsia << 0xffff00ffu;
    QTest::newRow("SVG gainsboro") << QColorConstants::Svg::gainsboro << 0xffdcdcdcu;
    QTest::newRow("SVG ghostwhite") << QColorConstants::Svg::ghostwhite << 0xfff8f8ffu;
    QTest::newRow("SVG gold") << QColorConstants::Svg::gold << 0xffffd700u;
    QTest::newRow("SVG goldenrod") << QColorConstants::Svg::goldenrod << 0xffdaa520u;
    QTest::newRow("SVG gray") << QColorConstants::Svg::gray << 0xff808080u;
    QTest::newRow("SVG green") << QColorConstants::Svg::green << 0xff008000u;
    QTest::newRow("SVG greenyellow") << QColorConstants::Svg::greenyellow << 0xffadff2fu;
    QTest::newRow("SVG grey") << QColorConstants::Svg::grey << 0xff808080u;
    QTest::newRow("SVG honeydew") << QColorConstants::Svg::honeydew << 0xfff0fff0u;
    QTest::newRow("SVG hotpink") << QColorConstants::Svg::hotpink << 0xffff69b4u;
    QTest::newRow("SVG indianred") << QColorConstants::Svg::indianred << 0xffcd5c5cu;
    QTest::newRow("SVG indigo") << QColorConstants::Svg::indigo << 0xff4b0082u;
    QTest::newRow("SVG ivory") << QColorConstants::Svg::ivory << 0xfffffff0u;
    QTest::newRow("SVG khaki") << QColorConstants::Svg::khaki << 0xfff0e68cu;
    QTest::newRow("SVG lavender") << QColorConstants::Svg::lavender << 0xffe6e6fau;
    QTest::newRow("SVG lavenderblush") << QColorConstants::Svg::lavenderblush << 0xfffff0f5u;
    QTest::newRow("SVG lawngreen") << QColorConstants::Svg::lawngreen << 0xff7cfc00u;
    QTest::newRow("SVG lemonchiffon") << QColorConstants::Svg::lemonchiffon << 0xfffffacdu;
    QTest::newRow("SVG lightblue") << QColorConstants::Svg::lightblue << 0xffadd8e6u;
    QTest::newRow("SVG lightcoral") << QColorConstants::Svg::lightcoral << 0xfff08080u;
    QTest::newRow("SVG lightcyan") << QColorConstants::Svg::lightcyan << 0xffe0ffffu;
    QTest::newRow("SVG lightgoldenrodyellow") << QColorConstants::Svg::lightgoldenrodyellow << 0xfffafad2u;
    QTest::newRow("SVG lightgray") << QColorConstants::Svg::lightgray << 0xffd3d3d3u;
    QTest::newRow("SVG lightgreen") << QColorConstants::Svg::lightgreen << 0xff90ee90u;
    QTest::newRow("SVG lightgrey") << QColorConstants::Svg::lightgrey << 0xffd3d3d3u;
    QTest::newRow("SVG lightpink") << QColorConstants::Svg::lightpink << 0xffffb6c1u;
    QTest::newRow("SVG lightsalmon") << QColorConstants::Svg::lightsalmon << 0xffffa07au;
    QTest::newRow("SVG lightseagreen") << QColorConstants::Svg::lightseagreen << 0xff20b2aau;
    QTest::newRow("SVG lightskyblue") << QColorConstants::Svg::lightskyblue << 0xff87cefau;
    QTest::newRow("SVG lightslategray") << QColorConstants::Svg::lightslategray << 0xff778899u;
    QTest::newRow("SVG lightslategrey") << QColorConstants::Svg::lightslategrey << 0xff778899u;
    QTest::newRow("SVG lightsteelblue") << QColorConstants::Svg::lightsteelblue << 0xffb0c4deu;
    QTest::newRow("SVG lightyellow") << QColorConstants::Svg::lightyellow << 0xffffffe0u;
    QTest::newRow("SVG lime") << QColorConstants::Svg::lime << 0xff00ff00u;
    QTest::newRow("SVG limegreen") << QColorConstants::Svg::limegreen << 0xff32cd32u;
    QTest::newRow("SVG linen") << QColorConstants::Svg::linen << 0xfffaf0e6u;
    QTest::newRow("SVG magenta") << QColorConstants::Svg::magenta << 0xffff00ffu;
    QTest::newRow("SVG maroon") << QColorConstants::Svg::maroon << 0xff800000u;
    QTest::newRow("SVG mediumaquamarine") << QColorConstants::Svg::mediumaquamarine << 0xff66cdaau;
    QTest::newRow("SVG mediumblue") << QColorConstants::Svg::mediumblue << 0xff0000cdu;
    QTest::newRow("SVG mediumorchid") << QColorConstants::Svg::mediumorchid << 0xffba55d3u;
    QTest::newRow("SVG mediumpurple") << QColorConstants::Svg::mediumpurple << 0xff9370dbu;
    QTest::newRow("SVG mediumseagreen") << QColorConstants::Svg::mediumseagreen << 0xff3cb371u;
    QTest::newRow("SVG mediumslateblue") << QColorConstants::Svg::mediumslateblue << 0xff7b68eeu;
    QTest::newRow("SVG mediumspringgreen") << QColorConstants::Svg::mediumspringgreen << 0xff00fa9au;
    QTest::newRow("SVG mediumturquoise") << QColorConstants::Svg::mediumturquoise << 0xff48d1ccu;
    QTest::newRow("SVG mediumvioletred") << QColorConstants::Svg::mediumvioletred << 0xffc71585u;
    QTest::newRow("SVG midnightblue") << QColorConstants::Svg::midnightblue << 0xff191970u;
    QTest::newRow("SVG mintcream") << QColorConstants::Svg::mintcream << 0xfff5fffau;
    QTest::newRow("SVG mistyrose") << QColorConstants::Svg::mistyrose << 0xffffe4e1u;
    QTest::newRow("SVG moccasin") << QColorConstants::Svg::moccasin << 0xffffe4b5u;
    QTest::newRow("SVG navajowhite") << QColorConstants::Svg::navajowhite << 0xffffdeadu;
    QTest::newRow("SVG navy") << QColorConstants::Svg::navy << 0xff000080u;
    QTest::newRow("SVG oldlace") << QColorConstants::Svg::oldlace << 0xfffdf5e6u;
    QTest::newRow("SVG olive") << QColorConstants::Svg::olive << 0xff808000u;
    QTest::newRow("SVG olivedrab") << QColorConstants::Svg::olivedrab << 0xff6b8e23u;
    QTest::newRow("SVG orange") << QColorConstants::Svg::orange << 0xffffa500u;
    QTest::newRow("SVG orangered") << QColorConstants::Svg::orangered << 0xffff4500u;
    QTest::newRow("SVG orchid") << QColorConstants::Svg::orchid << 0xffda70d6u;
    QTest::newRow("SVG palegoldenrod") << QColorConstants::Svg::palegoldenrod << 0xffeee8aau;
    QTest::newRow("SVG palegreen") << QColorConstants::Svg::palegreen << 0xff98fb98u;
    QTest::newRow("SVG paleturquoise") << QColorConstants::Svg::paleturquoise << 0xffafeeeeu;
    QTest::newRow("SVG palevioletred") << QColorConstants::Svg::palevioletred << 0xffdb7093u;
    QTest::newRow("SVG papayawhip") << QColorConstants::Svg::papayawhip << 0xffffefd5u;
    QTest::newRow("SVG peachpuff") << QColorConstants::Svg::peachpuff << 0xffffdab9u;
    QTest::newRow("SVG peru") << QColorConstants::Svg::peru << 0xffcd853fu;
    QTest::newRow("SVG pink") << QColorConstants::Svg::pink << 0xffffc0cbu;
    QTest::newRow("SVG plum") << QColorConstants::Svg::plum << 0xffdda0ddu;
    QTest::newRow("SVG powderblue") << QColorConstants::Svg::powderblue << 0xffb0e0e6u;
    QTest::newRow("SVG purple") << QColorConstants::Svg::purple << 0xff800080u;
    QTest::newRow("SVG red") << QColorConstants::Svg::red << 0xffff0000u;
    QTest::newRow("SVG rosybrown") << QColorConstants::Svg::rosybrown << 0xffbc8f8fu;
    QTest::newRow("SVG royalblue") << QColorConstants::Svg::royalblue << 0xff4169e1u;
    QTest::newRow("SVG saddlebrown") << QColorConstants::Svg::saddlebrown << 0xff8b4513u;
    QTest::newRow("SVG salmon") << QColorConstants::Svg::salmon << 0xfffa8072u;
    QTest::newRow("SVG sandybrown") << QColorConstants::Svg::sandybrown << 0xfff4a460u;
    QTest::newRow("SVG seagreen") << QColorConstants::Svg::seagreen << 0xff2e8b57u;
    QTest::newRow("SVG seashell") << QColorConstants::Svg::seashell << 0xfffff5eeu;
    QTest::newRow("SVG sienna") << QColorConstants::Svg::sienna << 0xffa0522du;
    QTest::newRow("SVG silver") << QColorConstants::Svg::silver << 0xffc0c0c0u;
    QTest::newRow("SVG skyblue") << QColorConstants::Svg::skyblue << 0xff87ceebu;
    QTest::newRow("SVG slateblue") << QColorConstants::Svg::slateblue << 0xff6a5acdu;
    QTest::newRow("SVG slategray") << QColorConstants::Svg::slategray << 0xff708090u;
    QTest::newRow("SVG slategrey") << QColorConstants::Svg::slategrey << 0xff708090u;
    QTest::newRow("SVG snow") << QColorConstants::Svg::snow << 0xfffffafau;
    QTest::newRow("SVG springgreen") << QColorConstants::Svg::springgreen << 0xff00ff7fu;
    QTest::newRow("SVG steelblue") << QColorConstants::Svg::steelblue << 0xff4682b4u;
    QTest::newRow("SVG tan") << QColorConstants::Svg::tan << 0xffd2b48cu;
    QTest::newRow("SVG teal") << QColorConstants::Svg::teal << 0xff008080u;
    QTest::newRow("SVG thistle") << QColorConstants::Svg::thistle << 0xffd8bfd8u;
    QTest::newRow("SVG tomato") << QColorConstants::Svg::tomato << 0xffff6347u;
    QTest::newRow("SVG turquoise") << QColorConstants::Svg::turquoise << 0xff40e0d0u;
    QTest::newRow("SVG violet") << QColorConstants::Svg::violet << 0xffee82eeu;
    QTest::newRow("SVG wheat") << QColorConstants::Svg::wheat << 0xfff5deb3u;
    QTest::newRow("SVG white") << QColorConstants::Svg::white << 0xffffffffu;
    QTest::newRow("SVG whitesmoke") << QColorConstants::Svg::whitesmoke << 0xfff5f5f5u;
    QTest::newRow("SVG yellow") << QColorConstants::Svg::yellow << 0xffffff00u;
    QTest::newRow("SVG yellowgreen") << QColorConstants::Svg::yellowgreen << 0xff9acd32u;
}

void tst_QColor::colorConstants()
{
    QFETCH(QColor, color);
    QFETCH(QRgb, argb);
    QCOMPARE(color.rgba(), argb);
}
#endif // defined(Q_COMPILER_CONSTEXPR) & defined(Q_COMPILER_UNIFORM_INIT)

/*
  CSS color names = SVG 1.0 color names + transparent (rgba(0,0,0,0))
*/

#ifdef rgb
#  undef rgb
#endif
#define rgb(r,g,b) (0xff000000 | r << 16 |  g << 8 | b)

static const struct RGBData {
    const char *name;
    uint  value;
} rgbTbl[] = {
    { "aliceblue", rgb(240, 248, 255) },
    { "antiquewhite", rgb(250, 235, 215) },
    { "aqua", rgb( 0, 255, 255) },
    { "aquamarine", rgb(127, 255, 212) },
    { "azure", rgb(240, 255, 255) },
    { "beige", rgb(245, 245, 220) },
    { "bisque", rgb(255, 228, 196) },
    { "black", rgb( 0, 0, 0) },
    { "blanchedalmond", rgb(255, 235, 205) },
    { "blue", rgb( 0, 0, 255) },
    { "blueviolet", rgb(138, 43, 226) },
    { "brown", rgb(165, 42, 42) },
    { "burlywood", rgb(222, 184, 135) },
    { "cadetblue", rgb( 95, 158, 160) },
    { "chartreuse", rgb(127, 255, 0) },
    { "chocolate", rgb(210, 105, 30) },
    { "coral", rgb(255, 127, 80) },
    { "cornflowerblue", rgb(100, 149, 237) },
    { "cornsilk", rgb(255, 248, 220) },
    { "crimson", rgb(220, 20, 60) },
    { "cyan", rgb( 0, 255, 255) },
    { "darkblue", rgb( 0, 0, 139) },
    { "darkcyan", rgb( 0, 139, 139) },
    { "darkgoldenrod", rgb(184, 134, 11) },
    { "darkgray", rgb(169, 169, 169) },
    { "darkgreen", rgb( 0, 100, 0) },
    { "darkgrey", rgb(169, 169, 169) },
    { "darkkhaki", rgb(189, 183, 107) },
    { "darkmagenta", rgb(139, 0, 139) },
    { "darkolivegreen", rgb( 85, 107, 47) },
    { "darkorange", rgb(255, 140, 0) },
    { "darkorchid", rgb(153, 50, 204) },
    { "darkred", rgb(139, 0, 0) },
    { "darksalmon", rgb(233, 150, 122) },
    { "darkseagreen", rgb(143, 188, 143) },
    { "darkslateblue", rgb( 72, 61, 139) },
    { "darkslategray", rgb( 47, 79, 79) },
    { "darkslategrey", rgb( 47, 79, 79) },
    { "darkturquoise", rgb( 0, 206, 209) },
    { "darkviolet", rgb(148, 0, 211) },
    { "deeppink", rgb(255, 20, 147) },
    { "deepskyblue", rgb( 0, 191, 255) },
    { "dimgray", rgb(105, 105, 105) },
    { "dimgrey", rgb(105, 105, 105) },
    { "dodgerblue", rgb( 30, 144, 255) },
    { "firebrick", rgb(178, 34, 34) },
    { "floralwhite", rgb(255, 250, 240) },
    { "forestgreen", rgb( 34, 139, 34) },
    { "fuchsia", rgb(255, 0, 255) },
    { "gainsboro", rgb(220, 220, 220) },
    { "ghostwhite", rgb(248, 248, 255) },
    { "gold", rgb(255, 215, 0) },
    { "goldenrod", rgb(218, 165, 32) },
    { "gray", rgb(128, 128, 128) },
    { "green", rgb( 0, 128, 0) },
    { "greenyellow", rgb(173, 255, 47) },
    { "grey", rgb(128, 128, 128) },
    { "honeydew", rgb(240, 255, 240) },
    { "hotpink", rgb(255, 105, 180) },
    { "indianred", rgb(205, 92, 92) },
    { "indigo", rgb( 75, 0, 130) },
    { "ivory", rgb(255, 255, 240) },
    { "khaki", rgb(240, 230, 140) },
    { "lavender", rgb(230, 230, 250) },
    { "lavenderblush", rgb(255, 240, 245) },
    { "lawngreen", rgb(124, 252, 0) },
    { "lemonchiffon", rgb(255, 250, 205) },
    { "lightblue", rgb(173, 216, 230) },
    { "lightcoral", rgb(240, 128, 128) },
    { "lightcyan", rgb(224, 255, 255) },
    { "lightgoldenrodyellow", rgb(250, 250, 210) },
    { "lightgray", rgb(211, 211, 211) },
    { "lightgreen", rgb(144, 238, 144) },
    { "lightgrey", rgb(211, 211, 211) },
    { "lightpink", rgb(255, 182, 193) },
    { "lightsalmon", rgb(255, 160, 122) },
    { "lightseagreen", rgb( 32, 178, 170) },
    { "lightskyblue", rgb(135, 206, 250) },
    { "lightslategray", rgb(119, 136, 153) },
    { "lightslategrey", rgb(119, 136, 153) },
    { "lightsteelblue", rgb(176, 196, 222) },
    { "lightyellow", rgb(255, 255, 224) },
    { "lime", rgb( 0, 255, 0) },
    { "limegreen", rgb( 50, 205, 50) },
    { "linen", rgb(250, 240, 230) },
    { "magenta", rgb(255, 0, 255) },
    { "maroon", rgb(128, 0, 0) },
    { "mediumaquamarine", rgb(102, 205, 170) },
    { "mediumblue", rgb( 0, 0, 205) },
    { "mediumorchid", rgb(186, 85, 211) },
    { "mediumpurple", rgb(147, 112, 219) },
    { "mediumseagreen", rgb( 60, 179, 113) },
    { "mediumslateblue", rgb(123, 104, 238) },
    { "mediumspringgreen", rgb( 0, 250, 154) },
    { "mediumturquoise", rgb( 72, 209, 204) },
    { "mediumvioletred", rgb(199, 21, 133) },
    { "midnightblue", rgb( 25, 25, 112) },
    { "mintcream", rgb(245, 255, 250) },
    { "mistyrose", rgb(255, 228, 225) },
    { "moccasin", rgb(255, 228, 181) },
    { "navajowhite", rgb(255, 222, 173) },
    { "navy", rgb( 0, 0, 128) },
    { "oldlace", rgb(253, 245, 230) },
    { "olive", rgb(128, 128, 0) },
    { "olivedrab", rgb(107, 142, 35) },
    { "orange", rgb(255, 165, 0) },
    { "orangered", rgb(255, 69, 0) },
    { "orchid", rgb(218, 112, 214) },
    { "palegoldenrod", rgb(238, 232, 170) },
    { "palegreen", rgb(152, 251, 152) },
    { "paleturquoise", rgb(175, 238, 238) },
    { "palevioletred", rgb(219, 112, 147) },
    { "papayawhip", rgb(255, 239, 213) },
    { "peachpuff", rgb(255, 218, 185) },
    { "peru", rgb(205, 133, 63) },
    { "pink", rgb(255, 192, 203) },
    { "plum", rgb(221, 160, 221) },
    { "powderblue", rgb(176, 224, 230) },
    { "purple", rgb(128, 0, 128) },
    { "red", rgb(255, 0, 0) },
    { "rosybrown", rgb(188, 143, 143) },
    { "royalblue", rgb( 65, 105, 225) },
    { "saddlebrown", rgb(139, 69, 19) },
    { "salmon", rgb(250, 128, 114) },
    { "sandybrown", rgb(244, 164, 96) },
    { "seagreen", rgb( 46, 139, 87) },
    { "seashell", rgb(255, 245, 238) },
    { "sienna", rgb(160, 82, 45) },
    { "silver", rgb(192, 192, 192) },
    { "skyblue", rgb(135, 206, 235) },
    { "slateblue", rgb(106, 90, 205) },
    { "slategray", rgb(112, 128, 144) },
    { "slategrey", rgb(112, 128, 144) },
    { "snow", rgb(255, 250, 250) },
    { "springgreen", rgb( 0, 255, 127) },
    { "steelblue", rgb( 70, 130, 180) },
    { "tan", rgb(210, 180, 140) },
    { "teal", rgb( 0, 128, 128) },
    { "thistle", rgb(216, 191, 216) },
    { "tomato", rgb(255, 99, 71) },
    { "transparent", 0 },
    { "turquoise", rgb( 64, 224, 208) },
    { "violet", rgb(238, 130, 238) },
    { "wheat", rgb(245, 222, 179) },
    { "white", rgb(255, 255, 255) },
    { "whitesmoke", rgb(245, 245, 245) },
    { "yellow", rgb(255, 255, 0) },
    { "yellowgreen", rgb(154, 205, 50) }
};
static const int rgbTblSize = sizeof(rgbTbl) / sizeof(RGBData);

#undef rgb

void tst_QColor::setNamedColor_data()
{
    QTest::addColumn<QColor>("byCtor");
    QTest::addColumn<QColor>("bySetNamedColor");
    QTest::addColumn<QColor>("expected");

    for (const auto e : rgbTbl) {
        QColor expected;
        expected.setRgba(e.value);

#define ROW(expr)                                \
        do {                                     \
            QColor bySetNamedColor;              \
            bySetNamedColor.setNamedColor(expr); \
            auto byCtor = QColor(expr);          \
            QTest::addRow("%s: %s", e.name, #expr) \
                << byCtor << bySetNamedColor << expected;    \
        } while (0)                              \
        /*end*/

        const auto l1 = QLatin1String(e.name);
        const auto l1UpperBA = QByteArray(e.name).toUpper();
        const auto l1Upper = QLatin1String(l1UpperBA);
        const auto l1SpaceBA = QByteArray(e.name).insert(1, ' ');
        const auto l1Space = QLatin1String(l1SpaceBA);

        const auto u16  = QString(l1);
        const auto u16Upper = u16.toUpper();
        const auto u16Space = QString(u16).insert(1, ' ');

        ROW(l1);
        ROW(u16);
        ROW(QStringView(u16));
        // name should be case insensitive
        ROW(l1Upper);
        ROW(u16Upper);
        ROW(QStringView(u16Upper));
        // spaces should be ignored
        ROW(l1Space);
        ROW(u16Space);
        ROW(QStringView(u16Space));
#undef ROW
    }
}

void tst_QColor::setNamedColor()
{
    QFETCH(QColor, byCtor);
    QFETCH(QColor, bySetNamedColor);
    QFETCH(QColor, expected);

    QCOMPARE(byCtor, expected);
    QCOMPARE(bySetNamedColor, expected);
}


void tst_QColor::constructNamedColorWithSpace()
{
    QColor whiteSmoke("white smoke");
    QCOMPARE(whiteSmoke, QColor(245, 245, 245));
}

void tst_QColor::colorNames()
{
    QStringList all = QColor::colorNames();
    QCOMPARE(all.size(), rgbTblSize);
    for (int i = 0; i < all.size(); ++i)
        QCOMPARE(all.at(i), QLatin1String(rgbTbl[i].name));
}

void tst_QColor::spec()
{
    QColor invalid;
    QCOMPARE(invalid.spec(), QColor::Invalid);

    QColor rgb = QColor::fromRgb(0, 0, 0);
    QCOMPARE(rgb.spec(), QColor::Rgb);

    QColor hsv = QColor::fromHsv(0, 0, 0);
    QCOMPARE(hsv.spec(), QColor::Hsv);

    QColor cmyk = QColor::fromCmyk(0, 0, 0, 0);
    QCOMPARE(cmyk.spec(), QColor::Cmyk);

    QColor hsl = QColor::fromHsl(0, 0, 0, 0);
    QCOMPARE(hsl.spec(), QColor::Hsl);

}

bool veryFuzzyCompare(double a, double b)
{
    return qAbs(a - b) < 0.01;
}

void tst_QColor::setRed()
{
    QColor c = QColor(Qt::blue).toHsv();
    c.setRed(127);
    QCOMPARE(c.red(), 127);
    QCOMPARE(c.green(), 0);
    QCOMPARE(c.blue(), 255);

    c = QColor(Qt::blue).toHsv();
    c.setRedF(0.5);
    QVERIFY(veryFuzzyCompare(c.redF(), 0.5));
    QCOMPARE(c.greenF(), qreal(0.0));
    QCOMPARE(c.blueF(), qreal(1.0));
}

void tst_QColor::setGreen()
{
    QColor c = QColor(Qt::blue).toHsv();
    c.setGreen(127);
    QCOMPARE(c.red(), 0);
    QCOMPARE(c.green(), 127);
    QCOMPARE(c.blue(), 255);

    c = QColor(Qt::blue).toHsv();
    c.setGreenF(0.5);
    QCOMPARE(c.redF(), qreal(0.0));
    QVERIFY(veryFuzzyCompare(c.greenF(), 0.5));
    QCOMPARE(c.blueF(), qreal(1.0));
}

void tst_QColor::setBlue()
{
    QColor c = QColor(Qt::red).toHsv();
    c.setBlue(127);
    QCOMPARE(c.red(), 255);
    QCOMPARE(c.green(), 0);
    QCOMPARE(c.blue(), 127);

    c = QColor(Qt::red).toHsv();
    c.setBlueF(0.5);
    QCOMPARE(c.redF(), qreal(1.0));
    QCOMPARE(c.greenF(), qreal(0.0));
    QVERIFY(veryFuzzyCompare(c.blueF(), 0.5));
}


void tst_QColor::setRgb()
{
    QColor color;

    for (int a = 0; a <= 255; ++a) {
        QRgb rgb = qRgba(0, 0, 0, a);

        color.setRgb(0, 0, 0, a);
        QCOMPARE(color.alpha(), a);
        QCOMPARE(color.rgb(),  qRgb(0, 0, 0));

        color.setRgb(rgb);
        QCOMPARE(color.alpha(), 255);
        QCOMPARE(color.rgb(),   qRgb(0, 0, 0));

        int r, g, b, a2;
        color.setRgb(0, 0, 0, a);
        color.getRgb(&r, &g, &b, &a2);
        QCOMPARE(a2, a);

        QColor c(0, 0, 0);
        c.setAlpha(a);
        QCOMPARE(c.alpha(), a);
    }

    for (int r = 0; r <= 255; ++r) {
        QRgb rgb = qRgb(r, 0, 0);

        color.setRgb(r, 0, 0);
        QCOMPARE(color.red(), r);
        QCOMPARE(color.rgb(), rgb);

        color.setRgb(rgb);
        QCOMPARE(color.red(), r);
        QCOMPARE(color.rgb(), rgb);

        int r2, g, b, a;
        color.getRgb(&r2, &g, &b, &a);
        QCOMPARE(r2, r);
    }

    for (int g = 0; g <= 255; ++g) {
        QRgb rgb = qRgb(0, g, 0);

        color.setRgb(0, g, 0);
        QCOMPARE(color.green(), g);
        QCOMPARE(color.rgb(),   rgb);

        color.setRgb(rgb);
        QCOMPARE(color.green(), g);
        QCOMPARE(color.rgb(),   rgb);

        int r, g2, b, a;
        color.getRgb(&r, &g2, &b, &a);
        QCOMPARE(g2, g);
    }

    for (int b = 0; b <= 255; ++b) {
        QRgb rgb = qRgb(0, 0, b);

        color.setRgb(0, 0, b);
        QCOMPARE(color.blue(),  b);
        QCOMPARE(color.rgb(),   rgb);

        color.setRgb(rgb);
        QCOMPARE(color.blue(),  b);
        QCOMPARE(color.rgb(),   rgb);

        int r, g, b2, a;
        color.getRgb(&r, &g, &b2, &a);
        QCOMPARE(b2, b);
    }
}

void tst_QColor::setRgbF()
{
    QColor color;

    for (int A = 0; A <= USHRT_MAX; ++A) {
        {
            // 0.0-1.0
            qreal a = A / qreal(USHRT_MAX);
            color.setRgbF(0.0, 0.0, 0.0, a);
            QCOMPARE(color.alphaF(), a);

            qreal r, g, b, a2;
            color.getRgbF(&r, &g, &b, &a2);
            QCOMPARE(a2, a);

            QColor c(0, 0, 0);
            c.setAlphaF(a);

            QCOMPARE(c.alphaF(), a);
        }
    }

    for (int R = 0; R <= USHRT_MAX; ++R) {
        {
            // 0.0-1.0
            qreal r = R / qreal(USHRT_MAX);
            color.setRgbF(r, 0.0, 0.0);
            QCOMPARE(color.redF(), r);

            qreal r2, g, b, a;
            color.getRgbF(&r2, &g, &b, &a);
            QCOMPARE(r2, r);
        }
    }

    for (int G = 0; G <= USHRT_MAX; ++G) {
        {
            // 0.0-1.0
            qreal g = G / qreal(USHRT_MAX);
            color.setRgbF(0.0, g, 0.0);
            QCOMPARE(color.greenF(), g);

            qreal r, g2, b, a;
            color.getRgbF(&r, &g2, &b, &a);
            QCOMPARE(g2, g);
        }
    }

    for (int B = 0; B <= USHRT_MAX; ++B) {
        {
            // 0.0-1.0
            qreal b = B / qreal(USHRT_MAX);
            color.setRgbF(0.0, 0.0, b);
            QCOMPARE(color.blueF(), b);

            qreal r, g, b2, a;
            color.getRgbF(&r, &g, &b2, &a);
            QCOMPARE(b2, b);
        }
    }

    for (int R = -128; R <= 512; ++R) {
        {
            // extended RGB
            qreal r = R / qreal(256);
            color.setRgbF(r, 0.0, 0.0);
            QCOMPARE(qfloat16(color.redF()), qfloat16(r));

            qreal r2, g, b, a;
            color.getRgbF(&r2, &g, &b, &a);
            QCOMPARE(qfloat16(r2), qfloat16(r));
        }
    }

    for (int G = -128; G <= 512; ++G) {
        {
            // extended RGB
            qreal g = G / qreal(256);
            color.setRgbF(0.0, g, 0.0);
            QCOMPARE(qfloat16(color.greenF()), qfloat16(g));

            qreal r, g2, b, a;
            color.getRgbF(&r, &g2, &b, &a);
            QCOMPARE(qfloat16(g2), qfloat16(g));
        }
    }

    for (int B = -128; B <= 512; ++B) {
        {
            // extended RGB
            qreal b = B / qreal(256);
            color.setRgbF(0.0, 0.0, b);
            QCOMPARE(qfloat16(color.blueF()), qfloat16(b));

            qreal r, g, b2, a;
            color.getRgbF(&r, &g, &b2, &a);
            QCOMPARE(qfloat16(b2), qfloat16(b));
        }
    }
}

void tst_QColor::setRgba()
{
    for (int a = 0; a < 255; ++a) {
        const QRgb rgba1 = qRgba(a, a, a, a);
        QColor color;
        color.setRgba(rgba1);
        QCOMPARE(color.alpha(), a);
        const QRgb rgba2 = color.rgba();
        QCOMPARE(rgba2, rgba1);
        QCOMPARE(qAlpha(rgba2), a);
    }
}

void tst_QColor::setHsv()
{
    QColor color;

    for (int A = 0; A <= USHRT_MAX; ++A) {
        {
            // 0-255
            int a = A >> 8;
            color.setHsv(0, 0, 0, a);
            QCOMPARE(color.alpha(), a);

            int h, s, v, a2;
            color.getHsv(&h, &s, &v, &a2);
            QCOMPARE(a2, a);
        }

        {
            // 0.0-1.0
            qreal a = A / qreal(USHRT_MAX);
            color.setHsvF(0.0, 0.0, 0.0, a); QCOMPARE(color.alphaF(), a);

            qreal h, s, v, a2;
            color.getHsvF(&h, &s, &v, &a2);
            QCOMPARE(a2, a);
        }
    }

    for (int H = 0; H < 36000; ++H) {
        {
            // 0-255
            int h = H / 100;

            color.setHsv(h, 0, 0, 0);
            QCOMPARE(color.hue(), h);

            int h2, s, v, a;
            color.getHsv(&h2, &s, &v, &a);
            QCOMPARE(h2, h);
        }

        {
            // 0.0-1.0
            qreal h = H / 36000.0;
            color.setHsvF(h, 0.0, 0.0, 0.0);
            QCOMPARE(color.hueF(), h);

            qreal h2, s, v, a;
            color.getHsvF(&h2, &s, &v, &a);
            QCOMPARE(h2, h);
        }
    }

    for (int S = 0; S <= USHRT_MAX; ++S) {
        {
            // 0-255
            int s = S >> 8;
            color.setHsv(0, s, 0, 0);
            QCOMPARE(color.saturation(), s);

            int h, s2, v, a;
            color.getHsv(&h, &s2, &v, &a);
            QCOMPARE(s2, s);
        }

        {
            // 0.0-1.0
            qreal s = S / qreal(USHRT_MAX);
            color.setHsvF(0.0, s, 0.0, 0.0);
            QCOMPARE(color.saturationF(), s);

            qreal h, s2, v, a;
            color.getHsvF(&h, &s2, &v, &a);
            QCOMPARE(s2, s);
        }
    }

    for (int V = 0; V <= USHRT_MAX; ++V) {
        {
            // 0-255
            int v = V >> 8;
            color.setHsv(0, 0, v, 0);
            QCOMPARE(color.value(),  v);

            int h, s, v2, a;
            color.getHsv(&h, &s, &v2, &a);
            QCOMPARE(v2, v);
        }

        {
            // 0.0-1.0
            qreal v = V / qreal(USHRT_MAX);
            color.setHsvF(0.0, 0.0, v, 0.0);
            QCOMPARE(color.valueF(), v);

            qreal h, s, v2, a;
            color.getHsvF(&h, &s, &v2, &a);
            QCOMPARE(v2, v);
        }
    }
}

void tst_QColor::setCmyk()
{
    QColor color;

    for (int A = 0; A <= USHRT_MAX; ++A) {
        {
            // 0-255
            int a = A >> 8;
            color.setCmyk(0, 0, 0, 0, a);
            QCOMPARE(color.alpha(), a);

            int c, m, y, k, a2;
            color.getCmyk(&c, &m, &y, &k, &a2);
            QCOMPARE(a2, a);
        }

        {
            // 0.0-1.0
            qreal a = A / qreal(USHRT_MAX);
            color.setCmykF(0.0, 0.0, 0.0, 0.0, a);
            QCOMPARE(color.alphaF(), a);

            qreal c, m, y, k, a2;
            color.getCmykF(&c, &m, &y, &k, &a2);
            QCOMPARE(a2, a);
        }
    }

    for (int C = 0; C <= USHRT_MAX; ++C) {
        {
            // 0-255
            int c = C >> 8;
            color.setCmyk(c, 0, 0, 0, 0);
            QCOMPARE(color.cyan(), c);

            int c2, m, y, k, a;
            color.getCmyk(&c2, &m, &y, &k, &a);
            QCOMPARE(c2, c);
        }

        {
            // 0.0-1.0
            qreal c = C / qreal(USHRT_MAX);
            color.setCmykF(c, 0.0, 0.0, 0.0, 0.0);
            QCOMPARE(color.cyanF(), c);

            qreal c2, m, y, k, a;
            color.getCmykF(&c2, &m, &y, &k, &a);
            QCOMPARE(c2, c);
        }
    }

    for (int M = 0; M <= USHRT_MAX; ++M) {
        {
            // 0-255
            int m = M >> 8;
            color.setCmyk(0, m, 0, 0, 0);
            QCOMPARE(color.magenta(), m);

            int c, m2, y, k, a;
            color.getCmyk(&c, &m2, &y, &k, &a);
            QCOMPARE(m2, m);
        }

        {
            // 0.0-1.0
            qreal m = M / qreal(USHRT_MAX);
            color.setCmykF(0.0, m, 0.0, 0.0, 0.0);
            QCOMPARE(color.magentaF(), m);

            qreal c, m2, y, k, a;
            color.getCmykF(&c, &m2, &y, &k, &a);
            QCOMPARE(m2, m);
        }
    }

    for (int Y = 0; Y <= USHRT_MAX; ++Y) {
        {
            // 0-255
            int y = Y >> 8;
            color.setCmyk(0, 0, y, 0, 0);
            QCOMPARE(color.yellow(), y);

            int c, m, y2, k, a;
            color.getCmyk(&c, &m, &y2, &k, &a);
            QCOMPARE(y2, y);
        }

        {
            // 0.0-1.0
            qreal y = Y / qreal(USHRT_MAX);
            color.setCmykF(0.0, 0.0, y, 0.0, 0.0);
            QCOMPARE(color.yellowF(), y);

            qreal c, m, y2, k, a;
            color.getCmykF(&c, &m, &y2, &k, &a);
            QCOMPARE(y2, y);
        }
    }

    for (int K = 0; K <= USHRT_MAX; ++K) {
        {
            // 0-255
            int k = K >> 8;
            color.setCmyk(0, 0, 0, k, 0);
            QCOMPARE(color.black(), k);

            int c, m, y, k2, a;
            color.getCmyk(&c, &m, &y, &k2, &a);
            QCOMPARE(k2, k);
        }

        {
            // 0.0-1.0
            qreal k = K / qreal(USHRT_MAX);
            color.setCmykF(0.0, 0.0, 0.0, k, 0.0);
            QCOMPARE(color.blackF(), k);

            qreal c, m, y, k2, a;
            color.getCmykF(&c, &m, &y, &k2, &a);
            QCOMPARE(k2, k);
        }
    }
}

void tst_QColor::setHsl()
{
    QColor color;

    for (int A = 0; A <= USHRT_MAX; ++A) {
        {
            // 0-255
            int a = A >> 8;
            color.setHsl(0, 0, 0, a);
            QCOMPARE(color.alpha(), a);

            int h, s, l, a2;
            color.getHsv(&h, &s, &l, &a2);
            QCOMPARE(a2, a);
        }

        {
            // 0.0-1.0
            qreal a = A / qreal(USHRT_MAX);
            color.setHslF(0.0, 0.0, 0.0, a); QCOMPARE(color.alphaF(), a);

            qreal h, s, l, a2;
            color.getHslF(&h, &s, &l, &a2);
            QCOMPARE(a2, a);
        }
    }

    for (int H = 0; H < 36000; ++H) {
        {
            // 0-255
            int h = H / 100;

            color.setHsl(h, 0, 0, 0);
            QCOMPARE(color.hslHue(), h);

            int h2, s, l, a;
            color.getHsl(&h2, &s, &l, &a);
            QCOMPARE(h2, h);
        }

        {
            // 0.0-1.0
            qreal h = H / 36000.0;
            color.setHslF(h, 0.0, 0.0, 0.0);
            QCOMPARE(color.hslHueF(), h);

            qreal h2, s, l, a;
            color.getHslF(&h2, &s, &l, &a);
            QCOMPARE(h2, h);
        }
    }

    for (int S = 0; S <= USHRT_MAX; ++S) {
        {
            // 0-255
            int s = S >> 8;
            color.setHsl(0, s, 0, 0);
            QCOMPARE(color.hslSaturation(), s);

            int h, s2, l, a;
            color.getHsl(&h, &s2, &l, &a);
            QCOMPARE(s2, s);
        }

        {
            // 0.0-1.0
            qreal s = S / qreal(USHRT_MAX);
            color.setHslF(0.0, s, 0.0, 0.0);
            QCOMPARE(color.hslSaturationF(), s);

            qreal h, s2, l, a;
            color.getHslF(&h, &s2, &l, &a);
            QCOMPARE(s2, s);
        }
    }

    for (int L = 0; L <= USHRT_MAX; ++L) {
        {
            // 0-255
            int l = L >> 8;
            color.setHsl(0, 0, l, 0);
            QCOMPARE(color.lightness(),  l);

            int h, s, l2, a;
            color.getHsl(&h, &s, &l2, &a);
            QCOMPARE(l2, l);
        }

        {
            // 0.0-1.0
            qreal l = L / qreal(USHRT_MAX);
            color.setHslF(0.0, 0.0, l, 0.0);
            QCOMPARE(color.lightnessF(), l);

            qreal h, s, l2, a;
            color.getHslF(&h, &s, &l2, &a);
            QCOMPARE(l2, l);
        }
    }
}

void tst_QColor::toRgb_data()
{
    QTest::addColumn<QColor>("expectedColor");
    QTest::addColumn<QColor>("hsvColor");
    QTest::addColumn<QColor>("cmykColor");
    QTest::addColumn<QColor>("hslColor");

    QTest::newRow("black")
        << QColor::fromRgbF(0.0, 0.0, 0.0)
        << QColor::fromHsvF(-1.0, 0.0, 0.0)
        << QColor::fromCmykF(0.0, 0.0, 0.0, 1.0)
        << QColor::fromHslF(-1.0, 0.0, 0.0);

    QTest::newRow("white")
        << QColor::fromRgbF(1.0, 1.0, 1.0)
        << QColor::fromHsvF(-1.0, 0.0, 1.0)
        << QColor::fromCmykF(0.0, 0.0, 0.0, 0.0)
        << QColor::fromHslF(-1.0, 0.0, 1.0);

    QTest::newRow("red")
        << QColor::fromRgbF(1.0, 0.0, 0.0)
        << QColor::fromHsvF(0.0, 1.0, 1.0)
        << QColor::fromCmykF(0.0, 1.0, 1.0, 0.0)
        << QColor::fromHslF(0.0, 1.0, 0.5, 1.0);

    QTest::newRow("green")
        << QColor::fromRgbF(0.0, 1.0, 0.0)
        << QColor::fromHsvF(0.33333, 1.0, 1.0)
        << QColor::fromCmykF(1.0, 0.0, 1.0, 0.0)
        << QColor::fromHslF(0.33333, 1.0, 0.5);

    QTest::newRow("blue")
        << QColor::fromRgbF(0.0, 0.0, 1.0)
        << QColor::fromHsvF(0.66667, 1.0, 1.0)
        << QColor::fromCmykF(1.0, 1.0, 0.0, 0.0)
        << QColor::fromHslF(0.66667, 1.0, 0.5);

    QTest::newRow("cyan")
        << QColor::fromRgbF(0.0, 1.0, 1.0)
        << QColor::fromHsvF(0.5, 1.0, 1.0)
        << QColor::fromCmykF(1.0, 0.0, 0.0, 0.0)
        << QColor::fromHslF(0.5, 1.0, 0.5);

    QTest::newRow("magenta")
        << QColor::fromRgbF(1.0, 0.0, 1.0)
        << QColor::fromHsvF(0.83333, 1.0, 1.0)
        << QColor::fromCmykF(0.0, 1.0, 0.0, 0.0)
        << QColor::fromHslF(0.83333, 1.0, 0.5);

    QTest::newRow("yellow")
        << QColor::fromRgbF(1.0, 1.0, 0.0)
        << QColor::fromHsvF(0.16667, 1.0, 1.0)
        << QColor::fromCmykF(0.0, 0.0, 1.0, 0.0)
        << QColor::fromHslF(0.16667, 1.0, 0.5);

    QTest::newRow("gray")
        << QColor::fromRgbF(0.6431375, 0.6431375, 0.6431375)
        << QColor::fromHsvF(-1.0, 0.0, 0.6431375)
        << QColor::fromCmykF(0.0, 0.0, 0.0, 0.356863)
        << QColor::fromHslF(-1.0, 0.0, 0.6431375);

    // ### add colors using the 0-255 functions
}

void tst_QColor::toRgb()
{
    // invalid should remain invalid
    QVERIFY(!QColor().toRgb().isValid());

    QFETCH(QColor, expectedColor);
    QFETCH(QColor, hsvColor);
    QFETCH(QColor, cmykColor);
    QFETCH(QColor, hslColor);
    QCOMPARE(hsvColor.toRgb(), expectedColor);
    QCOMPARE(cmykColor.toRgb(), expectedColor);
    QCOMPARE(hslColor.toRgb(), expectedColor);

}

void tst_QColor::toHsv_data()
{
    QTest::addColumn<QColor>("expectedColor");
    QTest::addColumn<QColor>("rgbColor");
    QTest::addColumn<QColor>("cmykColor");
    QTest::addColumn<QColor>("hslColor");

    QTest::newRow("data0")
        << QColor::fromHsv(300, 255, 255)
        << QColor(255, 0, 255)
        << QColor::fromCmyk(0, 255, 0, 0)
        << QColor::fromHslF(300./360., 1., 0.5, 1.0);

    QTest::newRow("data1")
        << QColor::fromHsvF(1., 1., 1., 1.)
        << QColor(255, 0, 0, 255)
        << QColor::fromCmykF(0., 1., 1., 0.)
        << QColor::fromHsvF(1., 1., 1., 1.);
}

void tst_QColor::toRgbNonDestructive()
{
    QColor aColor = QColor::fromRgbF(0.11, 0.22, 0.33, 0.44);
    QCOMPARE(aColor, aColor.toRgb());
}

void tst_QColor::toHsv()
{
    // invalid should remain invalid
    QVERIFY(!QColor().toHsv().isValid());

    QFETCH(QColor, expectedColor);
    QFETCH(QColor, rgbColor);
    QFETCH(QColor, cmykColor);
    QFETCH(QColor, hslColor);
    QCOMPARE(rgbColor.toHsv(), expectedColor);
    QCOMPARE(cmykColor.toHsv(), expectedColor);
    QCOMPARE(hslColor.toHsv(), expectedColor);
}

void tst_QColor::toHsvNonDestructive()
{
    QColor aColor = QColor::fromHsvF(0.11, 0.22, 0.33, 0.44);
    QCOMPARE(aColor, aColor.toHsv());
}

void tst_QColor::toCmyk_data()
{
    QTest::addColumn<QColor>("expectedColor");
    QTest::addColumn<QColor>("rgbColor");
    QTest::addColumn<QColor>("hsvColor");
    QTest::addColumn<QColor>("hslColor");

    QTest::newRow("data0")
        << QColor::fromCmykF(1.0, 0.0, 0.0, 0.0)
        << QColor(0, 255, 255)
        << QColor::fromHsv(180, 255, 255)
        << QColor::fromHslF(180./360., 1., 0.5, 1.0);

    QTest::newRow("data1")
        << QColor::fromCmyk(0, 0, 0, 255)
        << QColor::fromRgb(0, 0, 0)
        << QColor::fromRgb(0, 0, 0).toHsv()
        << QColor::fromRgb(0, 0, 0).toHsl();
}

void tst_QColor::toCmyk()
{
    // invalid should remain invalid
    QVERIFY(!QColor().toCmyk().isValid());

    QFETCH(QColor, expectedColor);
    QFETCH(QColor, rgbColor);
    QFETCH(QColor, hsvColor);
    QFETCH(QColor, hslColor);
    QCOMPARE(rgbColor.toHsv().toCmyk(), expectedColor);
    QCOMPARE(hsvColor.toCmyk(), expectedColor);
    QCOMPARE(hslColor.toCmyk(), expectedColor);
}

void tst_QColor::toCmykNonDestructive()
{
    QColor aColor = QColor::fromCmykF(0.11, 0.22, 0.33, 0.44);
    QCOMPARE(aColor, aColor.toCmyk());
}

void tst_QColor::toHsl_data()
{
    QTest::addColumn<QColor>("expectedColor");
    QTest::addColumn<QColor>("hsvColor");
    QTest::addColumn<QColor>("rgbColor");
    QTest::addColumn<QColor>("cmykColor");


    QTest::newRow("data0")
        << QColor::fromHslF(300./360., 1., 0.5, 1.0)
        << QColor::fromHsv(300, 255, 255)
        << QColor(255, 0, 255)
        << QColor::fromCmyk(0, 255, 0, 0);

    QTest::newRow("data1")
        << QColor::fromHslF(1., 1., 0.5, 1.0)
        << QColor::fromHsvF(1., 1., 1., 1.)
        << QColor(255, 0, 0, 255)
        << QColor::fromCmykF(0., 1., 1., 0.);
}

void tst_QColor::toHsl()
{
    // invalid should remain invalid
    QVERIFY(!QColor().toHsl().isValid());

    QFETCH(QColor, expectedColor);
    QFETCH(QColor, rgbColor);
    QFETCH(QColor, cmykColor);
    QFETCH(QColor, hsvColor);

    QCOMPARE(rgbColor.toHsl(), expectedColor);
    QCOMPARE(cmykColor.toHsl(), expectedColor);
    QCOMPARE(hsvColor.toHsl(), expectedColor);

}


void tst_QColor::toHslNonDestructive()
{
    QColor aColor = QColor::fromHslF(0.11, 0.22, 0.33, 0.44);
    QCOMPARE(aColor, aColor.toHsl());
}


void tst_QColor::convertTo()
{
    QColor color(Qt::black);

    QColor rgb = color.convertTo(QColor::Rgb);
    QCOMPARE(rgb.spec(), QColor::Rgb);

    QColor hsv = color.convertTo(QColor::Hsv);
    QCOMPARE(hsv.spec(), QColor::Hsv);

    QColor cmyk = color.convertTo(QColor::Cmyk);
    QCOMPARE(cmyk.spec(), QColor::Cmyk);

    QColor hsl = color.convertTo(QColor::Hsl);
    QCOMPARE(hsl.spec(), QColor::Hsl);

    QColor invalid = color.convertTo(QColor::Invalid);
    QCOMPARE(invalid.spec(), QColor::Invalid);
}

void tst_QColor::lighter()
{
    QColor gray(Qt::gray);
    QColor lighter = gray.lighter();
    QVERIFY(lighter.value() > gray.value());
}

void tst_QColor::darker()
{
    QColor gray(Qt::gray);
    QColor darker = gray.darker();
    QVERIFY(darker.value() < gray.value());
}

Q_DECLARE_METATYPE(QColor::Spec);

void tst_QColor::specConstructor_data()
{
    QTest::addColumn<QColor::Spec>("spec");

    QTest::newRow("Invalid") << QColor::Invalid;
    QTest::newRow("Rgb") << QColor::Rgb;
    QTest::newRow("Hsv") << QColor::Hsv;
    QTest::newRow("Cmyk") << QColor::Cmyk;
}

void tst_QColor::specConstructor()
{
    QFETCH(QColor::Spec, spec);
    QColor color = spec;
    QCOMPARE(color.spec(), spec);
}

void tst_QColor::achromaticHslHue()
{
    QColor color = Qt::black;

    QColor hsl = color.toHsl();
    QCOMPARE(hsl.hslHue(), -1);
}

void tst_QColor::equality()
{
    QColor red = Qt::red;
    QColor black = Qt::black;

    QCOMPARE(red, red);
    QCOMPARE(black, black);
    QVERIFY(red != black);

    // Encodings must match
    QVERIFY(red != red.toHsv());
    QVERIFY(black.toHsl() != black);

    // Except for ExtendedRgb and Rgb, as it can be an automatic upgrade.
    QCOMPARE(red, red.toExtendedRgb());
    QCOMPARE(black.toExtendedRgb(), black);
}

void tst_QColor::premultiply()
{
    // Tests that qPremultiply(qUnpremultiply(x)) returns x.
    for (uint a = 0; a < 256; a++) {
        for (uint c = 0; c <= a; c++) {
            QRgb p = qRgba(c, a-c, c, a);
            QCOMPARE(p, qPremultiply(qUnpremultiply(p)));
        }
    }
}

void tst_QColor::unpremultiply_sse4()
{
    // Tests that qUnpremultiply_sse4 returns the same as qUnpremultiply.
#if QT_COMPILER_SUPPORTS_HERE(SSE4_1)
    if (qCpuHasFeature(SSE4_1)) {
        int minorDifferences = 0;
        for (uint a = 0; a < 256; a++) {
            for (uint c = 0; c <= a; c++) {
                const QRgb p = qRgba(c, a-c, c/2, a);
                const uint u = qUnpremultiply(p);
                const uint usse4 = qUnpremultiply_sse4(p);
                if (u != usse4) {
                    QCOMPARE(qAlpha(u), qAlpha(usse4));
                    QVERIFY(qAbs(qRed(u) - qRed(usse4)) <= 1);
                    QVERIFY(qAbs(qGreen(u) - qGreen(usse4)) <= 1);
                    QVERIFY(qAbs(qBlue(u) - qBlue(usse4)) <= 1);
                    ++minorDifferences;
                }
            }
        }
        // Allow a few rounding differences as long as it still obeys
        // the qPremultiply(qUnpremultiply(x)) == x invariant
        QVERIFY(minorDifferences <= 16 * 255);
        for (uint a = 0; a < 256; a++) {
            for (uint c = 0; c <= a; c++) {
                QRgb p = qRgba(c, a-c, c, a);
                QCOMPARE(p, qPremultiply(qUnpremultiply_sse4(p)));
            }
        }
        return;
    }
#endif
    QSKIP("SSE4 not supported on this CPU.");
}

void tst_QColor::qrgba64()
{
    QRgba64 rgb64 = QRgba64::fromRgba(0x22, 0x33, 0x44, 0xff);
    QCOMPARE(rgb64.red(), quint16(0x2222));
    QCOMPARE(rgb64.green(), quint16(0x3333));
    QCOMPARE(rgb64.blue(), quint16(0x4444));
    QCOMPARE(rgb64.alpha(), quint16(0xffff));

    QColor c(rgb64);
    QCOMPARE(c.red(), 0x22);
    QCOMPARE(c.green(), 0x33);
    QCOMPARE(c.blue(), 0x44);

    QCOMPARE(c.rgba64(), rgb64);

    QColor c2 = QColor::fromRgb(0x22, 0x33, 0x44, 0xff);
    QCOMPARE(c, c2);
    QCOMPARE(c2.rgba64(), rgb64);

    rgb64.setAlpha(0x8000);
    rgb64.setGreen(0x8844);
    rgb64 = rgb64.premultiplied();
    QCOMPARE(rgb64.red(), quint16(0x1111));
    QCOMPARE(rgb64.blue(), quint16(0x2222));
    QCOMPARE(rgb64.green(), quint16(0x4422));
}

void tst_QColor::qrgba64MemoryLayout()
{
    QRgba64 rgb64 = QRgba64::fromRgba64(0x0123, 0x4567, 0x89ab, 0xcdef);
    QCOMPARE(rgb64.red(), quint16(0x0123));
    QCOMPARE(rgb64.green(), quint16(0x4567));
    QCOMPARE(rgb64.blue(), quint16(0x89ab));
    QCOMPARE(rgb64.alpha(), quint16(0xcdef));

    // Check in-memory order, so it can be used by things like SSE
    Q_STATIC_ASSERT(sizeof(QRgba64) == sizeof(quint64));
    quint16 memory[4];
    memcpy(memory, &rgb64, sizeof(QRgba64));
    QCOMPARE(memory[0], quint16(0x0123));
    QCOMPARE(memory[1], quint16(0x4567));
    QCOMPARE(memory[2], quint16(0x89ab));
    QCOMPARE(memory[3], quint16(0xcdef));
}

void tst_QColor::qrgba64Premultiply()
{
    // Tests that qPremultiply(qUnpremultiply(rgba64)) returns rgba64.
    for (uint a = 0; a < 0x10000; a+=7) {
        const uint step = std::max(a/1024, 1u);
        for (uint c = 0; c <= a; c+=step) {
            QRgba64 p = qRgba64(c, a-c, a-c/2, a);
            QRgba64 pp = qPremultiply(qUnpremultiply(p));
            QCOMPARE(pp, p);
        }
    }
}

void tst_QColor::qrgba64Equivalence()
{
    // Any ARGB32 converted back and forth.
    for (uint a = 0; a < 256; a++) {
        for (uint c = 0; c < 256; c++) {
            QRgb p1 = qRgba(c, 255-c, 255-c, a);
            QRgba64 p64 = QRgba64::fromArgb32(p1);
            QCOMPARE(p64.toArgb32(), p1);
        }
    }
    // Any unpremultiplied ARGB32 value premultipled in RGB64 (except alpha 0).
    for (uint a = 1; a < 256; a++) {
        for (uint c = 0; c < 256; c++) {
            QRgb p1 = qRgba(c, 255-c, 255-c, a);
            QRgb pp1 = qPremultiply(p1);
            QRgba64 pp64 = qPremultiply(QRgba64::fromArgb32(p1));
            QRgb pp2 = pp64.toArgb32();
            // 64bit premultiplied is more accurate than 32bit, so allow slight difference.
            QCOMPARE(qAlpha(pp2), qAlpha(pp1));
            QVERIFY(qAbs(qRed(pp2)-qRed(pp1)) <= 1);
            QVERIFY(qAbs(qGreen(pp2)-qGreen(pp1)) <= 1);
            QVERIFY(qAbs(qBlue(pp2)-qBlue(pp1)) <= 1);
            // But verify the added accuracy means we can return to accurate unpremultiplied ARGB32.
            QRgba64 pu64 = qUnpremultiply(pp64);
            QRgb p2 = pu64.toArgb32();
            QCOMPARE(p2, p1);
        }
    }
    // Any premultiplied ARGB32 value unpremultipled in RGB64.
    for (uint a = 0; a < 256; a++) {
        for (uint c = 0; c <= a; c++) {
            QRgb pp = qRgba(c, a-c, a-c, a);
            QRgb pu = qUnpremultiply(pp);
            QRgba64 pu64 = qUnpremultiply(QRgba64::fromArgb32(pp));
            QCOMPARE(pu64.toArgb32(), pu);
        }
    }
}

void tst_QColor::qcolorprofile_data()
{
    QTest::addColumn<qreal>("gammaC");
    QTest::addColumn<int>("tolerance");

    QTest::newRow("gamma=1.0") << qreal(1.0) << 0;
    QTest::newRow("gamma=1.5") << qreal(1.5) << 1;
    QTest::newRow("gamma=1.7") << qreal(1.7) << 2;
    QTest::newRow("gamma=2.0") << qreal(2.0) << 8;
    QTest::newRow("gamma=2.31") << qreal(2.31) << 33;
}

void tst_QColor::qcolorprofile()
{
    QFETCH(qreal, gammaC);
    QFETCH(int, tolerance);
    QColorTrcLut *cp = QColorTrcLut::fromGamma(gammaC);

    // Test we are accurate for most values after converting through gamma-correction.
    int error = 0;
    for (uint i = 0; i < 256; i++) {
        QRgb cin = qRgb(i, i, i);
        QRgba64 tmp = cp->toLinear64(cin);
        QRgb cout = cp->fromLinear64(tmp);
        error += qAbs(qRed(cin) - qRed(cout));
    }
    QVERIFY(error <= tolerance);
    delete cp;
}

QTEST_MAIN(tst_QColor)
#include "tst_qcolor.moc"
