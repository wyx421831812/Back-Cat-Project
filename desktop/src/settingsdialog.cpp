#include "settingsdialog.h"
#include <QColorDialog>
#include <QPainter>
#include <QPixmap>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    loadConfig();
}

void SettingsDialog::setupUI()
{
    setWindowTitle(QStringLiteral("BackPet 设置"));
    setFixedSize(440, 640);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // === 主题配色组 ===
    auto *themeGroup = new QGroupBox(QStringLiteral("主题配色"), this);
    auto *themeLayout = new QVBoxLayout(themeGroup);

    // 预设主题下拉
    auto *themeForm = new QFormLayout();
    m_themeCombo = new QComboBox();
    for (const auto &preset : AppConfig::instance().themePresets()) {
        m_themeCombo->addItem(preset.name);
    }
    themeForm->addRow(QStringLiteral("预设主题:"), m_themeCombo);

    // 主题预览
    m_themePreview = new QLabel();
    m_themePreview->setFixedHeight(40);
    m_themePreview->setAutoFillBackground(true);
    themeForm->addRow(QStringLiteral("预览:"), m_themePreview);

    themeLayout->addLayout(themeForm);

    // 自定义颜色按钮
    auto *colorLayout = new QHBoxLayout();
    m_primaryColorBtn = new QPushButton(QStringLiteral("主色"));
    m_secondaryColorBtn = new QPushButton(QStringLiteral("副色"));
    m_accentColorBtn = new QPushButton(QStringLiteral("点缀色"));
    colorLayout->addWidget(m_primaryColorBtn);
    colorLayout->addWidget(m_secondaryColorBtn);
    colorLayout->addWidget(m_accentColorBtn);
    themeLayout->addLayout(colorLayout);

    mainLayout->addWidget(themeGroup);

    // === 自定义背景主题组 ===
    auto *bgGroup = new QGroupBox(QStringLiteral("自定义背景主题"), this);
    auto *bgLayout = new QVBoxLayout(bgGroup);

    m_bgThemeCombo = new QComboBox();
    m_bgThemeCombo->addItem(QStringLiteral("透明背景"));
    m_bgThemeCombo->addItem(QStringLiteral("纯色背景"));
    m_bgThemeCombo->addItem(QStringLiteral("渐变背景"));
    m_bgThemeCombo->addItem(QStringLiteral("自定义图片背景"));
    m_bgThemeCombo->addItem(QStringLiteral("毛玻璃效果"));
    bgLayout->addWidget(m_bgThemeCombo);

    mainLayout->addWidget(bgGroup);

    // === 3D模型选择组 ===
    auto *modelGroup = new QGroupBox(QStringLiteral("3D 宠物模型"), this);
    auto *modelLayout = new QVBoxLayout(modelGroup);

    auto *modelForm = new QFormLayout();
    m_modelCombo = new QComboBox();
    m_modelCombo->addItem(QStringLiteral("🐱 猫咪 (默认)"));
    m_modelCombo->addItem(QStringLiteral("🐻 小熊"));
    m_modelCombo->addItem(QStringLiteral("🐰 兔子"));
    m_modelCombo->addItem(QStringLiteral("🦢 仙鸟"));
    m_modelCombo->addItem(QStringLiteral("👻 幻灵"));
    modelForm->addRow(QStringLiteral("模型类型:"), m_modelCombo);
    modelLayout->addLayout(modelForm);

    auto *modelHint = new QLabel(QStringLiteral("提示: 可在托盘菜单或右键菜单中快速切换"));
    modelHint->setStyleSheet("color: gray; font-size: 11px;");
    modelLayout->addWidget(modelHint);

    mainLayout->addWidget(modelGroup);

    // === 窗口设置组 ===
    auto *winGroup = new QGroupBox(QStringLiteral("窗口设置"), this);
    auto *winLayout = new QVBoxLayout(winGroup);

    // 透明度
    auto *opacityLayout = new QHBoxLayout();
    opacityLayout->addWidget(new QLabel(QStringLiteral("透明度:")));
    m_opacitySlider = new QSlider(Qt::Horizontal);
    m_opacitySlider->setRange(20, 100);
    m_opacitySlider->setValue(100);
    m_opacityValue = new QLabel("100%");
    m_opacityValue->setFixedWidth(50);
    opacityLayout->addWidget(m_opacitySlider);
    opacityLayout->addWidget(m_opacityValue);
    winLayout->addLayout(opacityLayout);

    // 复选框
    m_alwaysOnTopCheck = new QCheckBox(QStringLiteral("始终置顶显示"));
    m_clickThroughCheck = new QCheckBox(QStringLiteral("启用点击穿透"));
    m_startWithSystemCheck = new QCheckBox(QStringLiteral("开机自启动"));
    winLayout->addWidget(m_alwaysOnTopCheck);
    winLayout->addWidget(m_clickThroughCheck);
    winLayout->addWidget(m_startWithSystemCheck);

    mainLayout->addWidget(winGroup);

    // === 按钮 ===
    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    m_okBtn = new QPushButton(QStringLiteral("确定"));
    m_okBtn->setObjectName("primaryBtn");
    m_cancelBtn = new QPushButton(QStringLiteral("取消"));
    m_cancelBtn->setObjectName("secondaryBtn");
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_okBtn);
    mainLayout->addLayout(btnLayout);

    // 信号连接
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onThemeChanged);
    connect(m_primaryColorBtn, &QPushButton::clicked, this, &SettingsDialog::onPrimaryColor);
    connect(m_secondaryColorBtn, &QPushButton::clicked, this, &SettingsDialog::onSecondaryColor);
    connect(m_accentColorBtn, &QPushButton::clicked, this, &SettingsDialog::onAccentColor);
    connect(m_opacitySlider, &QSlider::valueChanged, this, &SettingsDialog::onOpacityChanged);
    connect(m_okBtn, &QPushButton::clicked, this, &SettingsDialog::onOk);
    connect(m_cancelBtn, &QPushButton::clicked, this, &SettingsDialog::onCancel);
}

