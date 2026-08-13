#include "petcanvas.h"
#include <QMouseEvent>
#include <QTime>
#include <QRadialGradient>
#include <QBrush>
#include <algorithm>

// ============================================================
//  软件3D渲染引擎 - 使用 QPainter 实现
//  透视投影 + 深度排序 + 径向光照
// ============================================================

PetCanvas::PetCanvas(QWidget *parent)
    : QWidget(parent)
    , m_primary("#fbbf24")
    , m_secondary("#8b5cf6")
    , m_accent("#6366f1")
    , m_mood(PetMood::Neutral)
    , m_modelType(PetModelType::CatLike)
    , m_bounceY(0)
    , m_wagAngle(0)
    , m_clapAngle(0)
    , m_rotation(0)
    , m_showBubble(false)
    , m_bubbleText(QStringLiteral("爱上雷神~"))
{
    setAttribute(Qt::WA_TranslucentBackground);

    // 动画定时器 ~30fps
    m_animTimer.setInterval(33);
    connect(&m_animTimer, &QTimer::timeout, this, &PetCanvas::onAnimTick);
    m_animTimer.start();

    // 情绪恢复定时器
    m_moodTimer.setSingleShot(true);
    connect(&m_moodTimer, &QTimer::timeout, this, [this]() {
        setMood(PetMood::Neutral);
    });
}

PetCanvas::~PetCanvas() {}

// === 3D 透视投影 ===
QPointF PetCanvas::project3D(const QVector3D &pos3D) const
{
    const float cameraZ = 6.0f;
    const float focal = 4.5f;
    const float unit = 55.0f;  // 缩放系数

    float depth = cameraZ - pos3D.z();
    if (depth < 0.1f) depth = 0.1f;

    float persp = focal / depth;
    float cx = width() / 2.0f;
    float cy = height() / 2.0f + 20.0f;  // 略微下移

    return QPointF(cx + pos3D.x() * persp * unit,
                   cy - pos3D.y() * persp * unit);
}

// === 绘制单个3D球体 (带光照) ===
void PetCanvas::drawSphere3D(QPainter &painter, const Sphere3D &sphere)
{
    QPointF center = project3D(sphere.pos);

    const float cameraZ = 6.0f;
    const float focal = 4.5f;
    const float unit = 55.0f;

    float depth = cameraZ - sphere.pos.z();
    if (depth < 0.1f) depth = 0.1f;
    float persp = focal / depth;

    float radiusX = sphere.scale.x() * persp * unit;
    float radiusY = sphere.scale.y() * persp * unit;

    if (radiusX < 0.5f || radiusY < 0.5f) return;

    // 径向渐变 (模拟3D光照: 左上亮, 右下暗)
    float lightOffsetX = radiusX * 0.35f;
    float lightOffsetY = radiusY * 0.35f;

    QRadialGradient gradient(center.x() - lightOffsetX,
                             center.y() - lightOffsetY,
                             qMax(radiusX, radiusY) * 1.3f);

    QColor base = sphere.color;
    QColor highlight = base.lighter(165);
    QColor shadow = base.darker(160);

    gradient.setColorAt(0.0, highlight);
    gradient.setColorAt(0.4, base);
    gradient.setColorAt(1.0, shadow);

    painter.setBrush(QBrush(gradient));
    painter.setPen(Qt::NoPen);

    // 椭球体 + 旋转
    bool isEllipse = qAbs(radiusX - radiusY) > 1.0f;
    if (isEllipse && sphere.rotation != 0.0f) {
        painter.save();
        painter.translate(center);
        painter.rotate(sphere.rotation);
        painter.drawEllipse(QPointF(0, 0), radiusX, radiusY);
        painter.restore();
    } else if (isEllipse) {
        painter.drawEllipse(center, radiusX, radiusY);
    } else {
        float r = (radiusX + radiusY) / 2.0f;
        painter.drawEllipse(center, r, r);
    }
}

// === 深度排序后绘制 (画家算法) ===
void PetCanvas::sortAndDrawSpheres(QPainter &painter)
{
    // 按 Z 值从远到近排序
    std::sort(m_spheres.begin(), m_spheres.end(),
              [](const Sphere3D &a, const Sphere3D &b) {
                  return a.pos.z() < b.pos.z();
              });

    for (const auto &sphere : m_spheres) {
        drawSphere3D(painter, sphere);
    }
}

