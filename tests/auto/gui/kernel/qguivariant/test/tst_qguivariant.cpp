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

#include <qvariant.h>

#include <qkeysequence.h>
#include <qbitmap.h>
#include <qcursor.h>
#include <qimage.h>
#include <qicon.h>
#include <qmatrix.h>
#include <qmatrix4x4.h>
#include <qpen.h>
#include <qpolygon.h>
#include <qpalette.h>
#include <qtransform.h>
#include <qvector2d.h>
#include <qvector3d.h>
#include <qvector4d.h>
#include <qquaternion.h>
#include <qtextdocument.h>
#include <qtextformat.h>
#include <qfont.h>

#include "tst_qvariant_common.h"

#include "../../../../qtest-config.h"

class tst_QGuiVariant : public QObject
{
    Q_OBJECT

private slots:
    void constructor_invalid_data();
    void constructor_invalid();

    void canConvert_data();
    void canConvert();

    void toInt_data();
    void toInt();

    void toFont_data();
    void toFont();

    void toKeySequence_data();
    void toKeySequence();

    void toString_data();
    void toString();

    void toColor_data();
    void toColor();

    void toPixmap_data();
    void toPixmap();

    void toImage_data();
    void toImage();

    void toBrush_data();
    void toBrush();

    void matrix();

    void transform();

    void matrix4x4();
    void vector2D();
    void vector3D();
    void vector4D();
    void quaternion();

    void writeToReadFromDataStream_data();
    void writeToReadFromDataStream();
    void writeToReadFromOldDataStream();

    void colorInteger();
    void invalidQColor();
    void validQColor();

    void debugStream_data();
    void debugStream();

    void implicitConstruction();

    void guiVariantAtExit();

    void iconEquality();
};

void tst_QGuiVariant::constructor_invalid_data()
{
    QTest::addColumn<uint>("typeId");

    QTest::newRow("LastGuiType + 1") << uint(QMetaType::LastGuiType + 1);
    QVERIFY(!QMetaType::isRegistered(QMetaType::LastGuiType + 1));
    QTest::newRow("LastWidgetsType + 1") << uint(QMetaType::LastWidgetsType + 1);
    QVERIFY(!QMetaType::isRegistered(QMetaType::LastWidgetsType + 1));
}

void tst_QGuiVariant::constructor_invalid()
{

    QFETCH(uint, typeId);
    {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("^Trying to construct an instance of an invalid type, type id:"));
        QVariant variant(static_cast<QVariant::Type>(typeId));
        QVERIFY(!variant.isValid());
        QCOMPARE(variant.userType(), int(QMetaType::UnknownType));
    }
    {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("^Trying to construct an instance of an invalid type, type id:"));
        QVariant variant(typeId, /* copy */ 0);
        QVERIFY(!variant.isValid());
        QCOMPARE(variant.userType(), int(QMetaType::UnknownType));
    }
}

