#ifndef BUTTONWIDGET_H
#define BUTTONWIDGET_H

#include <QWidget>
#include <QSignalMapper>

//! [0]
class ButtonWidget : public QWidget
{
    Q_OBJECT

public:
    ButtonWidget(QStringList texts, QWidget *parent = 0);

signals:
    void clicked(const QString &text);

private:
    QSignalMapper *signalMapper;
};
//! [0]

#endif // BUTTONWIDGET_H

