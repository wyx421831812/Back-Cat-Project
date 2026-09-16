#pragma once

#include <QFrame>

class QLabel;

// 设置条目卡片: 左侧标题/副说明, 右侧控件;
// vertical=true 时控件另起一行铺满 (用于滑块)。
// 对齐原版 ProListItem (b-1 p-4 rounded-lg)。
class ProListItem : public QFrame
{
    Q_OBJECT

public:
    explicit ProListItem(const QString &title,
                         const QString &description = QString(),
                         QWidget *control = nullptr,
                         bool vertical = false,
                         QWidget *parent = nullptr);

    void setTitle(const QString &title);
    void setDescription(const QString &description);
    void setControl(QWidget *control);
    void setDescriptionWidget(QWidget *widget);

private:
    void buildUi(bool vertical);

    QLabel *m_titleLabel = nullptr;
    QLabel *m_descLabel = nullptr;
    QWidget *m_control = nullptr;
    bool m_vertical;
};
