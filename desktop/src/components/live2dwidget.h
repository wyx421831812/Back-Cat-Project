#ifndef LIVE2DWIDGET_H
#define LIVE2DWIDGET_H

#include <QWidget>
#include <QQuickWidget>
#include <QUrl>
#include <QJsonObject>
#include <QString>

/**
 * @brief Live2D 模型渲染窗口
 *
 * 使用 QQuickWidget + Qt WebView (QML) + pixi.js + pixi-live2d-display 渲染 Live2D 模型
 *
 * 通信机制:
 *  - C++ -> JS: 通过 QML WebView 的 runJavaScript() 调用 window.Live2DAPI.*
 *  - JS  -> C++: 通过 document.title = "__bongocat__:<json>:<ts>" 触发
 *                QML WebView 的 titleChanged 信号, 在 onWebViewTitleChanged() 中解析
 *
 *  注意: Qt WebView (Windows WebView2 后端) 不支持 QWebChannel 的 webChannelTransport,
 *        因此不能使用 QWebChannel 方案。
 */
class Live2DWidget : public QWidget
{
    Q_OBJECT
public:
    explicit Live2DWidget(QWidget *parent = nullptr);
    ~Live2DWidget();

    // 加载模型
    bool loadModel(const QString &modelPath);

    // 播放动作
    bool playMotion(const QString &group, int index = 0);

    // 播放表情
    bool playExpression(int index);

    // 设置参数值
    bool setParameter(const QString &paramId, double value);

    // 处理键盘/鼠标输入
    void handleKeyDown(const QString &keyName);
    void handleKeyUp(const QString &keyName);
    void handleMouseMove(double x, double y);
    void handleMouseDown(int button);
    void handleMouseUp(int button);

    // L-104: 游戏手柄输入 (摇杆 [-1, 1], 按钮索引)
    void handleGamepadAxis(double leftX, double leftY, double rightX, double rightY);
    void handleGamepadButtonDown(int button);
    void handleGamepadButtonUp(int button);

    // 重置所有参数
    void resetParameters();

    // L-105: 设置自动释放延迟 (毫秒)
    void setAutoReleaseDelay(int ms);

    // === 背景和按键图 (与 BongoCat 官方渲染层次一致) ===
    void setBackgroundImage(const QString &path);
    void setKeyImage(const QString &keyName, const QString &path);
    void clearKeyImage(const QString &keyName);
    void clearAllKeyImages();

    // JavaScript是否就绪
    bool isReady() const { return m_ready; }

signals:
    void modelLoaded(int width, int height);
    void errorOccurred(const QString &errorMessage);
    void readyChanged(bool ready);

private slots:
    void onQuickWidgetStatusChanged(QQuickWidget::Status status);
    void onWebViewTitleChanged(const QString &title);

private:
    void runJavaScript(const QString &code);
    void loadCombinedHtml();
    void handleJsEvent(const QString &eventName, const QJsonObject &data);
    void setReady(bool ready);

    QQuickWidget *m_quickWidget = nullptr;
    QObject *m_webView = nullptr;  // QML WebView object (from live2d-webview.qml)

    bool m_ready = false;
    QString m_currentModelPath;
};

#endif // LIVE2DWIDGET_H
