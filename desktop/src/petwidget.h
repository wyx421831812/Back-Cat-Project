#ifndef PETWIDGET_H
#define PETWIDGET_H

#include <QWidget>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QStackedWidget>
#include "petcanvas.h"
#include "appconfig.h"

class ClockWidget;
class QuoteWidget;
class TodoWidget;
class BongoCatWidget;

/**
 * @brief 主窗口 - 无边框透明置顶桌面宠物
 *
 * 特性:
 * - 无边框透明窗口
 * - 始终置顶
 * - 支持点击穿透
 * - 可拖拽移动
 * - 右键菜单切换组件
 * - 系统托盘图标
 */
class PetWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PetWidget(QWidget *parent = nullptr);
    ~PetWidget();

protected:
    // 窗口事件
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

    // Windows 原生事件 (点击穿透)
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#endif

private:
    void setupUI();
    void setupTrayIcon();
    void setupContextMenu();
    void applyConfig();
    void updateClickThrough();
    void updateAlwaysOnTop();
    void updateOpacity();

    // 组件切换
    void switchToPet();
    void switchToClock();
    void switchToQuote();
    void switchToTodo();
    void switchToBongoCat();

    // 情绪控制
    void setMoodHappy();
    void setMoodSleep();
    void setMoodExcited();
    void setMoodNeutral();

    // 3D模型切换
    void setModelCat();
    void setModelBear();
    void setModelBunny();
    void setModelFairyBird();
    void setModelSpirit();

    // BongoCat 模型相关
    void rebuildBongoCatModelMenu();
    void switchToBongoCatModel(const QString &modelId);
    void importBongoCatModel();
    void deleteBongoCatModel(const QString &modelId);

private:
    // 拖拽 (放在前面以匹配初始化顺序)
    bool m_dragging;
    QPoint m_dragOffset;

    // 核心控件
    QStackedWidget *m_stack;
    PetCanvas *m_petCanvas;

    // 功能组件
    ClockWidget *m_clockWidget;
    QuoteWidget *m_quoteWidget;
    TodoWidget *m_todoWidget;
    BongoCatWidget *m_bongoCatWidget;

    // 系统托盘
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    QMenu *m_contextMenu;
    QMenu *m_trayBongoModelMenu;
    QMenu *m_contextBongoModelMenu;

    // 组件索引
    enum ComponentIndex {
        PetComponent = 0,
        ClockComponent = 1,
        QuoteComponent = 2,
        TodoComponent = 3,
        BongoCatComponent = 4
    };

private slots:
    void onSettings();
    void onQuit();
    void onPetClicked();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
};

#endif // PETWIDGET_H
