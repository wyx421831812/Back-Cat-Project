#ifndef TODOWIDGET_H
#define TODOWIDGET_H

#include "componentbase.h"
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

/**
 * @brief 待办清单组件
 *
 * 极简任务跟踪
 * 支持添加/完成/删除任务
 * 数据持久化到 JSON 文件
 */
class TodoWidget : public ComponentBase
{
    Q_OBJECT

public:
    explicit TodoWidget(QWidget *parent = nullptr);

    QString componentName() const override { return QStringLiteral("待办清单"); }
    void refresh() override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QListWidget *m_listWidget;
    QLineEdit *m_inputEdit;
    QPushButton *m_addBtn;

    void loadTodos();
    void saveTodos();

private slots:
    void onAdd();
    void onItemChanged(QListWidgetItem *item);
    void onItemDoubleClicked(QListWidgetItem *item);
};

#endif // TODOWIDGET_H