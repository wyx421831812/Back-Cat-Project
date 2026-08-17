#ifndef CLOCKWIDGET_H
#define CLOCKWIDGET_H

#include "componentbase.h"
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

/**
 * @brief 时钟组件
 *
 * 显示当前时间和日期
 * 悬浮在桌面上的小组件
 */
class ClockWidget : public ComponentBase
{
    Q_OBJECT

public:
    explicit ClockWidget(QWidget *parent = nullptr);

    QString componentName() const override { return QStringLiteral("时钟"); }
    void refresh() override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QLabel *m_timeLabel;
    QLabel *m_dateLabel;
    QTimer *m_timer;

private slots:
    void updateTime();
};

#endif // CLOCKWIDGET_H