void tst_QGuiVariant::canConvert_data()
{
    TST_QVARIANT_CANCONVERT_DATATABLE_HEADERS

#ifdef Y
#undef Y
#endif
#ifdef N
#undef N
#endif
#define Y true
#define N false

    QVariant var;

    //            bita bitm bool brsh byta col  curs date dt   dbl  font img  int  inv  kseq list ll   map  pal  pen  pix  pnt  rect reg  size sp   str  strl time uint ull


    var = QVariant::fromValue(QBitmap());
    QTest::newRow("Bitmap")
        << var << N << Y << N << N << N << N << N << N << N << N << N << Y << N << N << N << N << N << N << N << N << Y << N << N << N << N << N << N << N << N << N << N;
    var = QVariant::fromValue(QBrush());
    QTest::newRow("Brush")
        << var << N << N << N << Y << N << Y << N << N << N << N << N << N << N << N << N << N << N << N << N << N << Y << N << N << N << N << N << N << N << N << N << N;
    var = QVariant::fromValue(QColor());
    QTest::newRow("Color")
        << var << N << N << N << Y << Y << Y << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << Y << N << N << N << N;
#ifndef QTEST_NO_CURSOR
    var = QVariant::fromValue(QCursor());
    QTest::newRow("Cursor")
        << var << N << N << N << N << N << N << Y << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N;
#endif
    var = QVariant::fromValue(QFont());
    QTest::newRow("Font")
        << var << N << N << N << N << N << N << N << N << N << N << Y << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << Y << N << N << N << N;
    var = QVariant::fromValue(QIcon());
    QTest::newRow("Icon")
        << var << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N;
    var = QVariant::fromValue(QImage());
    QTest::newRow("Image")
        << var << N << Y << N << N << N << N << N << N << N << N << N << Y << N << N << N << N << N << N << N << N << Y << N << N << N << N << N << N << N << N << N << N;
    var = QVariant::fromValue(QKeySequence());
    QTest::newRow("KeySequence")
        << var << N << N << N << N << N << N << N << N << N << N << N << N << Y << N << Y << N << N << N << N << N << N << N << N << N << N << N << Y << N << N << N << N;
    var = QVariant::fromValue(QPalette());
    QTest::newRow("Palette")
        << var << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << Y << N << N << N << N << N << N << N << N << N << N << N << N;
    var = QVariant::fromValue(QPen());
    QTest::newRow("Pen")
        << var << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << Y << N << N << N << N << N << N << N << N << N << N << N;
    var = QVariant::fromValue(QPixmap());
    QTest::newRow("Pixmap")
        << var << N << Y << N << Y << N << N << N << N << N << N << N << Y << N << N << N << N << N << N << N << N << Y << N << N << N << N << N << N << N << N << N << N;
    var = QVariant::fromValue(QPolygon());
    QTest::newRow("PointArray")
        << var << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N;
    var = QVariant::fromValue(QRegion());
    QTest::newRow("Region")
        << var << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << Y << N << N << N << N << N << N << N;

#undef N
#undef Y
}

void tst_QGuiVariant::canConvert()
{
    TST_QVARIANT_CANCONVERT_FETCH_DATA

    TST_QVARIANT_CANCONVERT_COMPARE_DATA
}

void tst_QGuiVariant::toInt_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<int>("result");
    QTest::addColumn<bool>("valueOK");

    QTest::newRow( "keysequence" ) << QVariant::fromValue( QKeySequence( Qt::Key_A ) ) << 65 << true;
}

void tst_QGuiVariant::toInt()
{
    QFETCH( QVariant, value );
    QFETCH( int, result );
    QFETCH( bool, valueOK );
    QVERIFY( value.isValid() == value.canConvert( QVariant::Int ) );
    bool ok;
    int i = value.toInt( &ok );
    QCOMPARE( i, result );
    QVERIFY( ok == valueOK );
}

void tst_QGuiVariant::toColor_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QColor>("result");

    QColor c("red");
    QTest::newRow( "string" ) << QVariant( QString( "red" ) ) << c;
    QTest::newRow( "solid brush" ) << QVariant( QBrush(c) ) << c;
    QTest::newRow("qbytearray") << QVariant(QByteArray("red")) << c;
    QTest::newRow("same color") << QVariant(c) << c;
    QTest::newRow("qstring(#ff0000)") << QVariant(QString::fromUtf8("#ff0000")) << c;
    QTest::newRow("qbytearray(#ff0000)") << QVariant(QByteArray("#ff0000")) << c;

    c.setNamedColor("#88112233");
    QTest::newRow("qstring(#88112233)") << QVariant(QString::fromUtf8("#88112233")) << c;
    QTest::newRow("qbytearray(#88112233)") << QVariant(QByteArray("#88112233")) << c;
}