// === 收集猫咪模型球体 ===
void PetCanvas::collectCatModel(QVector<Sphere3D> &spheres)
{
    float y = m_bounceY;

    // 身体 (椭球体)
    spheres.append({QVector3D(0, -0.6f + y, 0), QVector3D(0.8f, 0.7f, 0.6f),
                     m_primary, 0, QVector3D(0, 1, 0)});
    // 头部
    spheres.append({QVector3D(0, 0.5f + y, 0.1f), QVector3D(0.6f, 0.55f, 0.55f),
                     m_secondary, 0, QVector3D(0, 1, 0)});
    // 左耳
    spheres.append({QVector3D(-0.35f, 1.0f + y, 0), QVector3D(0.12f, 0.25f, 0.1f),
                     m_secondary, -20.0f, QVector3D(0, 0, 1)});
    // 右耳
    spheres.append({QVector3D(0.35f, 1.0f + y, 0), QVector3D(0.12f, 0.25f, 0.1f),
                     m_secondary, 20.0f, QVector3D(0, 0, 1)});

    // 眼睛
    if (m_mood == PetMood::Sleep) {
        spheres.append({QVector3D(-0.2f, 0.55f + y, 0.5f), QVector3D(0.08f, 0.02f, 0.05f),
                         QColor("#1a1a1a"), 0, QVector3D(0, 1, 0)});
        spheres.append({QVector3D(0.2f, 0.55f + y, 0.5f), QVector3D(0.08f, 0.02f, 0.05f),
                         QColor("#1a1a1a"), 0, QVector3D(0, 1, 0)});
    } else {
        spheres.append({QVector3D(-0.2f, 0.55f + y, 0.5f), QVector3D(0.1f, 0.1f, 0.05f),
                         QColor("#ffffff"), 0, QVector3D(0, 1, 0)});
        spheres.append({QVector3D(0.2f, 0.55f + y, 0.5f), QVector3D(0.1f, 0.1f, 0.05f),
                         QColor("#ffffff"), 0, QVector3D(0, 1, 0)});
        float pupilY = (m_mood == PetMood::Happy) ? 0.04f : 0.0f;
        spheres.append({QVector3D(-0.2f, 0.55f + pupilY + y, 0.55f), QVector3D(0.05f, 0.05f, 0.03f),
                         QColor("#1a1a1a"), 0, QVector3D(0, 1, 0)});
        spheres.append({QVector3D(0.2f, 0.55f + pupilY + y, 0.55f), QVector3D(0.05f, 0.05f, 0.03f),
                         QColor("#1a1a1a"), 0, QVector3D(0, 1, 0)});
    }

    // 鼻子
    spheres.append({QVector3D(0, 0.35f + y, 0.55f), QVector3D(0.04f, 0.03f, 0.02f),
                     QColor("#ff6b6b"), 0, QVector3D(0, 1, 0)});

    // 嘴巴
    if (m_mood == PetMood::Happy || m_mood == PetMood::Excited) {
        float ms = (m_mood == PetMood::Excited) ? 0.12f : 0.08f;
        spheres.append({QVector3D(0, 0.2f + y, 0.5f), QVector3D(ms, ms * 0.6f, 0.03f),
                         QColor("#1a1a1a"), 0, QVector3D(0, 1, 0)});
    } else {
        spheres.append({QVector3D(0, 0.25f + y, 0.5f), QVector3D(0.04f, 0.02f, 0.02f),
                         QColor("#1a1a1a"), 0, QVector3D(0, 1, 0)});
    }

    // 手 (左右) - 带旋转
    spheres.append({QVector3D(-0.7f, -0.3f + y, 0.2f), QVector3D(0.15f, 0.3f, 0.15f),
                     m_secondary, m_clapAngle, QVector3D(0, 0, 1)});
    spheres.append({QVector3D(0.7f, -0.3f + y, 0.2f), QVector3D(0.15f, 0.3f, 0.15f),
                     m_secondary, -m_clapAngle, QVector3D(0, 0, 1)});

    // 尾巴 - 带摆动
    spheres.append({QVector3D(0.6f, -0.4f + y, -0.3f), QVector3D(0.08f, 0.3f, 0.08f),
                     m_secondary, m_wagAngle, QVector3D(0, 1, 0)});

    // 脚 (左右)
    spheres.append({QVector3D(-0.3f, -1.2f + y, 0.1f), QVector3D(0.2f, 0.1f, 0.25f),
                     m_accent, 0, QVector3D(0, 1, 0)});
    spheres.append({QVector3D(0.3f, -1.2f + y, 0.1f), QVector3D(0.2f, 0.1f, 0.25f),
                     m_accent, 0, QVector3D(0, 1, 0)});
}

