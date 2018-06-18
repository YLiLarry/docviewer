#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlField>
#include <QDebug>
#include <QCoreApplication>
#include <QSqlRelationalTableModel>
#include <QSharedPointer>
#include <QRegularExpression>
#include <QString>
#include <QMap>
#include <QtConcurrent>
#include <algorithm>
#include "searchdb.h"
#include "filemanager.h"
#include "webpage.h"
#include "qmlregister.h"
#include "tabsmodel.h"

SearchDB::SearchDB()
{
}

bool SearchDB::connect() {
    qDebug() << "libraryPaths:" << QCoreApplication::libraryPaths();
    _db = QSqlDatabase::addDatabase("QSQLITE");
    _dbPath = FileManager::dataPath() + "search.db";
    qDebug() << "SearchDB: connecting" << _dbPath;
    _db.setDatabaseName(_dbPath);
    _db.setConnectOptions("QSQLITE_OPEN_READONLY");
    if (! _db.open()) {
        qFatal("SearchDB Error: connection with database failed");
    }
    qDebug() << "SearchDB: connection ok";
    _db.exec("PRAGMA journal_mode=WAL");
    _webpage = QSharedPointer<QSqlRelationalTableModel>::create(nullptr, _db);
    _webpage->setTable("webpage");
    _webpage->setEditStrategy(QSqlTableModel::OnManualSubmit);
    _symbol = QSharedPointer<QSqlRelationalTableModel>::create(nullptr, _db);
    _symbol->setTable("symbol");
    _symbol->setEditStrategy(QSqlTableModel::OnManualSubmit);
    _webpage_symbol = QSharedPointer<QSqlRelationalTableModel>::create(nullptr, _db);
    _webpage_symbol->setTable("webpage_symbol");
    _webpage_symbol->setEditStrategy(QSqlTableModel::OnManualSubmit);
    if (! _webpage->select()) {
        qDebug() << "cannot find table 'webpage'";
    } else {
        qDebug() << "found table 'webpage'";
    };
    if (! _symbol->select()) {
        qDebug() << "cannot find table 'symbol'";
    } else {
        qDebug() << "found table 'symbol'";
    };
    if (! _webpage_symbol->select()) {
        qDebug() << "cannot find table 'webpage_symbol'";
    } else {
        qDebug() << "found table 'webpage_symbol'";
    };
    /* SearchWorker setup */
    _searchWorker = SearchWorker_::create(_db, _searchWorkerThread, *QThread::currentThread());
    _searchWorkerThread.start();
    qRegisterMetaType<Webpage_List>();
    QObject::connect(this, &SearchDB::searchAsync, _searchWorker.data(), &SearchWorker::search);
    QObject::connect(_searchWorker.data(), &SearchWorker::resultChanged, this, &SearchDB::setSearchResult);
    searchAsync("");
    /* UpdateWorker setup */
    _updateWorker = UpdateWorker_::create(_db, _updateWorkerThread, *QThread::currentThread());
    _updateWorkerThread.start();
    QObject::connect(this, &SearchDB::addSymbolsAsync, _updateWorker.data(), &UpdateWorker::addSymbols);
    QObject::connect(this, &SearchDB::addWebpageAsync, _updateWorker.data(), &UpdateWorker::addWebpage);
    QObject::connect(this, &SearchDB::updateSymbolAsync, _updateWorker.data(), &UpdateWorker::updateSymbol);
    QObject::connect(this, &SearchDB::updateWebpageAsync, _updateWorker.data(), &UpdateWorker::updateWebpage);
    return true;
}

//void SearchDB::searchAsync(const QString& words)
//{
//    qDebug() << "SearchDB::searchAsync" << words;
//    emit searchAsyncCalled(words);
//}

void SearchDB::disconnect() {
    _db.close();
    _searchWorkerThread.quit();
    _searchWorkerThread.wait();
    _updateWorkerThread.quit();
    _updateWorkerThread.wait();
    qDebug() << "SearchDB: disconnected";
}

bool UpdateWorker::execScript(QString filename)
{
    QString s = FileManager::readQrcFileS(filename);
    return execMany(s.replace("\n","").split(";", QString::SkipEmptyParts));
}

