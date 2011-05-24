#ifndef MCEVIBRATOR_H
#define MCEVIBRATOR_H

#include <QObject>
#include <QTextStream>
#include <QDBusInterface>

//! [0]
class MceVibrator : public QObject
{
    Q_OBJECT
public:
    explicit MceVibrator(QObject *parent = 0);
    ~MceVibrator();

    static const char defaultMceFilePath[];
    static QStringList parsePatternNames(QTextStream &stream);

public slots:
    void vibrate(const QString &patternName);

private:
    void deactivate(const QString &patternName);

    QDBusInterface mceInterface;
    QString lastPatternName;
};
//! [0]

#endif // MCEVIBRATOR_H