void tst_QGuiVariant::toColor()
{
    QFETCH( QVariant, value );
    QFETCH( QColor, result );
    QVERIFY( value.isValid() );
    QVERIFY( value.canConvert( QVariant::Color ) );
    QColor d = qvariant_cast<QColor>(value);
    QCOMPARE( d, result );
    QVERIFY(value.convert(QMetaType::QColor));
    QCOMPARE(d, QColor(value.toString()));
}

void tst_QGuiVariant::toPixmap_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QPixmap>("result");

    QPixmap pm(30, 30);
    pm.fill(Qt::red);
    QTest::newRow( "image" ) << QVariant( pm ) << pm;

    QBitmap bm(30, 30);
    bm.fill(Qt::color1);
    QTest::newRow( "bitmap" ) << QVariant( bm ) << QPixmap(bm);
}

void tst_QGuiVariant::toPixmap()
{
    QFETCH( QVariant, value );
    QFETCH( QPixmap, result );
    QVERIFY( value.isValid() );
    QVERIFY( value.canConvert( QVariant::Pixmap ) );
    QPixmap d = qvariant_cast<QPixmap>(value);
    QCOMPARE( d, result );
}

void tst_QGuiVariant::toImage_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QImage>("result");

    QImage im(30, 30, QImage::Format_ARGB32);
    im.fill(0x7fff0000);
    QTest::newRow( "image" ) << QVariant( im ) << im;
}

void tst_QGuiVariant::toImage()
{
    QFETCH( QVariant, value );
    QFETCH( QImage, result );
    QVERIFY( value.isValid() );
    QVERIFY( value.canConvert( QVariant::Image ) );
    QImage d = qvariant_cast<QImage>(value);
    QCOMPARE( d, result );
}

void tst_QGuiVariant::toBrush_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QBrush>("result");

    QColor c(Qt::red);
    QTest::newRow( "color" ) << QVariant( c ) << QBrush(c);
    QPixmap pm(30, 30);
    pm.fill(c);
    QTest::newRow( "pixmap" ) << QVariant( pm ) << QBrush(pm);
}

void tst_QGuiVariant::toBrush()
{
    QFETCH( QVariant, value );
    QFETCH( QBrush, result );
    QVERIFY( value.isValid() );
    QVERIFY( value.canConvert( QVariant::Brush ) );
    QBrush d = qvariant_cast<QBrush>(value);
    QCOMPARE( d, result );
}

void tst_QGuiVariant::toFont_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QFont>("result");

    QFont f("times",12,-1,false);
    QTest::newRow( "string" ) << QVariant( QString( "times,12,-1,5,50,0,0,0,0,0" ) ) << f;
}

void tst_QGuiVariant::toFont()
{
    QFETCH( QVariant, value );
    QFETCH( QFont, result );
    QVERIFY( value.isValid() );
    QVERIFY( value.canConvert( QVariant::Font ) );
    QFont d = qvariant_cast<QFont>(value);
    QCOMPARE( d, result );
}

void tst_QGuiVariant::toKeySequence_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QKeySequence>("result");


    QTest::newRow( "int" ) << QVariant( 67108929 ) << QKeySequence( Qt::CTRL + Qt::Key_A );


    QTest::newRow( "qstring" )
        << QVariant( QString( "Ctrl+A" ) )
        << QKeySequence( Qt::CTRL + Qt::Key_A );
}

void tst_QGuiVariant::toKeySequence()
{
    QFETCH( QVariant, value );
    QFETCH( QKeySequence, result );
    QVERIFY( value.isValid() );
    QVERIFY( value.canConvert( QVariant::KeySequence ) );
    QKeySequence d = qvariant_cast<QKeySequence>(value);
    QCOMPARE( d, result );
}

void tst_QGuiVariant::toString_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QString>("result");

    QTest::newRow( "qkeysequence" ) << QVariant::fromValue( QKeySequence( Qt::CTRL + Qt::Key_A ) )
#ifndef Q_OS_MAC
        << QString( "Ctrl+A" );
#else
        << QString(QChar(0x2318)) + QLatin1Char('A');
