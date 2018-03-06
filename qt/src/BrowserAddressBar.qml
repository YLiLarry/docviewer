import QtQuick 2.7

BrowserAddressBarForm {
    id: form
    function update(url, title) {
        textField.focus = false
        browserAddressBar.url = url
        browserAddressBar.text = title
        if (url !== title) {
            textField.horizontalAlignment = Text.AlignHCenter
        } else {
            textField.horizontalAlignment = Text.AlignLeft
            textField.ensureVisible(0)
        }
    }
    signal userEntersUrl(string url)
    onProgressChanged: {
        console.log("progress", progress)
        if (progress == 0) {
            barWidthAnimation.enabled = false
        } else if (progress == 100) {
            barWidthAnimation.enabled = true
            progressBar.opacity = 0
        } else {
            progressBar.opacity = 0.3
        }
    }

    Behavior on progressBarWidth {
        id: barWidthAnimation
        enabled: false
        SmoothedAnimation {
            duration: 1000
        }
    }

    Shortcut {
        sequence: "Ctrl+Space"
        onActivated: {
            textField.focus = true
            textField.selectAll()
        }
    }

    Connections {
        target: textField
        onAccepted: {
            var url = textField.text
            var exp = new RegExp("http://|https://")
            if (!exp.test(url)) {
                url = "http://www.google.com/search?query=" + url
            }
            textField.focus = false
            userEntersUrl(url)
        }
        onFocusChanged: {
            if (textField.activeFocus) {
                textField.horizontalAlignment = Text.AlignLeft
                textField.text = browserAddressBar.url
                textField.ensureVisible(0)
                textField.selectAll()
            } else {
                textField.deselect()
                textField.horizontalAlignment = Text.AlignHCenter
                textField.text = browserAddressBar.title
            }
        }
    }
}