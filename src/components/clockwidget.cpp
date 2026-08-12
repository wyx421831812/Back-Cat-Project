#include "clockwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QDateTime>
#include "appconfig.h"

ClockWidget::ClockWidget(QWidget *parent)
    : ComponentBase(parent)
{
    setupTransparentBackground();

    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    // 时间标签
    m_timeLabel = new QLabel();
    m_timeLabel->setObjectName("clockTimeLabel");
    m_timeLabel->setAlignment(Qt::AlignCenter);
    QFont timeFont("Microsoft YaHei", 24, QFont::Bold);
    m_timeLabel->setFont(timeFont);

    // 日期标签
    m_dateLabel = new QLabel();
    m_dateLabel->setObjectName("clockDateLabel");
    m_dateLabel->setAlignment(Qt::AlignCenter);
    QFont dateFont("Microsoft YaHei", 9);
    m_dateLabel->setFont(dateFont);

    layout->addStretch();
    layout->addWidget(m_timeLabel);
    layout->addWidget(m_dateLabel);
    layout->addStretch();

    // 定时器每秒更新
    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &ClockWidget::updateTime);
    m_timer->start();

    updateTime();
}

void ClockWidget::updateTime()
{
    QDateTime now = QDateTime::currentDateTime();
    m_timeLabel->setText(now.toString("HH:mm"));
    m_dateLabel->setText(now.toString("MM月dd日 dddd"));
}

void ClockWidget::refresh()
{
    updateTime();
}

void ClockWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 绘制圆角背景
    auto &cfg = AppConfig::instance();
    QColor bg = cfg.secondaryColor();
    bg.setAlpha(180);
    p.setBrush(bg);
    p.setPen(Qt::NoPen);

    QPainterPath path;
    path.addRoundedRect(rect().adjusted(4, 4, -4, -4), 15, 15);
    p.fillPath(path, bg);
}
