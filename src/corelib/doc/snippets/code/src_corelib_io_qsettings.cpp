// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QSettings settings("MySoft", "Star Runner");
QColor color = settings.value("DataPump/bgcolor").value<QColor>();
//! [0]


//! [1]
QSettings settings("MySoft", "Star Runner");
QColor color = palette().background().color();
settings.setValue("DataPump/bgcolor", color);
//! [1]


//! [2]
QSettings settings("/home/petra/misc/myapp.ini",
                   QSettings::IniFormat);
//! [2]


//! [3]
QSettings settings("/Users/petra/misc/myapp.plist",
                   QSettings::NativeFormat);
//! [3]


//! [4]
QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Office",
                   QSettings::NativeFormat);
//! [4]


//! [5]
settings.setValue("11.0/Outlook/Security/DontTrustInstalledFiles", 0);
//! [5]


//! [6]
settings.setValue("HKEY_CURRENT_USER\\MySoft\\Star Runner\\Galaxy", "Milkyway");
settings.setValue("HKEY_CURRENT_USER\\MySoft\\Star Runner\\Galaxy\\Sun", "OurStar");
settings.value("HKEY_CURRENT_USER\\MySoft\\Star Runner\\Galaxy\\Default"); // returns "Milkyway"
//! [6]


//! [7]
#ifdef Q_OS_DARWIN
    QSettings settings("grenoullelogique.fr", "Squash");
#else
    QSettings settings("Grenoulle Logique", "Squash");
#endif
//! [7]


//! [8]
pos = @Point(100 100)
//! [8]


//! [9]
windir = C:\Windows
//! [9]


//! [10]
QSettings settings("Moose Tech", "Facturo-Pro");
//! [10]


//! [11]
QSettings settings("Moose Soft", "Facturo-Pro");
//! [11]


//! [12]
QCoreApplication::setOrganizationName("Moose Soft");
QCoreApplication::setApplicationName("Facturo-Pro");
QSettings settings;
//! [12]


//! [13]
settings.beginGroup("mainwindow");
settings.setValue("size", win->size());
settings.setValue("fullScreen", win->isFullScreen());
settings.endGroup();

settings.beginGroup("outputpanel");
settings.setValue("visible", panel->isVisible());
settings.endGroup();
//! [13]


//! [14]
settings.beginGroup("alpha");
// settings.group() == "alpha"

settings.beginGroup("beta");
// settings.group() == "alpha/beta"

settings.endGroup();
// settings.group() == "alpha"

settings.endGroup();
// settings.group() == ""
//! [14]


//! [15]
struct Login {
    QString userName;
    QString password;
};
QList<Login> logins;
...

QSettings settings;
int size = settings.beginReadArray("logins");
for (int i = 0; i < size; ++i) {
    settings.setArrayIndex(i);
    Login login;
    login.userName = settings.value("userName").toString();
    login.password = settings.value("password").toString();
    logins.append(login);
}
settings.endArray();
//! [15]


//! [16]
struct Login {
    QString userName;
    QString password;
};
QList<Login> logins;
...

QSettings settings;
settings.beginWriteArray("logins");
for (qsizetype i = 0; i < logins.size(); ++i) {
    settings.setArrayIndex(i);
    settings.setValue("userName", logins.at(i).userName);
    settings.setValue("password", logins.at(i).password);
}
settings.endArray();
//! [16]


//! [17]
QSettings settings;
settings.setValue("fridge/color", QColor(Qt::white));
settings.setValue("fridge/size", QSize(32, 96));
settings.setValue("sofa", true);
settings.setValue("tv", false);

QStringList keys = settings.allKeys();
// keys: ["fridge/color", "fridge/size", "sofa", "tv"]
//! [17]


//! [18]
settings.beginGroup("fridge");
keys = settings.allKeys();
// keys: ["color", "size"]
//! [18]


//! [19]
QSettings settings;
settings.setValue("fridge/color", QColor(Qt::white));
settings.setValue("fridge/size", QSize(32, 96));
settings.setValue("sofa", true);
settings.setValue("tv", false);

QStringList keys = settings.childKeys();
// keys: ["sofa", "tv"]
//! [19]


//! [20]
settings.beginGroup("fridge");
keys = settings.childKeys();
// keys: ["color", "size"]
//! [20]


//! [21]
QSettings settings;
settings.setValue("fridge/color", QColor(Qt::white));
settings.setValue("fridge/size", QSize(32, 96));
settings.setValue("sofa", true);
settings.setValue("tv", false);

QStringList groups = settings.childGroups();
// groups: ["fridge"]
//! [21]


//! [22]
settings.beginGroup("fridge");
groups = settings.childGroups();
// groups: []
//! [22]


//! [23]
QSettings settings;
settings.setValue("interval", 30);
settings.value("interval").toInt();     // returns 30

settings.setValue("interval", 6.55);
settings.value("interval").toDouble();  // returns 6.55
//! [23]


//! [24]
QSettings settings;
settings.setValue("ape");
settings.setValue("monkey", 1);
settings.setValue("monkey/sea", 2);
settings.setValue("monkey/doe", 4);

settings.remove("monkey");
QStringList keys = settings.allKeys();
// keys: ["ape"]
//! [24]


//! [25]
QSettings settings;
settings.setValue("ape");
settings.setValue("monkey", 1);
settings.setValue("monkey/sea", 2);
settings.setValue("monkey/doe", 4);

settings.beginGroup("monkey");
settings.remove("");
settings.endGroup();

QStringList keys = settings.allKeys();
// keys: ["ape"]
//! [25]


//! [26]
QSettings settings;
settings.setValue("animal/snake", 58);
settings.value("animal/snake", 1024).toInt();   // returns 58
settings.value("animal/zebra", 1024).toInt();   // returns 1024
settings.value("animal/zebra").toInt();         // returns 0
//! [26]


//! [27]
bool myReadFunc(QIODevice &device, QSettings::SettingsMap &map);
//! [27]


//! [28]
bool myWriteFunc(QIODevice &device, const QSettings::SettingsMap &map);
//! [28]


//! [29]
bool readXmlFile(QIODevice &device, QSettings::SettingsMap &map);
bool writeXmlFile(QIODevice &device, const QSettings::SettingsMap &map);

int main(int argc, char *argv[])
{
    const QSettings::Format XmlFormat =
            QSettings::registerFormat("xml", readXmlFile, writeXmlFile);

    QSettings settings(XmlFormat, QSettings::UserScope, "MySoft",
                       "Star Runner");

    ...
}
//! [29]
