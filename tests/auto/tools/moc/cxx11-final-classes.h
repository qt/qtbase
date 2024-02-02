// Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef TESTS_AUTO_CORELIB_TOOLS_MOC_CXX11_FINAL_CLASSES_H
#define TESTS_AUTO_CORELIB_TOOLS_MOC_CXX11_FINAL_CLASSES_H

#include <QtCore/QObject>

#ifndef Q_MOC_RUN // hide from moc
# define final
# define sealed
# define EXPORT_MACRO
# define EXPORT_MACRO2(X,Y,Z)
#endif

class FinalTestClassQt Q_DECL_FINAL : public QObject
{
    Q_OBJECT
public:
    explicit FinalTestClassQt(QObject *parent = nullptr)
        : QObject(parent) {}
};


class EXPORT_MACRO ExportedFinalTestClassQt Q_DECL_FINAL : public QObject
{
    Q_OBJECT
public:
    explicit ExportedFinalTestClassQt(QObject *parent = nullptr)
        : QObject(parent) {}
};

class EXPORT_MACRO2(X,Y,Z) ExportedFinalTestClassQtX Q_DECL_FINAL : public QObject
{
    Q_OBJECT
public:
    explicit ExportedFinalTestClassQtX(QObject *parent = nullptr)
        : QObject(parent) {}
};

class FinalTestClassCpp11 final : public QObject
{
    Q_OBJECT
public:
    explicit FinalTestClassCpp11(QObject *parent = nullptr)
        : QObject(parent) {}
};

class EXPORT_MACRO ExportedFinalTestClassCpp11 final : public QObject
{
    Q_OBJECT
public:
    explicit ExportedFinalTestClassCpp11(QObject *parent = nullptr)
        : QObject(parent) {}
};

class EXPORT_MACRO2(X,Y,Z) ExportedFinalTestClassCpp11X final : public QObject
{
    Q_OBJECT
public:
    explicit ExportedFinalTestClassCpp11X(QObject *parent = nullptr)
        : QObject(parent) {}
};

class SealedTestClass sealed : public QObject
{
    Q_OBJECT
public:
    explicit SealedTestClass(QObject *parent = nullptr)
        : QObject(parent) {}
};

class EXPORT_MACRO ExportedSealedTestClass sealed : public QObject
{
    Q_OBJECT
public:
    explicit ExportedSealedTestClass(QObject *parent = nullptr)
        : QObject(parent) {}
};

class EXPORT_MACRO2(X,Y,Z) ExportedSealedTestClassX sealed : public QObject
{
    Q_OBJECT
public:
    explicit ExportedSealedTestClassX(QObject *parent = nullptr)
        : QObject(parent) {}
};

#ifndef Q_MOC_RUN
# undef final
# undef sealed
# undef EXPORT_MACRO
# undef EXPORT_MACRO2
#endif

#endif // TESTS_AUTO_CORELIB_TOOLS_MOC_CXX11_FINAL_CLASSES_H
