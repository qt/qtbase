// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

using namespace Qt::StringLiterals;

//! [0]
   QCborValue(QCborTag(2), QByteArray("\x01\0\0\0\0\0\0\0\0", 9));
//! [0]

//! [1]
   writer.startMap(4);    // 4 elements in the map

   writer.append("label"_L1);
   writer.append("journald"_L1);

   writer.append("autoDetect"_L1);
   writer.append(false);

   writer.append("condition"_L1);
   writer.append("libs.journald"_L1);

   writer.append("output"_L1);
   writer.startArray(1);
   writer.append("privateFeature"_L1);
   writer.endArray();

   writer.endMap();
//! [1]

//! [2]
   QFile f("output", QIODevice::WriteOnly);
   QCborStreamWriter writer(&f);
   writer.startMap(0);
   writer.endMap();
//! [2]

//! [3]
   QByteArray encodedNumber(qint64 value)
   {
       QByteArray ba;
       QCborStreamWriter writer(&ba);
       writer.append(value);
       return ba;
   }
//! [3]

//! [4]
   writer.append(0U);
   writer.append(Q_UINT64_C(4294967296));
   writer.append(std::numeric_limits<quint64>::max());
//! [4]

//! [5]
   writer.append(0);
   writer.append(-1);
   writer.append(Q_INT64_C(4294967296));
   writer.append(std::numeric_limits<qint64>::max());
//! [5]

//! [6]
   writer.append(QCborNegativeInteger(1));
   writer.append(QCborNegativeInteger(Q_INT64_C(4294967296)));
   writer.append(QCborNegativeInteger(-quint64(std::numeric_limits<qint64>::min())));
//! [6]

//! [7]
   void writeFile(QCborStreamWriter &writer, const QString &fileName)
   {
       QFile f(fileName);
       if (f.open(QIODevice::ReadOnly))
           writer.append(f.readAll());
   }
//! [7]

//! [8]
   writer.append("Hello, World"_L1);
//! [8]

//! [9]
   void writeString(QCborStreamWriter &writer, const QString &str)
   {
       writer.append(str);
   }
//! [9]

//! [10]
   void writeRxPattern(QCborStreamWriter &writer, const QRegularExpression &rx)
   {
       writer.append(QCborTag(36));
       writer.append(rx.pattern());
   }
//! [10]

//! [11]
   void writeCurrentTime(QCborStreamWriter &writer)
   {
       writer.append(QCborKnownTags::UnixTime_t);
       writer.append(time(nullptr));
   }
//! [11]

//! [12]
  writer.append(QCborSimpleType::Null);
  writer.append(QCborSimpleType(32));
//! [12]

//! [13]
   void writeFloat(QCborStreamWriter &writer, float f)
   {
       qfloat16 f16 = f;
       if (qIsNaN(f) || f16 == f)
           writer.append(f16);
       else
           writer.append(f);
   }
//! [13]

//! [14]
   void writeFloat(QCborStreamWriter &writer, double d)
   {
       float f = d;
       if (qIsNaN(d) || d == f)
           writer.append(f);
       else
           writer.append(d);
   }
//! [14]

//! [15]
   void writeDouble(QCborStreamWriter &writer, double d)
   {
       float f;
       if (qIsNaN(d)) {
           writer.append(qfloat16(qQNaN()));
       } else if (qIsInf(d)) {
           writer.append(d < 0 ? -qInf() : qInf());
       } else if ((f = d) == d) {
           qfloat16 f16 = f;
           if (f16 == f)
               writer.append(f16);
           else
               writer.append(f);
       } else {
           writer.append(d);
       }
   }
//! [15]

//! [16]
   writer.append(b ? QCborSimpleType::True : QCborSimpleType::False);
//! [16]

//! [17]
   writer.append(QCborSimpleType::Null);
//! [17]

//! [18]
   writer.append(QCborSimpleType::Null);
//! [18]

//! [19]
   writer.append(QCborSimpleType::Undefined);
