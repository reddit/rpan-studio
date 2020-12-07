#include "panel-reddit-chat.hpp"

#include <QTextTable>
#include <QScrollBar>
#include <QAbstractSocket>
#include <QTextDocumentFragment>
#include <QObject>
#include <json11.hpp>

#include "api-reddit.hpp"
#include "obs-frontend-api.h"
#include "panel-reddit-ama.hpp"
#include "qt-wrappers.hpp"
#include "remote-text.hpp"
#include "window-basic-main.hpp"

#define PAGE_CHAT 0
#define PAGE_OFFLINE 1
#define PAGE_CONNECTING 2

using namespace std;
using namespace json11;

namespace {

void on_frontend_event(enum obs_frontend_event event, void *param);

const int avatarCount = 20;
const vector<QString> avatarColors = {
	"FF4500", "0DD3BB", "24A0ED", "FFB000", "FF8717", "46D160", "25B79F",
	"0079D3", "4856A3", "C18D42", "A06A42", "46A508", "008985", "7193FF",
	"7E53C1", "FFD635", "DDBD37", "D4E815", "94E044", "FF66AC", "DB0064",
	"FF585B", "EA0027", "A5A4A4", "545452",
};

}

RedditChatPanel::RedditChatPanel(QWidget *parent)
	: QDockWidget(parent),
	  ui(new Ui::RpanChatPanel)
{
	ui->setupUi(this);

	ui->disconnectedWarningLbl->setProperty("themeID", "error");

	connect(ui->sendButton, SIGNAL(clicked()), this, SLOT(PostComment()));
	connect(ui->chatInputEdit, SIGNAL(returnPressed()), this,
	        SLOT(PostComment()));

	connect(&websocket, &QWebSocket::connected, this,
	        &RedditChatPanel::WebsocketConnected);
	connect(&websocket, &QWebSocket::disconnected, this,
	        &RedditChatPanel::WebsocketDisconnected);
	connect(&websocket,
	        QOverload<const QList<QSslError> &>::of(&QWebSocket::sslErrors),
	        this, &RedditChatPanel::WebsocketSSLError);
	connect(&websocket,
	        QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
	        this, &RedditChatPanel::WebsocketError);
	connect(&websocket, &QWebSocket::textMessageReceived, this,
	        &RedditChatPanel::MessageReceived);

	imageDownloader = new ImageDownloader(ui->chatOutput->document(), this);

	obs_frontend_add_event_callback(on_frontend_event, this);

	tableFormat.setBorder(0);

	auto *main = OBSBasic::Get();
	if (main->StreamingActive()) {
		SetPage(PAGE_CHAT);
	} else {
		SetPage(PAGE_OFFLINE);
	}
}

RedditChatPanel::~RedditChatPanel()
{
	obs_frontend_remove_event_callback(on_frontend_event, this);

	if (apiThread != nullptr) {
		apiThread->quit();
		apiThread->wait();
	}

	if (fetchCommentsThread != nullptr) {
		fetchCommentsThread->quit();
		fetchCommentsThread->wait();
	}

	if (postCommentThread != nullptr) {
		postCommentThread->quit();
		postCommentThread->wait();
	}
}

void RedditChatPanel::SetPage(int page)
{
	ui->stackedWidget->setCurrentIndex(page);

	switch (page) {
	case PAGE_CHAT: {
		ui->chatOutput->setEnabled(true);
		ui->chatInputEdit->setEnabled(true);
		ui->sendButton->setEnabled(true);
		break;
	}
	case PAGE_OFFLINE:
		connected = false;
		websocket.close();
		imageDownloader->Reset();
		break;
	case PAGE_CONNECTING:
		ui->chatOutput->clear();
		imageDownloader->Reset();
		ui->chatOutput->setEnabled(false);
		ui->chatInputEdit->setEnabled(false);
		ui->sendButton->setEnabled(false);
		ui->connectingLabel->setText(
			connected ? QTStr("Reddit.Panel.Chat.Reconnecting")
				: QTStr("Reddit.Panel.Chat.Connecting"));
		ui->disconnectedWarningLbl->setText(
			connected ? QTStr("Reddit.Panel.Chat.Disconnected")
				: "");

		auto *main = OBSBasic::Get();
		auto *broadcastId = config_get_string(main->Config(), "Reddit",
			"BroadcastId");

		auto *thread = RedditApi::FetchStream(broadcastId);
		connect(thread, &RemoteTextThread::Result, this,
		        &RedditChatPanel::OnLoadStream);
		apiThread.reset(thread);
		apiThread->start();
		break;
	}
}

#define FORMAT_BOLD 1
#define FORMAT_ITALICS 2

