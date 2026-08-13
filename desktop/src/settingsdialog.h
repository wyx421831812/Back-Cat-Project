#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QSlider>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include "appconfig.h"

/**
 * @brief 设置对话框
 *
 * 提供 GUI 界面让用户自定义:
 * - 主题配色 (预设)
 * - 自定义颜色
 * - 背景主题选项
 * - 窗口透明度
 * - 置顶/穿透/开机启动
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private:
    void setupUI();
    void loadConfig();
    void saveConfig();

private:
    // 主题选择
    QComboBox *m_themeCombo;
    QLabel *m_themePreview;

    // 自定义颜色
    QPushButton *m_primaryColorBtn;
    QPushButton *m_secondaryColorBtn;
    QPushButton *m_accentColorBtn;

    // 背景主题
    QComboBox *m_bgThemeCombo;

    // 3D模型选择
    QComboBox *m_modelCombo;

    // 窗口设置
    QSlider *m_opacitySlider;
    QLabel *m_opacityValue;
    QCheckBox *m_alwaysOnTopCheck;
    QCheckBox *m_clickThroughCheck;
    QCheckBox *m_startWithSystemCheck;

    // 按钮
    QPushButton *m_okBtn;
    QPushButton *m_cancelBtn;

    // 当前颜色
    QColor m_primaryColor;
    QColor m_secondaryColor;
    QColor m_accentColor;

private slots:
    void onThemeChanged(int index);
    void onPrimaryColor();
    void onSecondaryColor();
    void onAccentColor();
    void onOpacityChanged(int value);
    void onOk();
    void onCancel();
};

#endif // SETTINGSDIALOG_H