void SettingsDialog::loadConfig()
{
    auto &cfg = AppConfig::instance();

    // 主题
    int idx = m_themeCombo->findText(cfg.themeName());
    if (idx >= 0) m_themeCombo->setCurrentIndex(idx);

    // 颜色
    m_primaryColor = cfg.primaryColor();
    m_secondaryColor = cfg.secondaryColor();
    m_accentColor = cfg.accentColor();

    // 更新按钮颜色
    QString btnStyle = "QPushButton { background: %1; color: white; border-radius: 8px; padding: 8px; }";
    m_primaryColorBtn->setStyleSheet(btnStyle.arg(m_primaryColor.name()));
    m_secondaryColorBtn->setStyleSheet(btnStyle.arg(m_secondaryColor.name()));
    m_accentColorBtn->setStyleSheet(btnStyle.arg(m_accentColor.name()));

    // 3D模型
    int modelType = cfg.petModelType();
    if (modelType >= 0 && modelType < m_modelCombo->count()) {
        m_modelCombo->setCurrentIndex(modelType);
    }

    // 透明度
    m_opacitySlider->setValue(cfg.windowOpacity());
    m_opacityValue->setText(QString::number(cfg.windowOpacity()) + "%");

    // 复选框
    m_alwaysOnTopCheck->setChecked(cfg.alwaysOnTop());
    m_clickThroughCheck->setChecked(cfg.clickThrough());
    m_startWithSystemCheck->setChecked(cfg.startWithSystem());

    // 更新预览
    onThemeChanged(m_themeCombo->currentIndex());
}

void SettingsDialog::saveConfig()
{
    auto &cfg = AppConfig::instance();

    cfg.setThemeName(m_themeCombo->currentText());
    cfg.setPrimaryColor(m_primaryColor);
    cfg.setSecondaryColor(m_secondaryColor);
    cfg.setAccentColor(m_accentColor);
    cfg.setWindowOpacity(m_opacitySlider->value());
    cfg.setAlwaysOnTop(m_alwaysOnTopCheck->isChecked());
    cfg.setClickThrough(m_clickThroughCheck->isChecked());
    cfg.setStartWithSystem(m_startWithSystemCheck->isChecked());
    cfg.setPetModelType(m_modelCombo->currentIndex());
    cfg.save();
}

void SettingsDialog::onThemeChanged(int index)
{
    if (index < 0) return;

    auto presets = AppConfig::instance().themePresets();
    if (index >= presets.size()) return;

    const auto &preset = presets[index];
    m_primaryColor = preset.primary;
    m_secondaryColor = preset.secondary;
    m_accentColor = preset.accent;

    // 更新按钮颜色
    QString btnStyle = "QPushButton { background: %1; color: white; border-radius: 8px; padding: 8px; }";
    m_primaryColorBtn->setStyleSheet(btnStyle.arg(m_primaryColor.name()));
    m_secondaryColorBtn->setStyleSheet(btnStyle.arg(m_secondaryColor.name()));
    m_accentColorBtn->setStyleSheet(btnStyle.arg(m_accentColor.name()));

    // 更新预览
    QPixmap preview(200, 36);
    preview.fill(Qt::transparent);
    QPainter painter(&preview);
    QLinearGradient grad(0, 0, 200, 36);
    grad.setColorAt(0, m_primaryColor);
    grad.setColorAt(1, m_secondaryColor);
    painter.fillRect(0, 0, 200, 36, grad);
    m_themePreview->setPixmap(preview);
}

void SettingsDialog::onPrimaryColor()
{
    QColor c = QColorDialog::getColor(m_primaryColor, this, QStringLiteral("选择主色"));
    if (c.isValid()) {
        m_primaryColor = c;
        m_primaryColorBtn->setStyleSheet(
            QString("QPushButton { background: %1; color: white; border-radius: 8px; padding: 8px; }")
                .arg(c.name()));
    }
}

void SettingsDialog::onSecondaryColor()
{
    QColor c = QColorDialog::getColor(m_secondaryColor, this, QStringLiteral("选择副色"));
    if (c.isValid()) {
        m_secondaryColor = c;
        m_secondaryColorBtn->setStyleSheet(
            QString("QPushButton { background: %1; color: white; border-radius: 8px; padding: 8px; }")
                .arg(c.name()));
    }
}

void SettingsDialog::onAccentColor()
{
    QColor c = QColorDialog::getColor(m_accentColor, this, QStringLiteral("选择点缀色"));
    if (c.isValid()) {
        m_accentColor = c;
        m_accentColorBtn->setStyleSheet(
            QString("QPushButton { background: %1; color: white; border-radius: 8px; padding: 8px; }")
                .arg(c.name()));
    }
}

void SettingsDialog::onOpacityChanged(int value)
{
    m_opacityValue->setText(QString::number(value) + "%");
}

void SettingsDialog::onOk()
{
    saveConfig();
    accept();
}

void SettingsDialog::onCancel()
{
    reject();
}
