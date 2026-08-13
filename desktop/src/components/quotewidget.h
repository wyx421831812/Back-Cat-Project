#ifndef QUOTEWIDGET_H
#define QUOTEWIDGET_H

#include "componentbase.h"
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

/**
 * @brief 每日寄语组件
 *
 * 每天显示一句治愈系文字
 * 定时刷新不同寄语
 */
class QuoteWidget : public ComponentBase
{
    Q_OBJECT

public:
    explicit QuoteWidget(QWidget *parent = nullptr);

    QString componentName() const override { return QStringLiteral("每日寄语"); }
    void refresh() override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QLabel *m_quoteLabel;
    QLabel *m_authorLabel;
    QTimer *m_timer;

    QStringList m_quotes;
    int m_currentIndex;

private slots:
    void nextQuote();
};

#endif // QUOTEWIDGET_H
