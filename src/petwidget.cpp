#include "petwidget.h"
#include "settingsdialog.h"
#include "components/clockwidget.h"
#include "components/quotewidget.h"
#include "components/todowidget.h"
#include "components/bongocatwidget.h"

#include <QApplication>
#include <QScreen>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>

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
    , m_trayIcon(nullptr)
    , m_trayMenu(nullptr)
    , m_contextMenu(nullptr)
{
    setupUI();
    setupTrayIcon();
    setupContextMenu();
    applyConfig();
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

    // 堆叠窗口 (切换不同组件)
    m_stack = new QStackedWidget(this);
    m_stack->setGeometry(0, 0, 200, 250);

    // 创建宠物画布
    m_petCanvas = new PetCanvas();
    m_petCanvas->setGeometry(0, 0, 200, 250);

    // 创建功能组件
    m_clockWidget = new ClockWidget();
    m_quoteWidget = new QuoteWidget();
    m_todoWidget = new TodoWidget();

    // 创建 Bongo Cat 组件
    m_bongoCatWidget = new BongoCatWidget();

    // 添加到堆叠
    m_stack->addWidget(m_petCanvas);     // index 0: 宠物
    m_stack->addWidget(m_clockWidget);   // index 1: 时钟
    m_stack->addWidget(m_quoteWidget);   // index 2: 寄语
    m_stack->addWidget(m_todoWidget);    // index 3: 待办
    m_stack->addWidget(m_bongoCatWidget); // index 4: Bongo Cat

    m_stack->setCurrentIndex(0);

    // 信号连接
    connect(m_petCanvas, &PetCanvas::clicked, this, &PetWidget::onPetClicked);

    // 组件切换时管理 Bongo Cat 键盘钩子
    connect(m_stack, &QStackedWidget::currentChanged, this, [this](int index) {
        if (index == BongoCatComponent) {
            m_bongoCatWidget->onShow();
        } else {
            m_bongoCatWidget->onHide();
        }
    });
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

    m_trayMenu->addSeparator();
    m_trayMenu->addAction(QStringLiteral("⚙️ 设置"), this, &PetWidget::onSettings);
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

    m_contextMenu->addSeparator();
    m_contextMenu->addAction(QStringLiteral("⚙️ 设置"), this, &PetWidget::onSettings);
    m_contextMenu->addAction(QStringLiteral("❌ 退出"), this, &PetWidget::onQuit);
}

void PetWidget::applyConfig()
{
    auto &cfg = AppConfig::instance();

    // 颜色
    m_petCanvas->setColors(cfg.primaryColor(), cfg.secondaryColor(), cfg.accentColor());

    // 3D模型类型
    int modelType = cfg.petModelType();
    if (modelType >= 0 && modelType <= 4) {
        m_petCanvas->setModelType(static_cast<PetModelType>(modelType));
    }

    // 窗口位置
    QPoint pos = cfg.windowPosition();
    if (pos.x() >= 0 && pos.y() >= 0) {
        move(pos);
    } else {
        // 默认右下角
        QScreen *screen = QGuiApplication::primaryScreen();
        QRect geo = screen->availableGeometry();
        move(geo.right() - 220, geo.bottom() - 300);
    }

    // 窗口大小
    QSize sz = cfg.windowSize();
    if (sz.width() > 0 && sz.height() > 0) {
        setFixedSize(sz);
        m_stack->setGeometry(0, 0, sz.width(), sz.height());
        m_petCanvas->setGeometry(0, 0, sz.width(), sz.height());
    }

    // 组件选择
    m_stack->setCurrentIndex(cfg.currentComponentIndex());

    // 其他属性
    updateAlwaysOnTop();
    updateClickThrough();
    updateOpacity();
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
    if (AppConfig::instance().alwaysOnTop()) {
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    } else {
        setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    }
    show();
}

void PetWidget::updateOpacity()
{
    // 透明度 (0-100 -> 0.0-1.0)
    qreal op = AppConfig::instance().windowOpacity() / 100.0;
    setWindowOpacity(op);
}

// === 组件切换 ===

void PetWidget::switchToPet()
{
    m_stack->setCurrentIndex(PetComponent);
    AppConfig::instance().setCurrentComponentIndex(PetComponent);
    AppConfig::instance().save();
}

void PetWidget::switchToClock()
{
    m_stack->setCurrentIndex(ClockComponent);
    AppConfig::instance().setCurrentComponentIndex(ClockComponent);
    AppConfig::instance().save();
}

void PetWidget::switchToQuote()
{
    m_stack->setCurrentIndex(QuoteComponent);
    AppConfig::instance().setCurrentComponentIndex(QuoteComponent);
    AppConfig::instance().save();
}

void PetWidget::switchToTodo()
{
    m_stack->setCurrentIndex(TodoComponent);
    AppConfig::instance().setCurrentComponentIndex(TodoComponent);
    AppConfig::instance().save();
}

void PetWidget::switchToBongoCat()
{
    m_stack->setCurrentIndex(BongoCatComponent);
    AppConfig::instance().setCurrentComponentIndex(BongoCatComponent);
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
    switchToPet();
    m_petCanvas->setModelType(PetModelType::CatLike);
    AppConfig::instance().setPetModelType(0);
    AppConfig::instance().save();
}

void PetWidget::setModelBear()
{
    switchToPet();
    m_petCanvas->setModelType(PetModelType::BearLike);
    AppConfig::instance().setPetModelType(1);
    AppConfig::instance().save();
}

void PetWidget::setModelBunny()
{
    switchToPet();
    m_petCanvas->setModelType(PetModelType::BunnyLike);
    AppConfig::instance().setPetModelType(2);
    AppConfig::instance().save();
}

void PetWidget::setModelFairyBird()
{
    switchToPet();
    m_petCanvas->setModelType(PetModelType::FairyBirdLike);
    AppConfig::instance().setPetModelType(3);
    AppConfig::instance().save();
}

void PetWidget::setModelSpirit()
{
    switchToPet();
    m_petCanvas->setModelType(PetModelType::SpiritLike);
    AppConfig::instance().setPetModelType(4);
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
    AppConfig::instance().setWindowPosition(pos());
    AppConfig::instance().save();
    QWidget::mouseReleaseEvent(event);
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
    // 处理 Windows 原生消息
    // 可以在这里处理 WM_NCHITTEST 实现自定义点击穿透区域
    return QWidget::nativeEvent(eventType, message, result);
}
#endif

void PetWidget::onSettings()
{
    SettingsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        applyConfig();
    }
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
        // 双击托盘图标切换宠物/时钟
        if (m_stack->currentIndex() == PetComponent) {
            switchToClock();
        } else {
            switchToPet();
        }
    }
}
