#include "localfileserver.h"
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QMimeDatabase>
#include <QHostAddress>
#include <QRegularExpression>
#include <QLoggingCategory>

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
#include <QTcpServer>
#include <QHttpHeaders>
#endif

Q_LOGGING_CATEGORY(lcFileServer, "localfileserver")

LocalFileServer *LocalFileServer::s_instance = nullptr;

LocalFileServer *LocalFileServer::instance()
{
    if (!s_instance) {
        s_instance = new LocalFileServer();
    }
    return s_instance;
}

LocalFileServer::LocalFileServer(QObject *parent)
    : QObject(parent)
{
}

quint16 LocalFileServer::start()
{
    if (m_server) return m_port;

    m_server = new QHttpServer(this);

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    // Qt 6.8+: setHeader/listen 被移除，使用 setHeaders(QHttpHeaders) + QTcpServer+bind
    auto applyHeaders = [](QHttpServerResponse &resp) {
        QHttpHeaders h;
        h.append("Access-Control-Allow-Origin", "*");
        h.append("Access-Control-Allow-Methods", "GET, OPTIONS");
        h.append("Access-Control-Allow-Headers", "*");
        h.append("Cache-Control", "no-cache");
        resp.setHeaders(h);
    };
#else
    // Qt 6.7 及更早: setHeader 可用
    auto applyHeaders = [](QHttpServerResponse &resp) {
        resp.setHeader("Access-Control-Allow-Origin", "*");
        resp.setHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
        resp.setHeader("Access-Control-Allow-Headers", "*");
        resp.setHeader("Cache-Control", "no-cache");
    };
#endif

    // catchAll 路由: 所有 GET 请求都映射到本地文件
    m_server->route("?", QHttpServerRequest::Method::Get,
        [applyHeaders](const QHttpServerRequest &request) {
            // request.url().path() 形如 "/D:/Workspace/.../file.json"
            QString path = request.url().path();
            // URL 解码 (处理中文路径等)
            path = QUrl::fromPercentEncoding(path.toUtf8());
            // 去掉前导 "/"
            while (path.startsWith('/')) path = path.mid(1);

            if (path.isEmpty()) {
                QHttpServerResponse resp("text/plain", "LocalFileServer: missing path",
                                         QHttpServerResponse::StatusCode::BadRequest);
                applyHeaders(resp);
                return resp;
            }

            QFile file(path);
            if (!file.open(QIODevice::ReadOnly)) {
                qCWarning(lcFileServer) << "File not found:" << path;
                QHttpServerResponse resp("text/plain", "File not found: " + path.toUtf8(),
                                         QHttpServerResponse::StatusCode::NotFound);
                applyHeaders(resp);
                return resp;
            }

            QByteArray data = file.readAll();
            QMimeDatabase mimeDb;
            QByteArray contentType = mimeDb.mimeTypeForFile(path).name().toUtf8();

            QHttpServerResponse resp(contentType, data);
            applyHeaders(resp);
            return resp;
        });

    // OPTIONS 预检
    m_server->route("?", QHttpServerRequest::Method::Options, [applyHeaders]() {
        QHttpServerResponse resp("text/plain", QByteArray());
        applyHeaders(resp);
        return resp;
    });

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    // Qt 6.8+: QHttpServer::listen() 被移除，必须 bind(QTcpServer*)
    auto tcpServer = new QTcpServer(m_server);
    if (!tcpServer->listen(QHostAddress::LocalHost, 0) || !m_server->bind(tcpServer)) {
        qCCritical(lcFileServer) << "Failed to start local file server";
        delete m_server;
        m_server = nullptr;
        return 0;
    }
    m_port = tcpServer->serverPort();
#else
    m_port = m_server->listen(QHostAddress::LocalHost, 0);
    if (m_port == 0) {
        qCCritical(lcFileServer) << "Failed to start local file server";
        delete m_server;
        m_server = nullptr;
        return 0;
    }
#endif

    qCInfo(lcFileServer) << "Local file server started on http://127.0.0.1:" << m_port;
    return m_port;
}

QString LocalFileServer::toUrl(const QString &localPath) const
{
    if (localPath.isEmpty()) return QString();

    // qrc 资源文件: 返回 qrc URL (JS 库已内联，模型纹理不会走 qrc)
    if (localPath.startsWith(":/") || localPath.startsWith("qrc:")) {
        return localPath;
    }

    // 文件系统路径 → HTTP URL
    // 不编码 ":" 和 "/" 和 "\"，保证盘符和路径分隔符完整
    QString encoded = QString::fromUtf8(QUrl::toPercentEncoding(localPath, "/:\\"));
    // Windows 反斜杠转为正斜杠
    encoded.replace('\\', '/');
    return QStringLiteral("http://127.0.0.1:%1/%2").arg(m_port).arg(encoded);
}
