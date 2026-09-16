#include "catpage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QSpinBox>
#include <QSlider>
#include <QtGlobal>

#include "appconfig.h"
#include "preference/widgets/prolist.h"
#include "preference/widgets/prolistitem.h"
#include "preference/widgets/switchbutton.h"

namespace {

// 原版 InputNumber 风格的定宽数字框
QSpinBox *makeSpinBox(int min, int max, const QString &suffix = QString())
{
    auto *spin = new QSpinBox;
    spin->setRange(min, max);
    spin->setFixedWidth(84);
    if (!suffix.isEmpty()) {
        spin->setSuffix(suffix);
    }
    spin->setKeyboardTracking(false);  // 松手/回车后才提交, 避免拖动窗口频繁缩放
    return spin;
}

SwitchButton *makeSwitch(bool checked)
{
    auto *sw = new SwitchButton;
    sw->setChecked(checked);
    return sw;
}

// 控件右侧的 "开关 | 延迟s" 组合 (原版 hideOnHover 行)
QWidget *makeSwitchWithAddon(SwitchButton *sw, QSpinBox *spin)
{
    auto *box = new QWidget;
    auto *lay = new QHBoxLayout(box);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(10);
    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::VLine);
    sep->setFixedWidth(1);
    lay->addWidget(sw);
    lay->addWidget(sep);
    lay->addWidget(spin);
    return box;
}

}  // namespace

CatPage::CatPage(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
}

