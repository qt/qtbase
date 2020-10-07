#ifndef TABBARFORM_H
#define TABBARFORM_H

#include <QWidget>
#include "ui_tabbarform.h"

namespace Ui {
class TabBarForm;
}

class TabBarForm : public QWidget
{
    Q_OBJECT

public:
    explicit TabBarForm(QWidget *parent = nullptr);
    ~TabBarForm();

    Ui::TabBarForm *ui;
};

#endif // TABBARFORM_H