// === 收集小熊模型球体 ===
void PetCanvas::collectBearModel(QVector<Sphere3D> &spheres)
{
    float y = m_bounceY;

    // 身体 (圆胖)
    spheres.append({QVector3D(0, -0.5f + y, 0), QVector3D(0.9f, 0.8f, 0.7f),
                     m_primary, 0, QVector3D(0, 1, 0)});
    // 头部
    spheres.append({QVector3D(0, 0.6f + y, 0.1f), QVector3D(0.65f, 0.6f, 0.6f),
                     m_primary, 0, QVector3D(0, 1, 0)});
    // 圆耳朵
    spheres.append({QVector3D(-0.4f, 1.1f + y, 0), QVector3D(0.18f, 0.18f, 0.12f),
                     m_secondary, 0, QVector3D(0, 1, 0)});
    spheres.append({QVector3D(0.4f, 1.1f + y, 0), QVector3D(0.18f, 0.18f, 0.12f),
                     m_secondary, 0, QVector3D(0, 1, 0)});

    // 眼睛
    if (m_mood == PetMood::Sleep) {
        spheres.append({QVector3D(-0.2f, 0.65f + y, 0.55f), QVector3D(0.07f, 0.02f, 0.04f),
                         QColor("#1a1a1a"), 0, QVector3D(0, 1, 0)});
        spheres.append({QVector3D(0.2f, 0.65f + y, 0.55f), QVector3D(0.07f, 0.02f, 0.04f),
                         QColor("#1a1a1a"), 0, QVector3D(0, 1, 0)});
    } else {
        spheres.append({QVector3D(-0.2f, 0.65f + y, 0.55f), QVector3D(0.08f, 0.08f, 0.04f),
                         QColor("#1a1a1a"), 0, QVector3D(0, 1, 0)});
        spheres.append({QVector3D(0.2f, 0.65f + y, 0.55f), QVector3D(0.08f, 0.08f, 0.04f),
                         QColor("#1a1a1a"), 0, QVector3D(0, 1, 0)});
    }

    // 鼻子 (大)
    spheres.append({QVector3D(0, 0.45f + y, 0.6f), QVector3D(0.08f, 0.06f, 0.04f),
                     QColor("#2d2d2d"), 0, QVector3D(0, 1, 0)});

    // 嘴巴
    if (m_mood == PetMood::Happy || m_mood == PetMood::Excited) {
        float ms = (m_mood == PetMood::Excited) ? 0.14f : 0.1f;
        spheres.append({QVector3D(0, 0.28f + y, 0.55f), QVector3D(ms, ms * 0.5f, 0.03f),
                         QColor("#1a1a1a"), 0, QVector3D(0, 1, 0)});
    } else {
        spheres.append({QVector3D(0, 0.33f + y, 0.55f), QVector3D(0.05f, 0.02f, 0.02f),
                         QColor("#1a1a1a"), 0, QVector3D(0, 1, 0)});
    }

    // 手
    spheres.append({QVector3D(-0.8f, -0.2f + y, 0.2f), QVector3D(0.18f, 0.25f, 0.18f),
                     m_primary, m_clapAngle, QVector3D(0, 0, 1)});
    spheres.append({QVector3D(0.8f, -0.2f + y, 0.2f), QVector3D(0.18f, 0.25f, 0.18f),
                     m_primary, -m_clapAngle, QVector3D(0, 0, 1)});

    // 脚
    spheres.append({QVector3D(-0.35f, -1.1f + y, 0.1f), QVector3D(0.22f, 0.12f, 0.28f),
                     m_accent, 0, QVector3D(0, 1, 0)});
    spheres.append({QVector3D(0.35f, -1.1f + y, 0.1f), QVector3D(0.22f, 0.12f, 0.28f),
                     m_accent, 0, QVector3D(0, 1, 0)});
}

// === 收集兔子模型球体 ===
void PetCanvas::collectBunnyModel(QVector<Sphere3D> &spheres)
{
    float y = m_bounceY;

    // 身体 (椭圆)
    spheres.append({QVector3D(0, -0.5f + y, 0), QVector3D(0.7f, 0.75f, 0.6f),
                     m_primary, 0, QVector3D(0, 1, 0)});
    // 头部
    spheres.append({QVector3D(0, 0.55f + y, 0.1f), QVector3D(0.55f, 0.5f, 0.5f),
                     m_primary, 0, QVector3D(0, 1, 0)});
    // 长耳朵 (兔子特征)
    spheres.append({QVector3D(-0.2f, 1.3f + y, 0), QVector3D(0.08f, 0.35f, 0.05f),
                     m_secondary, -10.0f, QVector3D(0, 0, 1)});
    spheres.append({QVector3D(0.2f, 1.3f + y, 0), QVector3D(0.08f, 0.35f, 0.05f),
                     m_secondary, 10.0f, QVector3D(0, 0, 1)});

    // 眼睛
    if (m_mood == PetMood::Sleep) {
        spheres.append({QVector3D(-0.18f, 0.6f + y, 0.48f), QVector3D(0.06f, 0.02f, 0.03f),
                         QColor("#1a1a1a"), 0, QVector3D(0, 1, 0)});
        spheres.append({QVector3D(0.18f, 0.6f + y, 0.48f), QVector3D(0.06f, 0.02f, 0.03f),
                         QColor("#1a1a1a"), 0, QVector3D(0, 1, 0)});
    } else {
        spheres.append({QVector3D(-0.18f, 0.6f + y, 0.48f), QVector3D(0.07f, 0.07f, 0.04f),
                         QColor("#ffffff"), 0, QVector3D(0, 1, 0)});
        spheres.append({QVector3D(0.18f, 0.6f + y, 0.48f), QVector3D(0.07f, 0.07f, 0.04f),
                         QColor("#ffffff"), 0, QVector3D(0, 1, 0)});
        spheres.append({QVector3D(-0.18f, 0.6f + y, 0.52f), QVector3D(0.04f, 0.04f, 0.02f),
                         QColor("#1a1a1a"), 0, QVector3D(0, 1, 0)});
        spheres.append({QVector3D(0.18f, 0.6f + y, 0.52f), QVector3D(0.04f, 0.04f, 0.02f),
                         QColor("#1a1a1a"), 0, QVector3D(0, 1, 0)});
    }

    // 鼻子 (粉色)
    spheres.append({QVector3D(0, 0.42f + y, 0.52f), QVector3D(0.04f, 0.03f, 0.02f),
                     QColor("#ff6b9d"), 0, QVector3D(0, 1, 0)});

    // 嘴巴
    if (m_mood == PetMood::Happy || m_mood == PetMood::Excited) {
        float ms = (m_mood == PetMood::Excited) ? 0.1f : 0.07f;
        spheres.append({QVector3D(0, 0.25f + y, 0.48f), QVector3D(ms, ms * 0.5f, 0.02f),
                         QColor("#1a1a1a"), 0, QVector3D(0, 1, 0)});
    } else {
        spheres.append({QVector3D(0, 0.28f + y, 0.48f), QVector3D(0.03f, 0.02f, 0.02f),
                         QColor("#1a1a1a"), 0, QVector3D(0, 1, 0)});
    }

    // 手 (小短手)
    spheres.append({QVector3D(-0.6f, -0.3f + y, 0.15f), QVector3D(0.12f, 0.2f, 0.12f),
                     m_secondary, m_clapAngle, QVector3D(0, 0, 1)});
    spheres.append({QVector3D(0.6f, -0.3f + y, 0.15f), QVector3D(0.12f, 0.2f, 0.12f),
                     m_secondary, -m_clapAngle, QVector3D(0, 0, 1)});

    // 圆尾巴
    spheres.append({QVector3D(0, -0.7f + y, -0.5f), QVector3D(0.12f, 0.12f, 0.12f),
                     QColor("#ffffff"), 0, QVector3D(0, 1, 0)});

    // 脚
    spheres.append({QVector3D(-0.25f, -1.1f + y, 0.1f), QVector3D(0.18f, 0.1f, 0.25f),
                     m_accent, 0, QVector3D(0, 1, 0)});
    spheres.append({QVector3D(0.25f, -1.1f + y, 0.1f), QVector3D(0.18f, 0.1f, 0.25f),
                     m_accent, 0, QVector3D(0, 1, 0)});
}