bool UpdateWorker::execMany(const QStringList& lines)
{
    for (const QString l : lines) {
        qDebug() << "UpdateWorker::execMany" << l;
        QSqlQuery query = _db.exec(l);
        if (query.lastError().isValid()) {
            qCritical() << "UpdateWorker::execMany error when executing"
                        << query.executedQuery()
                        << query.lastError();
            return false;
        }
    }
    return true;
}

bool UpdateWorker::updateWebpage(const QString& url, const QString& property, const QVariant& value)
{
    qDebug() << "UpdateWorker::updateWebpage" << property << value << url;
    QSqlQuery query(_db);
    query.prepare("UPDATE webpage SET " + property + " = ? WHERE url = '" + url + "'");
    query.addBindValue(value);
    if (! query.exec() || query.numRowsAffected() < 1) {
        qCritical() << "UpdateWorker::updateWebpage failed" << url << property << value
                    << query.numRowsAffected()
                    << query.executedQuery()
                    << query.lastError();
        return false;
    } else {
        qDebug() << "UpdateWorker::updateWebpage" << query.executedQuery();
    }
    return true;
}


bool UpdateWorker::updateSymbol(const QString &hash, const QString &property, const QVariant &value)
{
    qDebug() << "SearchDB::updateSymbol" << property << value << hash;
    QSqlQuery query(_db);
    query.prepare("UPDATE symbol SET " + property + " = :value WHERE hash = '" + hash +"'");
    query.bindValue(":value", value);
    if (! query.exec() || query.numRowsAffected() < 1) {
        qCritical() << "UpdateWorker::updateSymbol failed" << hash << property << value
                    << query.numRowsAffected()
                    << query.executedQuery()
                    << query.lastError();
        return false;
    } else {
        qDebug() << "UpdateWorker::updateSymbol" << query.executedQuery();
    }
    return true;
}


bool UpdateWorker::addSymbols(const QString& url, const QVariantMap& symbols)
{
    qDebug() << "UpdateWorker::addSymbols" << url << symbols;
    QSqlQuery query(_db);
    query.prepare("SELECT id FROM webpage WHERE url = :url");
    query.bindValue(":url", url);
    if (! query.exec() || ! query.first() || !query.isValid()) {
        qCritical() << "UpdateWorker::addSymbols didn't find the webpage" << url
                    << query.executedQuery()
                    << query.lastError();
        return false;
    }
    const QVariant wid = query.record().value("id");
    for (auto i = symbols.keyBegin();
         i != symbols.keyEnd();
         i++) {
        QString hash = (*i);
        QString text = symbols[hash].value<QString>();
        query.clear();
        query.prepare("INSERT INTO symbol (hash,text,visited) VALUES (:hash,:text,:visited)");
        query.bindValue(":hash", hash);
        query.bindValue(":text", text);
        query.bindValue(":visited", 0);
        if (query.exec() && query.numRowsAffected() > 0) {
            QVariant sid = query.lastInsertId();
            if (sid.isValid()) {
                query.clear();
                query.prepare("INSERT INTO webpage_symbol (webpage,symbol) VALUES (:webpage,:symbol)");
                query.bindValue(":symbol", sid);
                query.bindValue(":webpage", wid);
                if (query.exec() && query.numRowsAffected() > 0) {
                    qDebug() << "UpdateWorker::addSymbols inserted" << hash << text;
                } else {
                    qDebug() << "UpdateWorker::addSymbols failed to insert into webpage_symbol" << hash << text
                             << query.lastError();
                }
            } else {
                qCritical() << "UpdateWorker::addSymbols datatbase does not support QSqlQuery::lastInsertId()";
                return false;
            }
        } else {
            qCritical() << "UpdateWorker::addSymbols failed to insert to symbol" << hash << text
                        << query.lastError();
        }
    }
    query.clear();
    return true;
}

bool UpdateWorker::addWebpage(const QString& url)
{
    qDebug() << "UpdateWorker::addWebpage" << url;
    QSqlQuery query(_db);
    query.prepare("REPLACE INTO webpage (url, title, visited, html) VALUES (:url,'','',0)");
    query.bindValue(":url", url);
    if (! query.exec()) {
        qCritical() << "ERROR: UpdateWorker::addWebpage failed!" << query.lastError();
        return false;
    };
    return true;
}

