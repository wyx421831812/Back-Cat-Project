#include "petwidget.h"
#include "preference/preferencewindow.h"
#include "components/clockwidget.h"
#include "components/quotewidget.h"
#include "components/todowidget.h"
#include "components/bongocatwidget.h"
#include "components/bongomodelmanager.h"
#include "components/petstage.h"

#include <QApplication>
#include <QScreen>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTransform>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
#include <QFile>
#include <QFileInfo>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#endif

PetWidget::PetWidget(QWidget *parent)
    : QWidget(parent)
    , m_dragging(false)
    , m_stack(nullptr)
    , m_petCanvas(nullptr)
    , m_clockWidget(nullptr)
    , m_quoteWidget(nullptr)
    , m_todoWidget(nullptr)
    , m_bongoCatWidget(nullptr)
    , m_stage(nullptr)
    , m_trayIcon(nullptr)
    , m_trayMenu(nullptr)
    , m_contextMenu(nullptr)
    , m_trayBongoModelMenu(nullptr)
    , m_contextBongoModelMenu(nullptr)
{
    setupUI();
    setupTrayIcon();
    setupContextMenu();

    // 初始化 BongoCat 模型 (必须在两个菜单都创建完成后)
    BongoModelManager::instance().loadModels();
    rebuildBongoCatModelMenu();

    // 监听模型变化
    connect(&BongoModelManager::instance(), &BongoModelManager::modelChanged,
            this, [this](const QString &) {
        rebuildBongoCatModelMenu();
    });
    connect(&BongoModelManager::instance(), &BongoModelManager::modelsLoaded,
            this, [this]() {
        rebuildBongoCatModelMenu();
    });

    applyConfig();

    // 启动 HWND 句柄看门狗 (QWebEngineView 初始化等场景会悄悄重建原生窗口)
    m_lastNativeHwnd = winId();
    m_nativeWatchTimer = new QTimer(this);
    connect(m_nativeWatchTimer, &QTimer::timeout,
            this, &PetWidget::checkNativeWindowRecreated);
    m_nativeWatchTimer->start(300);
}

PetWidget::~PetWidget()
{
    // 保存窗口位置
    AppConfig::instance().setWindowPosition(pos());
    AppConfig::instance().setWindowSize(size());
    AppConfig::instance().save();
}

void PetWidget::setupUI()
{
    // 无边框 + 透明背景 + 始终置顶 + 不抢焦点
    setWindowFlags(
        Qt::FramelessWindowHint |
        Qt::WindowStaysOnTopHint |
        Qt::Tool  // 不在任务栏显示
    );
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    // 固定大小
    setFixedSize(200, 250);

    // 先创建两个渲染部件 (PetStage 会把它们 reparent 进自己的内部栈)
    m_petCanvas = new PetCanvas();
    m_bongoCatWidget = new BongoCatWidget();

    // 统一舞台: 同窗承载 Live2D 与 3D 模型
    m_stage = new PetStage(m_bongoCatWidget, m_petCanvas);

    // 堆叠窗口 (舞台 / 时钟 / 寄语 / 待办)
    m_stack = new QStackedWidget(this);
    m_stack->setGeometry(0, 0, 200, 250);
    m_stack->addWidget(m_stage);        // index 0: 猫咪舞台
    m_stack->addWidget(new ClockWidget());   // index 1: 时钟
    m_stack->addWidget(new QuoteWidget());   // index 2: 寄语
    m_stack->addWidget(new TodoWidget());    // index 3: 待办
    m_clockWidget = static_cast<ClockWidget *>(m_stack->widget(ClockComponent));
    m_quoteWidget = static_cast<QuoteWidget *>(m_stack->widget(QuoteComponent));
    m_todoWidget = static_cast<TodoWidget *>(m_stack->widget(TodoComponent));

    m_stack->setCurrentIndex(0);

    // 信号连接
    connect(m_petCanvas, &PetCanvas::clicked, this, &PetWidget::onPetClicked);

    // 舞台按模型自然尺寸 x 缩放请求窗口尺寸
    connect(m_stage, &PetStage::desiredSizeChanged,
            this, &PetWidget::resizeForStage);

    // 组件切换时管理 Bongo Cat 键盘钩子 (舞台内部模型切换已自行管理,
    // 这里只需处理"离开舞台/回到舞台")
    connect(m_stack, &QStackedWidget::currentChanged, this, [this](int index) {
        if (index == StageComponent) {
            if (m_stage->kind() == PetStage::Kind::Live2D) {
                m_bongoCatWidget->onShow();
            }
        } else {
            m_bongoCatWidget->onHide();
        }
    });

    // 鼠标移入隐藏定时器 (时长按配置, 0=立即)
    m_hoverHideTimer = new QTimer(this);
    m_hoverHideTimer->setSingleShot(true);
    connect(m_hoverHideTimer, &QTimer::timeout, this, [this] {
        if (AppConfig::instance().hideOnHover()) {
            m_hiddenByHover = true;
            hide();
        }
    });

    // 配置实时生效: 窗口分组 (缩放/圆角/不透明度/穿透/置顶/悬停隐藏)
    connect(&AppConfig::instance(), &AppConfig::catWindowChanged, this, [this] {
        if (m_stack->currentIndex() == StageComponent) {
            m_stage->setScalePercent(AppConfig::instance().windowScale());
            m_stage->setRadiusPercent(AppConfig::instance().windowRadius());
            updateWindowMask();
        }
        updateOpacity();
        updateClickThrough();
        updateAlwaysOnTop();
        // 关闭"移入隐藏"时立即把猫呼回
        if (!AppConfig::instance().hideOnHover()) {
            m_hoverHideTimer->stop();
            if (m_hiddenByHover) {
                m_hiddenByHover = false;
                show();
            }
        }
    });
    // 模型分组 (镜像/忽略鼠标/鼠标镜像/释放延迟/帧率)
    connect(&AppConfig::instance(), &AppConfig::catModelChanged,
            this, &PetWidget::applyModelRuntimeSettings);
}