// === 收集仙鸟模型球体 (基于3D渲染图设计) ===
// 特征: 白色羽翼 + 绿色身体 + 鸟嘴 + 扇形尾羽 + 头冠
void PetCanvas::collectFairyBirdModel(QVector<Sphere3D> &spheres)
{
    float y = m_bounceY;

    // === 身体 (蛋形/直立椭圆, 仿仙鸟躯干) ===
    spheres.append({QVector3D(0, -0.4f + y, 0), QVector3D(0.65f, 0.8f, 0.6f),
                     m_primary, 0, QVector3D(0, 1, 0)});

    // === 头部 (圆球, 略前倾) ===
    spheres.append({QVector3D(0, 0.7f + y, 0.1f), QVector3D(0.5f, 0.5f, 0.5f),
                     m_primary, 0, QVector3D(0, 1, 0)});

    // === 翅膀 (左右展开, 白色羽翼) ===
    // 主翅
    spheres.append({QVector3D(-0.8f, -0.1f + y, -0.1f), QVector3D(0.15f, 0.4f, 0.08f),
                     m_secondary, 25.0f + m_clapAngle * 0.5f, QVector3D(0, 0, 1)});
    spheres.append({QVector3D(0.8f, -0.1f + y, -0.1f), QVector3D(0.15f, 0.4f, 0.08f),
                     m_secondary, -25.0f - m_clapAngle * 0.5f, QVector3D(0, 0, 1)});
    // 副翅 (下层羽毛)
    spheres.append({QVector3D(-0.9f, -0.4f + y, -0.1f), QVector3D(0.1f, 0.3f, 0.06f),
                     m_secondary, 35.0f + m_clapAngle * 0.5f, QVector3D(0, 0, 1)});
    spheres.append({QVector3D(0.9f, -0.4f + y, -0.1f), QVector3D(0.1f, 0.3f, 0.06f),
                     m_secondary, -35.0f - m_clapAngle * 0.5f, QVector3D(0, 0, 1)});

    // === 头冠 (三根羽毛) ===
    spheres.append({QVector3D(0, 1.2f + y, 0), QVector3D(0.08f, 0.22f, 0.05f),
                     m_secondary, 0, QVector3D(0, 0, 1)});
    spheres.append({QVector3D(-0.12f, 1.15f + y, 0.05f), QVector3D(0.06f, 0.16f, 0.04f),
                     m_secondary, -15.0f, QVector3D(0, 0, 1)});
    spheres.append({QVector3D(0.12f, 1.15f + y, 0.05f), QVector3D(0.06f, 0.16f, 0.04f),
                     m_secondary, 15.0f, QVector3D(0, 0, 1)});

    // === 眼睛 (大眼, 绿色虹膜) ===
    if (m_mood == PetMood::Sleep) {
        spheres.append({QVector3D(-0.15f, 0.75f + y, 0.45f), QVector3D(0.06f, 0.02f, 0.03f),
                         QColor("#1a1a1a"), 0, QVector3D(0, 1, 0)});
        spheres.append({QVector3D(0.15f, 0.75f + y, 0.45f), QVector3D(0.06f, 0.02f, 0.03f),
                         QColor("#1a1a1a"), 0, QVector3D(0, 1, 0)});
    } else {
        // 白眼底
        spheres.append({QVector3D(-0.15f, 0.75f + y, 0.45f), QVector3D(0.08f, 0.1f, 0.04f),
                         QColor("#ffffff"), 0, QVector3D(0, 1, 0)});
        spheres.append({QVector3D(0.15f, 0.75f + y, 0.45f), QVector3D(0.08f, 0.1f, 0.04f),
                         QColor("#ffffff"), 0, QVector3D(0, 1, 0)});
        // 绿色虹膜 (匹配角色绿眼)
        float pupilY = (m_mood == PetMood::Happy) ? 0.03f : 0.0f;
        spheres.append({QVector3D(-0.15f, 0.75f + pupilY + y, 0.49f), QVector3D(0.05f, 0.06f, 0.02f),
                         QColor("#22c55e"), 0, QVector3D(0, 1, 0)});
        spheres.append({QVector3D(0.15f, 0.75f + pupilY + y, 0.49f), QVector3D(0.05f, 0.06f, 0.02f),
                         QColor("#22c55e"), 0, QVector3D(0, 1, 0)});
        // 瞳孔
        spheres.append({QVector3D(-0.15f, 0.75f + pupilY + y, 0.51f), QVector3D(0.025f, 0.03f, 0.01f),
                         QColor("#1a1a1a"), 0, QVector3D(0, 1, 0)});
        spheres.append({QVector3D(0.15f, 0.75f + pupilY + y, 0.51f), QVector3D(0.025f, 0.03f, 0.01f),
                         QColor("#1a1a1a"), 0, QVector3D(0, 1, 0)});
    }

    // === 鸟嘴 (小尖嘴, 橙色) ===
    spheres.append({QVector3D(0, 0.58f + y, 0.5f), QVector3D(0.06f, 0.04f, 0.1f),
                     m_accent, 0, QVector3D(0, 1, 0)});

    // === 腮红 ===
    spheres.append({QVector3D(-0.28f, 0.62f + y, 0.4f), QVector3D(0.04f, 0.03f, 0.02f),
                     QColor("#ff9999"), 0, QVector3D(0, 1, 0)});
    spheres.append({QVector3D(0.28f, 0.62f + y, 0.4f), QVector3D(0.04f, 0.03f, 0.02f),
                     QColor("#ff9999"), 0, QVector3D(0, 1, 0)});

    // === 嘴巴 (开心时显示) ===
    if (m_mood == PetMood::Happy || m_mood == PetMood::Excited) {
        float ms = (m_mood == PetMood::Excited) ? 0.1f : 0.07f;
        spheres.append({QVector3D(0, 0.45f + y, 0.5f), QVector3D(ms, ms * 0.5f, 0.02f),
                         QColor("#1a1a1a"), 0, QVector3D(0, 1, 0)});
    }

    // === 尾羽 (扇形展开, 多层) ===
    spheres.append({QVector3D(0, -0.5f + y, -0.4f), QVector3D(0.1f, 0.35f, 0.06f),
                     m_secondary, m_wagAngle * 0.3f, QVector3D(0, 1, 0)});
    spheres.append({QVector3D(-0.22f, -0.55f + y, -0.35f), QVector3D(0.08f, 0.3f, 0.05f),
                     m_secondary, -15.0f + m_wagAngle * 0.3f, QVector3D(0, 1, 0)});
    spheres.append({QVector3D(0.22f, -0.55f + y, -0.35f), QVector3D(0.08f, 0.3f, 0.05f),
                     m_secondary, 15.0f + m_wagAngle * 0.3f, QVector3D(0, 1, 0)});

    // === 脚 (鸟爪, 橙色) ===
    spheres.append({QVector3D(-0.2f, -1.15f + y, 0.1f), QVector3D(0.1f, 0.06f, 0.15f),
                     m_accent, 0, QVector3D(0, 1, 0)});
    spheres.append({QVector3D(0.2f, -1.15f + y, 0.1f), QVector3D(0.1f, 0.06f, 0.15f),
                     m_accent, 0, QVector3D(0, 1, 0)});
}