bool SearchDB::removeWebpage(const QString& url)
{
    qDebug() << "SearchDB::removeWebpage" << url;
    _webpage->setFilter("url = '" + url + "'");
    _webpage->select();
    if (_webpage->rowCount() == 0) {
        qCritical() << "SearchDB::removeWebpage didn't find the webpage" << url;
        return false;
    }
    const QSqlRecord wpr = _webpage->record(0);
    _webpage_symbol->setFilter("webpage = '" + wpr.value("id").value<QString>() + "'");
    qDebug() << "SearchDB::removeWebpage removing _webpage_symbol where" << "webpage = '" + wpr.value("id").value<QString>() + "'";
    _webpage_symbol->select();
    if (_webpage_symbol->rowCount() > 0) {
        if (! _webpage_symbol->removeRows(0, _webpage_symbol->rowCount())) {
            qCritical() << "SearchDB::removeWebpage couldn't remove row from _webpage_symbol"
                        << _webpage_symbol->lastError();
            goto whenFailed;
        }
    }
    if (! _webpage->removeRows(0, _webpage->rowCount())) {
        qCritical() << "SearchDB::removeWebpage couldn't remove row from _webpage"
                    << _webpage->lastError();
        goto whenFailed;
    }
    _webpage->submitAll();
    return _webpage_symbol->submitAll();
whenFailed:
    qCritical() << "SearchDB::removeWebpage failed";
    _webpage->revertAll();
    _webpage_symbol->revertAll();
    return false;
}

Webpage_ SearchDB::findWebpage_(const QString& url) const
{
    qDebug() << "SearchDB::findWebpage_" << url;
    const QString query = "url = '" + url + "'";
    _webpage->setFilter(query);
    _webpage->select();
    if (_webpage->rowCount() == 0) {
        qCritical() << "SearchDB::findWebpage_ not found!" << url;
        return QSharedPointer<Webpage>(nullptr);
    }
    QSqlRecord r = _webpage->record(0);
    qDebug() << "SearchDB::findWebpage_ found " << r;
    Webpage_ wp = Webpage_::create(url);
    wp->set_title(r.value("title").value<QString>());
    wp->set_visited(r.value("visited").value<int>());
    return wp;
}

QVariantMap SearchDB::findWebpage(const QString& url) const
{
    qDebug() << "SearchDB::findWebpage" << url;
    Webpage_ p = SearchDB::findWebpage_(url);
    if (p.isNull()) {
        qCritical() << "SearchDB::findWebpage not found!" << url;
        return QVariantMap();
    }
    return p->toQVariantMap();
}

bool SearchDB::hasWebpage(const QString& url) const
{
    const QString query = "url = '" + url + "'";
    _webpage->setFilter(query);
    _webpage->select();
    bool b = _webpage->rowCount() > 0;
    qDebug() << "SearchDB::hasWebpage" << url << b;
    return b;
}

SearchWorker::SearchWorker(const QSqlDatabase& db, QThread& _thread, QThread& _qmlThread)
    : _db(QSqlDatabase::cloneDatabase(db, "SearchWorker")), _qmlThread(&_qmlThread)
{
    this->moveToThread(&_thread);
    _db.setConnectOptions("QSQLITE_OPEN_READONLY");
    _db.open();
    _db.exec("PRAGMA journal_mode=WAL");
    qDebug() << "SearchWorker::SearchWorker initialized and moved to thread" << &_thread;
}
SearchWorker::~SearchWorker()
{
    _db.close();
}

UpdateWorker::UpdateWorker(const QSqlDatabase& db, QThread& _thread, QThread& _qmlThread)
    : _db(QSqlDatabase::cloneDatabase(db, "UpdateWorker")), _qmlThread(&_qmlThread)
{
    this->moveToThread(&_thread);
    _db.setConnectOptions();
    _db.open();
    _db.exec("PRAGMA journal_mode=WAL");
    qDebug() << "UpdateWorker::UpdateWorker initialized and moved to thread" << &_thread;
}

