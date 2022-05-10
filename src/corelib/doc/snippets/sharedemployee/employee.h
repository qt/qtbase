// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef EMPLOYEE_H
#define EMPLOYEE_H

//! [0]
#include <QSharedData>
#include <QString>

class EmployeeData : public QSharedData
{
  public:
    EmployeeData() : id(-1) { }
    EmployeeData(const EmployeeData &other)
        : QSharedData(other), id(other.id), name(other.name) { }
    ~EmployeeData() { }

    int id;
    QString name;
};

class Employee
{
  public:
//! [1]
    Employee() { d = new EmployeeData; }
//! [1] //! [2]
    Employee(int id, const QString &name) {
        d = new EmployeeData;
        setId(id);
        setName(name);
    }
//! [2] //! [7]
    Employee(const Employee &other)
          : d (other.d)
    {
    }
//! [7]
//! [3]
    void setId(int id) { d->id = id; }
//! [3] //! [4]
    void setName(const QString &name) { d->name = name; }
//! [4]

//! [5]
    int id() const { return d->id; }
//! [5] //! [6]
    QString name() const { return d->name; }
//! [6]

  private:
    QSharedDataPointer<EmployeeData> d;
};
//! [0]

#endif
