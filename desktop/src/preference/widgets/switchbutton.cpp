#include "switchbutton.h"

#include <QPainter>
#include <QPaintEvent>

#include "preference/thememanager.h"

namespace {
constexpr int kTrackW = 44;
constexpr int kTrackH = 24;
constexpr int kKnob = 18;
constexpr int kMargin = 3;
}

SwitchButton::SwitchButton(QWidget *parent)
    : QAbstractButton(parent)
    , m_animation(new QPropertyAnimation(this, "position", this))
{
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_animation->setDuration(160);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);

    connect(this, &QAbstractButton::toggled, this, &SwitchButton::animateTo);
    connect(ThemeManager::instance(), &ThemeManager::themeChanged, this,
            [this]() { update(); });
}

QSize SwitchButton::sizeHint() const
{
    return QSize(kTrackW, kTrackH);
}

QSize SwitchButton::minimumSizeHint() const
{
    return sizeHint();
}

void SwitchButton::setPosition(qreal pos)
{
    m_position = qBound(0.0, pos, 1.0);
    update();
}

void SwitchButton::animateTo(bool checked)
{
    m_animation->stop();
    m_animation->setStartValue(m_position);
    m_animation->setEndValue(checked ? 1.0 : 0.0);
    m_animation->start();
}

void SwitchButton::enterEvent(QEnterEvent *event)
{
    m_hover = true;
    update();
    QAbstractButton::enterEvent(event);
}

void SwitchButton::leaveEvent(QEvent *event)
{
    m_hover = false;
    update();
    QAbstractButton::leaveEvent(event);
}

void SwitchButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const qreal x = (width() - kTrackW) / 2.0;
    const qreal y = (height() - kTrackH) / 2.0;
    const QRectF trackRect(x, y, kTrackW, kTrackH);

    // 轨道
    QBrush trackBrush(ThemeManager::instance()->switchOffTrack());
    if (isChecked()) {
        QLinearGradient grad(trackRect.topLeft(), trackRect.bottomRight());
        grad.setColorAt(0.0, QColor("#FBBF24"));
        grad.setColorAt(1.0, QColor("#8B5CF6"));
        trackBrush = QBrush(grad);
    }
    p.setPen(Qt::NoPen);
    p.setBrush(trackBrush);
    p.drawRoundedRect(trackRect, kTrackH / 2.0, kTrackH / 2.0);

    // 滑块
    const qreal travel = kTrackW - kKnob - kMargin * 2;
    const qreal knobX = x + kMargin + travel * m_position;
    const qreal knobY = y + kMargin;
    QRectF knobRect(knobX, knobY, kKnob, kKnob);

    p.setBrush(QColor("#FFFFFF"));
    if (!isEnabled()) {
        p.setOpacity(0.55);
    } else if (m_hover) {
        p.setPen(QPen(QColor(255, 255, 255, 90), 2));
    }
    p.drawEllipse(knobRect);
}