void RedditChatPanel::ParseComment(const string &author,
                                   const string &authorId,
                                   Json document,
                                   const string &awardIcon)
{
	vector<Json> items = document.array_items();
	vector<string> blocks;
	for (Json item : items) {
		vector<Json> contentItems = item["c"].array_items();
		string blockText;
		for (Json contentItem : contentItems) {
			string type = contentItem["e"].string_value();
			string text = contentItem["t"].string_value();

			if (type == "link") {
				string url = contentItem["u"].string_value();
				if (url[0] == '/') {
					url = "https://www.reddit.com" + url;
				}
				text = "<a href='" + url + "'>" + text + "</a>";
			} else if (type == "r/") {
				text = "<a href='https://www.reddit.com/r/" +
				       text + "'>r/" + text + "</a>";
			}

			if (contentItem["f"].is_array()) {
				Json format = contentItem["f"].array_items()[0];

				vector<Json> settings = format.array_items();
				int formatType = settings[0].int_value();
				int formatStart = settings[1].int_value();
				int formatEnd = settings[2].int_value();

				string leftString;
				if (formatStart > 0) {
					leftString = text.
						substr(0, formatStart);
				}
				string midString = text.substr(
					formatStart, formatEnd);
				string rightString;
				if (formatStart + formatEnd < text.length()) {
					rightString = text.substr(
						formatStart + formatEnd);
				}

				switch (formatType) {
				case FORMAT_BOLD:
					midString = "<b>" + midString + "</b>";
					break;
				case FORMAT_ITALICS:
					midString = "<i>" + midString + "</i>";
					break;
				}

				blockText +=
					"<span>" +
					leftString +
					midString +
					rightString +
					"</span>";
			} else {
				blockText += text;
			}
		}

		QString qAuthor = QString::fromStdString(author);
		QString body = QString::fromStdString(blockText).remove(
			QRegExp("<[^>]*>")).trimmed();
		bool isQuestion = false;
		if (config_get_bool(OBSBasic::Get()->Config(), "Reddit",
		                    "AMAMode")) {
			QString bodyLower = body.toLower();
			if (bodyLower.contains("#ama") ||
			    bodyLower.contains("/ama") ||
			    bodyLower.contains("/q")) {
				body = body.replace("/ama", "",
				                    Qt::CaseInsensitive)
				           .replace(
					           "#ama", "",
					           Qt::CaseInsensitive)
				           .replace(
					           "/q", "",
					           Qt::CaseInsensitive)
				           .trimmed();
				isQuestion = true;
			}
		}
		if (isQuestion) {
			RedditAMAPanel::Model()->addQuestion(body, qAuthor);
		}
		blocks.emplace_back(blockText);
	}

	QTextCursor cursor(ui->chatOutput->textCursor());
	cursor.movePosition(QTextCursor::End);
	QTextTable *table = cursor.insertTable(blocks.size() + 1, 2,
	                                       tableFormat);
	QTextImageFormat avatar;
	avatar.setName(GetAvatar(authorId));
	avatar.setWidth(32);
	avatar.setHeight(32);
	table->cellAt(0, 0).firstCursorPosition().insertImage(avatar);
	table->mergeCells(0, 0, 2, 1);

	table->cellAt(0, 1)
	     .firstCursorPosition()
	     .insertHtml(QString("<b>%1</b>")
		     .arg(QString::fromStdString(author))
		     );

	for (int i = 0; i < blocks.size(); ++i) {
		QTextCursor awardCell = table->cellAt(i + 1, 1)
		                             .firstCursorPosition();
		if (i == 0 && !awardIcon.empty()) {
			QTextImageFormat awardImage;
			awardImage.setName(
				GetImage(QString::fromStdString(awardIcon)));
			awardImage.setWidth(16);
			awardImage.setHeight(16);
			awardCell.insertImage(awardImage);
		}
		awardCell.insertHtml(QString::fromStdString(blocks[i]));
	}

	QScrollBar *bar = ui->chatOutput->verticalScrollBar();
	bar->setValue(bar->maximum());
}

void RedditChatPanel::WebsocketConnected()
{
	blog(LOG_INFO, "Reddit: Connected to chat");
	SetPage(PAGE_CHAT);
	connected = true;
}

void RedditChatPanel::WebsocketDisconnected()
{
	if (ui->stackedWidget->currentIndex() == PAGE_CHAT) {
		blog(LOG_INFO,
		     "Reddit: Lost connection to chat. Reconnecting...");
		SetPage(PAGE_CONNECTING);
	}
}

void RedditChatPanel::WebsocketSSLError(const QList<QSslError> &)
{
	websocket.ignoreSslErrors();
}

void RedditChatPanel::WebsocketError(QAbstractSocket::SocketError error)
{
	blog(LOG_ERROR, "Websocket error: (%d) %s", error,
	     websocket.errorString().toStdString().c_str());

	websocket.close();
	WebsocketDisconnected();
}

void RedditChatPanel::MessageReceived(QString message)
{
	string result = QT_TO_UTF8(message);
	string error;
	Json output = Json::parse(result, error);

	string type = output["type"].string_value();

	if (type != "new_comment") {
		return;
	}

	Json contentJson = output["payload"];

	string author = contentJson["author"].string_value();
	string authorId = contentJson["author_fullname"].string_value();
	string awardIcon;
	if (contentJson["associated_award"].is_object()) {
		vector<Json> icons = contentJson["associated_award"][
			"resized_icons"].array_items();
		for (Json icon : icons) {
			if (icon["width"].int_value() == 32) {
				awardIcon = icon["url"].string_value();
				break;
			}
		}
	}

	ParseComment(author, authorId, contentJson["rtjson"]["document"],
	             awardIcon);
}