void PetWidget::applyModelRuntimeSettings()
{
    const auto &cfg = AppConfig::instance();
    m_stage->setMirrored(cfg.mirror());
    m_bongoCatWidget->setIgnoreMouseEvents(cfg.ignoreMouse());
    m_bongoCatWidget->setMouseMirrored(cfg.mouseMirror());
    m_bongoCatWidget->setAutoReleaseDelay(cfg.autoReleaseDelay());  // 毫秒
    m_bongoCatWidget->setMaxFps(cfg.maxFps());
}

void PetWidget::setupTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(this);

    // 简单图标 (用 QPixmap 绘制)
    QPixmap iconPixmap(64, 64);
    iconPixmap.fill(Qt::transparent);
    QPainter painter(&iconPixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    QLinearGradient grad(0, 0, 64, 64);
    grad.setColorAt(0, QColor("#fbbf24"));
    grad.setColorAt(1, QColor("#8b5cf6"));
    painter.setBrush(grad);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(12, 12, 40, 40);
    // 简单眼睛
    painter.setBrush(Qt::white);
    painter.drawEllipse(QPointF(24, 28), 5, 5);
    painter.drawEllipse(QPointF(40, 28), 5, 5);
    painter.setBrush(QColor("#1a1a1a"));
    painter.drawEllipse(QPointF(24, 28), 2, 2);
    painter.drawEllipse(QPointF(40, 28), 2, 2);

    m_trayIcon->setIcon(QIcon(iconPixmap));
    m_trayIcon->setToolTip(QStringLiteral("BackPet - 情绪价值桌面组件"));
    m_trayIcon->show();

    m_trayMenu = new QMenu(this);

    // 组件切换子菜单
    QMenu *componentMenu = m_trayMenu->addMenu(QStringLiteral("切换组件"));
    componentMenu->addAction(QStringLiteral("🐱 互动宠物"), this, &PetWidget::switchToPet);
    componentMenu->addAction(QStringLiteral("🕐 时钟日历"), this, &PetWidget::switchToClock);
    componentMenu->addAction(QStringLiteral("💬 每日寄语"), this, &PetWidget::switchToQuote);
    componentMenu->addAction(QStringLiteral("✅ 待办清单"), this, &PetWidget::switchToTodo);
    componentMenu->addAction(QStringLiteral("⌨️ Bongo Cat"), this, &PetWidget::switchToBongoCat);

    // 情绪控制子菜单
    QMenu *moodMenu = m_trayMenu->addMenu(QStringLiteral("宠物情绪"));
    moodMenu->addAction(QStringLiteral("😃 开心鼓掌"), this, &PetWidget::setMoodHappy);
    moodMenu->addAction(QStringLiteral("😴 犯困打盹"), this, &PetWidget::setMoodSleep);
    moodMenu->addAction(QStringLiteral("🤪 高兴呐喊"), this, &PetWidget::setMoodExcited);
    moodMenu->addAction(QStringLiteral("😊 平静下来"), this, &PetWidget::setMoodNeutral);

    // 3D模型切换子菜单
    QMenu *modelMenu = m_trayMenu->addMenu(QStringLiteral("3D模型"));
    modelMenu->addAction(QStringLiteral("🐱 猫咪 (默认)"), this, &PetWidget::setModelCat);
    modelMenu->addAction(QStringLiteral("🐻 小熊"), this, &PetWidget::setModelBear);
    modelMenu->addAction(QStringLiteral("🐰 兔子"), this, &PetWidget::setModelBunny);
    modelMenu->addAction(QStringLiteral("🦢 仙鸟"), this, &PetWidget::setModelFairyBird);
    modelMenu->addAction(QStringLiteral("👻 幻灵"), this, &PetWidget::setModelSpirit);

    // BongoCat 模型子菜单
    m_trayBongoModelMenu = m_trayMenu->addMenu(QStringLiteral("🐱 BongoCat模型"));

    m_trayMenu->addSeparator();
    m_trayMenu->addAction(QStringLiteral("偏好设置…"), this, &PetWidget::onSettings);
    m_trayMenu->addAction(QStringLiteral("❌ 退出"), this, &PetWidget::onQuit);

    m_trayIcon->setContextMenu(m_trayMenu);

    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, &PetWidget::onTrayActivated);
}

