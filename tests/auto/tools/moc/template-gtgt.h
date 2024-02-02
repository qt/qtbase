// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TEMPLATE_GTGT_H
#define TEMPLATE_GTGT_H
template<class TYPE, size_t COUNT>
class myTemplate :
      QString,
      QList<TYPE, QList<COUNT>>
{};

template<class TYPE, size_t COUNT>
class myTemplate2 :
      QString,
      QList<TYPE, QList< (4 >> 2) >>
{};

class Widget : public QWidget
{
    Q_OBJECT
public:
    Widget()
    {
    }
};
#endif // TEMPLATE_GTGT_H
