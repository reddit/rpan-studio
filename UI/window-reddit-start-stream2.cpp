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
#define PAGE_INVALID_SETTINGS 3

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
	        SLOT(ValidateConfig()));
	connect(ui->subredditCombo, SIGNAL(currentTextChanged(const QString &)),
	        this, SLOT(UpdateButtons()));
	connect(ui->subredditCombo, SIGNAL(editTextChanged(const QString &)),
	        this, SLOT(UpdateButtons()));
	connect(ui->subredditCombo->lineEdit(), SIGNAL(returnPressed()), this,
	        SLOT(ValidateConfig()));
	connect(ui->startButton, SIGNAL(clicked()), this,
	        SLOT(ValidateConfig()));

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
	QString title = ui->titleEdit->text().trimmed();
	QString subreddit = ui->subredditCombo->currentText().trimmed();

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

	ui->subtitle->setText(Str("Reddit.StartStream.Starting.Message"));
}

void RedditStartStreamDialog2::ValidateConfig()
{
	if (ui->stackedWidget->currentIndex() == PAGE_INVALID_SETTINGS) {
                ui->subtitle->setText(Str("Reddit.StartStream.Configuring"));
		SetPage(PAGE_WAITING);
		UpdateEncoderSettings();
		return;
	}

        if (!ui->startButton->isEnabled()) {
                return;
        }

	SaveStreamSettings();
        LoadDynamicConfig();

	SetPage(PAGE_WAITING);
        ui->subtitle->setText(Str("Reddit.StartStream.Configuring"));
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
	ui->startButton->setText(Str("Reddit.StartStream.Button.StartStream"));

	switch (page) {
	case PAGE_CONFIG:
		ui->subtitle->setText(Str("Reddit.StartStream.Subtitle"));
		ui->titleEdit->setFocus();
		UpdateButtons();
		break;
	case PAGE_INVALID_SETTINGS: {
		ui->startButton->setEnabled(true);

		ui->subtitle->setText(Str("Reddit.StartStream.Configuring.Error.Title"));
		ui->startButton->setText(
			Str("Reddit.StartStream.Configuring.Button.Accept"));
		QPixmap okIcon = QPixmap(QString::fromUtf8(":/res/images/ok.png"));
		QPixmap invalidIcon = QPixmap(QString::fromUtf8(":/res/images/invalid.png"));
                ui->invalidAudioIcon->setPixmap(fixAudioTracks ? invalidIcon : okIcon);
		ui->invalidVideoIcon->setPixmap(fixVideoTracks ? invalidIcon : okIcon);

		QString audioText = QTStr("Basic.Settings.Audio");
		if (fixAudioTracks) {
			audioText = audioText.append(": %1kbps -> %2kbps")
					    .arg(currentAudioBitrate)
					    .arg(maxAudioBitrate);
		}
                ui->invalidAudioLabel->setText(audioText);

                QString videoText = QTStr("Basic.Settings.Video");
		if (fixVideoTracks) {
			videoText = videoText.append(": %1kbps -> %2kbps")
					    .arg(currentVideoBitrate)
					    .arg(maxVideoBitrate);
		}
		ui->invalidVideoLabel->setText(videoText);
		break;
	}
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

	accept();
}

void RedditStartStreamDialog2::OnDynamicConfig(const QString &text,
                                               const QString &,
                                               const QStringList &)
{
	string result = QT_TO_UTF8(text);
	string parseError;
	Json json = Json::parse(result, parseError);

        maxVideoBitrate = 2048;
	maxAudioBitrate = 128;

	// Parse Dynamic Config
	Json globalConfig = json["global"];
	if (globalConfig.is_object()) {
		Json maxVideoBitrateJson =
			globalConfig["broadcast_studio_max_video_bitrate"];
		Json maxAudioBitrateJson =
			globalConfig["broadcast_max_audio_bitrate"];
		Json maxKeyframeIntervalJson =
			globalConfig["broadcast_max_keyframe_interval"];

		if (maxVideoBitrateJson.is_number()) {
			maxVideoBitrate = maxVideoBitrateJson.int_value();
		}
                if (maxAudioBitrateJson.is_number()) {
                        maxAudioBitrate = maxAudioBitrateJson.int_value();
                }
		if (maxKeyframeIntervalJson.is_number()) {
			maxKeyframeInterval = maxKeyframeIntervalJson.int_value();
		}
	}

	// Check existing audio config
	auto *main = OBSBasic::Get();
	auto *config = main->Config();

	char trackBitrateBuf[] = "TrackXBitrate";

	for (int i = 1; i <= 6; ++i) {
		sprintf(trackBitrateBuf, "Track%dBitrate", i);
		currentAudioBitrate = config_get_int(config, "AdvOut", trackBitrateBuf);
		if (currentAudioBitrate > maxAudioBitrate) {
			fixAudioTracks = true;
			break;
		}
	}

	// Check existing video config
	Json currentEncoderConfigJson = LoadEncoderSettings();
	if (currentEncoderConfigJson.is_object()) {
		Json bitrateJson = currentEncoderConfigJson["bitrate"];
		if (bitrateJson.is_number() &&
		    bitrateJson.int_value() > maxVideoBitrate) {
			fixVideoTracks = true;
			currentVideoBitrate = bitrateJson.int_value();
		}
	}

	if (fixAudioTracks || fixVideoTracks) {
		SetPage(PAGE_INVALID_SETTINGS);
	} else {
		CreateStream();
	}
}

void RedditStartStreamDialog2::UpdateEncoderSettings()
{
        auto *main = OBSBasic::Get();
        auto *config = main->Config();

        config_set_string(config, "Output", "Mode", "Advanced");

	// Update audio settings
	char track[] = "TrackXBitrate";
	for (int i = 1; i <= 6; ++i) {
		sprintf(track, "Track%dBitrate", i);
		int bitrate = config_get_int(config, "AdvOut", track);
		if(bitrate > maxAudioBitrate) {
		        config_set_int(config, "AdvOut", track, maxAudioBitrate);
		}
	}

	// Update video settings
        char jsonPath[512];
	string parseError;
        if (GetProfilePath(jsonPath,
                           sizeof(jsonPath),
                           "streamEncoder.json") > 0) {
                char *currentConfig = os_quick_read_utf8_file(jsonPath);
                Json currentConfigJson = Json::parse(currentConfig, parseError);
        }

        Json currentEncoderConfigJson = LoadEncoderSettings();
        Json newConfig;
	if (currentEncoderConfigJson.is_object()) {
		Json::object encoderConfig =
			currentEncoderConfigJson.object_items();
		if (encoderConfig.find("bitrate") != encoderConfig.end()) {
                        encoderConfig.erase("bitrate");
		}
		encoderConfig.emplace("bitrate", Json(maxVideoBitrate));
		newConfig = encoderConfig;
	} else {
		newConfig = Json::object{{"bitrate", maxVideoBitrate},
					 {"keyint_sec", maxKeyframeInterval},
					 {"profile", "baseline"},
					 {"rate_control", "ABR"}};
	}

        string streamEncoderJson = newConfig.dump();
        if (GetProfilePath(jsonPath,
                           sizeof(jsonPath),
                           "streamEncoder.json") > 0) {
                os_quick_write_utf8_file_safe(jsonPath,
                                              streamEncoderJson.c_str(),
                                              streamEncoderJson.length(), false,
                                              "tmp", "bak");
        }

	CreateStream();
}

Json RedditStartStreamDialog2::LoadEncoderSettings()
{
	string parseError;
        char encoderJsonPath[512];
        Json currentEncoderConfigJson;
        if (GetProfilePath(encoderJsonPath,
                           sizeof(encoderJsonPath),
                           "streamEncoder.json") > 0) {
                char *encoderConfig = os_quick_read_utf8_file(encoderJsonPath);
                currentEncoderConfigJson = Json::parse(encoderConfig,
                                                       parseError);
                bfree(encoderConfig);
        }
	return currentEncoderConfigJson;
}

void RedditStartStreamDialog2::UpdateButtons()
{
	bool hasTitle = ui->titleEdit->text().trimmed().length() > 0;
	bool hasSubreddit = ui->subredditCombo->currentText()
	                      .trimmed().length() > 0;

	ui->startButton->setEnabled(hasTitle && hasSubreddit);
}

void RedditStartStreamDialog2::SaveStreamSettings()
{
        QString title = ui->titleEdit->text().trimmed();
        QString subreddit = ui->subredditCombo->currentText().trimmed();

        auto *main = OBSBasic::Get();
	auto *config = main->Config();
        config_set_string(config, CONF_SECTION, CONF_STREAM_TITLE,
                          title.toStdString().c_str());
        config_set_string(config, CONF_SECTION, CONF_LAST_SUBREDDIT,
                          subreddit.toStdString().c_str());
}