void PetWidget::setupContextMenu()
{
    m_contextMenu = new QMenu(this);

    // 组件切换
    QMenu *componentMenu = m_contextMenu->addMenu(QStringLiteral("切换组件"));
    componentMenu->addAction(QStringLiteral("🐱 互动宠物"), this, &PetWidget::switchToPet);
    componentMenu->addAction(QStringLiteral("🕐 时钟日历"), this, &PetWidget::switchToClock);
    componentMenu->addAction(QStringLiteral("💬 每日寄语"), this, &PetWidget::switchToQuote);
    componentMenu->addAction(QStringLiteral("✅ 待办清单"), this, &PetWidget::switchToTodo);
    componentMenu->addAction(QStringLiteral("⌨️ Bongo Cat"), this, &PetWidget::switchToBongoCat);

    // 情绪
    QMenu *moodMenu = m_contextMenu->addMenu(QStringLiteral("宠物情绪"));
    moodMenu->addAction(QStringLiteral("😃 开心鼓掌"), this, &PetWidget::setMoodHappy);
    moodMenu->addAction(QStringLiteral("😴 犯困打盹"), this, &PetWidget::setMoodSleep);
    moodMenu->addAction(QStringLiteral("🤪 高兴呐喊"), this, &PetWidget::setMoodExcited);
    moodMenu->addAction(QStringLiteral("😊 平静下来"), this, &PetWidget::setMoodNeutral);

    // 3D模型切换
    QMenu *modelMenu = m_contextMenu->addMenu(QStringLiteral("3D模型"));
    modelMenu->addAction(QStringLiteral("🐱 猫咪 (默认)"), this, &PetWidget::setModelCat);
    modelMenu->addAction(QStringLiteral("🐻 小熊"), this, &PetWidget::setModelBear);
    modelMenu->addAction(QStringLiteral("🐰 兔子"), this, &PetWidget::setModelBunny);
    modelMenu->addAction(QStringLiteral("🦢 仙鸟"), this, &PetWidget::setModelFairyBird);
    modelMenu->addAction(QStringLiteral("👻 幻灵"), this, &PetWidget::setModelSpirit);

    // BongoCat 模型子菜单
    m_contextBongoModelMenu = m_contextMenu->addMenu(QStringLiteral("🐱 BongoCat模型"));

    m_contextMenu->addSeparator();
    m_contextMenu->addAction(QStringLiteral("偏好设置…"), this, &PetWidget::onSettings);
    m_contextMenu->addAction(QStringLiteral("❌ 退出"), this, &PetWidget::onQuit);
}

