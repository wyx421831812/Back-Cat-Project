#include "navitem.h"

#include <QFont>
#include <QPainter>
#include <QPaintEvent>

#include "preference/iconkit.h"
#include "preference/thememanager.h"

namespace {
constexpr int kSide = 80;
constexpr int kIcon = 30;
constexpr int kRadius = 10;
}

NavItem::NavItem(const QString &iconName, const QString &text, QWidget *parent)
    : QAbstractButton(parent)
    , m_iconName(iconName)
{
    setText(text);
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    connect(ThemeManager::instance(), &ThemeManager::themeChanged, this,
            [this]() { refreshPixmap(); });
    refreshPixmap();
}

QSize NavItem::sizeHint() const
{
    return QSize(kSide, kSide);
}

QSize NavItem::minimumSizeHint() const
{
    return sizeHint();
}

void NavItem::refreshPixmap()
{
    const auto *tm = ThemeManager::instance();
    const QColor color = isChecked() ? tm->navSelectedText() : tm->textColor();
    m_pixmap = IconKit::pixmap(m_iconName, kIcon, color);
    update();
}

void NavItem::checkStateSet()
{
    QAbstractButton::checkStateSet();
    refreshPixmap();
}

void NavItem::enterEvent(QEnterEvent *event)
{
    m_hover = true;
    update();
    QAbstractButton::enterEvent(event);
}

void NavItem::leaveEvent(QEvent *event)
{
    m_hover = false;
    update();
    QAbstractButton::leaveEvent(event);
}

void NavItem::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF r = rect().adjusted(8, 0, 8, 0);

    if (isChecked()) {
        p.setPen(Qt::NoPen);
        p.setBrush(ThemeManager::instance()->navSelectedBg());
        p.drawRoundedRect(r, kRadius, kRadius);
    } else if (m_hover) {
        p.setPen(Qt::NoPen);
        p.setBrush(ThemeManager::instance()->navHoverBg());
        p.drawRoundedRect(r, kRadius, kRadius);
    }

    // 图标
    const QRectF iconRect((width() - kIcon) / 2.0, 12, kIcon, kIcon);
    p.drawPixmap(iconRect.toRect(), m_pixmap);

    // 文字
    QFont font = this->font();
    font.setPointSize(9); // ~12px
    font.setBold(isChecked());
    p.setFont(font);
    p.setPen(isChecked() ? ThemeManager::instance()->navSelectedText()
                         : ThemeManager::instance()->textColor());
    const QRectF textRect(8, 12 + kIcon + 6, width() - 16, 18);
    p.drawText(textRect, Qt::AlignHCenter | Qt::AlignVCenter, text());
}