#endif

    QFont font( "times", 12 );
    QTest::newRow( "qfont" ) << QVariant::fromValue( font ) << QString("times,12,-1,5,50,0,0,0,0,0");
    QTest::newRow( "qcolor" ) << QVariant::fromValue( QColor( 10, 10, 10 ) ) << QString( "#0a0a0a" );
}

void tst_QGuiVariant::toString()
{
    QFETCH( QVariant, value );
    QFETCH( QString, result );
    QVERIFY( value.isValid() );
    QVERIFY( value.canConvert( QVariant::String ) );
    QString str = value.toString();
    QCOMPARE( str, result );
}

void tst_QGuiVariant::matrix()
{
    QVariant variant;
    QMatrix matrix = qvariant_cast<QMatrix>(variant);
    QVERIFY(matrix.isIdentity());
    variant.setValue(QMatrix().rotate(90));
    QCOMPARE(QMatrix().rotate(90), qvariant_cast<QMatrix>(variant));

    void *mmatrix = QMetaType::create(QVariant::Matrix, 0);
    QVERIFY(mmatrix);
    QMetaType::destroy(QVariant::Matrix, mmatrix);
}

void tst_QGuiVariant::matrix4x4()
{
    QVariant variant;
    QMatrix4x4 matrix = qvariant_cast<QMatrix4x4>(variant);
    QVERIFY(matrix.isIdentity());
    QMatrix4x4 m;
    m.scale(2.0f);
    variant.setValue(m);
    QCOMPARE(m, qvariant_cast<QMatrix4x4>(variant));

    void *mmatrix = QMetaType::create(QVariant::Matrix4x4, 0);
    QVERIFY(mmatrix);
    QMetaType::destroy(QVariant::Matrix4x4, mmatrix);
}

void tst_QGuiVariant::transform()
{
    QVariant variant;
    QTransform matrix = qvariant_cast<QTransform>(variant);
    QVERIFY(matrix.isIdentity());
    variant.setValue(QTransform().rotate(90));
    QCOMPARE(QTransform().rotate(90), qvariant_cast<QTransform>(variant));

    void *mmatrix = QMetaType::create(QVariant::Transform, 0);
    QVERIFY(mmatrix);
    QMetaType::destroy(QVariant::Transform, mmatrix);
}


void tst_QGuiVariant::vector2D()
{
    QVariant variant;
    QVector2D vector = qvariant_cast<QVector2D>(variant);
    QVERIFY(vector.isNull());
    variant.setValue(QVector2D(0.1f, 0.2f));
    QCOMPARE(QVector2D(0.1f, 0.2f), qvariant_cast<QVector2D>(variant));

    void *pvector = QMetaType::create(QVariant::Vector2D, 0);
    QVERIFY(pvector);
    QMetaType::destroy(QVariant::Vector2D, pvector);
}

void tst_QGuiVariant::vector3D()
{
    QVariant variant;
    QVector3D vector = qvariant_cast<QVector3D>(variant);
    QVERIFY(vector.isNull());
    variant.setValue(QVector3D(0.1f, 0.2f, 0.3f));
    QCOMPARE(QVector3D(0.1f, 0.2f, 0.3f), qvariant_cast<QVector3D>(variant));

    void *pvector = QMetaType::create(QVariant::Vector3D, 0);
    QVERIFY(pvector);
    QMetaType::destroy(QVariant::Vector3D, pvector);
}

void tst_QGuiVariant::vector4D()
{
    QVariant variant;
    QVector4D vector = qvariant_cast<QVector4D>(variant);
    QVERIFY(vector.isNull());
    variant.setValue(QVector4D(0.1f, 0.2f, 0.3f, 0.4f));
    QCOMPARE(QVector4D(0.1f, 0.2f, 0.3f, 0.4f), qvariant_cast<QVector4D>(variant));

    void *pvector = QMetaType::create(QVariant::Vector4D, 0);
    QVERIFY(pvector);
    QMetaType::destroy(QVariant::Vector4D, pvector);
}

