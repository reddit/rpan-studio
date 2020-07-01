#include "window-reddit-start-stream2.hpp"

#include <QMovie>
#include <QUrl>
#include <json11.hpp>

#include "api-reddit.hpp"
#include "obs-app.hpp"
#include "qt-wrappers.hpp"
#include "window-basic-main.hpp"

using namespace std;
using namespace json11;

#define PAGE_CONFIG 0
#define PAGE_WAITING 1
#define PAGE_FAILURE 2

namespace {
const char *CONF_SECTION = "Reddit";

const char *CONF_STREAM_TITLE = "StreamTitle";
const char *CONF_LAST_SUBREDDIT = "LastSubreddit";
const char *CONF_BROADCAST_ID = "BroadcastId";
}

RedditStartStreamDialog2::RedditStartStreamDialog2(QWidget *parent)
	: QDialog(parent),
	  ui(new Ui::RedditStartStreamDialog2)
{
	ui->setupUi(this);

	ui->errorLabel->setText("");
	ui->errorLabel->setProperty("themeID", "error");
	ui->failureLabel->setProperty("themeID", "error");

	connect(ui->titleEdit, SIGNAL(textChanged(const QString&)), this,
	        SLOT(UpdateButtons()));
	connect(ui->titleEdit, SIGNAL(returnPressed()), this,
	        SLOT(CreateStream()));
	connect(ui->subredditCombo, SIGNAL(currentTextChanged(const QString &)),
	        this, SLOT(UpdateButtons()));
	connect(ui->subredditCombo, SIGNAL(editTextChanged(const QString &)),
	        this, SLOT(UpdateButtons()));
	connect(ui->subredditCombo->lineEdit(), SIGNAL(returnPressed()), this,
	        SLOT(CreateStream()));
	connect(ui->startButton, SIGNAL(clicked()), this,
	        SLOT(CreateStream()));

	LoadSubreddits();
}

RedditStartStreamDialog2::~RedditStartStreamDialog2()
{
	if (loadSubredditsThread != nullptr) {
		loadSubredditsThread->requestInterruption();
		loadSubredditsThread->wait();
	}

	if (createStreamThread != nullptr) {
		createStreamThread->requestInterruption();
		createStreamThread->wait();
	}
}

void RedditStartStreamDialog2::LoadSubreddits()
{
	auto *thread = RedditApi::FetchSubreddits();
	connect(thread, &RemoteTextThread::Result, this,
	        &RedditStartStreamDialog2::OnLoadSubreddits);
	if (loadSubredditsThread != nullptr) {
		loadSubredditsThread->requestInterruption();
		loadSubredditsThread->wait();
	}
	loadSubredditsThread.reset(thread);
	loadSubredditsThread->start();

	SetPage(PAGE_WAITING);
	ui->subtitle->setText(
		Str("Reddit.StartStream.Loading.Subreddits.Message"));
}

void RedditStartStreamDialog2::CreateStream()
{
	if (!ui->startButton->isEnabled()) {
		return;
	}

	QString title = ui->titleEdit->text().trimmed();
	QString subreddit = ui->subredditCombo->currentText().trimmed();

	auto *main = OBSBasic::Get();
	config_set_string(main->Config(), CONF_SECTION, CONF_STREAM_TITLE,
	                  title.toStdString().c_str());
	config_set_string(main->Config(), CONF_SECTION, CONF_LAST_SUBREDDIT,
	                  subreddit.toStdString().c_str());

	title = QUrl::toPercentEncoding(title);
	auto *thread = RedditApi::CreateStream(title.toStdString(),
	                                       subreddit.toStdString());
	connect(thread, &RemoteTextThread::Result, this,
	        &RedditStartStreamDialog2::OnCreateStream);
	if (createStreamThread != nullptr) {
		createStreamThread->requestInterruption();
		createStreamThread->wait();
	}
	createStreamThread.reset(thread);
	createStreamThread->start();

	SetPage(PAGE_WAITING);
	ui->subtitle->setText(Str("Reddit.StartStream.Starting.Message"));
}

void RedditStartStreamDialog2::LoadDynamicConfig()
{
	auto *thread = RedditApi::FetchConfig();
	connect(thread, &RemoteTextThread::Result, this,
	        &RedditStartStreamDialog2::OnDynamicConfig);
	loadDynamicConfigThread.reset(thread);
	loadDynamicConfigThread->start();
}

void RedditStartStreamDialog2::SetPage(int page)
{
	ui->stackedWidget->setCurrentIndex(page);

	switch (page) {
	case PAGE_CONFIG:
		ui->subtitle->setText(Str("Reddit.StartStream.Subtitle"));
		ui->titleEdit->setFocus();
		UpdateButtons();
		break;
	case PAGE_WAITING:
		ui->startButton->setEnabled(false);
		break;
	case PAGE_FAILURE:
		ui->subtitle->setText(
			Str("Reddit.StartStream.Error.Failure.Title"));
		break;
	}
}

