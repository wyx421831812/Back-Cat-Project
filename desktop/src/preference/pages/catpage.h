#ifndef PREFERENCE_CATPAGE_H
#define PREFERENCE_CATPAGE_H

#include <QWidget>

// 猫咪设置页 (对齐原版 preference/components/cat/index.vue):
// 模型设置 7 项 + 窗口设置 7 项, 全部双向绑定 AppConfig,
// 变更经 catModelChanged/catWindowChanged 即时作用于 PetWidget/PetStage。
class CatPage : public QWidget
{
    Q_OBJECT

public:
    explicit CatPage(QWidget *parent = nullptr);

private:
    void buildUi();
    void loadFromConfig();

    // 初始化期间屏蔽控件回写
    bool m_syncing = false;
};

#endif // PREFERENCE_CATPAGE_H