void RedditChatPanel::PostComment()
{
	if (!ui->sendButton->isEnabled()) {
		return;
	}

	auto *main = OBSBasic::Get();
	string broadcastId = config_get_string(main->Config(),
	                                       "Reddit",
	                                       "BroadcastId");

	QString comment = ui->chatInputEdit->text().trimmed();
	ui->chatInputEdit->clear();

	if (comment.startsWith("/")) {
		ParseCommand(comment);
		return;
	}

	auto *thread = RedditApi::PostComment(broadcastId,
	                                      comment.toStdString());
	connect(thread, &RemoteTextThread::Result, this,
	        &RedditChatPanel::PostCommentDone);
	postCommentThread.reset(thread);
	postCommentThread->start();

	ui->sendButton->setEnabled(false);
}

void RedditChatPanel::PopulateComments(const QString &text,
                                       const QString &,
                                       const QStringList &)
{
	// output["comments"] -> "linked list" dictionary of comments
	// first comment: output["commentLists"][videoId]["head"]["id"]
	// comment:
	//   author:        comment["author"]
	//   account image: comment["profileImage"]
	//   body:          comment["media"]["richtextContent"]["document"][0]["c"][0]["t"]
	//   award image:   comment["associatedAward"]["icon32"]
	//   next comment:  comment["next"]["id"] (if comment["next"]["type"] == "comment")
	//   
	string result = QT_TO_UTF8(text);
	string error;
	Json output = Json::parse(result, error);

	map<string, Json> commentLists =
		output["commentLists"].object_items();
	Json postComments = commentLists.begin()->second;
	if (postComments["head"]["type"] == "comment") {
		string nextComment = postComments["head"]["id"].string_value();
		Json comments = output["comments"];

		while (!nextComment.empty()) {
			Json comment = comments[nextComment];

			string author = comment["author"].string_value();
			string authorId = comment["authorId"].string_value();
			string awardIcon;
			if (comment["associatedAward"].is_object()) {
				awardIcon =
					comment["associatedAward"]["icon"]
					["url"].string_value();
			}
			ParseComment(
				author,
				authorId,
				comment["media"]["richtextContent"]["document"],
				awardIcon);

			if (comment["next"]["type"] != "comment") {
				nextComment.clear();
			} else {
				nextComment = comment["next"]["id"]
					.string_value();
			}
		}
	}
	// connect to live updates
	websocket.open(QUrl(QString::fromStdString(websocketUrl)));
}

void RedditChatPanel::PostCommentDone(const QString &,
                                      const QString &,
                                      const QStringList &)
{
	ui->sendButton->setEnabled(true);
}

void RedditChatPanel::OnLoadStream(const QString &text,
                                   const QString &,
                                   const QStringList &)
{
	string result = QT_TO_UTF8(text);
	string error;
	Json output = Json::parse(result, error);

	string status = output["status"].string_value();
	if (status != "success") {
		return;
	}

	Json streamData = output["data"];
	Json postData = streamData["post"];

	postId = postData["id"].string_value();
	websocketUrl = postData["liveCommentsWebsocket"]
		.string_value();

	auto *thread = RedditApi::FetchPostComments(postId);
	connect(thread, &RemoteTextThread::Result, this,
	        &RedditChatPanel::PopulateComments);
	fetchCommentsThread.reset(thread);
	fetchCommentsThread->start();
}

QString RedditChatPanel::GetAvatar(const string &authorId)
{
	int id = QString::fromStdString(authorId).split('_')[1].toInt(nullptr,
		36);

	int avatar = (id % avatarCount) + 1;
	QString color = avatarColors[(id / avatarCount) % avatarColors.size()];

	return GetImage(QString(
		                "https://www.redditstatic.com/avatars/avatar_default_%1_%2.png")
	                .arg(avatar, 2, 10, QChar('0'))
	                .arg(color));
}

QString RedditChatPanel::GetImage(const QString &url)
{
	return QString("img://%1").arg(imageDownloader->Download(url));
}

void RedditChatPanel::ParseCommand(const QString &comment)
{
	if (comment.mid(1).startsWith("disconnect")) {
		websocket.close();
	} else {
		QTextCursor cursor(ui->chatOutput->textCursor());
		cursor.movePosition(QTextCursor::End);
		QTextTable *table = cursor.insertTable(1, 1, tableFormat);
		table->cellAt(0, 0).firstCursorPosition().insertHtml(
			QString("<i>Invalid command: %1</i>").arg(
				comment));
	}
}

namespace {

void on_frontend_event(enum obs_frontend_event event, void *param)
{
	auto *panel = static_cast<RedditChatPanel *>(param);

	switch (event) {
	case OBS_FRONTEND_EVENT_STREAMING_STARTED:
		panel->SetPage(PAGE_CONNECTING);
		break;
	case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
		panel->SetPage(PAGE_OFFLINE);
		break;
	}
}

}
