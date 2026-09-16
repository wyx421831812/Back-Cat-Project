#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QObject>
#include <QVariantMap>
#include <QStringList>
#include <QVariant>
#include <QColor>
#include <QPoint>
#include <QSize>
#include <QStandardPaths>
#include <QDir>

/**
 * @brief 应用配置管理类 (v2, 对齐 BongoCat 分组结构)
 *
 * JSON 结构:
 * {
 *   "schemaVersion": 2,
 *   "cat": {
 *     "model":  { mirror, mouseMirror, ignoreMouse, motionSound, behavior,
 *                 autoReleaseDelaySec, maxFPS },
 *     "window": { visible, passThrough, alwaysOnTop, scale, opacity, radius,
 *                 hideOnHover, hideOnHoverDelay, keepInScreen }
 *   },
 *   "general": {
 *     "app":        { autostart, taskbarVisible, trayVisible },
 *     "appearance": { theme }              // auto | light | dark
 *   },
 *   "shortcuts": { toggleCat, togglePreference, mirror, passThrough, alwaysOnTop },
 *   "widgets":   { clock, quote }
 * }
 *
 * 旧版扁平键 (windowOpacity/alwaysOnTop/clickThrough/startWithSystem/
 * autoReleaseDelay 等) 在 load() 时自动迁移到分组。
 */
class AppConfig : public QObject
{
    Q_OBJECT
public:
    // 预设主题配色 (旧版设置对话框遗留, 供 3D 宠物上色复用)
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

    // 通用 getter/setter (兼容旧调用)
    QVariant get(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void set(const QString &key, const QVariant &value);
    // 原始 JSON 是否真正包含该扁平键 (构造函数预植默认值不算)
    bool rawHasKey(const QString &key) const;

    // 应用数据目录 (日志/待办/用户模型等)
    QString appDataDir() const;

    // ---- cat.model ----
    bool mirror() const;
    bool mouseMirror() const;
    bool ignoreMouse() const;
    bool motionSound() const;
    bool behavior() const;
    int  autoReleaseDelaySec() const;
    int  maxFps() const;
    QString modelKind() const;  // "live2d" | "pet3d" (PetStage 当前模型族)
    int  pet3dType() const;     // 0-4, 新分组缺省时回退旧 petModelType

    void setMirror(bool on);
    void setMouseMirror(bool on);
    void setIgnoreMouse(bool on);
    void setMotionSound(bool on);
    void setBehavior(bool on);
    void setAutoReleaseDelaySec(int sec);
    void setMaxFps(int fps);
    void setModelKind(const QString &kind);
    void setPet3dType(int type);

    // ---- cat.window ----
    bool catVisible() const;
    bool passThrough() const;
    bool alwaysOnTop() const;
    int  windowScale() const;      // 1-500 (%)
    int  windowOpacity() const;    // 10-100 (%)
    int  windowRadius() const;     // (%)
    bool hideOnHover() const;
    int  hideOnHoverDelay() const; // 秒, 0=立即
    bool keepInScreen() const;

    void setCatVisible(bool on);
    void setPassThrough(bool on);
    void setAlwaysOnTop(bool on);
    void setWindowScale(int percent);
    void setWindowOpacity(int percent);
    void setWindowRadius(int percent);
    void setHideOnHover(bool on);
    void setHideOnHoverDelay(int sec);
    void setKeepInScreen(bool on);

    // ---- general ----
    bool autoStart() const;
    bool taskbarVisible() const;
    bool trayVisible() const;
    QString themeMode() const;     // auto | light | dark

    void setAutoStart(bool on);
    void setTaskbarVisible(bool on);
    void setTrayVisible(bool on);
    void setThemeMode(const QString &mode);

    // ---- shortcuts (空串=未设置) ----
    QString shortcut(const QString &action) const;
    void setShortcut(const QString &action, const QString &accelerator);

    // ---- 桌面挂件 ----
    bool clockWidgetVisible() const;
    bool quoteWidgetVisible() const;
    void setClockWidgetVisible(bool on);
    void setQuoteWidgetVisible(bool on);

    // ---- 旧版扁平 API (迁移期保留, 内部映射到分组) ----
    QColor primaryColor() const;
    QColor secondaryColor() const;
    QColor accentColor() const;
    bool clickThrough() const;
    bool startWithSystem() const;
    int  currentComponentIndex() const;
    QString themeName() const;
    int  petModelType() const;
    QPoint windowPosition() const;
    QSize windowSize() const;
    int  autoReleaseDelay() const;  // 毫秒 (由秒换算)

    void setPrimaryColor(const QColor &c);
    void setSecondaryColor(const QColor &c);
    void setAccentColor(const QColor &c);
    void setClickThrough(bool on);
    void setStartWithSystem(bool on);
    void setCurrentComponentIndex(int idx);
    void setThemeName(const QString &name);
    void setPetModelType(int type);
    void setWindowPosition(const QPoint &pos);
    void setWindowSize(const QSize &size);
    void setAutoReleaseDelay(int ms);

    QList<ThemePreset> themePresets() const;
    void applyTheme(const QString &themeName);

signals:
    void catModelChanged();
    void catWindowChanged();
    void generalChanged();
    void shortcutsChanged();
    void widgetsChanged();

private:
    AppConfig();
    AppConfig(const AppConfig &) = delete;
    AppConfig &operator=(const AppConfig &) = delete;

    void seedDefaults();
    void migrateLegacy(bool fromLegacy);

    // 分组读写: groupPath 形如 "cat.model"
    QVariant groupValue(const QString &groupPath, const QString &key,
                        const QVariant &def = QVariant()) const;
    void setGroupValue(const QString &groupPath, const QString &key,
                       const QVariant &value, const char *signal = nullptr);

    QString configPath() const;
    QVariantMap m_config;
    QStringList m_rawKeys;   // 原始 JSON 中实际出现的扁平键
};

#endif // APPCONFIG_H
