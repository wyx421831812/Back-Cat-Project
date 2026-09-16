#pragma once

#include <QWidget>

class QButtonGroup;
class QStackedWidget;
class QScrollArea;
class NavItem;

// 偏好设置窗 (对齐原版 preference/index.vue):
// 原生标题栏, 最小 800x600, 120px 侧栏 + 右侧 QStackedWidget,
// 单实例, 关闭即隐藏, Windows 下不显示任务栏图标。
class PreferenceWindow : public QWidget
{
    Q_OBJECT

public:
    enum Page {
        CatPage = 0,
        GeneralPage,
        ModelPage,
        ShortcutPage,
        TodoPage,
        AboutPage,
    };
    Q_ENUM(Page)

    static PreferenceWindow *instance();

    // 显示并前置; page>=0 时同时切换页
    void showWindow(int page = -1);
    void navigateTo(int page);

    // 用真实页面替换占位页 (Task 5/6/7/10/11/14 使用)
    void setPageWidget(Page page, QWidget *widget);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    PreferenceWindow();

    QWidget *buildSidebar();
    QScrollArea *buildPlaceholderPage(const QString &text);

    QButtonGroup *m_navGroup = nullptr;
    QStackedWidget *m_stack = nullptr;
    QList<NavItem *> m_navItems;
    bool m_firstShow = true;
};
