#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QtWebView>
#include "qmlregister.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    // test
    QMLRegister::tabsModel->insertTab(0,QString("https://google.ca"),QString("google"),QString(""));

    QMLRegister::registerToQML();
    QtWebView::initialize();
    QQuickWindow::setSceneGraphBackend(QSGRendererInterface::Software);
    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}