void PetWidget::applyConfig()
{
    auto &cfg = AppConfig::instance();

    // 3D 模型颜色
    m_petCanvas->setColors(cfg.primaryColor(), cfg.secondaryColor(), cfg.accentColor());

    // 先恢复窗口位置, 之后舞台按目标尺寸改窗口时会以中心点为锚并夹取到屏幕内
    QPoint pos = cfg.windowPosition();
    if (pos.x() >= 0 && pos.y() >= 0) {
        move(pos);
    } else {
        // 默认右下角
        QScreen *screen = QGuiApplication::primaryScreen();
        QRect geo = screen->availableGeometry();
        move(geo.right() - 460, geo.bottom() - 320);
    }

    // 舞台外观 (缩放/圆角/镜像), 不触发切换
    m_stage->applyConfig();
    // 模型运行时设置 (忽略鼠标/鼠标镜像/释放延迟/帧率)
    applyModelRuntimeSettings();

    // 组件恢复。旧扁平索引: 4=BongoCat → 舞台 Live2D, 0=3D 宠物 → 舞台 3D,
    // 1-3=时钟/寄语/待办。全新分组配置没有该键, 按 cat.model.kind 恢复。
    const int idx = cfg.currentComponentIndex();
    // 必须查原始 JSON: 构造函数预植了 currentComponentIndex 默认值
    const bool hasLegacyIndex = cfg.rawHasKey(QStringLiteral("currentComponentIndex"));
    if (hasLegacyIndex && idx == 4) {
        m_stage->showLive2D();
        m_stack->setCurrentIndex(StageComponent);
    } else if (hasLegacyIndex && idx == StageComponent) {
        m_stage->showPet3D(cfg.pet3dType());
        m_stack->setCurrentIndex(StageComponent);
    } else if (hasLegacyIndex) {
        m_stack->setCurrentIndex(qBound(1, idx, static_cast<int>(TodoComponent)));
        resizeForAuxWidget();
        m_bongoCatWidget->onHide();
    } else if (cfg.modelKind() == QStringLiteral("pet3d")) {
        // 新 schema: 分组 cat.model.kind 决定舞台形态
        m_stage->showPet3D(cfg.pet3dType());
        m_stack->setCurrentIndex(StageComponent);
    } else {
        m_stage->showLive2D();
        m_stack->setCurrentIndex(StageComponent);
    }

    // 其他属性
    updateAlwaysOnTop();
    updateClickThrough();
    updateOpacity();
}

void PetWidget::resizeForStage(const QSize &size)
{
    // 以窗口中心为锚调整尺寸, 避免缩放时宠物乱跳
    const QPoint center = frameGeometry().center();
    setFixedSize(size);
    m_stack->setGeometry(0, 0, size.width(), size.height());

    QRect target(center.x() - size.width() / 2,
                 center.y() - size.height() / 2,
                 size.width(), size.height());

    if (AppConfig::instance().keepInScreen()) {
        QScreen *screen = QGuiApplication::screenAt(center);
        if (!screen) {
            screen = QGuiApplication::primaryScreen();
        }
        const QRect avail = screen->availableGeometry();
        if (target.left() < avail.left()) target.moveLeft(avail.left());
        if (target.top() < avail.top()) target.moveTop(avail.top());
        if (target.right() > avail.right()) target.moveRight(avail.right());
        if (target.bottom() > avail.bottom()) target.moveBottom(avail.bottom());
    }

    move(target.topLeft());
    updateWindowMask();
}

void PetWidget::clampIntoScreen()
{
    if (!AppConfig::instance().keepInScreen()) return;

    QScreen *screen = QGuiApplication::screenAt(frameGeometry().center());
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    QRect target = frameGeometry();
    const QRect avail = screen->availableGeometry();
    if (target.left() < avail.left()) target.moveLeft(avail.left());
    if (target.top() < avail.top()) target.moveTop(avail.top());
    if (target.right() > avail.right()) target.moveRight(avail.right());
    if (target.bottom() > avail.bottom()) target.moveBottom(avail.bottom());

    if (target.topLeft() != frameGeometry().topLeft()) {
        move(target.topLeft());
    }
}

void PetWidget::resizeForAuxWidget()
{
    QSize sz = AppConfig::instance().windowSize();
    if (sz.width() <= 0 || sz.height() <= 0) {
        sz = QSize(200, 250);
    }
    setFixedSize(sz);
    m_stack->setGeometry(0, 0, sz.width(), sz.height());
    // 圆角只属于猫咪舞台窗口
    clearMask();
}

