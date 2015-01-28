/****************************************************************************
**
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef TESTS_AUTO_CORELIB_TOOLS_MOC_CXX11_EXPLICIT_OVERRIDE_CONTROL_H
#define TESTS_AUTO_CORELIB_TOOLS_MOC_CXX11_EXPLICIT_OVERRIDE_CONTROL_H

#include <QtCore/QObject>

#ifndef Q_MOC_RUN // hide from moc
# define override
# define final
# define sealed
#endif

class ExplicitOverrideControlBase : public QObject
{
    Q_OBJECT
public:
    explicit ExplicitOverrideControlBase(QObject *parent = 0)
        : QObject(parent) {}

private Q_SLOTS:
    virtual void pureSlot0() = 0;
    virtual void pureSlot1() = 0;
    virtual void pureSlot2() const = 0;
    virtual void pureSlot3() const = 0;
#if 0 // moc doesn't support volatile slots
    virtual void pureSlot4() volatile = 0;
    virtual void pureSlot5() volatile = 0;
    virtual void pureSlot6() const volatile = 0;
    virtual void pureSlot7() volatile const = 0;
    virtual void pureSlot8() const volatile = 0;
    virtual void pureSlot9() volatile const = 0;
#endif
};

class ExplicitOverrideControlFinalQt : public ExplicitOverrideControlBase
{
    Q_OBJECT
public:
    explicit ExplicitOverrideControlFinalQt(QObject *parent = 0)
        : ExplicitOverrideControlBase(parent) {}

private Q_SLOTS:
    void pureSlot0() Q_DECL_FINAL {}
    void pureSlot1() Q_DECL_FINAL {}
    void pureSlot2() const Q_DECL_FINAL {}
    void pureSlot3() Q_DECL_FINAL const {}
#if 0 // moc doesn't support volatile slots
    void pureSlot4() volatile Q_DECL_FINAL {}
    void pureSlot5() Q_DECL_FINAL volatile {}
    void pureSlot6() const volatile Q_DECL_FINAL {}
    void pureSlot7() volatile Q_DECL_FINAL const {}
    void pureSlot8() const Q_DECL_FINAL volatile {}
    void pureSlot9() Q_DECL_FINAL volatile const {}
#endif
};

class ExplicitOverrideControlFinalCxx11 : public ExplicitOverrideControlBase
{
    Q_OBJECT
public:
    explicit ExplicitOverrideControlFinalCxx11(QObject *parent = 0)
        : ExplicitOverrideControlBase(parent) {}

private Q_SLOTS:
    void pureSlot0() final {}
    void pureSlot1() final {}
    void pureSlot2() const final {}
    void pureSlot3() final const {}
#if 0 // moc doesn't support volatile slots
    void pureSlot4() volatile final {}
    void pureSlot5() final volatile {}
    void pureSlot6() const volatile final {}
    void pureSlot7() volatile final const {}
    void pureSlot8() const final volatile {}
    void pureSlot9() final volatile const {}
#endif
};

class ExplicitOverrideControlSealed : public ExplicitOverrideControlBase
{
    Q_OBJECT
public:
    explicit ExplicitOverrideControlSealed(QObject *parent = 0)
        : ExplicitOverrideControlBase(parent) {}

private Q_SLOTS:
    void pureSlot0() sealed {}
    void pureSlot1() sealed {}
    void pureSlot2() const sealed {}
    void pureSlot3() sealed const {}
#if 0 // moc doesn't support volatile slots
    void pureSlot4() volatile sealed {}
    void pureSlot5() sealed volatile {}
    void pureSlot6() const volatile sealed {}
    void pureSlot7() volatile sealed const {}
    void pureSlot8() const sealed volatile {}
    void pureSlot9() sealed volatile const {}
#endif
};

class ExplicitOverrideControlOverrideQt : public ExplicitOverrideControlBase
{
    Q_OBJECT
public:
    explicit ExplicitOverrideControlOverrideQt(QObject *parent = 0)
        : ExplicitOverrideControlBase(parent) {}

private Q_SLOTS:
    void pureSlot0() Q_DECL_OVERRIDE {}
    void pureSlot1() Q_DECL_OVERRIDE {}
    void pureSlot2() const Q_DECL_OVERRIDE {}
    void pureSlot3() Q_DECL_OVERRIDE const {}
#if 0 // moc doesn't support volatile slots
    void pureSlot4() volatile Q_DECL_OVERRIDE {}
    void pureSlot5() Q_DECL_OVERRIDE volatile {}
    void pureSlot6() const volatile Q_DECL_OVERRIDE {}
    void pureSlot7() volatile Q_DECL_OVERRIDE const {}
    void pureSlot8() const Q_DECL_OVERRIDE volatile {}
    void pureSlot9() Q_DECL_OVERRIDE volatile const {}
#endif
};

class ExplicitOverrideControlOverrideCxx11 : public ExplicitOverrideControlBase
{
    Q_OBJECT
public:
    explicit ExplicitOverrideControlOverrideCxx11(QObject *parent = 0)
        : ExplicitOverrideControlBase(parent) {}

private Q_SLOTS:
    void pureSlot0() override {}
    void pureSlot1() override {}
    void pureSlot2() const override {}
    void pureSlot3() override const {}
#if 0 // moc doesn't support volatile slots
    void pureSlot4() volatile override {}
    void pureSlot5() override volatile {}
    void pureSlot6() const volatile override {}
    void pureSlot7() volatile override const {}
    void pureSlot8() const override volatile {}
    void pureSlot9() override volatile const {}
#endif
};

class ExplicitOverrideControlFinalQtOverrideQt : public ExplicitOverrideControlBase
{
    Q_OBJECT
public:
    explicit ExplicitOverrideControlFinalQtOverrideQt(QObject *parent = 0)
        : ExplicitOverrideControlBase(parent) {}

private Q_SLOTS:
    void pureSlot0() Q_DECL_FINAL Q_DECL_OVERRIDE {}
    void pureSlot1() Q_DECL_OVERRIDE Q_DECL_FINAL {}
    void pureSlot2() Q_DECL_OVERRIDE const Q_DECL_FINAL {}
    void pureSlot3() Q_DECL_FINAL const Q_DECL_OVERRIDE {}
#if 0 // moc doesn't support volatile slots
    void pureSlot4() volatile Q_DECL_FINAL Q_DECL_OVERRIDE {}
    void pureSlot5() Q_DECL_OVERRIDE Q_DECL_FINAL volatile {}
    void pureSlot6() Q_DECL_OVERRIDE const volatile Q_DECL_FINAL {}
    void pureSlot7() volatile Q_DECL_OVERRIDE Q_DECL_FINAL const {}
    void pureSlot8() const Q_DECL_FINAL Q_DECL_OVERRIDE volatile {}
    void pureSlot9() Q_DECL_FINAL volatile const Q_DECL_OVERRIDE {}
#endif
};

class ExplicitOverrideControlFinalCxx11OverrideCxx11 : public ExplicitOverrideControlBase
{
    Q_OBJECT
public:
    explicit ExplicitOverrideControlFinalCxx11OverrideCxx11(QObject *parent = 0)
        : ExplicitOverrideControlBase(parent) {}

private Q_SLOTS:
    void pureSlot0() final override {}
    void pureSlot1() override final {}
    void pureSlot2() override const final {}
    void pureSlot3() final const override {}
#if 0 // moc doesn't support volatile slots
    void pureSlot4() volatile final override {}
    void pureSlot5() override final volatile {}
    void pureSlot6() const volatile final override {}
    void pureSlot7() volatile final override const {}
    void pureSlot8() const override final volatile {}
    void pureSlot9() override final volatile const {}
#endif
};

class ExplicitOverrideControlSealedOverride : public ExplicitOverrideControlBase
{
    Q_OBJECT
public:
    explicit ExplicitOverrideControlSealedOverride(QObject *parent = 0)
        : ExplicitOverrideControlBase(parent) {}

private Q_SLOTS:
    void pureSlot0() sealed override {}
    void pureSlot1() override sealed {}
    void pureSlot2() override const sealed {}
    void pureSlot3() sealed const override {}
#if 0 // moc doesn't support volatile slots
    void pureSlot4() volatile sealed override {}
    void pureSlot5() sealed override volatile {}
    void pureSlot6() const override volatile sealed {}
    void pureSlot7() volatile sealed override const {}
    void pureSlot8() const sealed volatile override {}
    void pureSlot9() override sealed volatile const {}
#endif
};

#ifndef Q_MOC_RUN
# undef final
# undef sealed
# undef override
#endif

#endif // TESTS_AUTO_CORELIB_TOOLS_MOC_CXX11_EXPLICIT_OVERRIDE_CONTROL_H