void tst_QGuiVariant::quaternion()
{
    QVariant variant;
    QQuaternion quaternion = qvariant_cast<QQuaternion>(variant);
    QVERIFY(quaternion.isIdentity());
    variant.setValue(QQuaternion(0.1f, 0.2f, 0.3f, 0.4f));
    QCOMPARE(QQuaternion(0.1f, 0.2f, 0.3f, 0.4f), qvariant_cast<QQuaternion>(variant));

    void *pquaternion = QMetaType::create(QVariant::Quaternion, 0);
    QVERIFY(pquaternion);
    QMetaType::destroy(QVariant::Quaternion, pquaternion);
}

void tst_QGuiVariant::writeToReadFromDataStream_data()
{
    QTest::addColumn<QVariant>("writeVariant");
    QTest::addColumn<bool>("isNull");

    QTest::newRow( "bitmap_invalid" ) << QVariant::fromValue( QBitmap() ) << true;
    QBitmap bitmap( 10, 10 );
    bitmap.fill( Qt::red );
    QTest::newRow( "bitmap_valid" ) << QVariant::fromValue( bitmap ) << false;
    QTest::newRow( "brush_valid" ) << QVariant::fromValue( QBrush( Qt::red ) ) << false;
    QTest::newRow( "color_valid" ) << QVariant::fromValue( QColor( Qt::red ) ) << false;
#ifndef QTEST_NO_CURSOR
    QTest::newRow( "cursor_valid" ) << QVariant::fromValue( QCursor( Qt::PointingHandCursor ) ) << false;
#endif
    QTest::newRow( "font_valid" ) << QVariant::fromValue( QFont( "times", 12 ) ) << false;
    QTest::newRow( "pixmap_invalid" ) << QVariant::fromValue( QPixmap() ) << true;
    QPixmap pixmap( 10, 10 );
    pixmap.fill( Qt::red );
    QTest::newRow( "pixmap_valid" ) << QVariant::fromValue( pixmap ) << false;
    QTest::newRow( "image_invalid" ) << QVariant::fromValue( QImage() ) << true;
    QTest::newRow( "keysequence_valid" ) << QVariant::fromValue( QKeySequence( Qt::CTRL + Qt::Key_A ) ) << false;
    QTest::newRow( "palette_valid" ) << QVariant::fromValue(QPalette(QColor("turquoise"))) << false;
    QTest::newRow( "pen_valid" ) << QVariant::fromValue( QPen( Qt::red ) ) << false;
    QTest::newRow( "pointarray_invalid" ) << QVariant::fromValue( QPolygon() ) << true;
    QTest::newRow( "pointarray_valid" ) << QVariant::fromValue( QPolygon( QRect( 10, 10, 20, 20 ) ) ) << false;
    QTest::newRow( "region_invalid" ) << QVariant::fromValue( QRegion() ) << true;
    QTest::newRow( "region_valid" ) << QVariant::fromValue( QRegion( 10, 10, 20, 20 ) ) << false;
    QTest::newRow("polygonf_invalid") << QVariant::fromValue(QPolygonF()) << true;
    QTest::newRow("polygonf_valid") << QVariant::fromValue(QPolygonF(QRectF(10, 10, 20, 20))) << false;
}

void tst_QGuiVariant::invalidQColor()
{
    QVariant va("An invalid QColor::name() value.");
    QVERIFY(va.canConvert(QVariant::Color));

    QVERIFY(!va.convert(QVariant::Color));

    QVERIFY(!qvariant_cast<QColor>(va).isValid());
}

void tst_QGuiVariant::validQColor()
{
    QColor col(Qt::red);
    QVariant va(col.name());
    QVERIFY(va.canConvert(QVariant::Color));

    QVERIFY(va.convert(QVariant::Color));

    QVERIFY(col.isValid());

    QVERIFY(va.convert(QVariant::String));

    QCOMPARE(qvariant_cast<QString>(va), col.name());
}