UpdateWorker::~UpdateWorker()
{
    if (! execScript("db/exit.sqlite3")) {
        qDebug() << "UpdateWorker::disconnect failed";
    }
    _db.close();
}

void SearchWorker::search(const QString& word)
{
    qDebug() << "SearchWorker::search" << word;
    Webpage_List pages;
    emit resultChanged(pages);
    QStringList ws = word.split(QRegularExpression(" "), QString::SkipEmptyParts);
    QString q;
    if (word == "") {
        q = QStringLiteral("SELECT DISTINCT webpage.id, url, COALESCE(title, '') as title, visited FROM webpage ORDER BY visited LIMIT 50");
    } else {
        if (ws.length() == 0) { return; }
        q = QStringLiteral() +
            "SELECT DISTINCT" +
            "   webpage.id, url, COALESCE(title, '') as title"
            " , CASE WHEN hash IS NULL THEN webpage.visited ELSE symbol.visited END as visited" +
            " , hash, COALESCE(symbol.text,'') as symbol" +
            " FROM webpage" +
            " LEFT JOIN webpage_symbol ON webpage.id = webpage_symbol.webpage" +
            " LEFT JOIN symbol ON symbol.id = webpage_symbol.symbol" +
            " WHERE ";
        for (auto w = ws.begin(); w != ws.end(); w++) {
            q += QStringLiteral() +
                 " (" +
                 "    INSTR(LOWER(symbol.text),LOWER('" + (*w) + "'))" +
                 "    OR INSTR(LOWER(symbol.hash),LOWER('" + (*w) + "'))" +
                 "    OR INSTR(LOWER(webpage.title),LOWER('" + (*w) + "'))" +
                 "    OR INSTR(LOWER(webpage.url),LOWER('" + (*w) + "'))" +
                 " )";
            if (w != ws.end() - 1) {
                q += " AND ";
            }
        }
        q += " ORDER BY visited DESC";
        q += ", CASE WHEN LENGTH(symbol.text) = 0 THEN 99999 ELSE LENGTH(symbol.text) END ASC";
        q += ", CASE WHEN LENGTH(symbol.hash) = 0 THEN 99999 ELSE LENGTH(symbol.hash) END ASC";
        q += ", CASE WHEN LENGTH(webpage.title) = 0 THEN 99999 ELSE LENGTH(webpage.title) END ASC";
        q += ", LENGTH(url) ASC";
        q += " LIMIT 50";
    }
    qDebug() << "SearchWorker::search" << q;
    QSqlQuery r = _db.exec(q);
    if (r.lastError().isValid()) {
        qCritical() << "SearchWorker::search failed" << r.lastError();
        return;
    }
    r.first();
    QRegularExpression searchRegex(ws.join("|"), QRegularExpression::CaseInsensitiveOption);
    QRegularExpression slash("/");
    while (r.isValid()) {
        QSqlRecord record = r.record();
        QString url = record.value("url").value<QString>();
        QStringList path = url.split(slash, QString::SkipEmptyParts);
        QString last = path.length() > 0 ? path[path.length() - 1] : "";
        QString title = record.value("title").value<QString>();
        QString symbol = record.value("symbol").value<QString>();
        QString hash = record.value("hash").value<QString>();
        QString display =
                (symbol.length() > 0 ? "@"+symbol+"  " : "") +
                (hash.length() > 0 ? "#"+hash+"  " : "") +
                "/"+last+"  " +
                (title.length() > 0 ? "\""+title+"\"" : "") +
                (symbol.length() == 0 && hash.length() == 0 && title.length() == 0 ? ""+url+"  " : "");
        Webpage_ wp = Webpage_::create(url);
        wp->_title = title;
        wp->_symbol = symbol;
        wp->_hash = hash;
        wp->_display = display;
        pages << wp;
        wp->moveToThread(_qmlThread);
        r.next();
    }
    emit resultChanged(pages);
    qDebug() << "SearchWorker::search found" << pages.count();
}

QSqlRelationalTableModel* SearchDB::webpageTable() const
{
    return _webpage.data();
}

TabsModel* SearchDB::searchResult()
{
    return &_searchResult;
}

void SearchDB::setSearchResult(const Webpage_List& results)
{
    _searchResult.replaceModel(results);
}