void RedditStartStreamDialog2::OnLoadSubreddits(const QString &text,
                                                const QString &error,
                                                const QStringList &)
{
	string response = QT_TO_UTF8(text);
	string parseError;
	Json json = Json::parse(response, parseError);

	if (!json.is_object()) {
		// something went really wrong. try one more time.
		if (!retried) {
			retried = true;
			LoadSubreddits();
		}
		return;
	}

	bool success = json["status"].string_value() == "success";

	if (!success) {
		string message = json["status_message"].string_value();
		if (message.empty()) {
			message = json["status"].string_value();
		}

		if(!retried && message == "Cannot recommend prompts") {
			retried = true;
			LoadSubreddits();
			return;
		}

		blog(LOG_DEBUG, "Failed to load subreddits: text=%s; error=%s",
		     text.toStdString().c_str(), error.toStdString().c_str());

		SetPage(PAGE_CONFIG);
		ui->errorLabel->setText(QString::fromStdString(message));
		return;
	}

	retried = false;

	const Json::array &subreddits = json["data"].array_items();

	QStringList subredditList;
	for (const Json &subreddit : subreddits) {
		string name = subreddit["subreddit_name"].string_value();
		subredditList.append(QString::fromUtf8(name.c_str()));
	}

	ui->subredditCombo->addItems(subredditList);

	auto *main = OBSBasic::Get();

	const char *previousTitle = config_get_string(
		main->Config(), CONF_SECTION, CONF_STREAM_TITLE);
	if (previousTitle) {
		ui->titleEdit->setText(QString(previousTitle));
	}
	const char *previousSubreddit = config_get_string(
		main->Config(), CONF_SECTION, CONF_LAST_SUBREDDIT);
	if (previousSubreddit) {
		ui->subredditCombo->lineEdit()->setText(
			QString(previousSubreddit));
	}

	SetPage(PAGE_CONFIG);
}

void RedditStartStreamDialog2::OnCreateStream(const QString &text,
                                              const QString &,
                                              const QStringList &)
{
	string result = QT_TO_UTF8(text);
	string parseError;
	Json json = Json::parse(result, parseError);
	Json responseData = json["data"];

	bool success = json["status"].string_value() == "success";
	bool canBroadcast = responseData["can_broadcast"].bool_value();
	string message = responseData["status_message"].string_value();

	if (!canBroadcast && !success) {
		if (message.empty()) {
			message = json["status"].string_value();
		}

		ui->errorLabel->setText(QString::fromStdString(message));
		SetPage(PAGE_CONFIG);

		return;
	}

	auto *main = OBSBasic::Get();

	string rtmpUrl = responseData["rtmp_url"].string_value();
	rtmpUrl = rtmpUrl.substr(0, rtmpUrl.find_last_of('/'));
	string streamKey = responseData["streamer_key"].string_value();
	string broadcastId = responseData["video_id"].string_value();
	if (streamKey.empty()) {
		setResult(0);
		hide();
	}

	auto *service = main->GetService();
	obs_data_t *settings = obs_service_get_settings(service);
	obs_data_set_string(settings, "server", rtmpUrl.c_str());
	obs_data_set_string(settings, "key", streamKey.c_str());
	obs_service_update(service, settings);
	obs_data_release(settings);

	config_set_string(main->Config(),
	                  CONF_SECTION,
	                  CONF_BROADCAST_ID,
	                  broadcastId.c_str());

	LoadDynamicConfig();
	ui->subtitle->setText(Str("Reddit.StartStream.Configuring"));
}

void RedditStartStreamDialog2::OnDynamicConfig(const QString &text,
                                               const QString &,
                                               const QStringList &)
{
	string result = QT_TO_UTF8(text);
	string parseError;
	Json json = Json::parse(result, parseError);

	Json globalConfig = json["global"];
	int fps = globalConfig["broadcast_fps"].number_value();
	int videoBitrate =
		globalConfig["broadcast_max_video_bitrate"].number_value();
	int maxKeyframeInterval =
		globalConfig["broadcast_max_keyframe_interval"].number_value();
	int audioBitrate =
		globalConfig["broadcast_max_audio_bitrate"].number_value();

	auto *main = OBSBasic::Get();
	auto *config = main->Config();
	config_set_string(config, "Output", "Mode", "Advanced");
	config_set_default_int(config, "Video", "FPSType", 1);
	config_set_int(config, "Video", "FPSInt", fps);
	char track[] = "TrackXBitrate";
	for (int i = 1; i <= 6; i++) {
		sprintf(track, "Track%dBitrate", i);
		config_set_int(config, "AdvOut", track, audioBitrate);
	}

	Json streamEncoder = Json::object{{"bitrate", videoBitrate},
	                                  {"keyint_sec", maxKeyframeInterval},
	                                  {"profile", "baseline"},
	                                  {"rate_control", "ABR"}};
	string streamEncoderJson = streamEncoder.dump();
	char jsonPath[512];
	if (GetProfilePath(jsonPath,
	                   sizeof(jsonPath),
	                   "streamEncoder.json") > 0) {
		os_quick_write_utf8_file_safe(jsonPath,
		                              streamEncoderJson.c_str(),
		                              streamEncoderJson.length(), false,
		                              "tmp", "bak");
	}

	accept();
}

void RedditStartStreamDialog2::UpdateButtons()
{
	bool hasTitle = ui->titleEdit->text().trimmed().length() > 0;
	bool hasSubreddit = ui->subredditCombo->currentText()
	                      .trimmed().length() > 0;

	ui->startButton->setEnabled(hasTitle && hasSubreddit);
}
