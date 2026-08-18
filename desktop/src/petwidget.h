#ifndef PETWIDGET_H
#define PETWIDGET_H

#include <QWidget>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QStackedWidget>
#include "petcanvas.h"
#include "appconfig.h"

class ClockWidget;
class QuoteWidget;
class TodoWidget;
class BongoCatWidget;

/**
 * @brief 涓荤獥鍙?- 鏃犺竟妗嗛€忔槑缃《妗岄潰瀹犵墿
 *
 * 鐗规€?
 * - 鏃犺竟妗嗛€忔槑绐楀彛
 * - 濮嬬粓缃《
 * - 鏀寔鐐瑰嚮绌块€?
 * - 鍙嫋鎷界Щ鍔?
 * - 鍙抽敭鑿滃崟鍒囨崲缁勪欢
 * - 绯荤粺鎵樼洏鍥炬爣
 */
class PetWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PetWidget(QWidget *parent = nullptr);
    ~PetWidget();

protected:
    // 绐楀彛浜嬩欢
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

    // Windows 鍘熺敓浜嬩欢 (鐐瑰嚮绌块€?
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#endif

private:
    void setupUI();
    void setupTrayIcon();
    void setupContextMenu();
    void applyConfig();
    void updateClickThrough();
    void updateAlwaysOnTop();
    void updateOpacity();

    // 缁勪欢鍒囨崲
    void switchToPet();
    void switchToClock();
    void switchToQuote();
    void switchToTodo();
    void switchToBongoCat();

    // 鎯呯华鎺у埗
    void setMoodHappy();
    void setMoodSleep();
    void setMoodExcited();
    void setMoodNeutral();

    // 3D妯″瀷鍒囨崲
    void setModelCat();
    void setModelBear();
    void setModelBunny();
    void setModelFairyBird();
    void setModelSpirit();

    // BongoCat 妯″瀷鐩稿叧
    void rebuildBongoCatModelMenu();
    void switchToBongoCatModel(const QString &modelId);
    void importBongoCatModel();
    void deleteBongoCatModel(const QString &modelId);

private:
    // 鎷栨嫿 (鏀惧湪鍓嶉潰浠ュ尮閰嶅垵濮嬪寲椤哄簭)
    bool m_dragging;
    QPoint m_dragOffset;

    // 鏍稿績鎺т欢
    QStackedWidget *m_stack;
    PetCanvas *m_petCanvas;

    // 鍔熻兘缁勪欢
    ClockWidget *m_clockWidget;
    QuoteWidget *m_quoteWidget;
    TodoWidget *m_todoWidget;
    BongoCatWidget *m_bongoCatWidget;

    // 绯荤粺鎵樼洏
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    QMenu *m_contextMenu;
    QMenu *m_trayBongoModelMenu;
    QMenu *m_contextBongoModelMenu;

    // 缁勪欢绱㈠紩
    enum ComponentIndex {
        PetComponent = 0,
        ClockComponent = 1,
        QuoteComponent = 2,
        TodoComponent = 3,
        BongoCatComponent = 4
    };

private slots:
    void onSettings();
    void onQuit();
    void onPetClicked();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
};

#endif // PETWIDGET_H
