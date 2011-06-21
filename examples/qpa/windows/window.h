#include <QWindow>
#include <QImage>

class Window : public QWindow
{
public:
    Window(QWindow *parent = 0);

protected:
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);

    void keyPressEvent(QKeyEvent *);

    void exposeEvent(QExposeEvent *);
    void resizeEvent(QResizeEvent *);

private:
    void render();

    QString m_text;
    QImage m_image;
    QPoint m_lastPos;
    int m_backgroundColorIndex;
    QBackingStore *m_backingStore;
};
