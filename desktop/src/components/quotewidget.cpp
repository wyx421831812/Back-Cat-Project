#include "quotewidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QDateTime>
#include "appconfig.h"

QuoteWidget::QuoteWidget(QWidget *parent)
    : ComponentBase(parent)
    , m_currentIndex(0)
{
    setupTransparentBackground();

    // 治愈系语录
    m_quotes = {
        QStringLiteral("生活不止眼前的苟且，还有诗和远方。"),
        QStringLiteral("每一天都是新的开始，加油！"),
        QStringLiteral("你比你想象中更强大。"),
        QStringLiteral("星星之火，可以燎原。"),
        QStringLiteral("今天也要元气满满哦~"),
        QStringLiteral("不管多难，都要笑着面对。"),
        QStringLiteral("温暖的人，运气都不会太差。"),
        QStringLiteral("保持热爱，奔赴山海。"),
        QStringLiteral("愿你成为自己的太阳。"),
        QStringLiteral("世界很大，想做的事很多，慢慢来。"),
        QStringLiteral("爱上雷神~"),
        QStringLiteral("每一步都算数，每一刻都珍贵。"),
        QStringLiteral("你今天的努力，是明天的底气。"),
        QStringLiteral("别急，好戏还在后头。"),
        QStringLiteral("做自己的光，不需要太亮。"),
    };

    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins(15, 15, 15, 15);

    m_quoteLabel = new QLabel();
    m_quoteLabel->setObjectName("quoteTextLabel");
    m_quoteLabel->setWordWrap(true);
    m_quoteLabel->setAlignment(Qt::AlignCenter);
    QFont quoteFont("Microsoft YaHei", 10);
    quoteFont.setItalic(true);
    m_quoteLabel->setFont(quoteFont);

    m_authorLabel = new QLabel();
    m_authorLabel->setObjectName("quoteAuthorLabel");
    m_authorLabel->setAlignment(Qt::AlignRight);
    m_authorLabel->setText(QStringLiteral("— BackPet"));

    layout->addStretch();
    layout->addWidget(m_quoteLabel);
    layout->addWidget(m_authorLabel);
    layout->addStretch();

    // 根据日期选择今天的寄语
    int dayOfYear = QDateTime::currentDateTime().date().dayOfYear();
    m_currentIndex = dayOfYear % m_quotes.size();
    m_quoteLabel->setText(m_quotes[m_currentIndex]);

    // 每小时刷新一次
    m_timer = new QTimer(this);
    m_timer->setInterval(3600000); // 1 小时
    connect(m_timer, &QTimer::timeout, this, &QuoteWidget::nextQuote);
    m_timer->start();
}

void QuoteWidget::nextQuote()
{
    m_currentIndex = (m_currentIndex + 1) % m_quotes.size();
    m_quoteLabel->setText(m_quotes[m_currentIndex]);
}

void QuoteWidget::refresh()
{
    nextQuote();
}

void QuoteWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    auto &cfg = AppConfig::instance();
    QColor bg = cfg.secondaryColor();
    bg.setAlpha(180);
    p.setBrush(bg);
    p.setPen(Qt::NoPen);

    QPainterPath path;
    path.addRoundedRect(rect().adjusted(4, 4, -4, -4), 15, 15);
    p.fillPath(path, bg);
}
