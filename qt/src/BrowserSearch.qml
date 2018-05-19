import QtQuick 2.7

BrowserSearchForm {
//    signal userSearchesWordInBrowser(string word)
    signal userSearchesNextInBrowser()
    signal userSearchesPreviousInBrowser()
    signal userClosesSearch()
    signal userTypesInSearch()
    function updateCount(count) {
        highlightCount = count
        console.log("updateCount " + highlightCount)
    }
    function updateCurrent(current) {
        currentHighlight = current
        console.log("updateCurrent " + currentHighlight)
    }
    function current() {
        return currentHighlight;
    }
    function count() {
        return highlightCount;
    }
    function hideCount() {
        counter.visible = false
    }
    function showCount() {
        counter.visible = true
    }
    //    function lock() {
    //        locked = true
    //    }
    //    function unlock() {
    //        locked = false
    //    }
    property bool locked: false
    id: form
    counter.visible: false
    textfield.onAccepted: {
//        if (locked) {
        if (textfield.text) {
            userSearchesNextInBrowser()
        }
//        } else {
//            userSearchesWordInBrowser(textfield.text)
//            updateCount(0)
//            updateCurrent(-1)
//        }
//        locked = true
//        console.log("userSearchesWordInBrowser", textfield.text)
    }
    textfield.onTextEdited: {
//        if (locked) {
            userTypesInSearch()
//            updateCount(0)
//            updateCurrent(-1)
//            locked = false
//        }
    }
    prevBtn.onClicked: {
        userSearchesPreviousInBrowser()
        console.log("userSearchesPreviousInBrowser")
    }
    nextBtn.onClicked: {
        userSearchesNextInBrowser()
        console.log("userSearchesNextInBrowser")
    }
    closeBtn.onClicked: {
        userClosesSearch()
    }
}
