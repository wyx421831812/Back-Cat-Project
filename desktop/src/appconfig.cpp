#include "appconfig.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtMath>

AppConfig &AppConfig::instance()
{
    static AppConfig inst;
    return inst;
}

AppConfig::AppConfig()
    : QObject()
{
    seedDefaults();
}

void AppConfig::seedDefaults()
{
    // 旧版键默认值 (迁移期保留)
    m_config["themeName"] = QStringLiteral("yellowPurple");
    m_config["primaryColor"] = QStringLiteral("#fbbf24");
    m_config["secondaryColor"] = QStringLiteral("#8b5cf6");
    m_config["accentColor"] = QStringLiteral("#6366f1");
    m_config["currentComponentIndex"] = 0;
    m_config["petModelType"] = 0;       // 默认3D模型: CatLike
    m_config["windowX"] = -1;
    m_config["windowY"] = -1;
    m_config["windowWidth"] = 200;
    m_config["windowHeight"] = 250;

    // ---- cat.model ----
    QVariantMap catModel {
        {"mirror", false},
        {"mouseMirror", false},
        {"ignoreMouse", false},
        {"motionSound", true},
        {"behavior", true},
        {"autoReleaseDelaySec", 3},
        {"maxFPS", 60}
    };

    // ---- cat.window ----
    QVariantMap catWindow {
        {"visible", true},
        {"passThrough", false},
        {"alwaysOnTop", true},
        {"scale", 100},
        {"opacity", 100},
        {"radius", 0},
        {"hideOnHover", false},
        {"hideOnHoverDelay", 0},
        {"keepInScreen", true}
    };

    QVariantMap cat {
        {"model", catModel},
        {"window", catWindow}
    };

    // ---- general ----
    QVariantMap generalApp {
        {"autostart", false},
        {"taskbarVisible", false},
        {"trayVisible", true}
    };
    QVariantMap appearance {
        {"theme", QStringLiteral("auto")}
    };
    QVariantMap general {
        {"app", generalApp},
        {"appearance", appearance}
    };

    // ---- shortcuts (空串=未设置) ----
    QVariantMap shortcuts {
        {"toggleCat", QString()},
        {"togglePreference", QString()},
        {"mirror", QString()},
        {"passThrough", QString()},
        {"alwaysOnTop", QString()}
    };

    // ---- 桌面挂件 ----
    QVariantMap widgets {
        {"clock", false},
        {"quote", false}
    };

    m_config["schemaVersion"] = 2;
    m_config["cat"] = cat;
    m_config["general"] = general;
    m_config["shortcuts"] = shortcuts;
    m_config["widgets"] = widgets;
}

QString AppConfig::configPath() const
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(base);
    return base + "/backpet_config.json";
}

QString AppConfig::appDataDir() const
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(base);
    return base;
}

void AppConfig::load()
{
    QFile file(configPath());
    if (!file.open(QFile::ReadOnly)) {
        return;
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    // 以原始 JSON 是否带 schemaVersion 判定旧版扁平配置
    // (不能在合并后判断: 构造函数已预植默认分组)
    bool fromLegacy = false;
    m_rawKeys.clear();
    if (doc.isObject()) {
        const QJsonObject obj = doc.object();
        fromLegacy = !obj.contains("schemaVersion");
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            m_config[it.key()] = it.value().toVariant();
            m_rawKeys.append(it.key());
        }
    }

    migrateLegacy(fromLegacy);
}

