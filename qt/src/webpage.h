#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <QObject>
#include <QString>

class Webpage : public QObject
{
        Q_OBJECT
        Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
        Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY urlChanged)
        Q_PROPERTY(QString storeFile READ storeFile WRITE setStoreFile NOTIFY storeFileChanged)
        Q_PROPERTY(QString html READ html WRITE setHtml NOTIFY htmlChanged)

    public:
        explicit Webpage(QObject *parent = nullptr);
        Webpage(QString url, QString title, QString html);

        QString title() const;
        QString html() const;
        QString url() const;
        QString storeFile() const;
        void setTitle(QString);
        void setHtml(QString);
        void setUrl(QString);
        void setStoreFile(QString);

    signals:
        void titleChanged(QString);
        void urlChanged(QString);
        void storeFileChanged(QString);
        void htmlChanged(QString);

    public slots:

    private:
        QString _title;
        QString _url;
        QString _storeFile;
        QString _html;
};

Q_DECLARE_METATYPE(Webpage*)

#endif // WEBPAGE_H