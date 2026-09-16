#ifndef PETWIDGET_H
#define PETWIDGET_H

#include <QWidget>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QStackedWidget>
#include <QTimer>
#include "petcanvas.h"
#include "appconfig.h"

class ClockWidget;
class QuoteWidget;
class TodoWidget;
class BongoCatWidget;
class PetStage;

/**
 * @brief 主窗口 - 无边框透明置顶桌面宠物
 *
 * 特性：
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
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

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
    void resizeForStage(const QSize &size);
    void resizeForAuxWidget();
    void updateWindowMask();
    // 原生表面(HWND) (重新)创建后, 重新套用穿透/半透明等直接改 HWND 的样式
    void reapplyNativeExtras();
    // 保持在屏幕内: 将当前窗口几何夹取到所在屏幕可用区域
    void clampIntoScreen();
    // 应用 cat.model 分组的运行时设置到渲染部件
    void applyModelRuntimeSettings();

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

    // 核心组件
    QStackedWidget *m_stack;
    PetCanvas *m_petCanvas;

    // 功能组件
    ClockWidget *m_clockWidget;
    QuoteWidget *m_quoteWidget;
    TodoWidget *m_todoWidget;
    BongoCatWidget *m_bongoCatWidget;
    PetStage *m_stage;

    // 系统托盘
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    QMenu *m_contextMenu;
    QMenu *m_trayBongoModelMenu;
    QMenu *m_contextBongoModelMenu;

    // 鼠标移入隐藏 (cat.window.hideOnHover)
    QTimer *m_hoverHideTimer = nullptr;
    bool m_hiddenByHover = false;

    // 不透明度曾 <100%: 回 100% 时需重建 HWND 清除常量 alpha (LWA_ALPHA),
    // 否则窗口会一直停留在上次的半透明状态
    bool m_opacityReduced = false;

    // HWND 句柄看门狗: Qt (如 QWebEngineView 初始化) 可能悄悄重建原生窗口,
    // 周期性比对句柄, 变化后重套穿透/半透明等直接改 HWND 的样式
    QTimer *m_nativeWatchTimer = nullptr;
    WId m_lastNativeHwnd = 0;
    void checkNativeWindowRecreated();

    // 组件索引 (0 是统一舞台 PetStage; 时钟/格言/待办在 Task 11/12 迁出)
    enum ComponentIndex {
        StageComponent = 0,
        ClockComponent = 1,
        QuoteComponent = 2,
        TodoComponent = 3
    };

private slots:
    void onSettings();
    void onQuit();
    void onPetClicked();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
};

#endif // PETWIDGET_H