// === 收集幻灵模型球体 (基于图片头部+仙鸟身体结构) ===
// 特征: 银灰头发 + 呆毛 + 不对称刘海 + 紫色上身 + 黄色下身 + 白色翅膀
void PetCanvas::collectSpiritModel(QVector<Sphere3D> &spheres)
{
    float y = m_bounceY;

    // 专属颜色
    QColor bodyUpper("#7c3aed");  // 上身紫色
    QColor bodyLower("#fbbf24");  // 下身黄色
    QColor hairMain("#c8d4e0");   // 银灰主发色
    QColor hairShadow("#6b7b8f"); // 深灰蓝阴影
    QColor skin("#f5e6d8");       // 肤色

    // === 下身 (黄色, 较大圆球) ===
    spheres.append({QVector3D(0, -0.7f + y, 0), QVector3D(0.6f, 0.55f, 0.55f),
                     bodyLower, 0, QVector3D(0, 1, 0)});

    // === 上身 (紫色, 椭球) ===
    spheres.append({QVector3D(0, -0.05f + y, 0), QVector3D(0.55f, 0.5f, 0.5f),
                     bodyUpper, 0, QVector3D(0, 1, 0)});

    // === 脖子 (过渡, 小球) ===
    spheres.append({QVector3D(0, 0.3f + y, 0), QVector3D(0.2f, 0.15f, 0.2f),
                     skin, 0, QVector3D(0, 1, 0)});

    // === 头部 (圆脸, 肤色) ===
    spheres.append({QVector3D(0, 0.65f + y, 0.05f), QVector3D(0.5f, 0.48f, 0.48f),
                     skin, 0, QVector3D(0, 1, 0)});

    // === 后脑头发 (银灰, 覆盖头部后半) ===
    spheres.append({QVector3D(0, 0.75f + y, -0.15f), QVector3D(0.52f, 0.5f, 0.35f),
                     hairMain, 0, QVector3D(0, 1, 0)});

    // === 头顶头发 (银灰, 蓬松) ===
    spheres.append({QVector3D(0, 1.0f + y, 0), QVector3D(0.48f, 0.3f, 0.4f),
                     hairMain, 0, QVector3D(0, 1, 0)});

    // === 呆毛 (标志性向上翘起的发束) ===
    spheres.append({QVector3D(0.05f, 1.25f + y, -0.02f), QVector3D(0.06f, 0.18f, 0.05f),
                     hairMain, -25.0f, QVector3D(0, 0, 1)});
    spheres.append({QVector3D(0.12f, 1.35f + y, -0.05f), QVector3D(0.04f, 0.1f, 0.03f),
                     hairMain, -35.0f, QVector3D(0, 0, 1)});

    // === 刘海 (不对称, 银灰) ===
    // 左侧短刘海
    spheres.append({QVector3D(-0.18f, 0.85f + y, 0.35f), QVector3D(0.12f, 0.2f, 0.1f),
                     hairMain, 15.0f, QVector3D(0, 0, 1)});
    spheres.append({QVector3D(-0.28f, 0.75f + y, 0.3f), QVector3D(0.08f, 0.15f, 0.08f),
                     hairShadow, 25.0f, QVector3D(0, 0, 1)});
    // 右侧长刘海 (遮眼方向)
    spheres.append({QVector3D(0.15f, 0.8f + y, 0.35f), QVector3D(0.1f, 0.25f, 0.1f),
                     hairMain, -10.0f, QVector3D(0, 0, 1)});
    spheres.append({QVector3D(0.25f, 0.7f + y, 0.3f), QVector3D(0.07f, 0.18f, 0.08f),
                     hairShadow, -20.0f, QVector3D(0, 0, 1)});

    // === 侧发 (两侧, 带反翘效果) ===
    // 左侧 (向外翻翘)
    spheres.append({QVector3D(-0.4f, 0.55f + y, 0.0f), QVector3D(0.1f, 0.3f, 0.1f),
                     hairMain, -15.0f, QVector3D(0, 0, 1)});
    spheres.append({QVector3D(-0.48f, 0.35f + y, -0.05f), QVector3D(0.07f, 0.15f, 0.07f),
                     hairMain, -30.0f, QVector3D(0, 0, 1)});
    // 右侧 (相对服帖, 向内卷)
    spheres.append({QVector3D(0.4f, 0.55f + y, 0.0f), QVector3D(0.1f, 0.3f, 0.1f),
                     hairMain, 12.0f, QVector3D(0, 0, 1)});
    spheres.append({QVector3D(0.45f, 0.4f + y, -0.02f), QVector3D(0.06f, 0.12f, 0.06f),
                     hairMain, 25.0f, QVector3D(0, 0, 1)});

    // === 眼睛 (大眼, 深色) ===
    if (m_mood == PetMood::Sleep) {
        spheres.append({QVector3D(-0.13f, 0.65f + y, 0.42f), QVector3D(0.07f, 0.02f, 0.03f),
                         QColor("#2d3748"), 0, QVector3D(0, 1, 0)});
        spheres.append({QVector3D(0.13f, 0.65f + y, 0.42f), QVector3D(0.07f, 0.02f, 0.03f),
                         QColor("#2d3748"), 0, QVector3D(0, 1, 0)});
    } else {
        // 白眼底
        spheres.append({QVector3D(-0.13f, 0.65f + y, 0.4f), QVector3D(0.09f, 0.1f, 0.04f),
                         QColor("#ffffff"), 0, QVector3D(0, 1, 0)});
        spheres.append({QVector3D(0.13f, 0.65f + y, 0.4f), QVector3D(0.09f, 0.1f, 0.04f),
                         QColor("#ffffff"), 0, QVector3D(0, 1, 0)});
        // 深色虹膜
        float pupilY = (m_mood == PetMood::Happy) ? 0.04f : 0.0f;
        spheres.append({QVector3D(-0.13f, 0.65f + pupilY + y, 0.44f), QVector3D(0.055f, 0.07f, 0.02f),
                         QColor("#2d3748"), 0, QVector3D(0, 1, 0)});
        spheres.append({QVector3D(0.13f, 0.65f + pupilY + y, 0.44f), QVector3D(0.055f, 0.07f, 0.02f),
                         QColor("#2d3748"), 0, QVector3D(0, 1, 0)});
        // 高光点
        spheres.append({QVector3D(-0.11f, 0.68f + pupilY + y, 0.46f), QVector3D(0.02f, 0.02f, 0.01f),
                         QColor("#ffffff"), 0, QVector3D(0, 1, 0)});
        spheres.append({QVector3D(0.15f, 0.68f + pupilY + y, 0.46f), QVector3D(0.02f, 0.02f, 0.01f),
                         QColor("#ffffff"), 0, QVector3D(0, 1, 0)});
    }

    // === 腮红 ===
    spheres.append({QVector3D(-0.28f, 0.55f + y, 0.38f), QVector3D(0.04f, 0.03f, 0.02f),
                     QColor("#ff9999"), 0, QVector3D(0, 1, 0)});
    spheres.append({QVector3D(0.28f, 0.55f + y, 0.38f), QVector3D(0.04f, 0.03f, 0.02f),
                     QColor("#ff9999"), 0, QVector3D(0, 1, 0)});

    // === 嘴巴 ===
    if (m_mood == PetMood::Happy || m_mood == PetMood::Excited) {
        float ms = (m_mood == PetMood::Excited) ? 0.1f : 0.07f;
        spheres.append({QVector3D(0, 0.42f + y, 0.45f), QVector3D(ms, ms * 0.5f, 0.02f),
                         QColor("#2d3748"), 0, QVector3D(0, 1, 0)});
    } else {
        spheres.append({QVector3D(0, 0.45f + y, 0.45f), QVector3D(0.03f, 0.02f, 0.02f),
                         QColor("#2d3748"), 0, QVector3D(0, 1, 0)});
    }

    // === 翅膀 (左右展开, 白色羽翼 - 同仙鸟结构) ===
    // 主翅
    spheres.append({QVector3D(-0.75f, 0.0f + y, -0.1f), QVector3D(0.13f, 0.38f, 0.07f),
                     m_secondary, 25.0f + m_clapAngle * 0.5f, QVector3D(0, 0, 1)});
    spheres.append({QVector3D(0.75f, 0.0f + y, -0.1f), QVector3D(0.13f, 0.38f, 0.07f),
                     m_secondary, -25.0f - m_clapAngle * 0.5f, QVector3D(0, 0, 1)});
    // 副翅
    spheres.append({QVector3D(-0.85f, -0.3f + y, -0.1f), QVector3D(0.09f, 0.28f, 0.05f),
                     m_secondary, 35.0f + m_clapAngle * 0.5f, QVector3D(0, 0, 1)});
    spheres.append({QVector3D(0.85f, -0.3f + y, -0.1f), QVector3D(0.09f, 0.28f, 0.05f),
                     m_secondary, -35.0f - m_clapAngle * 0.5f, QVector3D(0, 0, 1)});

    // === 手臂 (紫色, 带拍手动画) ===
    spheres.append({QVector3D(-0.6f, -0.1f + y, 0.2f), QVector3D(0.12f, 0.25f, 0.12f),
                     bodyUpper, m_clapAngle, QVector3D(0, 0, 1)});
    spheres.append({QVector3D(0.6f, -0.1f + y, 0.2f), QVector3D(0.12f, 0.25f, 0.12f),
                     bodyUpper, -m_clapAngle, QVector3D(0, 0, 1)});

    // === 尾羽 (扇形, 白色) ===
    spheres.append({QVector3D(0, -0.6f + y, -0.4f), QVector3D(0.09f, 0.3f, 0.05f),
                     m_secondary, m_wagAngle * 0.3f, QVector3D(0, 1, 0)});
    spheres.append({QVector3D(-0.2f, -0.65f + y, -0.35f), QVector3D(0.07f, 0.25f, 0.04f),
                     m_secondary, -15.0f + m_wagAngle * 0.3f, QVector3D(0, 1, 0)});
    spheres.append({QVector3D(0.2f, -0.65f + y, -0.35f), QVector3D(0.07f, 0.25f, 0.04f),
                     m_secondary, 15.0f + m_wagAngle * 0.3f, QVector3D(0, 1, 0)});

    // === 脚 (黄色) ===
    spheres.append({QVector3D(-0.2f, -1.1f + y, 0.1f), QVector3D(0.12f, 0.08f, 0.18f),
                     bodyLower, 0, QVector3D(0, 1, 0)});
    spheres.append({QVector3D(0.2f, -1.1f + y, 0.1f), QVector3D(0.12f, 0.08f, 0.18f),
                     bodyLower, 0, QVector3D(0, 1, 0)});
}

