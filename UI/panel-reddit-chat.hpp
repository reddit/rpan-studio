#pragma once

#include <QtWebSockets/QWebSocket>


#include "image-downloader.hpp"
#include "ui_RedditChatPanel.h"

namespace json11 {
class Json;
}

class RedditChatPanel : public QDockWidget {
Q_OBJECT

public:
	explicit RedditChatPanel(QWidget *parent = nullptr);

	~RedditChatPanel();

	void SetPage(int page);

	QScopedPointer<Ui::RpanChatPanel> ui;

private:
	void ParseComment(const std::string &author, const std::string &authorId, json11::Json document, const std::string &awardIcon = "");

	QString GetAvatar(const std::string &avatarId);
	QString GetImage(const QString &imageUrl);
	void ParseCommand(const QString &comment);

	QWebSocket websocket;
	QScopedPointer<QThread> apiThread;
	QScopedPointer<QThread> fetchCommentsThread;
	QScopedPointer<QThread> postCommentThread;

	std::string websocketUrl;
	std::string postId;
	bool connected = false;

	QTextTableFormat tableFormat;
	ImageDownloader *imageDownloader;

private slots:
	void WebsocketConnected();
	void WebsocketDisconnected();
	void WebsocketSSLError(const QList<QSslError> &errors);
	void WebsocketError(QAbstractSocket::SocketError error);
	void MessageReceived(QString message);
	void PostComment();
	void PopulateComments(const QString &text, const QString &error,
	                      const QStringList &responseHeaders);
	void PostCommentDone(const QString &text, const QString &error,
	                     const QStringList &responseHeaders);
	void OnLoadStream(const QString &text, const QString &error,
	                  const QStringList &responseHeaders);
};
