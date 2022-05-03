// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtCore/qmetaobject.h>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qlayout.h>
#include <QtWidgets/qmessagebox.h>

QT_REQUIRE_CONFIG(permissions);
#include <QtCore/qpermissions.h>

class PermissionWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PermissionWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        QVBoxLayout *layout = new QVBoxLayout(this);

        static const QPermission permissions[] = {
            QCameraPermission{},
            QMicrophonePermission{},
            QBluetoothPermission{},
            QContactsPermission{},
            QCalendarPermission{},
            QLocationPermission{}
        };

        for (auto permission : permissions) {
            auto permissionName = QString::fromLatin1(permission.name());
            QPushButton *button = new QPushButton(permissionName.sliced(1, permissionName.length() - 11));
            connect(button, &QPushButton::clicked, this, &PermissionWidget::buttonClicked);
            button->setProperty("permission", QVariant::fromValue(permission));
            layout->addWidget(button);
        }

        QPalette pal = palette();
        pal.setBrush(QPalette::Window, QGradient(QGradient::HappyAcid));
        setPalette(pal);
    }

private:
    void buttonClicked()
    {
        auto *button = static_cast<QPushButton*>(sender());

        auto permission = button->property("permission").value<QPermission>();
        Q_ASSERT(permission.type().isValid());

        switch (qApp->checkPermission(permission)) {
        case Qt::PermissionStatus::Undetermined:
                qApp->requestPermission(permission, this,
                    [this, button](const QPermission &permission) {
                        emit button->clicked(); // Try again
                    }
                );
            return;
        case Qt::PermissionStatus::Denied:
            QMessageBox::warning(this, button->text(),
                tr("Permission is needed to use %1. Please grant permission "\
                "to this application in the system settings.").arg(button->text()));
            return;
        case Qt::PermissionStatus::Granted:
            break; // Proceed
        }

        // All good, can use the feature
        QMessageBox::information(this, button->text(),
            tr("Accessing %1").arg(button->text()));
    }
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    PermissionWidget widget;
    widget.show();
    return app.exec();
}

#include "main.moc"
