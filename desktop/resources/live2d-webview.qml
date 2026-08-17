import QtQuick 2.15
import QtWebView 1.1

// Live2D 渲染容器。
//
// 通信方式 (Qt WebView / WebView2 后端):
//  - C++ -> JS: QMetaObject::invokeMethod(..., "runJavaScript", ...)
//  - JS  -> C++: JS 设置 document.title = "__bongocat__:<json>:<ts>"，
//                触发 titleChanged 信号，C++ 端在 onWebViewTitleChanged 解析。
//
// 注意: Qt WebView 的 QQuickWebView 没有 webChannel 属性，不能使用 QWebChannel。
Item {
    id: root
    width: parent ? parent.width : 400
    height: parent ? parent.height : 300

    WebView {
        id: live2dWebView
        objectName: "live2dWebView"
        anchors.fill: parent

        onLoadFinished: {
            console.log("Live2D WebView loadFinished: ok=" + ok);
        }

        onLoadingChanged: {
            console.log("Live2D WebView loadingChanged: " + loadRequest);
        }

        onTitleChanged: {
            // C++ 端通过 connect(m_webView, SIGNAL(titleChanged(QString)), ...)
            // 接收，这里无需处理，仅保留日志便于调试
            console.log("Live2D WebView titleChanged: " + title);
        }

        // 供 C++ 通过 QMetaObject::invokeMethod 调用
        function runJavaScript(code, callback) {
            if (callback) {
                live2dWebView.runJavaScript(code, callback);
            } else {
                live2dWebView.runJavaScript(code);
            }
        }
    }
}
