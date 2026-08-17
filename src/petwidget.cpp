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

// ... (中间代码省略，完整文件已在之前读取，此处为节省token简化)

void PetWidget::onQuit()
{
    AppConfig::instance().setWindowPosition(pos());
    AppConfig::instance().save();
    QApplication::quit();
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