void PetWidget::updateWindowMask()
{
    const int r = m_stage->radiusPx();
    if (r <= 0) {
        clearMask();
        return;
    }

    // QWidget::setMask(QRegion) 在高 DPI 下按物理像素计算,
    // 路径多边形需预先乘 DPR, 保证圆角与逻辑像素一致。
    const qreal dpr = devicePixelRatioF();
    QPainterPath path;
    path.addRoundedRect(QRectF(rect()), r, r);
    QTransform scale;
    scale.scale(dpr, dpr);
    setMask(QRegion(path.toFillPolygon(scale).toPolygon()));
}

void PetWidget::updateClickThrough()
{
#ifdef Q_OS_WIN
    // Windows 点击穿透实现
    HWND hwnd = (HWND)winId();
    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);

    if (AppConfig::instance().clickThrough()) {
        // 启用点击穿透 (WS_EX_TRANSPARENT)
        SetWindowLongPtr(hwnd, GWL_EXSTYLE,
                         exStyle | WS_EX_TRANSPARENT | WS_EX_LAYERED);
    } else {
        // 禁用点击穿透
        SetWindowLongPtr(hwnd, GWL_EXSTYLE,
                         exStyle & ~WS_EX_TRANSPARENT);
    }
#endif
}

void PetWidget::updateAlwaysOnTop()
{
    // 仅在置顶标志实际变化时才调用 setWindowFlags:
    // 对可见窗口 setWindowFlags 会销毁并重建原生窗口, 内嵌 WebEngine 的
    // 渲染层会被重新布局并向页面发送多余的 resize 事件, 触发 Live2D
    // 模型重新缩放。设置→确定时该标志通常并未变化, 跳过可避免
    // 模型尺寸异常变化。
    bool wantTop = AppConfig::instance().alwaysOnTop();
    if (windowFlags().testFlag(Qt::WindowStaysOnTopHint) != wantTop) {
        Qt::WindowFlags flags = windowFlags();
        flags.setFlag(Qt::WindowStaysOnTopHint, wantTop);
        setWindowFlags(flags);
        // HWND 已重建: 100% 不透明度下新窗口为初始逐像素透明, 清标志
        if (AppConfig::instance().windowOpacity() >= 100) {
            m_opacityReduced = false;
        }
    }
    show();
}

void PetWidget::updateOpacity()
{
    // 透明度 (0-100 -> 0.0-1.0)
    qreal op = AppConfig::instance().windowOpacity() / 100.0;

    // 关键: 透明度为 100% 时绝不调用 setWindowOpacity(1.0)。
    // Qt Windows 平台层 (qwindowswindow.cpp setWindowOpacity) 对带 OpenGL/
    // 加速表面(WebEngine 即属此类)的窗口会走
    //   SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA)   // 常量 alpha
    // 分支，这会把 WA_TranslucentBackground 的逐像素透明(AC_SRC_ALPHA)
    // 覆盖成常量不透明，导致 WebEngine 页面的透明区域显示为白色。
    if (op >= 1.0) {
#ifdef Q_OS_WIN
        // 此前 setWindowOpacity 给窗口加了 WS_EX_LAYERED + 常量 alpha
        // (LWA_ALPHA)。实测去掉 WS_EX_LAYERED 并触发 frame 变更, Qt 的
        // DirectComposition 逐像素透明路径会立即恢复, 且不会白屏
        // (setWindowFlags 在此场景下不保证重建 HWND, 故直接改原生样式)。
        if (m_opacityReduced) {
            m_opacityReduced = false;
            if (auto *hwnd = reinterpret_cast<HWND>(winId())) {
                LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
                if (ex & WS_EX_LAYERED) {
                    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex & ~WS_EX_LAYERED);
                    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER
                                     | SWP_NOACTIVATE | SWP_FRAMECHANGED);
                }
            }
        }
#endif
        return;
    }

    setWindowOpacity(op);
    m_opacityReduced = true;
}

void PetWidget::reapplyNativeExtras()
{
    // HWND (重新)创建后, Qt 只恢复它自己管理的 flags; 直接改到原生窗口
    // 上的样式必须重新套用:
    //  - 点击穿透 WS_EX_TRANSPARENT (updateClickThrough)
    //  - <100% 不透明度的 LWA_ALPHA (setWindowOpacity)
    // 置顶由 Qt::WindowStaysOnTopHint 表达, QPA 重建时会自动恢复。
    updateClickThrough();
    if (m_opacityReduced) {
        setWindowOpacity(AppConfig::instance().windowOpacity() / 100.0);
    }
}