// === 绘制对话框 ===
void PetCanvas::drawSpeechBubble(QPainter &painter)
{
    if (!m_showBubble) return;

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // 对话框尺寸
    int bubbleWidth = 130;
    int bubbleHeight = 45;
    int x = width() - bubbleWidth - 15;
    int y = 15;

    QRect bubbleRect(x, y, bubbleWidth, bubbleHeight);

    // 阴影
    QRect shadowRect = bubbleRect.adjusted(2, 3, 2, 3);
    painter.setBrush(QColor(0, 0, 0, 60));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(shadowRect, 18, 18);

    // 对话框背景
    painter.setBrush(QColor(255, 255, 255, 240));
    painter.setPen(QPen(QColor("#8b5cf6"), 2));
    painter.drawRoundedRect(bubbleRect, 18, 18);

    // 小尾巴
    QPainterPath tail;
    int tailX = x + 25;
    int tailY = y + bubbleHeight;
    tail.moveTo(tailX, tailY);
    tail.lineTo(tailX + 8, tailY + 18);
    tail.lineTo(tailX + 20, tailY);
    painter.setBrush(QColor(255, 255, 255, 240));
    painter.setPen(Qt::NoPen);
    painter.drawPath(tail);

    // 尾巴边框
    painter.setPen(QPen(QColor("#8b5cf6"), 2));
    painter.drawLine(tailX, tailY, tailX + 8, tailY + 18);
    painter.drawLine(tailX + 8, tailY + 18, tailX + 20, tailY);

    // 文字
    painter.setPen(QColor("#1a1a1a"));
    QFont font(QStringLiteral("Microsoft YaHei"), 11, QFont::Bold);
    painter.setFont(font);
    painter.drawText(bubbleRect, Qt::AlignCenter, m_bubbleText);

    painter.restore();
}

