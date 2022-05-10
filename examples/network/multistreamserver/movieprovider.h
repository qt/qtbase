// Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MOVIEPROVIDER_H
#define MOVIEPROVIDER_H

#include "provider.h"

QT_BEGIN_NAMESPACE
class QMovie;
QT_END_NAMESPACE

class MovieProvider : public Provider
{
    Q_OBJECT
public:
    explicit MovieProvider(QObject *parent = nullptr);

private slots:
    void frameChanged();

private:
    QMovie *movie;
};

#endif