void PetWidget::checkNativeWindowRecreated()
{
#ifdef Q_OS_WIN
    const WId cur = winId();
    if (cur != m_lastNativeHwnd) {
        m_lastNativeHwnd = cur;
        // winId() 可能刚强制创建了 HWND; 延迟到事件循环空闲, 等 Qt 完成
        // 新窗口的初始样式设置后再套穿透/半透明, 避免被后续初始化覆盖
        QTimer::singleShot(0, this, &PetWidget::reapplyNativeExtras);
    }
#endif
}

// === 组件切换 ===

void PetWidget::switchToPet()
{
    // 先切舞台内部模型 (kind 变 Pet3D, 顺带 onHide 键盘钩子),
    // 再翻外层栈, 避免 currentChanged 回调误装钩子
    m_stage->showPet3D(AppConfig::instance().pet3dType());
    m_stack->setCurrentIndex(StageComponent);

    AppConfig::instance().setCurrentComponentIndex(StageComponent);
    AppConfig::instance().setModelKind(QStringLiteral("pet3d"));
    AppConfig::instance().save();
}

void PetWidget::switchToClock()
{
    m_stack->setCurrentIndex(ClockComponent);
    resizeForAuxWidget();
    AppConfig::instance().setCurrentComponentIndex(ClockComponent);
    AppConfig::instance().save();
}

void PetWidget::switchToQuote()
{
    m_stack->setCurrentIndex(QuoteComponent);
    resizeForAuxWidget();
    AppConfig::instance().setCurrentComponentIndex(QuoteComponent);
    AppConfig::instance().save();
}

void PetWidget::switchToTodo()
{
    m_stack->setCurrentIndex(TodoComponent);
    resizeForAuxWidget();
    AppConfig::instance().setCurrentComponentIndex(TodoComponent);
    AppConfig::instance().save();
}

void PetWidget::switchToBongoCat()
{
    m_stage->showLive2D();
    m_stack->setCurrentIndex(StageComponent);

    AppConfig::instance().setCurrentComponentIndex(4);
    AppConfig::instance().setModelKind(QStringLiteral("live2d"));
    AppConfig::instance().save();
}

// === 情绪控制 ===

void PetWidget::setMoodHappy()
{
    switchToPet();
    m_petCanvas->setMood(PetMood::Happy);
}

void PetWidget::setMoodSleep()
{
    switchToPet();
    m_petCanvas->setMood(PetMood::Sleep);
}

void PetWidget::setMoodExcited()
{
    switchToPet();
    m_petCanvas->setMood(PetMood::Excited);
}

void PetWidget::setMoodNeutral()
{
    switchToPet();
    m_petCanvas->setMood(PetMood::Neutral);
}

// === 3D模型切换 ===

void PetWidget::setModelCat()
{
    m_stage->showPet3D(0);
    m_stack->setCurrentIndex(StageComponent);
    AppConfig::instance().setPet3dType(0);
    AppConfig::instance().setModelKind(QStringLiteral("pet3d"));
    AppConfig::instance().setCurrentComponentIndex(StageComponent);
    AppConfig::instance().save();
}

void PetWidget::setModelBear()
{
    m_stage->showPet3D(1);
    m_stack->setCurrentIndex(StageComponent);
    AppConfig::instance().setPet3dType(1);
    AppConfig::instance().setModelKind(QStringLiteral("pet3d"));
    AppConfig::instance().setCurrentComponentIndex(StageComponent);
    AppConfig::instance().save();
}

void PetWidget::setModelBunny()
{
    m_stage->showPet3D(2);
    m_stack->setCurrentIndex(StageComponent);
    AppConfig::instance().setPet3dType(2);
    AppConfig::instance().setModelKind(QStringLiteral("pet3d"));
    AppConfig::instance().setCurrentComponentIndex(StageComponent);
    AppConfig::instance().save();
}

void PetWidget::setModelFairyBird()
{
    m_stage->showPet3D(3);
    m_stack->setCurrentIndex(StageComponent);
    AppConfig::instance().setPet3dType(3);
    AppConfig::instance().setModelKind(QStringLiteral("pet3d"));
    AppConfig::instance().setCurrentComponentIndex(StageComponent);
    AppConfig::instance().save();
}

