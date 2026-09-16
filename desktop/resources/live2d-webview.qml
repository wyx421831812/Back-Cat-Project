import QtQuick 2.15
import QtWebView 1.1

// Live2D 渲染容器。
//
// 通信方式 (Qt WebView / WebView2 后端):
//  - C++ -> JS: QMetaObject::invokeMethod(..., "runJavaScript", ...)
//  - JS  -> C++: JS 设置 document.title = "__bongocat__:<json>:<ts>"，
//                触发 titleChanged，这里转发给 C++ 的 bridgeTitleChanged。
//
// 注意 (Qt 6.7 实测源码/插件签名):
//  - WebView 没有 loadFinished 信号! 只有 loadingChanged(loadRequest)。
//    声明不存在的 onXxx 处理器会导致整个 QML 加载失败 (status=Error, 白块)。
//  - 原生 titleChanged() 是无参信号 (title 通过属性读取)，
//    因此由根对象声明带参数的 bridgeTitleChanged 统一转发给 C++，
//    避免 C++ 端按 "titleChanged(QString)" 连接原生信号失败。
Item {
    id: root
    width: parent ? parent.width : 400
    height: parent ? parent.height : 300

    // C++ 端通过 SIGNAL(bridgeTitleChanged(QString)) 连接此信号接收页面标题事件
    signal bridgeTitleChanged(string title)

    WebView {
        id: live2dWebView
        objectName: "live2dWebView"
        anchors.fill: parent

        onLoadingChanged: {
            console.log("Live2D WebView loadingChanged: status=" + loadRequest.status
                        + " url=" + loadRequest.url
                        + " err=" + loadRequest.errorString);
        }

        onTitleChanged: {
            console.log("Live2D WebView titleChanged: " + live2dWebView.title);
            root.bridgeTitleChanged(live2dWebView.title);
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
