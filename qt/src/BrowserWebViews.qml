import QtQuick 2.7
import QtWebView 1.1
import Backend 1.0

BrowserWebViewsForm {
    id: listView

    property alias tabsModel: listView.repeaterModel

    signal userOpensLinkInCurrentWebView(string url)
    signal webViewLoadingSucceeded(string index)

    repeaterDelegate: WebView {
        id: webview
        property bool success: false
        focus: true
        url: model.url
        Keys.onPressed: main.currentKeyPress = event.key
        Keys.onReleased: main.currentKeyPress = -1
        onLoadingChanged: {
            console.log(index, loadRequest, loadRequest.url)
//            switch (loadRequest.status) {
//            case WebView.LoadStartedStatus:
//                if (index === getCurrentIndex()) {
//                    // if control key is held, then stop loading
//                    // and open a new tab. If the tab already exists,
//                    // do nothing
//                    if (main.currentKeyPress === Qt.Key_Control) {
//                        this.stop()
//                        var idx = TabsModel.findTab(loadRequest.url)
//                        if (idx === -1) {
//                            idx = TabsModel.insertTab(0,
//                                                      loadRequest.url, "", "")
//                            getWebViewAt(idx).stop()
//                        }
//                    } else {
//                        userOpensLinkInCurrentWebView(loadRequest.url)
//                    }
//                }
//                break
//            case WebView.LoadSucceededStatus:
//                this.success = true
//                var wp = getWebViewAt(index)
//                tabsModel.setProperty(index, "title", wp.title)
//                tabsModel.setProperty(index, "url", wp.url.toString())
//                webViewLoadingSucceeded(index)
//                break
//            }
        }
    }

    //    Component.onCompleted: {
    //        listView.setCurrentIndex(0)
    //        listView.repeater.model = TabsModel.tabs
    //    }
    Connections {
        target: listView.stackLayout
        onCurrentIndexChanged: {
            console.log("listView.stackLayout.onCurrentIndexChanged",
                        listView.stackLayout.currentIndex)
        }
    }
    property string url: getCurrentWebView() ? getCurrentWebView().url : ""
    property string title: getCurrentWebView() ? getCurrentWebView().title : ""
    function setCurrentIndex(idx) {
        listView.stackLayout.currentIndex = idx
        if (!getWebViewAt(idx).success) {
            reloadWebViewAt(idx)
        }
    }
    function getWebViewAt(idx) {
        return listView.repeater.itemAt(idx)
    }
    function getCurrentIndex() {
        return listView.stackLayout.currentIndex
    }
    function getCurrentWebView() {
        var idx = listView.getCurrentIndex()
        return listView.getWebViewAt(idx)
    }
    function reloadWebViewAt(index) {
        console.log("reloadWebViewAt", index)
        main.currentKeyPress = -1
        getWebViewAt().reload()
    }
    function reloadCurrentWebView() {
        // ignore Ctrl in this function
        reloadWebViewAt(getCurrentIndex())
    }
}
