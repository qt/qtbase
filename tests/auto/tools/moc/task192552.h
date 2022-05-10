// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TASK192552_H
#define TASK192552_H
/*
  <:: is not valid C++, but we want moc to treat it as < :: since this
  is usually the intention
 */

#include <qobject.h>

class Task192552 : public QObject
{
    Q_OBJECT
public:
#ifdef Q_MOC_RUN
    QList<::QObject*> m_objects;
#endif
};
#endif // TASK192552_H
