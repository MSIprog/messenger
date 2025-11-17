#pragma once

#include <QSettings>

class Settings
{
public:
    static Settings &get();

    void setValue(QAnyStringView a_key, const QVariant &a_value);
    QVariant value(QAnyStringView a_key);

private:
    Settings();

    std::unique_ptr<QSettings> m_settings;
};