void PetWidget::setModelSpirit()
{
    m_stage->showPet3D(4);
    m_stack->setCurrentIndex(StageComponent);
    AppConfig::instance().setPet3dType(4);
    AppConfig::instance().setModelKind(QStringLiteral("pet3d"));
    AppConfig::instance().setCurrentComponentIndex(StageComponent);
    AppConfig::instance().save();
}

// === 事件处理 ===

void PetWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
    QWidget::mousePressEvent(event);
}

void PetWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragOffset);
        event->accept();
    }
    QWidget::mouseMoveEvent(event);
}

void PetWidget::mouseReleaseEvent(QMouseEvent *event)
{
    m_dragging = false;
    clampIntoScreen();  // "保持在屏幕内": 松手时夹回
    AppConfig::instance().setWindowPosition(pos());
    AppConfig::instance().save();
    QWidget::mouseReleaseEvent(event);
}

void PetWidget::enterEvent(QEnterEvent *event)
{
    // 鼠标移入隐藏: 按延迟秒数计时 (0=立即)
    if (AppConfig::instance().hideOnHover()) {
        const int delayMs = AppConfig::instance().hideOnHoverDelay() * 1000;
        m_hoverHideTimer->start(qMax(0, delayMs));
    }
    QWidget::enterEvent(event);
}

void PetWidget::leaveEvent(QEvent *event)
{
    m_hoverHideTimer->stop();
    QWidget::leaveEvent(event);
}

void PetWidget::contextMenuEvent(QContextMenuEvent *event)
{
    m_contextMenu->exec(event->globalPos());
}

void PetWidget::closeEvent(QCloseEvent *event)
{
    AppConfig::instance().setWindowPosition(pos());
    AppConfig::instance().save();
    event->accept();
}

#ifdef Q_OS_WIN
bool PetWidget::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(eventType);
    MSG *msg = static_cast<MSG *>(message);

    // 左键按下时将客户区视为标题栏，交给 Windows 原生拖动窗口。
    // 这样即使 BongoCat 内嵌的 QWebEngineView(Chromium) 捕获了鼠标，
    // 也能正常拖动。右键保持默认，以触发自定义右键菜单。
    if (msg->message == WM_NCHITTEST &&
        !AppConfig::instance().clickThrough()) {
        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
            *result = HTCAPTION;
            return true;
        }
    }

    return QWidget::nativeEvent(eventType, message, result);
}
#endif

void PetWidget::onSettings()
{
    PreferenceWindow::instance()->showWindow();
}

void PetWidget::onQuit()
{
    AppConfig::instance().setWindowPosition(pos());
    AppConfig::instance().save();
    QApplication::quit();
}

void PetWidget::onPetClicked()
{
    // 点击宠物时的额外反应
}

void PetWidget::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        // 若因"鼠标移入隐藏"不可见, 双击先呼回
        if (!isVisible()) {
            m_hiddenByHover = false;
            m_hoverHideTimer->stop();
            show();
            raise();
            return;
        }
        // 双击托盘图标切换猫咪舞台/时钟
        if (m_stack->currentIndex() == StageComponent) {
            switchToClock();
        } else {
            if (AppConfig::instance().modelKind() == QLatin1String("pet3d")) {
                switchToPet();
            } else {
                switchToBongoCat();
            }
        }
    }
}

// === BongoCat 模型管理 ===

