#ifndef ELIDEDLABEL_H
#define ELIDEDLABEL_H

#include <QFrame>
#include <QRect>
#include <QResizeEvent>
#include <QString>
#include <QWidget>

//! [0]
class ElidedLabel : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(bool isElided READ isElided)

public:
    ElidedLabel(const QString &text, QWidget *parent = 0);

    void setText(const QString &text);
    const QString & text() const { return content; }
    bool isElided() const { return elided; }

protected:
    void paintEvent(QPaintEvent *event);

signals:
    void elisionChanged(bool elided);

private:
    bool elided;
    QString content;
};
//! [0]

#endif // TEXTWRAPPINGWIDGET_H
