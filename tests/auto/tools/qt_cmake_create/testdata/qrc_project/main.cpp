#include <QFile>
#include <QDebug>

int main(int, char **)
{
    QFile file(":/test.txt");
    if (!file.open(QFile::ReadOnly))
        return 1;

    QString data = QString::fromUtf8(file.readAll());
    qDebug() << data;
    return 0;
}
