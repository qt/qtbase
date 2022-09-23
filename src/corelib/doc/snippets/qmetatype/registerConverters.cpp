// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QJsonObject>
#include <QMetaType>
#include <QString>

int main() {
  //! [member]
  struct Coordinates {
    int x;
    int y;
    int z;

    QString toString() const { return u"[x: %1; y: %2, z: %3]"_s.arg(QString::number(x),
      QString::number(y),
      QString::number(z)); }
  };
  QMetaType::registerConverter<Coordinates, QString>(&Coordinates::toString);
  //! [member]

  //! [memberOk]
  struct BigNumber {
      long long l;

      int toInt(bool *ok = nullptr) const {
        const bool canConvertSafely = l < std::numeric_limits<int>::max();
        if (ok)
          *ok = canConvertSafely;
        return l;
      }
  };
  QMetaType::registerConverter<BigNumber, int>(&BigNumber::toInt);
  //! [memberOk]

  //! [implicit]
  class Counter {
    int number = 0;
  public:
    int value() const { return number; }
    operator int() const { return value(); }
    void increment() {++number;}
  };
  QMetaType::registerConverter<Counter, int>();
  //! [implicit]

  struct CustomStringType {
    const char *data() const {return nullptr;}
  };
  //! [unaryfunc]
  QMetaType::registerConverter<CustomStringType, QString>([](const CustomStringType &str) {
      return QString::fromUtf8(str.data());
  });
  QMetaType::registerConverter<QJsonValue, QPointF>(
            [](const QJsonValue &value) -> std::optional<QPointF> {
      const auto object = value.toObject();
      if (!object.contains("x") || !object.contains("y"))
          return std::nullopt;  // The conversion fails if the required properties are missing
      return QPointF{object["x"].toDouble(), object["y"].toDouble()};
  });
  //! [unaryfunc]
}