// === 主绘制事件 ===
void PetCanvas::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    float t = QTime::currentTime().msecsSinceStartOfDay() / 1000.0f;

    // 全体轻微旋转 (Y轴)
    m_rotation = sinf(t * 0.3f) * 8.0f;

    // 跳跃偏移
    float bounce = 0.0f;
    if (m_mood == PetMood::Happy) {
        bounce = fabsf(sinf(t * 4.0f)) * 0.3f;
    } else if (m_mood == PetMood::Excited) {
        bounce = fabsf(sinf(t * 6.0f)) * 0.4f;
    } else if (m_mood == PetMood::Sleep) {
        bounce = sinf(t * 0.5f) * 0.05f;
    }
    m_bounceY = bounce;

    // 尾巴摆动
    m_wagAngle = sinf(t * 2.0f) * 20.0f;

    // 手臂摆动
    if (m_mood == PetMood::Happy) {
        m_clapAngle = sinf(t * 6.0f) * 25.0f;
    } else if (m_mood == PetMood::Excited) {
        m_clapAngle = sinf(t * 8.0f) * 35.0f;
    } else {
        m_clapAngle = 0.0f;
    }

    // 收集球体
    m_spheres.clear();
    switch (m_modelType) {
    case PetModelType::CatLike:
        collectCatModel(m_spheres);
        break;
    case PetModelType::BearLike:
        collectBearModel(m_spheres);
        break;
    case PetModelType::BunnyLike:
        collectBunnyModel(m_spheres);
        break;
    case PetModelType::FairyBirdLike:
        collectFairyBirdModel(m_spheres);
        break;
    case PetModelType::SpiritLike:
        collectSpiritModel(m_spheres);
        break;
    }

    // 应用全局 Y 轴旋转
    float rad = m_rotation * M_PI / 180.0f;
    float cosR = cosf(rad);
    float sinR = sinf(rad);
    for (auto &s : m_spheres) {
        float x = s.pos.x();
        float z = s.pos.z();
        s.pos.setX(x * cosR + z * sinR);
        s.pos.setZ(-x * sinR + z * cosR);
    }

    // 绘制3D模型
    sortAndDrawSpheres(painter);

    // 绘制对话框
    drawSpeechBubble(painter);
}

// === 接口实现 ===

void PetCanvas::setColors(const QColor &primary, const QColor &secondary, const QColor &accent)
{
    m_primary = primary;
    m_secondary = secondary;
    m_accent = accent;
    update();
}

void PetCanvas::setMood(PetMood mood)
{
    m_mood = mood;
    m_showBubble = (mood == PetMood::Excited);

    if (mood == PetMood::Happy || mood == PetMood::Excited) {
        m_moodTimer.start(3000);
    } else {
        m_moodTimer.stop();
    }
    update();
}

void PetCanvas::setModelType(PetModelType type)
{
    m_modelType = type;
    update();
}

void PetCanvas::startWagging()
{
    if (!m_animTimer.isActive()) {
        m_animTimer.start();
    }
}

void PetCanvas::stopWagging()
{
    m_animTimer.stop();
}

void PetCanvas::onAnimTick()
{
    update();
}

void PetCanvas::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked();
        if (QRandomGenerator::global()->bounded(2) == 0) {
            setMood(PetMood::Happy);
        } else {
            setMood(PetMood::Excited);
        }
    }
    QWidget::mousePressEvent(event);
}

void PetCanvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    emit doubleClicked();
    QWidget::mouseDoubleClickEvent(event);
}
