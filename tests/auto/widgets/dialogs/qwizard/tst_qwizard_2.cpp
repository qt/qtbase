/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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


#include <QComboBox>
#include <QDebug>
#include <QLineEdit>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QWizard>
#include <QWizardPage>

#include <QtTest/QtTest>

class taskQTBUG_25691 : public QWizard
{
    Q_OBJECT
public:
    taskQTBUG_25691( QWidget * parent = 0 );

    ~taskQTBUG_25691(void);
};

class taskQTBUG_25691Page1 : public QWizardPage
{
    Q_OBJECT
public:
    taskQTBUG_25691Page1( QWidget * parent = 0 );

    ~taskQTBUG_25691Page1(void);
};

class taskQTBUG_25691Page2 : public QWizardPage
{
    Q_OBJECT
public:
    taskQTBUG_25691Page2( QWidget * parent = 0 );

    virtual void initializePage(void);

    ~taskQTBUG_25691Page2(void);

private:
    QVBoxLayout * layout;
    QLineEdit * field0_value;
    QLineEdit * field1_value;
    QLineEdit * field2_value;
};


taskQTBUG_25691::taskQTBUG_25691( QWidget * parent )
    : QWizard( parent )
{
    this->addPage( new taskQTBUG_25691Page1 );
    this->addPage( new taskQTBUG_25691Page2 );
    this->show();
}

taskQTBUG_25691::~taskQTBUG_25691(void)
{
}

taskQTBUG_25691Page1::taskQTBUG_25691Page1( QWidget * parent )
    : QWizardPage( parent )
{
    QComboBox * field0_needed = new QComboBox( this );
    field0_needed->addItem( "No" );
    field0_needed->addItem( "Yes" );
    field0_needed->setCurrentIndex(0);
    this->registerField( "field0_needed", field0_needed );

    QComboBox * field1_needed = new QComboBox( this );
    field1_needed->addItem( "No" );
    field1_needed->addItem( "Yes" );
    field1_needed->setCurrentIndex(0);
    this->registerField( "field1_needed", field1_needed );

    QComboBox * field2_needed = new QComboBox( this );
    field2_needed->addItem( "No" );
    field2_needed->addItem( "Yes" );
    field2_needed->setCurrentIndex(0);
    this->registerField( "field2_needed", field2_needed );

    QVBoxLayout * layout = new QVBoxLayout;
    layout->addWidget( field0_needed );
    layout->addWidget( field1_needed );
    layout->addWidget( field2_needed );
    this->setLayout( layout );
}

taskQTBUG_25691Page1::~taskQTBUG_25691Page1(void)
{
}

taskQTBUG_25691Page2::taskQTBUG_25691Page2( QWidget * parent )
    : QWizardPage( parent )
{
    this->layout = new QVBoxLayout;
    this->setLayout( this->layout );

    this->field0_value = 0;
    this->field1_value = 0;
    this->field2_value = 0;
}

void taskQTBUG_25691Page2::initializePage(void)
{
    QWizard * wizard = this->wizard();
    bool field0_needed = wizard->field( "field0_needed" ).toBool();
    bool field1_needed = wizard->field( "field1_needed" ).toBool();
    bool field2_needed = wizard->field( "field2_needed" ).toBool();

    if ( field0_needed  &&  this->field0_value == 0 ){
        this->field0_value = new QLineEdit( "field0_default" );
        this->registerField( "field0_value", this->field0_value );
        this->layout->addWidget( this->field0_value );
    } else if ( ! field0_needed  &&  this->field0_value != 0 ){
        this->layout->removeWidget( this->field0_value );
        delete this->field0_value;
        this->field0_value = 0;
    }

    if ( field1_needed  &&  this->field1_value == 0  ){
        this->field1_value = new QLineEdit( "field1_default" );
        this->registerField( "field1_value", this->field1_value );
        this->layout->addWidget( this->field1_value );
    } else if ( ! field1_needed  &&  this->field1_value != 0 ){
        this->layout->removeWidget( this->field1_value );
        delete this->field1_value;
        this->field1_value = 0;
    }

    if ( field2_needed  &&  this->field2_value == 0  ){
        this->field2_value = new QLineEdit( "field2_default" );
        this->registerField( "field2_value", this->field2_value );
        this->layout->addWidget( this->field2_value );
    } else if ( ! field2_needed  &&  this->field2_value != 0 ){
        this->layout->removeWidget( this->field2_value );
        delete this->field2_value;
        this->field2_value = 0;
    }
}

taskQTBUG_25691Page2::~taskQTBUG_25691Page2(void)
{
}

void taskQTBUG_25691_fieldObjectDestroyed2(void)
{
    QMainWindow mw;
    taskQTBUG_25691 wb( &mw );

    wb.setField( "field0_needed", true );
    wb.setField( "field1_needed", true );
    wb.setField( "field2_needed", true );
    wb.next(); // Results in registration of all three field_Nvalue fields
    wb.back(); // Back up to cancel need for field1_value
    wb.setField( "field1_needed", false ); // cancel need for field1_value
    wb.next(); // Results in destruction of field field1_value's widget
    wb.next(); // Commit wizard's results

    // Now collect the value from fields that was not destroyed.
    QString field0_value = wb.field( "field0_value" ).toString();
    QCOMPARE( field0_value, QString("field0_default") );

    QString field2_value = wb.field( "field2_value" ).toString();
    QCOMPARE( field2_value, QString("field2_default") );
}

#include "tst_qwizard_2.moc"
