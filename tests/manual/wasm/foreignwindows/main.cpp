// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QtWidgets>

#include <emscripten.h>
#include <emscripten/val.h>

using emscripten::val;


val createInputElemet(std::string subtype)
{
    val document = val::global("document");
    val input = document.call<val>("createElement", std::string("input"));
    input.set("type", subtype);
    return input;
}

val createCalendar()
{
    val calendar = val::global("document").call<val>("createElement", std::string("calendar-date"));
    calendar.set("innerHTML", std::string("<calendar-month></calendar-month>"));
    return calendar;
}

val createCallyElemet()
{
    static bool initializedCalendarComponent = []{
        return EM_ASM_INT(
            console.log("Loading calendar module");
            var script = document.createElement('script');
            script.src = "https://unpkg.com/cally";
            script.type = "module";
            document.head.appendChild(script);
            console.log(script);
            return true;
        );
    }();
    Q_ASSERT(initializedCalendarComponent);

    return createCalendar();
}

class ForeginWindowContainer : public QWidget
{
Q_OBJECT
public:
    ForeginWindowContainer() {

        QCheckBox *test = new QCheckBox("Qt CheckBox");
        test->setGeometry(20, 20, 150, 20);
        test->setParent(this);

        m_calendarInput = std::make_unique<val>(createInputElemet("date"));
        m_colorPickerInput = std::make_unique<val>(createInputElemet("color"));

        QWindow *calendarWindow = QWindow::fromWinId(WId(m_calendarInput.get()));
        QWindow *colorPickerWindow = QWindow::fromWinId(WId(m_colorPickerInput.get()));

        QWidget *calendarContainer = QWidget::createWindowContainer(calendarWindow, this);
        calendarContainer->setGeometry(20, 50, 300, 40);

        QWidget *colorPickerContainer = QWidget::createWindowContainer(colorPickerWindow, this);
        colorPickerContainer->setGeometry(20, 90, 300, 40);

        val callyCalendarElement = createCallyElemet();
        QWindow *callyWindow =QWindow::fromWinId(WId(new val(callyCalendarElement)));
        QWidget *callyContainer = QWidget::createWindowContainer(callyWindow, this);
        callyContainer->setGeometry(20, 130, 300, 400);
    }

    ~ForeginWindowContainer() {
    }

private:
    std::unique_ptr<val> m_calendarInput;
    std::unique_ptr<val> m_colorPickerInput;
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);

    ForeginWindowContainer container;
    container.showNormal();

    return app.exec();
}

#include "main.moc"
