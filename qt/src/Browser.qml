import Backend 1.0
import QtQuick 2.9
import QtWebView 1.1
import QtQuick.Controls 2.3

BrowserForm {
    id: browser
    function currentWebView() {
        return browserWebViews.currentWebView()
    }

    function currentIndex() {
        return browserWebViews.currentIndex()
    }

    Component.onCompleted: {
        if (TabsModel.count > 0) {
            openTab(0)
        }
    }

    browserAddressBar.progress: browserWebViews.loadProgress

    function newTab(url, switchToView) {
        console.log("newTab:", url, switchToView)
        url = url || "https://google.ca"
        TabsModel.insertTab(0, url, "", "")
        if (switchToView) {
            openTab(0)
        } else {
            openTab(currentIndex() + 1)
        }
    }

    function openTab(index) {
        console.log("openTab", "index=", index, "TabsModel.count=", TabsModel.count)

        browserWebViews.setCurrentIndex(index)
        tabsPanel.setCurrentIndex(index)
        var wp = currentWebView()
        browserAddressBar.update(currentIndex())
        browserBookmarkButton.checked = true
        prevEnabled = wp && wp.canGoBack
        nextEnabled = wp && wp.canGoForward
    }

    function closeTab(index) {
        console.log("closeTab", "index=", index, "TabsModel.count=", TabsModel.count)
        // todo: remove from backend
        if (currentIndex() === index) {
            // when removing current tab
            // if there's one before, open that
            if (index >= 1) {
                TabsModel.removeTab(index)
                openTab(index - 1)
                // if there's one after, open that
            } else if (index + 1 < TabsModel.count) {
                TabsModel.removeTab(index)
                openTab(index)
                // if this is the only one
            } else {
                newTab("",true)
                TabsModel.removeTab(index+1)
            }
        } else if (currentIndex() > index) {
            TabsModel.removeTab(index)
            openTab(currentIndex() - 1)
        } else {
            TabsModel.removeTab(index)
        }
    }

    Connections {
        target: tabsPanel
        onUserOpensNewTab: newTab("", true)
    }

    Connections {
        target: tabsPanel
        onUserOpensTab: openTab(index)
        onUserClosesTab: closeTab(index)
    }

    Connections {
        target: browserWebViews
        onUserOpensLinkInWebView: {
            browserAddressBar.update(url, "")
            currentWebView().forceActiveFocus()
            //            prevEnabled = true
            //            nextEnabled = false
        }
        onUserOpensLinkInNewTab: {
            newTab(url)
        }
        onWebViewLoadingSucceeded: {
            var wp = browserWebViews.webViewAt(index)
            var js = FileManager.readQrcFileS("docview.js")
            wp.runJavaScript(js)
            if (index === currentIndex()) {
                browserBookmarkButton.checked = true
                browserAddressBar.update(wp.url, wp.title)
            }
        }
        onWebViewLoadingStarted: {
            //            TabsModel.updateTab(index, "title", "")
            //            TabsModel.updateTab(index, "url", url)
        }
        onWebViewLoadingStopped: {
            var cw = currentWebView()
            prevEnabled = cw && cw.canGoBack
            nextEnabled = cw && cw.canGoForward
            var wp = browserWebViews.webViewAt(index)
            //            TabsModel.updateTab(index, "title", wp.title)
            //            TabsModel.updateTab(index, "url", wp.url)
        }
    }

    Connections {
        target: browserAddressBar
        onUserEntersUrl: {
            currentWebView().url = url
        }
    }

    Connections {
        target: browserBackButton
        onClicked: {
            currentWebView().goBack()
        }
    }

    Connections {
        target: browserForwardButton
        onClicked: {
            currentWebView().goForward()
        }
    }

    Connections {
        target: browserRefreshButton
        onClicked: {
            browserWebViews.reloadCurrentWebView()
        }
    }

    Connections {
        target: browserDocviewSwitch
        onCheckedChanged: {
            if (browserDocviewSwitch.checked) {
                console.log("Docview.turnOn()")
                currentWebView().runJavaScript("Docview.turnOn()",
                                               function (result) {
                                                   print(result)
                                               })
            } else {
                console.log("Docview.turnOff()")
                currentWebView().runJavaScript("Docview.turnOff()",
                                               function (result) {
                                                   print(result)
                                               })
            }

        }
    }

    Shortcut {
        sequence: "Ctrl+R"
        autoRepeat: false
        onActivated: {
            EventFilter.ctrlKeyDown = false
            browserWebViews.reloadCurrentWebView()
        }
    }

    Timer {
        id: ctrl_w_timeout
        interval: 500
        triggeredOnStart: false
        onTriggered: {
            ctrl_w.guard = true
        }
        repeat: false
    }

    Shortcut {
        id: ctrl_w
        property bool guard: true
        autoRepeat: true
        sequence: "Ctrl+W"
        context: Qt.ApplicationShortcut
        onActivated: {
            if (guard) {
                guard = false
                EventFilter.ctrlKeyDown = false
                console.log("Ctrl+W", ctrl_w)
                closeTab(currentIndex())
                ctrl_w_timeout.start()
            }
        }
    }
}
