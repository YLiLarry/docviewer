#include <QtCore/QtCore>
#include "controller.hpp"
#include "global.hpp"
#include "tabsmodel.hpp"

QLoggingCategory ControllerLogging("Controller");

Controller::Controller()
{
    viewTab(TabStateEmpty, -1);
}

int Controller::newTab()
{
    return Controller::newTab(TabStateOpen, home_url(), WhenCreatedViewNew, WhenExistsOpenNew);
}

int Controller::newTab(TabState state,
                       Url const& uri,
                       WhenCreated newBehavior,
                       WhenExists whenExists)
{
    qInfo(ControllerLogging) << "BrowserController::newTab"
                             << state
                             << uri
                             << newBehavior
                             << whenExists;
    int idx = 0;
    if (state == TabStateOpen) {
        bool inserted = false;
        if (whenExists == WhenExistsViewExisting) {
            idx = open_tabs()->findTab(uri);
            if (idx == -1) {
                open_tabs()->insertTab(idx = 0, uri);
                inserted = true;
            }
        } else {
            open_tabs()->insertTab(idx = 0, uri);
            inserted = true;
        }
        if (newBehavior == WhenCreatedViewNew) {
            viewTab(state, idx);
        } else if (newBehavior == WhenCreatedViewCurrent) {
            if (current_tab_state() == TabStateOpen) {
                if (inserted) {
                    viewTab(TabStateOpen, current_open_tab_index() + 1);
                }
            }
        }
        if (inserted) {
            saveLastOpen();
        }
    } else if (state == TabStatePreview) {
        if (whenExists == WhenExistsViewExisting) {
            idx = open_tabs()->findTab(uri);
            if (idx > -1) {
                viewTab(TabStateOpen, idx);
                return idx;
            }
            idx = preview_tabs()->findTab(uri);
            if (idx == -1) {
                preview_tabs()->insertTab(idx = 0, uri);
            }
        } else {
            preview_tabs()->insertTab(idx = 0, uri);
        }
        if (newBehavior == WhenCreatedViewNew) {
            viewTab(state, idx);
        }
    }
    return idx;
}

// switch view
int Controller::viewTab(TabState state, int i)
{
    qInfo(ControllerLogging) << "BrowserController::viewTab" << state << i;
    static Webpage_ old_page = nullptr;
    hideCrawlerRuleTable();
    // disconnect from old
    if (old_page != nullptr) {
        QObject::disconnect(old_page.get(), &Webpage::load_progress_changed, this, &Controller::set_address_bar_load_progress);
        QObject::disconnect(old_page.get(), &Webpage::title_changed, this, &Controller::set_address_bar_title);
        QObject::disconnect(old_page.get(), &Webpage::find_text_state_changed, this, &Controller::set_current_webpage_find_text_state);
        QObject::disconnect(old_page.get(), &Webpage::crawler_rule_table_changed, this, &Controller::set_current_webpage_crawler_rule_table);
        old_page = nullptr;
    }
    if (state == TabStateEmpty) {
        old_page = nullptr;
        emit_tf_disable_crawler_rule_table();
        set_current_open_tab_index(-1);
        set_current_open_tab_highlight_index(-1);
        set_current_preview_tab_index(-1);
        set_current_tab_search_highlight_index(-1);
        set_current_tab_state(TabStateEmpty);
        set_current_tab_webpage(nullptr);
        set_welcome_page_visible(true);
        FindTextState state = current_webpage_find_text_state();
        state.visiable = false;
        set_current_webpage_find_text_state(state);
        set_address_bar_title("");
        set_address_bar_load_progress(0);
        set_current_webpage_crawler_rule_table(CrawlerRuleTable_::create());
        return -1;
    }
    emit_tf_enable_crawler_rule_table();
    if (i < 0) { i = 0; }
    Webpage_ page = nullptr;
    if (state == TabStateOpen) {
        if (i >= open_tabs()->count()) { i = open_tabs()->count() - 1; }
        page = open_tabs()->webpage_(i);
        set_current_open_tab_index(i);
        set_current_open_tab_highlight_index(i);
        set_current_tab_search_highlight_index(-1);
        set_current_preview_tab_index(-1);
    } else if (state == TabStatePreview) {
        if (i >= preview_tabs()->count()) { i = preview_tabs()->count() - 1; }
        page = preview_tabs()->webpage_(i);
        set_current_preview_tab_index(i);
        set_current_open_tab_index(-1);
        set_current_open_tab_highlight_index(-1);
    }
    Q_ASSERT(page != nullptr);
    set_current_tab_state(state);
    set_current_tab_webpage(page.get());
    set_address_bar_load_progress(page->load_progress());
    set_address_bar_title(page->title()); // title must be set after uri
    set_current_webpage_find_text_state(page->find_text_state());
    set_welcome_page_visible(false);
    set_current_webpage_crawler_rule_table(page->crawler_rule_table());
    if (current_tab_search_word().isEmpty()) {
        Global::searchDB->searchAsync(page->url().domain());
    }
    // set up load progress watcher
    QObject::connect(page.get(), &Webpage::load_progress_changed, this, &Controller::set_address_bar_load_progress);
    QObject::connect(page.get(), &Webpage::title_changed, this, &Controller::set_address_bar_title);
    QObject::connect(page.get(), &Webpage::find_text_state_changed, this, &Controller::set_current_webpage_find_text_state);
    QObject::connect(page.get(), &Webpage::crawler_rule_table_changed, this, &Controller::set_current_webpage_crawler_rule_table);
    old_page = page;
    return i;
}

