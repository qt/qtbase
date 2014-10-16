#include <QtCore>
#include <QtNetwork>
#include <QtGui>

#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/url_loader.h"
#include "ppapi/cpp/url_request_info.h"
#include "ppapi/cpp/url_response_info.h"
#include "ppapi/utility/completion_callback_factory.h"

class First
{
public:
    First() {
        // Enable spesific logging here
        QLoggingCategory::setFilterRules(QStringLiteral("qt.platform.pepper.network*=true"));
    }
};
First first;


const int ReadBufferSize = 32768;
extern void *qtPepperInstance; // extern pp::instance pointer set by the pepper platform plugin

// UrlLoader class assembpled from url_loader sample code
class UrlLoader
{
public:
    UrlLoader(const QString &url)
        :m_url(url)
        ,m_instance(reinterpret_cast<pp::Instance*>(qtPepperInstance))
        ,m_urlRequest(m_instance)
        ,m_urlLoader(m_instance)  
        ,m_buffer(new char[ReadBufferSize])
        ,callbackFactory(this)
    {
        m_urlRequest.SetURL(m_url.toStdString());
        m_urlRequest.SetMethod("GET");
        m_urlRequest.SetRecordDownloadProgress(true);
    }
    
    void load()
    {
        qDebug() << "loading" << m_url;
        
        // Request url Open
        pp::CompletionCallback openCallback = callbackFactory.NewCallback(&UrlLoader::onOpen);
        m_urlLoader.Open(m_urlRequest, openCallback);
    }
    
    void onOpen(int32_t result)
    {
        // Print 
        qDebug() << "onOpen" << m_url;
        qDebug() << "onOpen callback status" << result;
        pp::URLResponseInfo response = m_urlLoader.GetResponseInfo();
        qDebug() << "onOpen http status" << response.GetStatusCode();
        int64_t sofar = 0;
        int64_t total = 0;
        m_urlLoader.GetDownloadProgress(&sofar, &total);
        qDebug() << "content sizes" << sofar << total;
        m_urlRequest.SetRecordDownloadProgress(false);
        
        read();
    }

    void read()
    {
        int32_t result = PP_OK;
        pp::CompletionCallback readCallback = callbackFactory.NewCallback(&UrlLoader::onRead);
        do {
          result = m_urlLoader.ReadResponseBody(m_buffer, ReadBufferSize, readCallback);
          // Handle streaming data directly. Note that we *don't* want to call
          // OnRead here, since in the case of result > 0 it will schedule
          // another call to this function. If the network is very fast, we could
          // end up with a deeply recursive stack.
          if (result > 0) {
              qDebug() << "got data bytes" << result;
            //AppendDataBytes(buffer_, result);
          }
        } while (result > 0);

        if (result != PP_OK_COMPLETIONPENDING) {
          // Either we reached the end of the stream (result == PP_OK) or there was
          // an error. We want OnRead to get called no matter what to handle
          // that case, whether the error is synchronous or asynchronous. If the
          // result code *is* COMPLETIONPENDING, our callback will be called
          // asynchronously.
          readCallback.Run(result);
        }
    }
    
    void onRead(int32_t result)
    {
        if (result == PP_OK) {
            // Streaming the file is complete, delete the read buffer since it is
            // no longer needed.
            delete[] m_buffer;
            m_buffer = NULL;
//          ReportResultAndDie(url_, url_response_body_, true);
        } else if (result > 0) {
            // The URLLoader just filled "result" number of bytes into our buffer.
            // Save them and perform another read.
            qDebug() << "got data bytes" << result;
            //AppendDataBytes(buffer_, result);
            read();
        } else {
            // A read error occurred.
            qDebug() << "error";
        }
    }


private:
    QString m_url;

    pp::Instance* m_instance;
    pp::URLRequestInfo m_urlRequest;
    pp::URLLoader m_urlLoader;
    char* m_buffer;
    std::string m_urlResponseBody;
    pp::CompletionCallbackFactory<UrlLoader> callbackFactory;
};


QWindow *window = 0;

void fireAndForgetRequest(const QString &url)
{
    UrlLoader* urlLoader = new UrlLoader(url); 
    urlLoader->load();
}

QNetworkAccessManager *qnam;
void qnamRequest(const QString &url)
{
    QNetworkRequest request;
    request.setUrl(QUrl(url));
//    request.setRawHeader("User-Agent", "MyOwnBrowser 1.0");

    QNetworkReply *reply = qnam->get(request);
}

// App-provided init and exit functions:
void app_init(const QStringList &arguments)
{
    qDebug() << "Running app_init";

    window = new QWindow();
    window->show();
    
//    fireAndForgetRequest("urlload.nmf"); // small file
//    fireAndForgetRequest("http://localhost:8000/urlload.nmf"); // full url
//    fireAndForgetRequest("urlload.nexe"); // large file
//    fireAndForgetRequest("foobar2000"); // missing file

    qnam = new QNetworkAccessManager;
    qnamRequest("urlload.nmf");
    qnamRequest("urlload.nexe");
}

void app_exit()
{
    delete window;
    qDebug() << "Running app_exit";
}

// Register functions with Qt. The type of register function
// selects the QApplicaiton type.
Q_GUI_MAIN(app_init, app_exit);
