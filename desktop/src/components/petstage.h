#ifndef PETSTAGE_H
#define PETSTAGE_H

#include <QWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QTimer>
#include <QSize>

class BongoCatWidget;
class PetCanvas;

/**
 * @brief 猫咪统一舞台
 *
 * 在同一个窗口位置承载两种模型族:
 *  - Live2D/BongoCat (BongoCatWidget, 内含静态图回退)
 *  - 软件 3D 宠物   (PetCanvas)
 *
 * 职责:
 *  - 双向切换并显示"切换中…"遮罩
 *  - 按模型自然尺寸 x scale% 计算窗口目标尺寸
 *  - Live2D resize 时显示"重绘中…"遮罩, 100ms 防抖隐藏
 *  - 水平镜像 / 圆角半径 (mask 由宿主窗口负责 setMask)
 */
class PetStage : public QWidget
{
    Q_OBJECT

public:
    enum class Kind { Live2D, Pet3D };

    explicit PetStage(BongoCatWidget *bongo, PetCanvas *canvas,
                      QWidget *parent = nullptr);

    // 切换模型。showLive2D 的 modelId 为空串时使用模型管理器当前模型
    void showLive2D(const QString &modelId = QString());
    void showPet3D(int type);

    Kind kind() const { return m_kind; }
    QString live2dModelId() const { return m_live2dId; }
    int pet3dType() const { return m_pet3dType; }

    // 尺寸
    QSize baseSize() const;      // scale=100% 时的自然逻辑尺寸
    QSize desiredSize() const;   // baseSize * scale%
    int scalePercent() const { return m_scale; }
    void setScalePercent(int percent);

    // 圆角百分比 (0-100), radiusPx() 给出当前尺寸下的像素半径
    int radiusPercent() const { return m_radius; }
    void setRadiusPercent(int percent);
    int radiusPx() const;

    void setMirrored(bool on);
    bool isMirrored() const { return m_mirror; }

    // 从 AppConfig 读取 scale/radius/mirror (不触发切换)
    void applyConfig();

signals:
    void desiredSizeChanged(const QSize &size);

private slots:
    void onLive2dLoaded(int width, int height);
    void onModelBackgroundSize(const QSize &size);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void present();
    void showMask(const QString &text);
    void hideMask();

    BongoCatWidget *m_bongo;
    PetCanvas *m_petCanvas;
    QStackedWidget *m_stack;
    QLabel *m_mask;
    QTimer m_maskFallback;    // 切换遮罩兜底隐藏 (模型加载信号可能不发)
    QTimer m_resizeDebounce;  // "重绘中…" 100ms 防抖

    Kind m_kind = Kind::Live2D;
    QString m_live2dId;
    int m_pet3dType = 0;

    QSize m_live2dBase{420, 243};  // 宽度基准 420, 高度按模型自然宽高比
    int m_scale = 100;
    int m_radius = 0;
    bool m_mirror = false;
};

#endif // PETSTAGE_H
