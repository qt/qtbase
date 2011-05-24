#ifndef TOUCHAREA_H
#define TOUCHAREA_H

#include <QWidget>

class XQVibra;

//! [0]
class VibrationSurface : public QWidget
{
    Q_OBJECT
public:
    explicit VibrationSurface(XQVibra *vibra, QWidget *parent = 0);

protected:
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);
    virtual void paintEvent(QPaintEvent *);

private:

    int getIntensity(int x, int y);
    void applyIntensity(int x, int y);

    XQVibra *vibra;
    int lastIntensity;
};
//! [0]

#endif // TOUCHAREA_H
