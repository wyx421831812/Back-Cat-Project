#ifndef PETCANVAS_H
#define PETCANVAS_H

#include <QWidget>
#include <QColor>
#include <QTimer>
#include <QVector3D>
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QRandomGenerator>
#include <cmath>

/**
 * @brief 宠物情绪枚举
 */
enum class PetMood {
    Neutral,   // 安静待机
    Happy,      // 开心鼓掌
    Sleep,      // 犯困打盹
    Excited     // 高兴呐喊
};

/**
 * @brief 3D 宠物模型类型
 */
enum class PetModelType {
    CatLike,      // 默认猫咪风格
    BearLike,     // 小熊风格
    BunnyLike,    // 兔子风格
    FairyBirdLike, // 仙鸟风格 (白色羽翼)
    SpiritLike     // 幻灵风格 (银发呆毛+紫黄双色身体)
};

/**
 * @brief 3D 球体数据 (用于软件3D渲染)
 */
struct Sphere3D {
    QVector3D pos;      // 3D 位置
    QVector3D scale;    // 缩放 (非等比 -> 椭球)
    QColor color;       // 颜色
    float rotation;     // 旋转角度
    QVector3D rotAxis;  // 旋转轴
};

/**
 * @brief 3D 宠物绘制画布
 *
 * 使用 QPainter 软件3D渲染
 * 通过球体/椭球体组合构建3D角色
 * 支持透视投影、深度排序和光照着色
 */
class PetCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit PetCanvas(QWidget *parent = nullptr);
    ~PetCanvas();

    // 颜色设置
    void setColors(const QColor &primary, const QColor &secondary, const QColor &accent);
    QColor primaryColor() const { return m_primary; }
    QColor secondaryColor() const { return m_secondary; }

    // 情绪
    void setMood(PetMood mood);
    PetMood mood() const { return m_mood; }

    // 模型类型
    void setModelType(PetModelType type);
    PetModelType modelType() const { return m_modelType; }

    // 动画控制
    void startWagging();
    void stopWagging();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

signals:
    void clicked();
    void doubleClicked();

private:
    // 3D 渲染核心
    QPointF project3D(const QVector3D &pos3D) const;
    void drawSphere3D(QPainter &painter, const Sphere3D &sphere);
    void sortAndDrawSpheres(QPainter &painter);

    // 收集模型球体
    void collectCatModel(QVector<Sphere3D> &spheres);
    void collectBearModel(QVector<Sphere3D> &spheres);
    void collectBunnyModel(QVector<Sphere3D> &spheres);
    void collectFairyBirdModel(QVector<Sphere3D> &spheres);
    void collectSpiritModel(QVector<Sphere3D> &spheres);

    // 绘制对话框
    void drawSpeechBubble(QPainter &painter);

    // 颜色
    QColor m_primary;
    QColor m_secondary;
    QColor m_accent;

    // 状态
    PetMood m_mood;
    PetModelType m_modelType;
    qreal m_bounceY;
    qreal m_wagAngle;
    qreal m_clapAngle;
    qreal m_rotation;

    // 球体列表 (每帧重建)
    QVector<Sphere3D> m_spheres;

    // 定时器
    QTimer m_animTimer;
    QTimer m_moodTimer;

    // 对话框
    bool m_showBubble;
    QString m_bubbleText;

private slots:
    void onAnimTick();
};

#endif // PETCANVAS_H
