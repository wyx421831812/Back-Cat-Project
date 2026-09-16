#include "petstage.h"
#include "bongocatwidget.h"
#include "petcanvas.h"
#include "bongomodelmanager.h"
#include "appconfig.h"

#include <QResizeEvent>

namespace {
// 3D 模型 100% 缩放时的基准逻辑像素 (球体模型在 300x300 画布中绘制)
constexpr int kPet3DBase = 300;
// Live2D 宽度基准 (与历史窗口宽度一致), 高度按 modelLoaded 宽高比换算
constexpr int kLive2DBaseWidth = 420;
// 切换遮罩兜底超时 (静态模型不发 modelLoaded)
constexpr int kSwitchMaskTimeoutMs = 1200;
// 3D 切换遮罩时长
constexpr int kPet3DMaskTimeoutMs = 250;
// resize "重绘中…" 防抖
constexpr int kResizeDebounceMs = 100;
}

PetStage::PetStage(BongoCatWidget *bongo, PetCanvas *canvas, QWidget *parent)
    : QWidget(parent)
    , m_bongo(bongo)
    , m_petCanvas(canvas)
{
    Q_ASSERT(m_bongo);
    Q_ASSERT(m_petCanvas);

    setAttribute(Qt::WA_TranslucentBackground);

    m_stack = new QStackedWidget(this);
    m_stack->setAttribute(Qt::WA_TranslucentBackground);
    // addWidget 会自动 reparent, 把两个已存在的渲染部件收编进舞台
    m_stack->addWidget(m_bongo);       // index 0: Live2D/BongoCat
    m_stack->addWidget(m_petCanvas);   // index 1: 3D 宠物

    // 遮罩层 (黄紫配色, 覆盖整个舞台)
    m_mask = new QLabel(this);
    m_mask->setAlignment(Qt::AlignCenter);
    m_mask->setStyleSheet(QStringLiteral(
        "QLabel {"
        "  background: rgba(30, 27, 46, 205);"
        "  color: #FBBF24;"
        "  border: 1px solid #8B5CF6;"
        "  border-radius: 12px;"
        "  font-size: 14px;"
        "  font-weight: 600;"
        "}"
    ));
    m_mask->hide();

    m_maskFallback.setSingleShot(true);
    connect(&m_maskFallback, &QTimer::timeout, this, &PetStage::hideMask);
    m_resizeDebounce.setSingleShot(true);
    m_resizeDebounce.setInterval(kResizeDebounceMs);
    connect(&m_resizeDebounce, &QTimer::timeout, this, &PetStage::hideMask);

    // BongoCat 构造时已把首次加载延迟到事件循环, 这里的连接一定早于加载完成
    connect(m_bongo, &BongoCatWidget::live2dModelLoaded,
            this, &PetStage::onLive2dLoaded);
    connect(m_bongo, &BongoCatWidget::modelBackgroundSize,
            this, &PetStage::onModelBackgroundSize);
}

QSize PetStage::baseSize() const
{
    if (m_kind == Kind::Pet3D) {
        return QSize(kPet3DBase, kPet3DBase);
    }
    return m_live2dBase;
}

QSize PetStage::desiredSize() const
{
    QSize s = baseSize();
    const int w = qMax(40, qRound(s.width() * m_scale / 100.0));
    const int h = qMax(40, qRound(s.height() * m_scale / 100.0));
    return QSize(w, h);
}

int PetStage::radiusPx() const
{
    if (m_radius <= 0) return 0;
    // CSS border-radius 百分比: 100% 时短边完全变圆
    const int r = qMin(width(), height()) / 2 * m_radius / 100;
    return qBound(0, r, qMin(width(), height()) / 2);
}

void PetStage::setScalePercent(int percent)
{
    percent = qBound(1, percent, 500);
    if (m_scale == percent) return;
    m_scale = percent;
    present();
}

void PetStage::setRadiusPercent(int percent)
{
    m_radius = qBound(0, percent, 100);
}

void PetStage::setMirrored(bool on)
{
    m_mirror = on;
    m_bongo->setMirror(on);
    m_petCanvas->setMirrored(on);
}

