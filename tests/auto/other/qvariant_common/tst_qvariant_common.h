// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_QVARIANT_COMMON
#define TST_QVARIANT_COMMON

#include <QString>

class MessageHandler {
public:
    MessageHandler(const int typeId, QtMessageHandler msgHandler = handler)
        : oldMsgHandler(qInstallMessageHandler(msgHandler))
    {
        currentId = typeId;
    }

    ~MessageHandler()
    {
        qInstallMessageHandler(oldMsgHandler);
    }

    bool testPassed() const
    {
        return ok;
    }
protected:
    static void handler(QtMsgType, const QMessageLogContext &, const QString &msg)
    {
        // Format itself is not important, but basic data as a type name should be included in the output
        ok = msg.startsWith("QVariant(");
        QVERIFY2(ok, (QString::fromLatin1("Message is not started correctly: '") + msg + '\'').toLatin1().constData());
        ok &= (currentId == QMetaType::UnknownType
             ? msg.contains("Invalid")
             : msg.contains(QMetaType(currentId).name()));
        QVERIFY2(ok, (QString::fromLatin1("Message doesn't contain type name: '") + msg + '\'').toLatin1().constData());
        if (currentId == QMetaType::Char || currentId == QMetaType::QChar) {
            // Chars insert '\0' into the qdebug stream, it is not possible to find a real string length
            return;
        }
        if (QMetaType(currentId).flags() & QMetaType::IsPointer) {
            QByteArray currentName = QMetaType(currentId).name();
            ok &= msg.contains(currentName + ", 0x");
        }
        ok &= msg.endsWith(QLatin1Char(')'));
        QVERIFY2(ok, (QString::fromLatin1("Message is not correctly finished: '") + msg + '\'').toLatin1().constData());

    }

    QtMessageHandler oldMsgHandler;
    inline static int currentId = {};
    inline static bool ok = {};
};

#define TST_QVARIANT_CANCONVERT_DATATABLE_HEADERS \
    QTest::addColumn<QVariant>("val"); \
    QTest::addColumn<bool>("BitArrayCast"); \
    QTest::addColumn<bool>("BitmapCast"); \
    QTest::addColumn<bool>("BoolCast"); \
    QTest::addColumn<bool>("BrushCast"); \
    QTest::addColumn<bool>("ByteArrayCast"); \
    QTest::addColumn<bool>("ColorCast"); \
    QTest::addColumn<bool>("CursorCast"); \
    QTest::addColumn<bool>("DateCast"); \
    QTest::addColumn<bool>("DateTimeCast"); \
    QTest::addColumn<bool>("DoubleCast"); \
    QTest::addColumn<bool>("FontCast"); \
    QTest::addColumn<bool>("ImageCast"); \
    QTest::addColumn<bool>("IntCast"); \
    QTest::addColumn<bool>("InvalidCast"); \
    QTest::addColumn<bool>("KeySequenceCast"); \
    QTest::addColumn<bool>("ListCast"); \
    QTest::addColumn<bool>("LongLongCast"); \
    QTest::addColumn<bool>("MapCast"); \
    QTest::addColumn<bool>("PaletteCast"); \
    QTest::addColumn<bool>("PenCast"); \
    QTest::addColumn<bool>("PixmapCast"); \
    QTest::addColumn<bool>("PointCast"); \
    QTest::addColumn<bool>("RectCast"); \
    QTest::addColumn<bool>("RegionCast"); \
    QTest::addColumn<bool>("SizeCast"); \
    QTest::addColumn<bool>("SizePolicyCast"); \
    QTest::addColumn<bool>("StringCast"); \
    QTest::addColumn<bool>("StringListCast"); \
    QTest::addColumn<bool>("TimeCast"); \
    QTest::addColumn<bool>("UIntCast"); \
    QTest::addColumn<bool>("ULongLongCast");

#define TST_QVARIANT_CANCONVERT_FETCH_DATA \
    QFETCH(QVariant, val); \
    QFETCH(bool, BitArrayCast); \
    QFETCH(bool, BitmapCast); \
    QFETCH(bool, BoolCast); \
    QFETCH(bool, BrushCast); \
    QFETCH(bool, ByteArrayCast); \
    QFETCH(bool, ColorCast); \
    QFETCH(bool, CursorCast); \
    QFETCH(bool, DateCast); \
    QFETCH(bool, DateTimeCast); \
    QFETCH(bool, DoubleCast); \
    QFETCH(bool, FontCast); \
    QFETCH(bool, ImageCast); \
    QFETCH(bool, IntCast); \
    QFETCH(bool, InvalidCast); \
    QFETCH(bool, KeySequenceCast); \
    QFETCH(bool, ListCast); \
    QFETCH(bool, LongLongCast); \
    QFETCH(bool, MapCast); \
    QFETCH(bool, PaletteCast); \
    QFETCH(bool, PenCast); \
    QFETCH(bool, PixmapCast); \
    QFETCH(bool, PointCast); \
    QFETCH(bool, RectCast); \
    QFETCH(bool, RegionCast); \
    QFETCH(bool, SizeCast); \
    QFETCH(bool, SizePolicyCast); \
    QFETCH(bool, StringCast); \
    QFETCH(bool, StringListCast); \
    QFETCH(bool, TimeCast); \
    QFETCH(bool, UIntCast); \
    QFETCH(bool, ULongLongCast);

#if QT_CONFIG(shortcut)
#define QMETATYPE_QKEYSEQUENCE \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QKeySequence)), KeySequenceCast);
#else
#define QMETATYPE_QKEYSEQUENCE
#endif

#define TST_QVARIANT_CANCONVERT_COMPARE_DATA \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QBitArray)), BitArrayCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QBitmap)), BitmapCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::Bool)), BoolCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QBrush)), BrushCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QByteArray)), ByteArrayCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QColor)), ColorCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QCursor)), CursorCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QDate)), DateCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QDateTime)), DateTimeCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::Double)), DoubleCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::Float)), DoubleCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QFont)), FontCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QImage)), ImageCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::Int)), IntCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::UnknownType)), InvalidCast); \
    QMETATYPE_QKEYSEQUENCE \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QVariantList)), ListCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::LongLong)), LongLongCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QVariantMap)), MapCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QPalette)), PaletteCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QPen)), PenCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QPixmap)), PixmapCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QPoint)), PointCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QRect)), RectCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QRegion)), RegionCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QSize)), SizeCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QSizePolicy)), SizePolicyCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QString)), StringCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QStringList)), StringListCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::QTime)), TimeCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::UInt)), UIntCast); \
    QCOMPARE(val.canConvert(QMetaType(QMetaType::ULongLong)), ULongLongCast);


#endif