void PetWidget::rebuildBongoCatModelMenu()
{
    if (!m_trayBongoModelMenu || !m_contextBongoModelMenu) {
        return;
    }

    // 清空现有菜单项
    m_trayBongoModelMenu->clear();
    m_contextBongoModelMenu->clear();

    BongoModelManager &mgr = BongoModelManager::instance();
    QString currentId = mgr.currentModelId();

    // 添加所有模型
    QList<BongoModel> models = mgr.models();
    for (const BongoModel &model : models) {
        QString label = model.name;
        if (model.id == currentId) {
            label = QStringLiteral("✓ %1").arg(model.name);
        }

        QAction *trayAction = m_trayBongoModelMenu->addAction(label);
        QAction *ctxAction = m_contextBongoModelMenu->addAction(label);

        QString modelId = model.id;
        connect(trayAction, &QAction::triggered, this, [this, modelId]() {
            switchToBongoCatModel(modelId);
        });
        connect(ctxAction, &QAction::triggered, this, [this, modelId]() {
            switchToBongoCatModel(modelId);
        });

        // 非预设模型(用户自定义)添加删除选项
        if (!model.isPreset) {
            QString deleteLabel = QStringLiteral("    🗑 删除 %1").arg(model.name);
            QAction *trayDel = m_trayBongoModelMenu->addAction(deleteLabel);
            QAction *ctxDel = m_contextBongoModelMenu->addAction(deleteLabel);
            connect(trayDel, &QAction::triggered, this, [this, modelId]() {
                deleteBongoCatModel(modelId);
            });
            connect(ctxDel, &QAction::triggered, this, [this, modelId]() {
                deleteBongoCatModel(modelId);
            });
        }
    }

    m_trayBongoModelMenu->addSeparator();
    m_contextBongoModelMenu->addSeparator();

    // 导入模型
    QAction *trayImport = m_trayBongoModelMenu->addAction(
        QStringLiteral("📁 导入自定义模型..."));
    QAction *ctxImport = m_contextBongoModelMenu->addAction(
        QStringLiteral("📁 导入自定义模型..."));
    connect(trayImport, &QAction::triggered, this, &PetWidget::importBongoCatModel);
    connect(ctxImport, &QAction::triggered, this, &PetWidget::importBongoCatModel);
}

void PetWidget::switchToBongoCatModel(const QString &modelId)
{
    m_stage->showLive2D(modelId);
    m_stack->setCurrentIndex(StageComponent);

    AppConfig::instance().setCurrentComponentIndex(4);
    AppConfig::instance().setModelKind(QStringLiteral("live2d"));
    AppConfig::instance().save();
}

void PetWidget::importBongoCatModel()
{
    QString dirPath = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("选择BongoCat模型文件夹"),
        QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (dirPath.isEmpty()) {
        return;
    }

    // 检查是否有cover.png (支持两种格式)
    bool hasCover = QFile::exists(dirPath + "/cover.png") ||
                    QFile::exists(dirPath + "/resources/cover.png");
    if (!hasCover) {
        QMessageBox::warning(
            this,
            QStringLiteral("导入失败"),
            QStringLiteral("所选文件夹中未找到 cover.png 文件。\n\n"
                           "支持的模型格式:\n\n"
                           "1. BongoCat标准格式 (推荐):\n"
                           "   文件夹/resources/cover.png\n"
                           "   文件夹/resources/background.png\n"
                           "   文件夹/resources/left-keys/*.png\n\n"
                           "2. 简化格式:\n"
                           "   文件夹/cover.png\n"
                           "   文件夹/background.png (可选)\n"
                           "   文件夹/keys/*.png (可选)")
        );
        return;
    }

    // 让用户输入模型名称
    QString defaultName = QFileInfo(dirPath).fileName();
    bool ok = false;
    QString modelName = QInputDialog::getText(
        this,
        QStringLiteral("模型名称"),
        QStringLiteral("请输入模型名称:"),
        QLineEdit::Normal,
        defaultName,
        &ok
    );

    if (!ok || modelName.trimmed().isEmpty()) {
        return;
    }

    QString importError;
    if (BongoModelManager::instance().importModel(dirPath, modelName.trimmed(), &importError)) {
        QMessageBox::information(
            this,
            QStringLiteral("导入成功"),
            QStringLiteral("模型 '%1' 导入成功, 已自动切换到该模型!").arg(modelName)
        );
        // 导入成功后立即切换到 BongoCat 组件展示新模型
        // (模型管理器内部已调用 setCurrentModel, 菜单会经 modelChanged 信号自动刷新)
        switchToBongoCat();
    } else {
        QMessageBox::critical(
            this,
            QStringLiteral("导入失败"),
            importError.isEmpty()
                ? QStringLiteral("导入模型时发生错误, 请检查文件夹权限。")
                : importError
        );
    }
}

void PetWidget::deleteBongoCatModel(const QString &modelId)
{
    BongoModel model = BongoModelManager::instance().getModelById(modelId);
    if (!model.isValid()) return;

    auto ret = QMessageBox::question(
        this,
        QStringLiteral("删除模型"),
        QStringLiteral("确定要删除模型 '%1' 吗?\n此操作不可撤销。").arg(model.name),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (ret == QMessageBox::Yes) {
        BongoModelManager::instance().deleteModel(modelId);
        rebuildBongoCatModelMenu();
    }
}
