#include "tabbarform.h"

TabBarForm::TabBarForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TabBarForm)
{
    ui->setupUi(this);
}

TabBarForm::~TabBarForm()
{
    delete ui;
}
