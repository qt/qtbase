#include <QtCore/QObject>

#include "qpepperinstance.h"

#include <ppapi/cpp/instance.h>

class QPepperJavascriptBridge : public QObject
{
Q_OBJECT
public:
    QPepperJavascriptBridge(pp::Instance *instance);

    QVariant callJavascriptFunction(const QByteArray &tag, const QByteArray &code);

    void evalSource(const QByteArray &code);
    void evalFile(const QString &fileName);
Q_SIGNALS:
    void evalFunctionReply(const QByteArray &tag, const QString &reply);

private:
    pp::Instance *m_instance;
};
