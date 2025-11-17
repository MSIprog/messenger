#include <QCommandLineParser>
#include <QDir>
#include "settings.h"

Settings &Settings::get()
{
    static Settings instance;
    return instance;
}

void Settings::setValue(QAnyStringView a_key, const QVariant &a_value)
{
    m_settings->setValue(a_key, a_value);
}

QVariant Settings::value(QAnyStringView a_key)
{
    return m_settings->value(a_key);
}

Settings::Settings()
{
    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    QCommandLineOption cfgOption("cfg", "Configuration file", "file");
    parser.addOption(cfgOption);
    parser.parse(QCoreApplication::instance()->arguments());
    if (parser.isSet(cfgOption))
    {
        auto fileName = parser.value(cfgOption);
        fileName = QCoreApplication::applicationDirPath() + QDir::separator() + fileName;
        m_settings = std::make_unique<QSettings>(fileName, QSettings::IniFormat);
    }
    else
        m_settings = std::make_unique<QSettings>();
}