Url Controller::address_bar_url()
{
    if (current_tab_webpage()) {
        return current_tab_webpage()->url();
    }
    return Url();
}

int Controller::closeTab(TabState state, int index)
{
    qInfo(ControllerLogging) << "BrowserController::closeTab" << state << index;
    if (state == TabStateOpen) {
        if (current_tab_state() == TabStateOpen) {
            if (current_open_tab_index() == index) {
                // when removing current tab
                // if there's one after, open that
                if (index + 1 < open_tabs()->count()) {
                    open_tabs()->removeTab(index);
                    viewTab(TabStateOpen, index);
                } // if there's one before, open that
                else if (index >= 1) {
                    open_tabs()->removeTab(index);
                    viewTab(TabStateOpen, index - 1);
                } // if this is the only one
                else {
                    open_tabs()->removeTab(index);
                    viewTab(TabStateEmpty, -1);
                }
            } else if (current_open_tab_index() > index) {
                open_tabs()->removeTab(index);
                viewTab(TabStateOpen, current_open_tab_index() - 1);
            } else {
                open_tabs()->removeTab(index);
            }
            preview_tabs()->clear();
            set_current_tab_search_highlight_index(-1);
            set_current_preview_tab_index(-1);
        } else if (current_tab_state() == TabStatePreview) {
            open_tabs()->removeTab(index);
        }
        saveLastOpen();
    } else if (state == TabStatePreview) {
        if (open_tabs()->count() > 0) {
            viewTab(TabStateOpen, 0);
        } else {
            viewTab(TabStateEmpty, -1);
        }
        // at the moment there is only one way to close a preview tab:
        // use ctrl+w when current view is a preview. in this case we
        // assume the user wants to close all preview tabs
        preview_tabs()->clear();
        set_current_tab_search_highlight_index(-1);
        set_current_preview_tab_index(-1);
    }
    return 0;
}

int Controller::closeTab(TabState state, Url const& uri)
{
    qInfo(ControllerLogging) << "BrowserController::closeTab" << state << uri;
    int idx = -1;
    if (state == TabStateOpen) {
        idx = open_tabs()->findTab(uri);
    } else if (state == TabStatePreview) {
        idx = preview_tabs()->findTab(uri);
    }
    return closeTab(state, idx);
}

