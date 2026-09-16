#include "thememanager.h"

#include <QApplication>
#include <QFile>
#include <QGuiApplication>
#include <QStyle>
#include <QStyleHints>
#include <QWidget>

ThemeManager *ThemeManager::instance()
{
    static ThemeManager s;
    return &s;
}

ThemeManager::ThemeManager()
    : QObject(nullptr)
{
}

ThemeManager::Mode ThemeManager::systemMode()
{
    // Qt 6.5+ 提供 colorScheme(); Qt6::ColorScheme::Dark 表示暗色
    const Qt::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme();
    return scheme == Qt::ColorScheme::Dark ? Dark : Light;
}

ThemeManager::Mode ThemeManager::applyFromConfig(const QString &themeMode)
{
    Mode mode = Light;
    if (themeMode.compare(QStringLiteral("dark"), Qt::CaseInsensitive) == 0) {
        mode = Dark;
    } else if (themeMode.compare(QStringLiteral("light"), Qt::CaseInsensitive) == 0) {
        mode = Light;
    } else {
        mode = systemMode();
    }
    apply(mode);
    return mode;
}

void ThemeManager::apply(Mode mode)
{
    const QString path = (mode == Dark)
                             ? QStringLiteral(":/assets/styles-dark.qss")
                             : QStringLiteral(":/assets/styles.qss");
    QFile f(path);
    if (f.open(QFile::ReadOnly)) {
        qApp->setStyleSheet(QString::fromUtf8(f.readAll()));
        f.close();
    } else {
        qWarning("ThemeManager: cannot open %s", qPrintable(path));
    }

    m_mode = mode;
    qApp->setProperty("theme", mode == Dark ? "dark" : "light");
    repolishAll();
    emit themeChanged(mode);
}

void ThemeManager::repolishAll()
{
    const auto widgets = QApplication::topLevelWidgets();
    for (QWidget *w : widgets) {
        w->style()->unpolish(w);
        w->style()->polish(w);
        w->update();
        const auto children = w->findChildren<QWidget *>();
        for (QWidget *c : children) {
            c->style()->unpolish(c);
            c->style()->polish(c);
            c->update();
        }
    }
}

QColor ThemeManager::textColor() const
{
    // 导航项默认色: 亮色三级文字, 暗色二级文字
    return m_mode == Dark ? QColor("#8A86A6") : QColor("#A09DB4");
}

QColor ThemeManager::navSelectedBg() const
{
    return m_mode == Dark ? QColor("#38325A") : QColor("#FFFFFF");
}

QColor ThemeManager::navSelectedText() const
{
    // 亮色紫, 暗色黄 —— 黄紫主题的核心撞色
    return m_mode == Dark ? QColor("#FBBF24") : QColor("#7C3AED");
}

QColor ThemeManager::navHoverBg() const
{
    return m_mode == Dark ? QColor("#34304E") : QColor("#F3F0FA");
}

QColor ThemeManager::switchOffTrack() const
{
    return m_mode == Dark ? QColor("#464066") : QColor("#D8D4E4");
}

QColor ThemeManager::cardBg() const
{
    return m_mode == Dark ? QColor("#2A2640") : QColor("#FFFFFF");
}

QColor ThemeManager::borderColor() const
{
    return m_mode == Dark ? QColor("#3A3556") : QColor("#E9E5F2");
}
