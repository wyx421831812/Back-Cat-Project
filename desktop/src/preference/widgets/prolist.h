#pragma once

#include <QWidget>

class QVBoxLayout;
class QLabel;

// 设置分组: 14px 分组标题 + 纵向卡片列表。
// 对齐原版 ProList (title + slot items, 卡片间距 8, 组间距 16)。
class ProList : public QWidget
{
    Q_OBJECT

public:
    explicit ProList(const QString &title, QWidget *parent = nullptr);

    void addItem(QWidget *item);
    void setTitle(const QString &title);

    // 用于动态增删
    QVBoxLayout *itemsLayout() const { return m_itemsLayout; }

private:
    QLabel *m_title;
    QVBoxLayout *m_itemsLayout;
};
