#include "appconfig.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

AppConfig &AppConfig::instance()
{
    static AppConfig inst;
    return inst;
}

AppConfig::AppConfig()
    : QObject()
{
    // 默认配置
    m_config["themeName"] = QStringLiteral("yellowPurple");
    m_config["primaryColor"] = QStringLiteral("#fbbf24");
    m_config["secondaryColor"] = QStringLiteral("#8b5cf6");
    m_config["accentColor"] = QStringLiteral("#6366f1");
    m_config["windowOpacity"] = 100;
    m_config["alwaysOnTop"] = true;
    m_config["clickThrough"] = false;
    m_config["startWithSystem"] = false;
    m_config["currentComponentIndex"] = 0;
    m_config["petModelType"] = 0;       // 默认3D模型: CatLike (猫咪)
    m_config["windowX"] = -1;
    m_config["windowY"] = -1;
    m_config["windowWidth"] = 200;
    m_config["windowHeight"] = 250;
}

QString AppConfig::configPath() const
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(base);
    return base + "/backpet_config.json";
}

void AppConfig::load()
{
    QFile file(configPath());
    if (!file.open(QFile::ReadOnly)) {
        return;
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            m_config[it.key()] = it.value().toVariant();
        }
    }
}

void AppConfig::save()
{
    QJsonObject obj;
    for (auto it = m_config.begin(); it != m_config.end(); ++it) {
        obj[it.key()] = QJsonValue::fromVariant(it.value());
    }

    QJsonDocument doc(obj);
    QFile file(configPath());
    if (file.open(QFile::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}

QVariant AppConfig::get(const QString &key, const QVariant &defaultValue) const
{
    return m_config.value(key, defaultValue);
}

void AppConfig::set(const QString &key, const QVariant &value)
{
    m_config[key] = value;
}

// === 快捷方法 ===

QColor AppConfig::primaryColor() const
{
    return QColor(m_config.value("primaryColor").toString());
}

QColor AppConfig::secondaryColor() const
{
    return QColor(m_config.value("secondaryColor").toString());
}

QColor AppConfig::accentColor() const
{
    return QColor(m_config.value("accentColor").toString());
}

int AppConfig::windowOpacity() const
{
    return m_config.value("windowOpacity").toInt();
}

bool AppConfig::alwaysOnTop() const
{
    return m_config.value("alwaysOnTop").toBool();
}

bool AppConfig::clickThrough() const
{
    return m_config.value("clickThrough").toBool();
}

bool AppConfig::startWithSystem() const
{
    return m_config.value("startWithSystem").toBool();
}

int AppConfig::currentComponentIndex() const
{
    return m_config.value("currentComponentIndex").toInt();
}

QString AppConfig::themeName() const
{
    return m_config.value("themeName").toString();
}

int AppConfig::petModelType() const
{
    return m_config.value("petModelType").toInt();
}

QPoint AppConfig::windowPosition() const
{
    int x = m_config.value("windowX").toInt();
    int y = m_config.value("windowY").toInt();
    return QPoint(x, y);
}

QSize AppConfig::windowSize() const
{
    int w = m_config.value("windowWidth").toInt();
    int h = m_config.value("windowHeight").toInt();
    return QSize(w, h);
}

void AppConfig::setPrimaryColor(const QColor &c)
{
    m_config["primaryColor"] = c.name();
}

void AppConfig::setSecondaryColor(const QColor &c)
{
    m_config["secondaryColor"] = c.name();
}

void AppConfig::setAccentColor(const QColor &c)
{
    m_config["accentColor"] = c.name();
}

void AppConfig::setWindowOpacity(int opacity)
{
    m_config["windowOpacity"] = opacity;
}

void AppConfig::setAlwaysOnTop(bool on)
{
    m_config["alwaysOnTop"] = on;
}

void AppConfig::setClickThrough(bool on)
{
    m_config["clickThrough"] = on;
}

void AppConfig::setStartWithSystem(bool on)
{
    m_config["startWithSystem"] = on;
}

void AppConfig::setCurrentComponentIndex(int idx)
{
    m_config["currentComponentIndex"] = idx;
}

void AppConfig::setThemeName(const QString &name)
{
    m_config["themeName"] = name;
}

void AppConfig::setPetModelType(int type)
{
    m_config["petModelType"] = type;
}

void AppConfig::setWindowPosition(const QPoint &pos)
{
    m_config["windowX"] = pos.x();
    m_config["windowY"] = pos.y();
}

void AppConfig::setWindowSize(const QSize &size)
{
    m_config["windowWidth"] = size.width();
    m_config["windowHeight"] = size.height();
}

int AppConfig::autoReleaseDelay() const
{
    return m_config.value("autoReleaseDelay", 100).toInt();
}

void AppConfig::setAutoReleaseDelay(int ms)
{
    m_config["autoReleaseDelay"] = ms;
}

QList<AppConfig::ThemePreset> AppConfig::themePresets() const
{
    return {
        {QStringLiteral("yellowPurple"), QColor("#fbbf24"), QColor("#8b5cf6"), QColor("#6366f1")},
        {QStringLiteral("cyanPurple"),   QColor("#06b6d4"), QColor("#8b5cf6"), QColor("#6366f1")},
        {QStringLiteral("pinkPurple"),   QColor("#ec4899"), QColor("#8b5cf6"), QColor("#6366f1")},
        {QStringLiteral("green"),        QColor("#10b981"), QColor("#06b6d4"), QColor("#3b82f6")},
        {QStringLiteral("orangeRed"),    QColor("#f59e0b"), QColor("#ef4444"), QColor("#6366f1")},
        {QStringLiteral("fairyBird"),    QColor("#06b6d4"), QColor("#f0f9ff"), QColor("#f59e0b")},
        {QStringLiteral("spirit"),       QColor("#7c3aed"), QColor("#f0f9ff"), QColor("#fbbf24")},
    };
}

void AppConfig::applyTheme(const QString &themeName)
{
    for (const auto &preset : themePresets()) {
        if (preset.name == themeName) {
            m_config["themeName"] = themeName;
            m_config["primaryColor"] = preset.primary.name();
            m_config["secondaryColor"] = preset.secondary.name();
            m_config["accentColor"] = preset.accent.name();
            break;
        }
    }
    save();
}
