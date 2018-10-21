#include "logging.hpp"

QtMsgType level = QtCriticalMsg;
QLoggingCategory WebpageLogging("Webpage", level);
QLoggingCategory SearchDBLogging("SearchDB", level);

QLoggingCategory GlobalLogging("Global", level);

QLoggingCategory FileLogging("File", level);

QLoggingCategory CrawlerLogging("Crawler", level);
QLoggingCategory CrawlerRuleLogging("CrawlerRule", level);
QLoggingCategory CrawlerRuleTableLogging("CrawlerRuleTable", level);

QLoggingCategory MacroLogging("Macro", level);

QLoggingCategory ControllerLogging("Controller", level);
