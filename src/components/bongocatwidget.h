#ifndef BONGOCATWIDGET_H
#define BONGOCATWIDGET_H

#include "componentbase.h"
#include <QMap>
#include <QSet>
#include <QPixmap>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

/**
 * @brief Bongo Cat 桌宠组件
 *
 * 参考Bongo Cat,显示角色趴在键盘上的2D图片
 * 监听全局键盘输入,按下时叠加对应按键图片
 * 支持的按键: A-Z(部分), 1-5, Space, Shift, Ctrl, Alt, Return
 */
class BongoCatWidget : public ComponentBase
{
    Q_OBJECT

public:
    explicit BongoCatWidget(QWidget *parent = nullptr);
    ~BongoCatWidget();

    QString componentName() const override { return QStringLiteral("BongoCat"); }
    void refresh() override {}
    void onShow() override;
    void onHide() override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    // 加载图片资源
    void loadImages();
    // 安装/卸载键盘钩子
    void installHook();
    void removeHook();
    // VK码 -> 图片资源路径的映射
    QMap<int, QPixmap> m_keyPixmaps;
    // 当前按下的按键集合
    QSet<int> m_pressedKeys;
    // 角色底图
    QPixmap m_cover;

#ifdef Q_OS_WIN
    static BongoCatWidget *s_instance;
    static HHOOK s_hook;
    static LRESULT CALLBACK hookProc(int nCode, WPARAM wParam, LPARAM lParam);
#endif
};

#endif // BONGOCATWIDGET_H