void tst_QGuiVariant::colorInteger()
{
    QVariant v = QColor(Qt::red);
    QCOMPARE(v.type(), QVariant::Color);
    QCOMPARE(v.value<QColor>(), QColor(Qt::red));

    v.setValue(1000);
    QCOMPARE(v.type(), QVariant::Int);
    QCOMPARE(v.toInt(), 1000);

    v.setValue(QColor(Qt::yellow));
    QCOMPARE(v.type(), QVariant::Color);
    QCOMPARE(v.value<QColor>(), QColor(Qt::yellow));
}

void tst_QGuiVariant::writeToReadFromDataStream()
{
    QFETCH( QVariant, writeVariant );
    QFETCH( bool, isNull );
    QByteArray data;

    QDataStream writeStream( &data, QIODevice::WriteOnly );
    writeStream << writeVariant;

    QVariant readVariant;
    QDataStream readStream( &data, QIODevice::ReadOnly );
    readStream >> readVariant;
    QVERIFY( readVariant.isNull() == isNull );
    // Best way to confirm the readVariant contains the same data?
    // Since only a few won't match since the serial numbers are different
    // I won't bother adding another bool in the data test.
    const int writeType = writeVariant.userType();
    if ( writeType != QVariant::Invalid && writeType != QVariant::Bitmap && writeType != QVariant::Pixmap
        && writeType != QVariant::Image) {
        switch (writeType) {
        default:
            QCOMPARE( readVariant, writeVariant );
            break;

        // compare types know by QMetaType but not QVariant (QVariant::operator==() knows nothing about them)
        case QMetaType::Long:
            QCOMPARE(qvariant_cast<long>(readVariant), qvariant_cast<long>(writeVariant));
            break;
        case QMetaType::ULong:
            QCOMPARE(qvariant_cast<ulong>(readVariant), qvariant_cast<ulong>(writeVariant));
            break;
        case QMetaType::Short:
            QCOMPARE(qvariant_cast<short>(readVariant), qvariant_cast<short>(writeVariant));
            break;
        case QMetaType::UShort:
            QCOMPARE(qvariant_cast<ushort>(readVariant), qvariant_cast<ushort>(writeVariant));
            break;
        case QMetaType::Char:
            QCOMPARE(qvariant_cast<char>(readVariant), qvariant_cast<char>(writeVariant));
            break;
        case QMetaType::UChar:
            QCOMPARE(qvariant_cast<uchar>(readVariant), qvariant_cast<uchar>(writeVariant));
            break;
        case QMetaType::Float:
            {
                // the uninitialized float can be NaN (observed on Windows Mobile 5 ARMv4i)
                float readFloat = qvariant_cast<float>(readVariant);
                float writtenFloat = qvariant_cast<float>(writeVariant);
                QCOMPARE(qIsNaN(readFloat), qIsNaN(writtenFloat));
                if (!qIsNaN(readFloat))
                    QCOMPARE(readFloat, writtenFloat);
            }
            break;
        }
    }
}

void tst_QGuiVariant::writeToReadFromOldDataStream()
{
    QPolygonF polyF(QRectF(10, 10, 50, 50));
    QVariant testVariant(polyF);
    {
        // Read into a variant and compare
        QFile file(":/data/qpolygonf.bin");
        QVERIFY(file.open(QIODevice::ReadOnly));
        QDataStream dataFileStream(&file);
        dataFileStream.setVersion(QDataStream::Qt_4_9);
        QVariant readVariant;
        dataFileStream >> readVariant;
        QCOMPARE(readVariant.userType(), int(QMetaType::QPolygonF));
        QCOMPARE(testVariant, readVariant);
        file.close();
    }
    {
        QByteArray variantData;
        {
            QDataStream varDataStream(&variantData, QIODevice::WriteOnly);
            varDataStream << testVariant;
        }
        // Read into a bytearray and compare
        QFile file(":/data/qpolygonf.bin");
        QVERIFY(file.open(QIODevice::ReadOnly));
        QDataStream dataFileStream(&file);
        dataFileStream.setVersion(QDataStream::Qt_4_9);
        int dummy;
        dataFileStream >> dummy;
        QByteArray polyData49;
        dataFileStream >> polyData49;
        file.close();
        QByteArray polyData50;
        QDataStream readVarData(variantData);
        readVarData >> dummy;
        readVarData >> polyData50;
        QCOMPARE(polyData49, polyData50);
    }
}

