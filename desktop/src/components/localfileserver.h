#ifndef LOCALFILESERVER_H
#define LOCALFILESERVER_H

#include <QObject>
#include <QHttpServer>
#include <QString>

/**
 * @brief 本地 HTTP 文件服务器
 *
 * 解决 WebView2 下 file:/// 协议被 CORS 策略阻止的问题。
 * pixi.js 需要通过 HTTP(S) 加载 .moc3/.model3.json/纹理等资源。
 *
 * URL 格式: http://127.0.0.1:<port>/<绝对路径>
 * 例如: http://127.0.0.1:23456/D:/Workspace/models/standard/cat.model3.json
 */
class LocalFileServer : public QObject
{
    Q_OBJECT
public:
    static LocalFileServer *instance();

    /**
     * @brief 启动服务器 (只监听 127.0.0.1)
     * @return 监听端口，失败返回 0
     */
    quint16 start();

    /**
     * @brief 将本地文件路径转换为 HTTP URL
     */
    QString toUrl(const QString &localPath) const;

    quint16 port() const { return m_port; }
    bool isRunning() const { return m_server != nullptr; }

private:
    explicit LocalFileServer(QObject *parent = nullptr);
    ~LocalFileServer() override = default;

    QHttpServer *m_server = nullptr;
    quint16 m_port = 0;

    static LocalFileServer *s_instance;
};

#endif // LOCALFILESERVER_H
