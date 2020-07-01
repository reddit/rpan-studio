#include "image-downloader.hpp"


#include <QFileDialog>
#include <QImage>
#include <QNetworkReply>

#include "obs-app.hpp"

ImageDownloader::ImageDownloader(QTextDocument *document, QObject *parent)
	: QObject(parent),
	  network(new QNetworkAccessManager(this)),
	  document(document)
{
	connect(network,
	        SIGNAL(finished(QNetworkReply*)),
	        this,
	        SLOT(RequestFinished(QNetworkReply*)));
}

void ImageDownloader::LoadImage(const QString &path, const QString &name)
{
	QImage image;
	image.load(path);
	QUrl resourceUrl = QUrl(QString("img://%1").arg(name));
	document->addResource(QTextDocument::ImageResource, resourceUrl, image);

	loaded.insert(name);
}

QString ImageDownloader::GetCachePath(const QString &name)
{
	QString cachePath = QString(CONFIG_DIR_NAME "/cache/%1").arg(name);
	char path[512];
	GetConfigPath(path, sizeof(path), cachePath.toStdString().c_str());
	
	return QString(path);
}

QString ImageDownloader::Download(const QString &url)
{
	QString name = QCryptographicHash::hash(
		url.toUtf8(), QCryptographicHash::Md5).toHex();

	if (!loaded.contains(name)) {
		 QString path = GetCachePath(name);

		if (QFile::exists(path)) {
			LoadImage(path, name);
		} else {
			QNetworkReply *reply =
				network->get(QNetworkRequest(QUrl(url)));
			requests.insert(reply, url);
		}
	}

	return name;
}

void ImageDownloader::Reset()
{
	loaded.clear();
}

void ImageDownloader::RequestFinished(QNetworkReply *reply)
{
	QString url = requests[reply];
	requests.remove(reply);

	QString name = QCryptographicHash::hash(url.toUtf8(),
	                                        QCryptographicHash::Md5)
		.toHex();

	QString path = GetCachePath(name);
	QFile cacheFile(path);
	cacheFile.open(QFile::WriteOnly);
	cacheFile.write(reply->readAll());
	cacheFile.close();

	LoadImage(path, name);

	reply->deleteLater();
}
