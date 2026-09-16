#pragma once

#include <QColor>
#include <QObject>
#include <QString>

// 主题管理: 在亮色 styles.qss 与暗色 styles-dark.qss 间切换,
// 并向自绘控件(NavItem/SwitchButton 等)提供配色令牌。
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    enum Mode {
        Light,
        Dark,
    };
    Q_ENUM(Mode)

    static ThemeManager *instance();

    // 按 "light"/"dark"/"auto" 应用; 返回实际生效模式
    Mode applyFromConfig(const QString &themeMode);
    void apply(Mode mode);

    Mode currentMode() const { return m_mode; }
    bool isDark() const { return m_mode == Dark; }

    // ---- 配色令牌 (供自绘控件使用) ----
    QColor textColor() const;       // 导航未选中文字/图标
    QColor navSelectedBg() const;   // 导航选中底
    QColor navSelectedText() const; // 导航选中文字/图标
    QColor navHoverBg() const;
    QColor switchOffTrack() const;
    QColor cardBg() const;
    QColor borderColor() const;

    // 读取系统深浅色 (Qt 6.5+ QStyleHints::colorScheme)
    static Mode systemMode();

signals:
    // 主题切换后发出, 自绘控件据此重绘
    void themeChanged(Mode mode);

private:
    ThemeManager();
    static void repolishAll();

    Mode m_mode = Light;
};
