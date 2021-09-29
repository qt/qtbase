/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QMetaType>
#include <QString>

int main() {
  //! [member]
  struct Coordinates {
    int x;
    int y;
    int z;

    QString toString() const { return u"[x: %1; y: %2, z: %3]"_qs.arg(QString::number(x),
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
  //! [unaryfunc]
}