void CatPage::buildUi()
{
    auto &cfg = AppConfig::instance();

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(16);

    // ============ 模型设置 ============
    auto *modelGroup = new ProList(QStringLiteral("模型设置"));

    auto *swMirror = makeSwitch(cfg.mirror());
    connect(swMirror, &SwitchButton::toggled, this, [](bool on) {
        AppConfig::instance().setMirror(on);
    });
    modelGroup->addItem(new ProListItem(
        QStringLiteral("镜像模式"),
        QStringLiteral("启用后，模型将水平镜像翻转。"),
        swMirror));

    auto *swMouseMirror = makeSwitch(cfg.mouseMirror());
    connect(swMouseMirror, &SwitchButton::toggled, this, [](bool on) {
        AppConfig::instance().setMouseMirror(on);
    });
    modelGroup->addItem(new ProListItem(
        QStringLiteral("鼠标镜像"),
        QStringLiteral("启用后，鼠标将镜像跟随手部移动。"),
        swMouseMirror));

    auto *swIgnoreMouse = makeSwitch(cfg.ignoreMouse());
    connect(swIgnoreMouse, &SwitchButton::toggled, this, [](bool on) {
        AppConfig::instance().setIgnoreMouse(on);
    });
    modelGroup->addItem(new ProListItem(
        QStringLiteral("忽略鼠标事件"),
        QStringLiteral("启用后，模型将不再响应鼠标事件，适用于键盘模式或手柄模式。"),
        swIgnoreMouse));

    auto *swSound = makeSwitch(cfg.motionSound());
    connect(swSound, &SwitchButton::toggled, this, [](bool on) {
        AppConfig::instance().setMotionSound(on);
    });
    modelGroup->addItem(new ProListItem(
        QStringLiteral("动作音效"),
        QStringLiteral("启用后，模型执行动作时会播放对应音效（如果存在）。"),
        swSound));

    auto *swBehavior = makeSwitch(cfg.behavior());
    connect(swBehavior, &SwitchButton::toggled, this, [](bool on) {
        AppConfig::instance().setBehavior(on);
    });
    modelGroup->addItem(new ProListItem(
        QStringLiteral("动作与表情"),
        QStringLiteral("启用后，可以配置和触发模型的动作与表情。"),
        swBehavior));

    auto *spinRelease = makeSpinBox(0, 60, QStringLiteral(" s"));
    spinRelease->setValue(cfg.autoReleaseDelaySec());
    connect(spinRelease, qOverload<int>(&QSpinBox::valueChanged), this, [](int sec) {
        AppConfig::instance().setAutoReleaseDelaySec(sec);
    });
    modelGroup->addItem(new ProListItem(
        QStringLiteral("按键自动释放延迟"),
        QStringLiteral("由于 Windows 下部分系统级按键无法捕获释放事件，超时后将自动视为已释放。"),
        spinRelease));

    auto *spinFps = makeSpinBox(0, 240, QStringLiteral(" FPS"));
    spinFps->setValue(cfg.maxFps());
    spinFps->setSpecialValueText(QStringLiteral("不限"));  // 值为 0 时显示
    connect(spinFps, qOverload<int>(&QSpinBox::valueChanged), this, [](int fps) {
        AppConfig::instance().setMaxFps(fps);
    });
    modelGroup->addItem(new ProListItem(
        QStringLiteral("最大帧率"),
        QStringLiteral("限制模型渲染的最大帧率，降低帧率可以减少 CPU/GPU 占用。"),
        spinFps));

    root->addWidget(modelGroup);

    // ============ 窗口设置 ============
    auto *winGroup = new ProList(QStringLiteral("窗口设置"));

    auto *swPassThrough = makeSwitch(cfg.passThrough());
    connect(swPassThrough, &SwitchButton::toggled, this, [](bool on) {
        AppConfig::instance().setPassThrough(on);
    });
    winGroup->addItem(new ProListItem(
        QStringLiteral("窗口穿透"),
        QStringLiteral("启用后，窗口不影响对其他应用程序的操作。"),
        swPassThrough));

    auto *swOnTop = makeSwitch(cfg.alwaysOnTop());
    connect(swOnTop, &SwitchButton::toggled, this, [](bool on) {
        AppConfig::instance().setAlwaysOnTop(on);
    });
    winGroup->addItem(new ProListItem(
        QStringLiteral("窗口置顶"),
        QStringLiteral("启用后，窗口始终显示在其他应用程序上方。"),
        swOnTop));

    auto *swHideHover = makeSwitch(cfg.hideOnHover());
    auto *spinHideDelay = makeSpinBox(0, 60, QStringLiteral(" s"));
    spinHideDelay->setValue(cfg.hideOnHoverDelay());
    spinHideDelay->setEnabled(cfg.hideOnHover());
    connect(swHideHover, &SwitchButton::toggled, this, [spinHideDelay](bool on) {
        AppConfig::instance().setHideOnHover(on);
        spinHideDelay->setEnabled(on);
    });
    connect(spinHideDelay, qOverload<int>(&QSpinBox::valueChanged), this, [](int sec) {
        AppConfig::instance().setHideOnHoverDelay(sec);
    });
    winGroup->addItem(new ProListItem(
        QStringLiteral("鼠标移入隐藏"),
        QStringLiteral("启用后，鼠标悬停窗口时自动隐藏。可设置延迟时间，0 表示立即隐藏。"),
        makeSwitchWithAddon(swHideHover, spinHideDelay)));

    auto *swKeepInScreen = makeSwitch(cfg.keepInScreen());
    connect(swKeepInScreen, &SwitchButton::toggled, this, [](bool on) {
        AppConfig::instance().setKeepInScreen(on);
    });
    winGroup->addItem(new ProListItem(
        QStringLiteral("保持在屏幕内"),
        QStringLiteral("启用后，窗口会自动调整位置，防止超出屏幕边界。"),
        swKeepInScreen));

    auto *spinScale = makeSpinBox(1, 500, QStringLiteral(" %"));
    spinScale->setValue(cfg.windowScale());
    connect(spinScale, qOverload<int>(&QSpinBox::valueChanged), this, [](int percent) {
        AppConfig::instance().setWindowScale(percent);
    });
    winGroup->addItem(new ProListItem(
        QStringLiteral("窗口尺寸"),
        QStringLiteral("将鼠标移至窗口边缘，或按住 Shift 并右键拖动，也可以调整窗口大小。"),
        spinScale));

    auto *spinRadius = makeSpinBox(0, 100, QStringLiteral(" %"));
    spinRadius->setValue(cfg.windowRadius());
    connect(spinRadius, qOverload<int>(&QSpinBox::valueChanged), this, [](int percent) {
        AppConfig::instance().setWindowRadius(percent);
    });
    winGroup->addItem(new ProListItem(
        QStringLiteral("窗口圆角"),
        QString(),
        spinRadius));

    // 不透明度: 原版为垂直条目, 滑块整行铺开, tooltip 显示百分比
    auto *sliderOpacity = new QSlider(Qt::Horizontal);
    sliderOpacity->setRange(10, 100);
    sliderOpacity->setValue(cfg.windowOpacity());
    sliderOpacity->setToolTip(QStringLiteral("%1%").arg(cfg.windowOpacity()));
    connect(sliderOpacity, &QSlider::valueChanged, this, [sliderOpacity](int percent) {
        sliderOpacity->setToolTip(QStringLiteral("%1%").arg(percent));
        AppConfig::instance().setWindowOpacity(percent);
    });
    winGroup->addItem(new ProListItem(
        QStringLiteral("不透明度"),
        QString(),
        sliderOpacity,
        /*vertical=*/true));

    root->addWidget(winGroup);
    root->addStretch(1);

    // 外部变更 (全局快捷键等) 回刷控件, 避免与配置脱节
    connect(&AppConfig::instance(), &AppConfig::catModelChanged, this,
            [this, swMirror, swMouseMirror, swIgnoreMouse, swSound, swBehavior,
             spinRelease, spinFps]() {
        // 外部 (全局快捷键等) 变更时回刷控件; setChecked/setValue 对相同值不发信号
        if (!isVisible()) return;
        m_syncing = true;
        swMirror->setChecked(AppConfig::instance().mirror());
        swMouseMirror->setChecked(AppConfig::instance().mouseMirror());
        swIgnoreMouse->setChecked(AppConfig::instance().ignoreMouse());
        swSound->setChecked(AppConfig::instance().motionSound());
        swBehavior->setChecked(AppConfig::instance().behavior());
        spinRelease->setValue(AppConfig::instance().autoReleaseDelaySec());
        spinFps->setValue(AppConfig::instance().maxFps());
        m_syncing = false;
    });
    connect(&AppConfig::instance(), &AppConfig::catWindowChanged, this,
            [this, swPassThrough, swOnTop, swHideHover, spinHideDelay,
             swKeepInScreen, spinScale, spinRadius, sliderOpacity]() {
        if (!isVisible()) return;
        m_syncing = true;
        swPassThrough->setChecked(AppConfig::instance().passThrough());
        swOnTop->setChecked(AppConfig::instance().alwaysOnTop());
        swHideHover->setChecked(AppConfig::instance().hideOnHover());
        spinHideDelay->setEnabled(AppConfig::instance().hideOnHover());
        spinHideDelay->setValue(AppConfig::instance().hideOnHoverDelay());
        swKeepInScreen->setChecked(AppConfig::instance().keepInScreen());
        spinScale->setValue(AppConfig::instance().windowScale());
        spinRadius->setValue(AppConfig::instance().windowRadius());
        sliderOpacity->setValue(AppConfig::instance().windowOpacity());
        m_syncing = false;
    });
}

void CatPage::loadFromConfig()
{
    // 控件初值已在 buildUi() 中直接从配置读取, 暂无额外逻辑
}
