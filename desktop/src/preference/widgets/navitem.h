#pragma once

#include <QAbstractButton>
#include <QPixmap>

// 偏好窗侧栏导航项: 80x80, 32px 图标 + 文字,
// 选中态亮紫/暗黄撞色。配合 QButtonGroup 互斥。
class NavItem : public QAbstractButton
{
    Q_OBJECT

public:
    explicit NavItem(const QString &iconName, const QString &text,
                     QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    QString iconName() const { return m_iconName; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void checkStateSet() override;

private:
    void refreshPixmap();

    QString m_iconName;
    QPixmap m_pixmap;
    bool m_hover = false;
};