void AppConfig::migrateLegacy(bool fromLegacy)
{

    // 重新基于默认分组补全缺失项 (保证新字段存在且不覆盖已有值)
    AppConfig fresh;
    const QStringList groups = {"cat", "general", "shortcuts", "widgets"};
    for (const QString &g : groups) {
        QVariantMap dst = m_config.value(g).toMap();
        if (dst.isEmpty()) {
            dst = fresh.m_config.value(g).toMap();
        } else {
            // 补全嵌套两层
            QVariantMap def = fresh.m_config.value(g).toMap();
            for (auto it = def.begin(); it != def.end(); ++it) {
                if (!dst.contains(it.key())) {
                    dst[it.key()] = it.value();
                } else if (it.value().type() == QVariant::Map && dst.value(it.key()).type() == QVariant::Map) {
                    QVariantMap sub = dst.value(it.key()).toMap();
                    const QVariantMap defSub = it.value().toMap();
                    for (auto sit = defSub.begin(); sit != defSub.end(); ++sit) {
                        if (!sub.contains(sit.key())) {
                            sub[sit.key()] = sit.value();
                        }
                    }
                    dst[it.key()] = sub;
                }
            }
        }
        m_config[g] = dst;
    }

    m_config["schemaVersion"] = 2;

    if (!fromLegacy) {
        return;
    }

    // 旧扁平键 -> 新分组迁移 (仅当旧值存在时覆盖默认分组值)
    QVariantMap cat = m_config.value("cat").toMap();
    QVariantMap catWindow = cat.value("window").toMap();
    QVariantMap catModel = cat.value("model").toMap();
    QVariantMap general = m_config.value("general").toMap();
    QVariantMap generalApp = general.value("app").toMap();

    if (m_config.contains("windowOpacity")) {
        catWindow["opacity"] = qBound(10, m_config.value("windowOpacity").toInt(), 100);
    }
    if (m_config.contains("alwaysOnTop")) {
        catWindow["alwaysOnTop"] = m_config.value("alwaysOnTop").toBool();
    }
    if (m_config.contains("clickThrough")) {
        catWindow["passThrough"] = m_config.value("clickThrough").toBool();
    }
    if (m_config.contains("startWithSystem")) {
        generalApp["autostart"] = m_config.value("startWithSystem").toBool();
    }
    if (m_config.contains("autoReleaseDelay")) {
        // 旧值为毫秒; 向下取整为秒 (整除, 避免四舍五入意外延长释放时间)
        const int ms = m_config.value("autoReleaseDelay").toInt();
        catModel["autoReleaseDelaySec"] = qMax(0, ms / 1000);
    }

    cat["window"] = catWindow;
    cat["model"] = catModel;
    general["app"] = generalApp;
    m_config["cat"] = cat;
    m_config["general"] = general;
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

bool AppConfig::rawHasKey(const QString &key) const
{
    return m_rawKeys.contains(key);
}

void AppConfig::set(const QString &key, const QVariant &value)
{
    m_config[key] = value;
}

// ---- 分组读写 ----

QVariant AppConfig::groupValue(const QString &groupPath, const QString &key,
                               const QVariant &def) const
{
    const QStringList parts = groupPath.split('.');
    QVariantMap cur = m_config;
    for (const QString &p : parts) {
        const QVariant v = cur.value(p);
        if (v.type() != QVariant::Map) {
            return def;
        }
        cur = v.toMap();
    }
    return cur.value(key, def);
}

void AppConfig::setGroupValue(const QString &groupPath, const QString &key,
                              const QVariant &value, const char *signal)
{
    const QStringList parts = groupPath.split('.');

    // 自顶向下收集每一级 map (maps[0] 为根, 缺失层级以空 map 补)
    QList<QVariantMap> maps;
    maps << m_config;
    for (const QString &p : parts) {
        const QVariant v = maps.last().value(p);
        maps << (v.type() == QVariant::Map ? v.toMap() : QVariantMap());
    }

    // 写入叶子, 再自底向上回写
    maps.last()[key] = value;
    for (int i = maps.size() - 2; i >= 0; --i) {
        maps[i][parts[i]] = maps[i + 1];
    }
    m_config = maps.first();

    save();
    if (signal) {
        // SIGNAL() 宏结果带前缀码 ('2'); invokeMethod 对其会静默失败,
        // 用 indexOfSignal + activate 直接发射, 保证信号真实送达
        const char *normSig = (signal[0] == '1' || signal[0] == '2') ? signal + 1 : signal;
        const int idx = metaObject()->indexOfSignal(normSig);
        if (idx >= 0) {
            void *args[] = {nullptr};
            QMetaObject::activate(this, idx, args);
        } else {
            qWarning() << "[AppConfig] unknown signal:" << normSig;
        }
    }
}

// ---- cat.model ----

bool AppConfig::mirror() const        { return groupValue("cat.model", "mirror", false).toBool(); }
bool AppConfig::mouseMirror() const   { return groupValue("cat.model", "mouseMirror", false).toBool(); }
bool AppConfig::ignoreMouse() const   { return groupValue("cat.model", "ignoreMouse", false).toBool(); }
bool AppConfig::motionSound() const   { return groupValue("cat.model", "motionSound", true).toBool(); }
bool AppConfig::behavior() const      { return groupValue("cat.model", "behavior", true).toBool(); }
int  AppConfig::autoReleaseDelaySec() const { return groupValue("cat.model", "autoReleaseDelaySec", 3).toInt(); }
int  AppConfig::maxFps() const        { return groupValue("cat.model", "maxFPS", 60).toInt(); }
QString AppConfig::modelKind() const  { return groupValue("cat.model", "kind", "live2d").toString(); }
int  AppConfig::pet3dType() const {
    // 分组键优先; 尚未迁移的旧配置回退到扁平 petModelType
    QVariant v = groupValue("cat.model", "pet3dType");
    if (v.isValid()) return v.toInt();
    return m_config.value("petModelType", 0).toInt();
}

void AppConfig::setMirror(bool on)        { setGroupValue("cat.model", "mirror", on, SIGNAL(catModelChanged())); }
void AppConfig::setMouseMirror(bool on)   { setGroupValue("cat.model", "mouseMirror", on, SIGNAL(catModelChanged())); }
void AppConfig::setIgnoreMouse(bool on)   { setGroupValue("cat.model", "ignoreMouse", on, SIGNAL(catModelChanged())); }
void AppConfig::setMotionSound(bool on)   { setGroupValue("cat.model", "motionSound", on, SIGNAL(catModelChanged())); }
void AppConfig::setBehavior(bool on)      { setGroupValue("cat.model", "behavior", on, SIGNAL(catModelChanged())); }
void AppConfig::setAutoReleaseDelaySec(int sec) { setGroupValue("cat.model", "autoReleaseDelaySec", qBound(0, sec, 60), SIGNAL(catModelChanged())); }
void AppConfig::setMaxFps(int fps)        { setGroupValue("cat.model", "maxFPS", qBound(0, fps, 240), SIGNAL(catModelChanged())); }
void AppConfig::setModelKind(const QString &kind) {
    setGroupValue("cat.model", "kind",
                  (kind == QLatin1String("pet3d")) ? QLatin1String("pet3d") : QLatin1String("live2d"),
                  SIGNAL(catModelChanged()));
}
void AppConfig::setPet3dType(int type)    { setGroupValue("cat.model", "pet3dType", qBound(0, type, 4), SIGNAL(catModelChanged())); }

// ---- cat.window ----

bool AppConfig::catVisible() const    { return groupValue("cat.window", "visible", true).toBool(); }
bool AppConfig::passThrough() const   { return groupValue("cat.window", "passThrough", false).toBool(); }
bool AppConfig::alwaysOnTop() const   { return groupValue("cat.window", "alwaysOnTop", true).toBool(); }
int  AppConfig::windowScale() const   { return groupValue("cat.window", "scale", 100).toInt(); }
int  AppConfig::windowOpacity() const { return groupValue("cat.window", "opacity", 100).toInt(); }
int  AppConfig::windowRadius() const  { return groupValue("cat.window", "radius", 0).toInt(); }
bool AppConfig::hideOnHover() const   { return groupValue("cat.window", "hideOnHover", false).toBool(); }
int  AppConfig::hideOnHoverDelay() const { return groupValue("cat.window", "hideOnHoverDelay", 0).toInt(); }
bool AppConfig::keepInScreen() const  { return groupValue("cat.window", "keepInScreen", true).toBool(); }

void AppConfig::setCatVisible(bool on)    { setGroupValue("cat.window", "visible", on, SIGNAL(catWindowChanged())); }
void AppConfig::setPassThrough(bool on)   { setGroupValue("cat.window", "passThrough", on, SIGNAL(catWindowChanged())); }
void AppConfig::setAlwaysOnTop(bool on)   { setGroupValue("cat.window", "alwaysOnTop", on, SIGNAL(catWindowChanged())); }
void AppConfig::setWindowScale(int p)     { setGroupValue("cat.window", "scale", qBound(1, p, 500), SIGNAL(catWindowChanged())); }
void AppConfig::setWindowOpacity(int p)   { setGroupValue("cat.window", "opacity", qBound(10, p, 100), SIGNAL(catWindowChanged())); }
void AppConfig::setWindowRadius(int p)    { setGroupValue("cat.window", "radius", qBound(0, p, 50), SIGNAL(catWindowChanged())); }
void AppConfig::setHideOnHover(bool on)   { setGroupValue("cat.window", "hideOnHover", on, SIGNAL(catWindowChanged())); }
void AppConfig::setHideOnHoverDelay(int s){ setGroupValue("cat.window", "hideOnHoverDelay", qBound(0, s, 60), SIGNAL(catWindowChanged())); }
void AppConfig::setKeepInScreen(bool on)  { setGroupValue("cat.window", "keepInScreen", on, SIGNAL(catWindowChanged())); }

// ---- general ----

bool AppConfig::autoStart() const      { return groupValue("general.app", "autostart", false).toBool(); }
bool AppConfig::taskbarVisible() const { return groupValue("general.app", "taskbarVisible", false).toBool(); }
bool AppConfig::trayVisible() const    { return groupValue("general.app", "trayVisible", true).toBool(); }
QString AppConfig::themeMode() const   { return groupValue("general.appearance", "theme", "auto").toString(); }

void AppConfig::setAutoStart(bool on)      { setGroupValue("general.app", "autostart", on, SIGNAL(generalChanged())); }
void AppConfig::setTaskbarVisible(bool on) { setGroupValue("general.app", "taskbarVisible", on, SIGNAL(generalChanged())); }
void AppConfig::setTrayVisible(bool on)    { setGroupValue("general.app", "trayVisible", on, SIGNAL(generalChanged())); }
void AppConfig::setThemeMode(const QString &mode)
{
    QString m = mode;
    if (m != "light" && m != "dark") {
        m = "auto";
    }
    setGroupValue("general.appearance", "theme", m, SIGNAL(generalChanged()));
}

// ---- shortcuts ----

QString AppConfig::shortcut(const QString &action) const
{
    return groupValue("shortcuts", action, QString()).toString();
}

void AppConfig::setShortcut(const QString &action, const QString &accelerator)
{
    setGroupValue("shortcuts", action, accelerator, SIGNAL(shortcutsChanged()));
}

// ---- widgets ----

bool AppConfig::clockWidgetVisible() const { return groupValue("widgets", "clock", false).toBool(); }
bool AppConfig::quoteWidgetVisible() const { return groupValue("widgets", "quote", false).toBool(); }

void AppConfig::setClockWidgetVisible(bool on) { setGroupValue("widgets", "clock", on, SIGNAL(widgetsChanged())); }
void AppConfig::setQuoteWidgetVisible(bool on) { setGroupValue("widgets", "quote", on, SIGNAL(widgetsChanged())); }

// ---- 旧版扁平 API (映射到新分组/遗留键) ----

QColor AppConfig::primaryColor() const   { return QColor(m_config.value("primaryColor").toString()); }
QColor AppConfig::secondaryColor() const { return QColor(m_config.value("secondaryColor").toString()); }
QColor AppConfig::accentColor() const    { return QColor(m_config.value("accentColor").toString()); }
bool AppConfig::clickThrough() const     { return passThrough(); }
bool AppConfig::startWithSystem() const  { return autoStart(); }
int AppConfig::currentComponentIndex() const { return m_config.value("currentComponentIndex").toInt(); }
QString AppConfig::themeName() const     { return m_config.value("themeName").toString(); }
int AppConfig::petModelType() const      { return m_config.value("petModelType").toInt(); }

QPoint AppConfig::windowPosition() const
{
    return QPoint(m_config.value("windowX").toInt(), m_config.value("windowY").toInt());
}

QSize AppConfig::windowSize() const
{
    return QSize(m_config.value("windowWidth").toInt(),
                 m_config.value("windowHeight").toInt());
}

int AppConfig::autoReleaseDelay() const
{
    return autoReleaseDelaySec() * 1000;
}

void AppConfig::setPrimaryColor(const QColor &c)   { m_config["primaryColor"] = c.name(); save(); }
void AppConfig::setSecondaryColor(const QColor &c) { m_config["secondaryColor"] = c.name(); save(); }
void AppConfig::setAccentColor(const QColor &c)    { m_config["accentColor"] = c.name(); save(); }
void AppConfig::setClickThrough(bool on)           { setPassThrough(on); }
void AppConfig::setStartWithSystem(bool on)        { setAutoStart(on); }
void AppConfig::setCurrentComponentIndex(int idx)  { m_config["currentComponentIndex"] = idx; save(); }
void AppConfig::setThemeName(const QString &name)  { m_config["themeName"] = name; save(); }
void AppConfig::setPetModelType(int type)          { m_config["petModelType"] = type; save(); }

void AppConfig::setWindowPosition(const QPoint &pos)
{
    m_config["windowX"] = pos.x();
    m_config["windowY"] = pos.y();
    save();
}

void AppConfig::setWindowSize(const QSize &size)
{
    m_config["windowWidth"] = size.width();
    m_config["windowHeight"] = size.height();
    save();
}

void AppConfig::setAutoReleaseDelay(int ms)
{
    setAutoReleaseDelaySec(qRound(ms / 1000.0));
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
