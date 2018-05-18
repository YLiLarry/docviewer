import QtQuick 2.0
import QtWebEngine 1.5
import Backend 1.0

WebEngineView {
    id: webview
    property bool docviewLoaded: false
    property bool inDocview: false

    function docviewOn(callback) {
        webview.runJavaScript("Docview.turnOn()", function() {
            webview.inDocview = true
            callback()
        })
    }

    function docviewOff(callback) {
        webview.runJavaScript("Docview.turnOff()", function() {
            webview.inDocview = false
            callback()
        })
    }

    implicitHeight: listView.height
    implicitWidth: listView.width
    Component.onCompleted: {
        url = TabsModel.at(index).url
    }
    onUrlChanged: TabsModel.updateTab(index, "url", url.toString())
    onTitleChanged: TabsModel.updateTab(index, "title", title)
    onLoadProgressChanged: {
        if (loading) {
            console.log("onLoadProgressChanged", index, loadProgress)
            webViewLoadingProgressChanged(index, loadProgress)
        }
    }
    onLoadingChanged: {
        switch (loadRequest.status) {
        case WebEngineView.LoadStartedStatus:
            webview.docviewLoaded = false
            console.log("WebEngineView.LoadStartedStatus", loadRequest.errorString)
            if (index === currentIndex()) {
                var url = loadRequest.url
                // if control key is held, then stop loading
                // and open a new tab. If the tab already exists,
                // do nothing
                if (EventFilter.ctrlKeyDown) {
                    this.stop()
                    console.log("userOpensLinkInNewTab:", url);
                    userOpensLinkInNewTab(url)
                } else {
                    console.log("userOpensLinkInWebView:", url, webview)
                    userOpensLinkInWebView(index, url)
                }
            }
            webViewLoadingStarted(index, loadRequest.url)
            break
        case WebEngineView.LoadSucceededStatus:
            console.log("WebEngineView.LoadSucceededStatus", loadRequest.errorString)
            var js = FileManager.readQrcFileS("js/docview.js")
            webview.runJavaScript(js, function() {
                webview.docviewLoaded = true
                if (webview.inDocview) {
                    docviewOn()
                }
                if (! SearchDB.hasWebpage(webview.url)) {
                    SearchDB.addWebpage(webview.url)
                    SearchDB.updateWebpage(webview.url, "title", webview.title)
                    SearchDB.updateWebpage(webview.url, "temporary", true)
                    webview.runJavaScript("Docview.symbols()", function(syms) {
                        SearchDB.addSymbols(webview.url, syms)
                        webViewLoadingSucceeded(index, loadRequest.url)
                        webViewLoadingStopped(index, loadRequest.url)
                    })
                } else {
                    webViewLoadingSucceeded(index, loadRequest.url)
                    webViewLoadingStopped(index, loadRequest.url)
                }
            })
            break
        case WebEngineView.LoadFailedStatus:
            console.log("WebEngineView.LoadFailedStatus",
                        loadRequest.errorString)
            webViewLoadingFailed(index, loadRequest.url)
            webViewLoadingStopped(index, loadRequest.url)
            break
        case WebEngineView.LoadStoppedStatus:
            console.log("WebEngineView.LoadStoppedStatus",
                        loadRequest.errorString)
            webViewLoadingStopped(index, loadRequest.url)
            break
        }
    }
}