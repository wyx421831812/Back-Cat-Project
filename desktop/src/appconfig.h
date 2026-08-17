#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QObject>
#include <QVariantMap>
#include <QVariant>
#include <QColor>
#include <QPoint>
#include <QSize>
#include <QStandardPaths>
#include <QDir>

/**
 * @brief 应用配置管理类
 *
 * 负责读取和保存用户配置 (JSON 格式)
 * 使用 QVariantMap 内部存储，避免 QJsonObject 的类型限制
 */
class AppConfig : public QObject
{
    Q_OBJECT

public:
    // 预设主题配色
    struct ThemePreset {
        QString name;
        QColor primary;
        QColor secondary;
        QColor accent;
    };

    static AppConfig &instance();

    // 加载/保存配置
    void load();
    void save();

    // 通用 getter/setter
    QVariant get(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void set(const QString &key, const QVariant &value);

    // 快捷方法
    QColor primaryColor() const;
    QColor secondaryColor() const;
    QColor accentColor() const;
    int windowOpacity() const;        // 0-100
    bool alwaysOnTop() const;
    bool clickThrough() const;
    bool startWithSystem() const;
    int currentComponentIndex() const;
    QString themeName() const;
    int petModelType() const;          // 0=CatLike, 1=BearLike, 2=BunnyLike
    QPoint windowPosition() const;
    QSize windowSize() const;
    int autoReleaseDelay() const;          // L-105: 按键松开延时恢复 (ms)

    void setPrimaryColor(const QColor &c);
    void setSecondaryColor(const QColor &c);
    void setAccentColor(const QColor &c);
    void setWindowOpacity(int opacity);
    void setAlwaysOnTop(bool on);
    void setClickThrough(bool on);
    void setStartWithSystem(bool on);
    void setCurrentComponentIndex(int idx);
    void setThemeName(const QString &name);
    void setPetModelType(int type);
    void setWindowPosition(const QPoint &pos);
    void setWindowSize(const QSize &size);
    void setAutoReleaseDelay(int ms);      // L-105: 设置延时恢复时间

    // 预设主题列表
    QList<ThemePreset> themePresets() const;
    void applyTheme(const QString &themeName);

private:
    AppConfig();
    AppConfig(const AppConfig &) = delete;
    AppConfig &operator=(const AppConfig &) = delete;

    QString configPath() const;
    QVariantMap m_config;
};

#endif // APPCONFIG_H