void PetStage::applyConfig()
{
    const auto &cfg = AppConfig::instance();
    m_scale = qBound(1, cfg.windowScale(), 500);
    m_radius = qBound(0, cfg.windowRadius(), 100);
    m_mirror = cfg.mirror();
    m_bongo->setMirror(m_mirror);
    m_petCanvas->setMirrored(m_mirror);
}

void PetStage::showLive2D(const QString &modelId)
{
    QString id = modelId;
    if (id.isEmpty()) {
        id = BongoModelManager::instance().currentModelId();
    }

    const bool kindChanged = (m_kind != Kind::Live2D);
    const bool modelChanged = (id != m_live2dId);

    m_kind = Kind::Live2D;
    m_live2dId = id;

    m_stack->setCurrentWidget(m_bongo);
    m_bongo->onShow();

    // setCurrentModel 会触发 BongoCatWidget 自动 reload;
    // 同模型时管理器不发 modelChanged, 无需重复调用
    if (!id.isEmpty() &&
        BongoModelManager::instance().currentModelId() != id) {
        BongoModelManager::instance().setCurrentModel(id);
    }

    if (kindChanged || modelChanged) {
        showMask(QStringLiteral("🐾 切换中…"));
        m_maskFallback.start(kSwitchMaskTimeoutMs);
    }

    present();
}

void PetStage::showPet3D(int type)
{
    type = qBound(0, type, 4);
    const bool kindChanged = (m_kind != Kind::Pet3D);
    const bool typeChanged = (m_pet3dType != type);

    m_kind = Kind::Pet3D;
    m_pet3dType = type;

    m_stack->setCurrentWidget(m_petCanvas);
    m_petCanvas->setModelType(static_cast<PetModelType>(type));
    m_bongo->onHide();

    if (kindChanged || typeChanged) {
        showMask(QStringLiteral("🐾 切换中…"));
        m_maskFallback.start(kPet3DMaskTimeoutMs);
    }

    present();
}

void PetStage::onLive2dLoaded(int width, int height)
{
    Q_UNUSED(width)
    Q_UNUSED(height)
    // 窗口宽高比以背景场景图为准 (见 onModelBackgroundSize),
    // Live2D 模型画布本身的尺寸不决定窗口比例; 此处仅负责隐藏切换遮罩。
    if (m_kind != Kind::Live2D) return;

    m_maskFallback.stop();
    hideMask();
}

void PetStage::onModelBackgroundSize(const QSize &size)
{
    if (m_kind != Kind::Live2D) return;
    if (!size.isValid() || size.width() <= 0 || size.height() <= 0) return;

    const double ratio = double(size.height()) / double(size.width());
    if (ratio < 0.3 || ratio > 3.0) return;  // 异常图不采用

    // 以 420 逻辑像素宽为基准, 按背景自然宽高比算高度
    const int h = qBound(40, qRound(kLive2DBaseWidth * ratio), 4000);
    const QSize base(kLive2DBaseWidth, h);
    if (base != m_live2dBase) {
        m_live2dBase = base;
        present();
    }
}

void PetStage::present()
{
    const QSize want = desiredSize();
    setFixedSize(want);
    m_stack->setGeometry(rect());
    m_mask->setGeometry(rect());
    m_mask->raise();
    emit desiredSizeChanged(want);
}

void PetStage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    m_stack->setGeometry(rect());
    m_mask->setGeometry(rect());
    m_mask->raise();

    // WebEngine 缩放过程中可能短暂闪烁, 显示"重绘中…", 停止 resize 100ms 后隐藏。
    // 3D 为 QPainter 即时绘制, 不需要。
    if (m_kind == Kind::Live2D) {
        if (!m_mask->isVisible()) {
            showMask(QStringLiteral("🐾 重绘中…"));
        }
        m_resizeDebounce.start();
    }
}

void PetStage::showMask(const QString &text)
{
    m_mask->setText(text);
    m_mask->setGeometry(rect());
    m_mask->raise();
    m_mask->show();
}

void PetStage::hideMask()
{
    m_mask->hide();
}