//! [19]

//! [20]
   void appendList(QCborStreamWriter &writer, const QList<QString> &values)
   {
       writer.startArray();
       for (const QString &s : values)
           writer.append(s);
       writer.endArray();
   }
//! [20]

//! [21]
   void appendList(QCborStreamWriter &writer, const QStringList &list)
   {
       writer.startArray(list.size());
       for (const QString &s : list)
           writer.append(s);
       writer.endArray();
   }
//! [21]

//! [22]
   void appendMap(QCborStreamWriter &writer, const QList<QPair<int, QString>> &values)
   {
       writer.startMap();
       for (const auto pair : values) {
           writer.append(pair.first)
           writer.append(pair.second);
       }
       writer.endMap();
   }
//! [22]

//! [23]
   void appendMap(QCborStreamWriter &writer, const QMap<int, QString> &map)
   {
       writer.startMap(map.size());
       for (auto it = map.cbegin(), end = map.cend(); it != end; ++it) {
           writer.append(it.key());
           writer.append(it.value());
       }
       writer.endMap();
   }
//! [23]

//! [24]
   void handleStream(QCborStreamReader &reader)
   {
       switch (reader.type())
       case QCborStreamReader::UnsignedInteger:
       case QCborStreamReader::NegativeInteger:
       case QCborStreamReader::SimpleType:
       case QCborStreamReader::Float16:
       case QCborStreamReader::Float:
       case QCborStreamReader::Double:
           handleFixedWidth(reader);
           reader.next();
           break;
       case QCborStreamReader::ByteArray:
       case QCborStreamReader::String:
           handleString(reader);
           break;
       case QCborStreamReader::Array:
       case QCborStreamReader::Map:
           reader.enterContainer();
           while (reader.lastError() == QCborError::NoError)
               handleStream(reader);
           if (reader.lastError() == QCborError::NoError)
               reader.leaveContainer();
       }
   }
//! [24]

//! [25]
   QVariantList populateFromCbor(QCborStreamReader &reader)
   {
       QVariantList list;
       if (reader.isLengthKnown())
           list.reserve(reader.length());

       reader.enterContainer();
       while (reader.lastError() == QCborError::NoError && reader.hasNext())
           list.append(readOneElement(reader));
       if (reader.lastError() == QCborError::NoError)
           reader.leaveContainer();
   }
//! [25]

//! [26]
   QVariantMap populateFromCbor(QCborStreamReader &reader)
   {
       QVariantMap map;
       if (reader.isLengthKnown())
           map.reserve(reader.length());

       reader.enterContainer();
       while (reader.lastError() == QCborError::NoError && reader.hasNext()) {
           QString key = readElementAsString(reader);
           map.insert(key, readOneElement(reader));
       }
       if (reader.lastError() == QCborError::NoError)
           reader.leaveContainer();
   }
//! [26]

//! [27]
   QString decodeString(QCborStreamReader &reader)
   {
       QString result;
       auto r = reader.readString();
       while (r.code == QCborStreamReader::Ok) {
           result += r.data;
           r = reader.readString();
       }

       if (r.code == QCborStreamReader::Error) {
           // handle error condition
           result.clear();
       }
       return result;
   }
//! [27]

//! [28]
   QBytearray decodeBytearray(QCborStreamReader &reader)
   {
       QBytearray result;
       auto r = reader.readBytearray();
       while (r.code == QCborStreamReader::Ok) {
           result += r.data;
           r = reader.readByteArray();
       }

       if (r.code == QCborStreamReader::Error) {
           // handle error condition
           result.clear();
       }
       return result;
   }
//! [28]

//! [29]
    QCborStreamReader<qsizetype> result;
    do {
        qsizetype size = reader.currentStringChunkSize();
        qsizetype oldsize = buffer.size();
        buffer.resize(oldsize + size);
        result = reader.readStringChunk(buffer.data() + oldsize, size);
    } while (result.status() == QCborStreamReader::Ok);
//! [29]
