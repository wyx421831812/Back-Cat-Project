#ifndef COMPONENTBASE_H
#define COMPONENTBASE_H

#include <QWidget>

/**
 * @brief 组件基类
 *
 * 所有功能组件继承此类
 * 提供统一接口: 刷新/显示/隐藏
 */
class ComponentBase : public QWidget
{
    Q_OBJECT

public:
    explicit ComponentBase(QWidget *parent = nullptr)
        : QWidget(parent) {}

    virtual ~ComponentBase() {}

    // 组件名称
    virtual QString componentName() const = 0;

    // 刷新组件内容
    virtual void refresh() = 0;

    // 组件显示/隐藏时的回调
    virtual void onShow() {}
    virtual void onHide() {}

protected:
    // 设置透明背景
    void setupTransparentBackground()
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
    }
};

#endif // COMPONENTBASE_H