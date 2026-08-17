#ifndef BONGOCATWIDGET_H
#define BONGOCATWIDGET_H

#include "componentbase.h"
#include <QMap>
#include <QPixmap>
#include <QString>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

/**
 * @brief Bongo Cat 桌宠组件
 *
 * 参考原版 BongoCat 的实现思路：
 * - 单张底图（cover.png）画猫咪身体 + 键盘
 * - 按键贴图分目录存放：left-keys/ 右手区、right-keys/ 右手区
 * - 按下某个键时，叠加对应按键贴图（手按下的状态）
 * - 同一只手同一时刻最多显示一个按键贴图（同手互斥）
 *   避免多个按键叠加导致「上下多只手同时出现」的问题
 *
 * 资源加载方式：
 * - 优先从文件系统加载 desktop/resources/models/keyboard/resources/ 真实目录
 * - 如果文件系统加载失败（例如部署时路径不存在），回退到 Qt 资源系统 qrc
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
    enum class HandSide { Left = 0, Right = 1 };

    struct ActiveKey {
        int vk = 0;
        QPixmap pixmap;
        bool valid = false;
    };

    // 加载底图 + 按键贴图，建立 vkCode -> pixmap / vkCode -> 左右手 映射
    void loadImages();
    // 扫描某个按键子目录（left-keys 或 right-keys），填充 m_keyPixmaps 和 m_keyHandMap
    void scanKeyDirectory(const QString &dirPath, HandSide side,
                          const QMap<int, QString> &nameToVk);
    // 安装 / 卸载全局键盘 Hook
    void installHook();
    void removeHook();
    void clearActiveKeys();

    // 按键贴图（vkCode -> pixmap）
    QMap<int, QPixmap> m_keyPixmaps;
    // 按键左右手归属（vkCode -> Left/Right），对应原版 left-keys / right-keys 目录分类
    QMap<int, HandSide> m_keyHandMap;
    // 每只手当前激活的按键（同手互斥，同一时刻一只手只显示一个按键贴图）
    ActiveKey m_activeKey[2];
    // 底图（猫咪趴在键盘上，不含手部或只含休息位的手）
    QPixmap m_cover;

#ifdef Q_OS_WIN
    static BongoCatWidget *s_instance;
    static HHOOK s_hook;
    static LRESULT CALLBACK hookProc(int nCode, WPARAM wParam, LPARAM lParam);
#endif
};

#endif // BONGOCATWIDGET_H