int Controller::closeTab()
{
    qInfo(ControllerLogging) << "Controller::closeTab()";
    if (current_tab_state() == TabState::TabStateEmpty)
    {
        qDebug(ControllerLogging) << "No current tab";
        return 0;
    }
    if (current_tab_state() == TabState::TabStateOpen)
    {
        Q_ASSERT(current_open_tab_index() >= 0);
        closeTab(TabState::TabStateOpen, current_open_tab_index());
        return 0;
    }
    if (current_tab_state() == TabState::TabStatePreview)
    {
        Q_ASSERT(current_preview_tab_index() >= 0);
        closeTab(TabState::TabStatePreview, current_preview_tab_index());
        return 0;
    }
    return 0;
}

int Controller::loadLastOpen()
{
    QVariantList contents = FileManager::readDataJsonFileA("open.json");
    qInfo(ControllerLogging) << "BrowserController::loadLastOpen";
    Webpage_List tabs;
    for (const QVariant& item : contents) {
        tabs << Webpage::fromQVariantMap(item.value<QVariantMap>());
    }
    open_tabs()->replaceModel(tabs);
    if (open_tabs()->count() > 0) {
        viewTab(TabStateOpen, 0);
    }
    return 0;
}

int Controller::saveLastOpen()
{
    QVariantList contents;
    int count = open_tabs()->count();
    for (int i = 0; i < count; i++) {
        contents << open_tabs()->webpage_(i)->toQVariantMap();
    }
    FileManager::writeDataJsonFileA("open.json", contents);
    return 0;
}

int Controller::moveTab(TabState fromState, int fromIndex, TabState toState, int toIndex)
{
    qInfo(ControllerLogging) << "BrowserController::moveTab" << fromState << fromIndex << toState << toIndex;
    if (fromIndex < 0 || toIndex < 0
            || fromIndex > open_tabs()->count()
            || toIndex > open_tabs()->count()
            || fromIndex == toIndex - 1
            || fromIndex == toIndex) {
        return 0;
    }
    if (fromState == TabStateOpen && toState == TabStateOpen) {
        open_tabs()->moveTab(fromIndex, toIndex);
        if (fromIndex <= toIndex) {
            viewTab(toState, toIndex - 1);
        } else {
            viewTab(toState, toIndex);
        }
    } else if (fromState == TabStateOpen && toState == TabStateOpen) {
        Global::searchDB->search_result()->moveTab(fromIndex, toIndex);
    }
    return 0;
}

int Controller::currentTabWebpageGo(QString const& u)
{
    qInfo(ControllerLogging) << "BrowserController::currentTabWebpageGo" << u;
    hideCrawlerRuleTable();
    Webpage* p = current_tab_webpage();
    if (p && current_tab_state() == TabStateOpen) {
        p->go(u);
        Global::crawler->crawlAsync(p->url());
        if (current_tab_search_word().isEmpty()) {
            Global::searchDB->searchAsync(p->url().domain());
        }
    } else {
        newTab(TabStateOpen, Url::fromAmbiguousText(u), WhenCreatedViewNew, WhenExistsOpenNew);
    }
    saveLastOpen();
    return 0;
}


int Controller::currentTabWebpageBack()
{
    qInfo(ControllerLogging) << "BrowserController::currentTabWebpageBack";
    hideCrawlerRuleTable();
    Webpage* p = current_tab_webpage();
    if (p) {
        emit p->emit_tf_back();
        p->findClear();
        if (current_tab_search_word().isEmpty()) {
            Global::searchDB->searchAsync(p->url().domain());
        }
    } else {
        qInfo(ControllerLogging) << "no current tab";
    }
    saveLastOpen();
    return 0;
}

int Controller::currentTabWebpageForward()
{
    qInfo(ControllerLogging) << "BrowserController::currentTabWebpageForward";
    hideCrawlerRuleTable();
    Webpage* p = current_tab_webpage();
    if (p) {
        emit p->emit_tf_forward();
        p->findClear();
        if (current_tab_search_word().isEmpty()) {
            Global::searchDB->searchAsync(p->url().domain());
        }
    } else {
        qInfo(ControllerLogging) << "no current tab";
    }
    saveLastOpen();
    return 0;
}