void tst_QGuiVariant::debugStream_data()
{
    QTest::addColumn<QVariant>("variant");
    QTest::addColumn<int>("typeId");
    for (int id = QMetaType::FirstGuiType; id <= QMetaType::LastGuiType; ++id) {
        const char *tagName = QMetaType::typeName(id);
        if (!tagName)
            continue;
        QTest::newRow(tagName) << QVariant(static_cast<QVariant::Type>(id)) << id;
    }
}

void tst_QGuiVariant::debugStream()
{
    QFETCH(QVariant, variant);
    QFETCH(int, typeId);

    MessageHandler msgHandler(typeId);
    qDebug() << variant;
    QVERIFY(msgHandler.testPassed());
}

void tst_QGuiVariant::implicitConstruction()
{
    // This is a compile-time test
    QVariant v;

#define FOR_EACH_GUI_CLASS_BASE(F) \
    F(Font) \
    F(Pixmap) \
    F(Brush) \
    F(Color) \
    F(Palette) \
    F(Icon) \
    F(Image) \
    F(Polygon) \
    F(Region) \
    F(Bitmap) \
    F(KeySequence) \
    F(Pen) \
    F(TextLength) \
    F(TextFormat) \
    F(Matrix) \
    F(Transform) \
    F(Matrix4x4) \
    F(Vector2D) \
    F(Vector3D) \
    F(Vector4D) \
    F(Quaternion) \
    F(PolygonF)

#ifndef QTEST_NO_CURSOR
#  define FOR_EACH_GUI_CLASS(F) \
    FOR_EACH_GUI_CLASS_BASE(F) \
    F(Cursor)
#else // !QTEST_NO_CURSOR
#  define FOR_EACH_GUI_CLASS(F) \
    FOR_EACH_GUI_CLASS_BASE(F)
#endif // QTEST_NO_CURSOR

#define CONSTRUCT(TYPE) \
    { \
        Q##TYPE t; \
        v = t; \
        QVERIFY(true); \
    }

    FOR_EACH_GUI_CLASS(CONSTRUCT)

#undef CONSTRUCT
#undef FOR_EACH_GUI_CLASS
}

void tst_QGuiVariant::guiVariantAtExit()
{
    // crash test, it should not crash at QGuiApplication exit
#ifndef QTEST_NO_CURSOR
    static QVariant cursor = QCursor();
#endif
    static QVariant point = QPoint();
    static QVariant icon = QIcon();
    static QVariant image = QImage();
    static QVariant palette = QPalette();
#ifndef QTEST_NO_CURSOR
    Q_UNUSED(cursor);
#endif
    Q_UNUSED(point);
    Q_UNUSED(icon);
    Q_UNUSED(image);
    Q_UNUSED(palette);
    QVERIFY(true);
}

void tst_QGuiVariant::iconEquality()
{
    QIcon i;
    QVariant a = i;
    QVariant b = i;
    QCOMPARE(a, b);

    i = QIcon(":/black.png");
    a = i;
    QVERIFY(a != b);

    b = a;
    QCOMPARE(a, b);

    i = QIcon(":/black2.png");
    a = i;
    QVERIFY(a != b);

    b = i;
    QCOMPARE(a, b);

    // This is a "different" QIcon
    // even if the contents are the same
    b = QIcon(":/black2.png");
    QVERIFY(a != b);
}

QTEST_MAIN(tst_QGuiVariant)
#include "tst_qguivariant.moc"
