// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QApplication>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QLabel>

QString createStyleSheet(const char *sortArrowPos)
{
    QString styleSheet {R"(
        QHeaderView::section {
            background-color: #f0f0f0;
            padding: 5px;
            border: 1px solid #ffffff;
            font-weight: bold;
        }

        QHeaderView::up-arrow, QHeaderView::down-arrow {
            width: 24px;
            height: 24px;
            subcontrol-position: %1;
            subcontrol-origin: padding;
        }
    )"};

    return styleSheet.arg(sortArrowPos);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget mainWidget;
    mainWidget.setWindowTitle("QTableWidget sort indicator overlap with clipping");

    const QStringList headerLabels { "Header", "LongHeaderText"};
    const QStringList column1 { "Alpha", "Beta", "Gamma" };
    const QStringList column2 { "1", "2", "3" };

    QTableWidget tableWidget(3, 2, &mainWidget);
    tableWidget.setHorizontalHeaderLabels(headerLabels);

    for (int i {0} ; i < column1.size() ; ++i) {
        tableWidget.setItem(i, 0, new QTableWidgetItem(column1[i]));
        tableWidget.setItem(i, 1, new QTableWidgetItem(column2[i]));
    }

    tableWidget.setSortingEnabled(true);
    tableWidget.setStyleSheet(createStyleSheet("center right"));
    tableWidget.adjustSize();

    QVBoxLayout mainLayout {&mainWidget};
    QGroupBox buttonBox;
    QVBoxLayout buttonLayout {&buttonBox};
    QRadioButton leftButton {"Left-aligned sort indicator"};
    QRadioButton centerButton {"Center-aligned sort indicator"};
    QRadioButton rightButton {"Right-aligned sort indicator"};

    buttonLayout.addWidget(&leftButton);
    buttonLayout.addWidget(&centerButton);
    buttonLayout.addWidget(&rightButton);

    mainLayout.addWidget(&tableWidget);
    mainLayout.addWidget(&buttonBox);

    QLabel instructions { QT_TR_NOOP(R"(<html>Instructions:
<ol>
<li>There are 3 options for alignment of the column header sort arrow: left, center, and right. Click one of the 3 radio buttons to select the sort arrow alignment.
</li>
<li>Click the left column header to sort the table. The sort arrow should appear at its correct alignment without overlapping the text.
</li>
<li>Click the right column header. The sort arrow should appear at its correct alignment. The left and right alignment should clip the text without changing its position. The center alignment should not clip the text at all.
</ol>
    </html>)")};
    instructions.setTextFormat(Qt::AutoText);
    instructions.setWordWrap(true);
    mainLayout.addWidget(&instructions);

    QObject::connect(&leftButton, &QRadioButton::clicked, &app, [&](bool checked) {
        if (checked)
            tableWidget.setStyleSheet(createStyleSheet("center left"));
    });
    QObject::connect(&centerButton, &QRadioButton::clicked, &app, [&](bool checked) {
        if (checked)
            tableWidget.setStyleSheet(createStyleSheet("top center"));
    });
    QObject::connect(&rightButton, &QRadioButton::clicked, &app, [&](bool checked) {
        if (checked)
            tableWidget.setStyleSheet(createStyleSheet("center right"));
    });

    leftButton.click();
    mainWidget.show();

    return app.exec();
}