int Controller::currentTabWebpageStop()
{
    qInfo(ControllerLogging) << "BrowserController::currentTabWebpageStop";
    hideCrawlerRuleTable();
    Webpage* p = current_tab_webpage();
    if (p) {
        emit p->emit_tf_stop();
    } else {
        qInfo(ControllerLogging) << "no current tab";
    }
    return 0;
}

int Controller::currentTabWebpageRefresh()
{
    qInfo(ControllerLogging) << "BrowserController::currentTabWebpageRefresh";
    hideCrawlerRuleTable();
    Webpage* p = current_tab_webpage();
    if (p) {
        emit p->emit_tf_refresh();
        p->findClear();
        Global::crawler->crawlAsync(p->url());
        if (current_tab_search_word().isEmpty()) {
            Global::searchDB->searchAsync(p->url().domain());
        }
    } else {
        qInfo(ControllerLogging) << "no current tab";
    }
    return 0;
}

bool Controller::updateWebpageUrl(Webpage_ wp, Url const& url)
{
    qInfo(ControllerLogging) << "Controller::updateWebpageUrl" << wp << url;
    wp->updateUrl(url);
    Global::crawler->crawlAsync(wp->url());
    if (current_tab_webpage() != nullptr
            && wp.get() == current_tab_webpage()
            && current_tab_search_word().isEmpty())
    {
        hideCrawlerRuleTable();
        Global::searchDB->searchAsync(wp->url().domain());
    }
    saveLastOpen();
    return true;
}

bool Controller::updateWebpageTitle(Webpage_ wp, QString const& title)
{
    qInfo(ControllerLogging) << "Controller::updateWebpageTitle" << wp << title;
    wp->updateTitle(title);
    return true;
}

bool Controller::updateWebpageProgress(Webpage_ wp, float progress)
{
    qInfo(ControllerLogging) << "Controller::updateWebpageProgress" << wp << progress;
    wp->updateProgress(progress);
    return true;
}


bool Controller::updateWebpageFindTextFound(Webpage_ wp, int found)
{
    qInfo(ControllerLogging) << "Controller::updateFindTextFound" << wp << found;
    wp->updateFindTextFound(found);
    return true;
}

void Controller::clearPreviews()
{
    if (current_tab_state() == TabStatePreview) {
        if (open_tabs()->count() > 0) {
            viewTab(TabStateOpen, 0);
        } else {
            viewTab(TabStateEmpty, -1);
        }
    }
    preview_tabs()->replaceModel(Webpage_List());
}

int Controller::currentTabWebpageFindTextNext(QString const& txt)
{
    if (current_tab_webpage()) {
        current_tab_webpage()->findNext(txt);
    } else {
        qInfo(ControllerLogging) << "no current tab";
    }
    return 0;
}

int Controller::currentTabWebpageFindTextPrev(QString const& txt)
{
    if (current_tab_webpage()) {
        current_tab_webpage()->findPrev(txt);
    } else {
        qInfo(ControllerLogging) << "no current tab";
    }
    return 0;
}

int Controller::currentTabWebpageFindTextShow()
{
    if (current_tab_webpage()) {
        FindTextState state = current_webpage_find_text_state();
        state.visiable = true;
        current_tab_webpage()->set_find_text_state(state);
    } else {
        qInfo(ControllerLogging) << "no current tab";
    }
    return 0;
}

int Controller::currentTabWebpageFindTextHide()
{
    if (current_tab_webpage()) {
        FindTextState state = current_webpage_find_text_state();
        state.visiable = false;
        current_tab_webpage()->set_find_text_state(state);
    } else {
        qInfo(ControllerLogging) << "no current tab";
    }
    return 0;
}

//bool Controller::currentTabWebpageCrawlerRuleTableEnableRule(CrawlerRule rule)
//{
//    if (! current_tab_webpage()) {
//        qInfo(ControllerLogging) << "no current tab";
//        return false;
//    }
//    if (! current_tab_webpage()->crawlerRuleTableEnableRule(rule))
//    {
//        emit_tf_show_crawler_rule_table_row_hint(current_webpage_crawler_rule_table()->rulesCount());
//        return false;
//    }
//    emit_tf_hide_crawler_rule_table_row_hint();
//    Global::crawler->updateRulesFromSettingsAsync();
//    current_tab_webpage()->crawler_rule_table()->writePartialTableToSettings();
//    return true;
//}


