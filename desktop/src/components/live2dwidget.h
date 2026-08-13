#ifndef LIVE2DWIDGET_H
#define LIVE2DWIDGET_H

#include <QWidget>
#include <QWebEngineView>
#include <QWebChannel>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QUrl>
#include <QJsonObject>
#include <QString>

/**
 * @brief Live2D 模型渲染窗口
 *
 * 使用 QWebEngineView + pixi.js + pixi-live2d-display 渲染 Live2D 模型
 * 通过 QWebChannel 实现 C++ 与 JavaScript 的双向通信
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

    // 重置所有参数
    void resetParameters();

    // JavaScript是否就绪
    bool isReady() const { return m_ready; }

signals:
    void modelLoaded(int width, int height);
    void errorOccurred(const QString &errorMessage);
    void readyChanged(bool ready);

private slots:
    void onJavaScriptReady();
    void onModelLoaded(const QString &modelInfoJson);
    void onJavaScriptError(const QString &errorMsg);

private:
    void setupWebChannel();
    void runJavaScript(const QString &code);

    QWebEngineView *m_webView = nullptr;
    QWebChannel *m_webChannel = nullptr;
    QWebEngineProfile *m_profile = nullptr;
    QWebEnginePage *m_page = nullptr;

    bool m_ready = false;
    QString m_currentModelPath;

    // JavaScript回调对象
    class JsCallbackObject : public QObject
    {
        Q_OBJECT
    public:
        explicit JsCallbackObject(Live2DWidget *parent)
            : QObject(parent), m_widget(parent) {}

    public slots:
        Q_INVOKABLE void onReady();
        Q_INVOKABLE void onModelLoaded(const QString &modelInfoJson);
        Q_INVOKABLE void onError(const QString &errorMsg);

    private:
        Live2DWidget *m_widget;
    };

    JsCallbackObject *m_callbackObject = nullptr;

    friend class JsCallbackObject;
};

#endif // LIVE2DWIDGET_H
