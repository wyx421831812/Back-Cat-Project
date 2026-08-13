#ifndef BONGOCATWIDGET_H
#define BONGOCATWIDGET_H

#include "componentbase.h"
#include "bongomodelmanager.h"
#include <QMap>
#include <QSet>
#include <QPixmap>
#include <QTimer>
#include <QPoint>
#include <QElapsedTimer>
#include <QPainter>

#ifdef QT_WEBENGINEWIDGETS_LIB
#include "live2dwidget.h"
#define HAS_LIVE2D_SUPPORT
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#endif

/**
 * @brief Bongo Cat 桌宠组件 (增强版)
 *
 * 参考BongoCat项目实现的完整功能:
 * - 根据键盘操作同步猫咪动作
 * - 根据鼠标操作同步猫咪动作
 * - 猫咪眼睛/头部跟随鼠标移动
 * - 支持多种模型切换
 * - 支持导入自定义模型
 * - 手部按下动画效果
 * - 支持Live2D动态模型（需WebEngine模块）
 */
class BongoCatWidget : public ComponentBase
{
    Q_OBJECT

public:
    explicit BongoCatWidget(QWidget *parent = nullptr);
    ~BongoCatWidget();

    QString componentName() const override { return QStringLiteral("BongoCat"); }
    void refresh() override;
    void onShow() override;
    void onHide() override;

    // 模型相关
    void loadModel(const BongoModel &model);
    void reloadCurrentModel();
    BongoModel currentModel() const;

    // 鼠标跟随设置
    void setMouseFollowEnabled(bool enabled);
    bool isMouseFollowEnabled() const;

    // Live2D支持
    bool hasLive2DSupport() const;
    bool isUsingLive2D() const;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    // 安装/卸载钩子
    void installHook();
    void removeHook();

    // 处理按键
    void handleKeyDown(int vkCode);
    void handleKeyUp(int vkCode);
    void handleMouseDown(bool isLeft);
    void handleMouseUp(bool isLeft);

    // VK码到按键名称的映射
    QString vkToKeyName(int vkCode) const;

    // 更新鼠标跟随参数
    void updateMouseFollow(const QPoint &globalPos);

    // 动画定时更新
    void onAnimationTick();

    // 计算手部状态(用于绘制)
    bool isLeftHandDown() const;
    bool isRightHandDown() const;

    // 绘制辅助函数
    void drawDefaultCat(QPainter &painter, const QRect &catRect);
    void drawHandPressEffect(QPainter &painter, const QRect &catRect,
                              int scaledW, int scaledH);
    void drawKeyPressRipple(QPainter &painter, const QRect &catRect,
                            qreal timeSec);

    // 根据模型类型选择渲染方式
    bool shouldUseLive2D() const;
    void switchToLive2D();
    void switchToStaticImage();

private:
    // 当前模型
    BongoModel m_model;

    // 按键状态
    QSet<QString> m_pressedKeys;
    QSet<int> m_pressedVkCodes;

    // 鼠标状态
    bool m_leftMouseDown = false;
    bool m_rightMouseDown = false;

    // 鼠标跟随
    bool m_mouseFollowEnabled = true;
    QPointF m_mouseOffset;  // -1.0 ~ 1.0
    QPointF m_targetOffset; // 目标偏移(用于平滑过渡)

    // 动画
    QTimer *m_animationTimer;
    QElapsedTimer m_elapsedTimer;
    qreal m_bobPhase = 0.0;  // 呼吸/浮动相位

    // 手部状态追踪
    bool m_leftHandActive = false;
    bool m_rightHandActive = false;
    qint64 m_leftHandPressTime = 0;
    qint64 m_rightHandPressTime = 0;

    // Live2D 渲染相关
    bool m_useLive2D = false;
#ifdef HAS_LIVE2D_SUPPORT
    Live2DWidget *m_live2dWidget = nullptr;
#endif

#ifdef Q_OS_WIN
    static BongoCatWidget *s_instance;
    static HHOOK s_keyboardHook;
    static HHOOK s_mouseHook;
    static LRESULT CALLBACK keyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK mouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);
#endif
};

#endif // BONGOCATWIDGET_H
