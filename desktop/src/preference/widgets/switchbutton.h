#pragma once

#include <QAbstractButton>
#include <QPropertyAnimation>

// 黄紫渐变开关 (自绘, 带滑块动画)。
// 直接当 QCheckBox 用: isChecked()/toggled(bool)。
class SwitchButton : public QAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(qreal position READ position WRITE setPosition)

public:
    explicit SwitchButton(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    qreal position() const { return m_position; }
    void setPosition(qreal pos);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void animateTo(bool checked);

    qreal m_position = 0.0; // 0=关 1=开
    bool m_hover = false;
    QPropertyAnimation *m_animation;
};
