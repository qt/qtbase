#include <QObject>
#include <QtTest/QtTest>
#include <QDebug>

#include "GConfItem"

// Helper class for listening to signals
class SignalListener : public QObject
{
    Q_OBJECT
public:
    int numberOfCalls;
    SignalListener() : numberOfCalls(0) {
    }

public slots:
    void valueChanged()
    {
        numberOfCalls++;
    }
};

// Tests for the public API
class GConfItemTests : public QObject
{
    Q_OBJECT

    // Stored pointers etc.
private:
    GConfItem *boolItem;
    GConfItem *intItem;
    GConfItem *stringItem;
    GConfItem *doubleItem;
    GConfItem *stringListItem;
    GConfItem *intListItem;
    GConfItem *doubleListItem;
    GConfItem *boolListItem;
    GConfItem *unsetBeforeItem;
    GConfItem *unsetAfterItem;

    SignalListener *signalSpy;

    QTimer timer;
    bool timed_out;

    // Tests
private slots:
    // Init and cleanup helper functions
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    void timeout ();

    // Public API
    void path();
    void external_values();
    void set_bool();
    void set_int();
    void set_string();
    void set_unicode_string();
    void set_double();
    void set_string_list();
    void set_int_list();
    void set_double_list();
    void set_bool_list();
    void unset();
    void get_default();
    void list_dirs();
    void list_entries();
    void propagate();
    void set_external();
};

// Useful if you need to process some events until a condition becomes
// true.

#define QVERIFY_TIMEOUT(msecs, expr)                                        \
    do {                                                                    \
        timed_out = false;                                                  \
        timer.start(msecs);                                                 \
        while (!timed_out && !(expr)) {                                     \
            QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents); \
        }                                                                   \
        QVERIFY((expr));                                                    \
    } while(0)
