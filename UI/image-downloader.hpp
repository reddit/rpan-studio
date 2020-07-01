#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QTextDocument>

class ImageDownloader : public QObject {
Q_OBJECT

public:
	ImageDownloader(QTextDocument *document, QObject *parent = nullptr);

	QString Download(const QString &url);

	void Reset();

private:
	void LoadImage(const QString &path, const QString &name);
	QString GetCachePath(const QString &name);

	QNetworkAccessManager *network;
	QMap<QNetworkReply *, QString> requests;
	QTextDocument *document;
	QSet<QString> loaded;

private slots:
	void RequestFinished(QNetworkReply *reply);
};