//bool Controller::currentTabWebpageCrawlerRuleTableDisableRule(CrawlerRule rule)
//{
//    if (! current_tab_webpage()) {
//        qInfo(ControllerLogging) << "no current tab";
//        return false;
//    }
//    if (! current_tab_webpage()->crawlerRuleTableDisableRule(rule))
//    {
//        emit_tf_show_crawler_rule_table_row_hint(current_webpage_crawler_rule_table()->rulesCount());
//        return false;
//    }
//    emit_tf_hide_crawler_rule_table_row_hint();
//    Global::crawler->updateRulesFromSettingsAsync();
//    current_tab_webpage()->crawler_rule_table()->writePartialTableToSettings();
//    return true;
//}


bool Controller::currentTabWebpageCrawlerRuleTableInsertRule(CrawlerRule rule)
{
    qInfo(ControllerLogging) << "Controller::currentTabWebpageCrawlerRuleTableInsertRule" << rule;
    if (! current_tab_webpage()) {
        qInfo(ControllerLogging) << "no current tab";
        return false;
    }
    if (! current_tab_webpage()->crawlerRuleTableInsertRule(rule)) {
        emit_tf_show_crawler_rule_table_row_hint(current_webpage_crawler_rule_table()->rulesCount());
        return false;
    }
    emit_tf_hide_crawler_rule_table_row_hint();
    current_tab_webpage()->crawler_rule_table()->writePartialTableToSettings();
    Global::crawler->updateRulesFromSettingsAsync();
    return true;
}


bool Controller::currentTabWebpageCrawlerRuleTableRemoveRule(int idx)
{
    if (! current_tab_webpage()) {
        qInfo(ControllerLogging) << "no current tab";
        return false;
    }
    emit_tf_hide_crawler_rule_table_row_hint();
    if (! current_tab_webpage()->crawlerRuleTableRemoveRule(idx)) {
        return false;
    }
    current_tab_webpage()->crawler_rule_table()->writePartialTableToSettings();
    Global::crawler->updateRulesFromSettingsAsync();
    return true;
}

bool Controller::currentTabWebpageCrawlerRuleTableModifyRule(int old, CrawlerRule modified)
{
    if (! current_tab_webpage()) {
        qInfo(ControllerLogging) << "no current tab";
        return false;
    }
    if (! current_tab_webpage()->crawlerRuleTableModifyRule(old, modified)) {
        emit_tf_show_crawler_rule_table_row_hint(old);
        return false;
    }
    emit_tf_hide_crawler_rule_table_row_hint();
    current_tab_webpage()->crawler_rule_table()->writePartialTableToSettings();
    Global::crawler->updateRulesFromSettingsAsync();
    return true;
}

int Controller::hideCrawlerRuleTable()
{
    emit_tf_hide_crawler_rule_table_row_hint();
    emit_tf_hide_crawler_rule_table();
    return 0;
}

int Controller::showCrawlerRuleTable()
{
    qInfo(ControllerLogging) << "Controller::showCrawlerRuleTable";
    if (! current_tab_webpage()) {
        qCritical(ControllerLogging) << "Controller::showCrawlerRuleTable no current tab";
        return false;
    }
    current_tab_webpage()->crawlerRuleTableReloadFromSettings();
    emit_tf_show_crawler_rule_table();
    return true;
}

int Controller::searchTabs(QString const& words)
{
    if (words == current_tab_search_word()) { return -1; }
    set_current_tab_search_word(words);
    if (words.isEmpty() && current_tab_webpage() != nullptr) {
        Global::searchDB->searchAsync(current_tab_webpage()->url().domain());
        return 0;
    }
    Global::searchDB->searchAsync(words);
    return 0;
}