#include "panel-reddit-stats.hpp"

#include <QClipboard>
#include <QDateTime>
#include <QToolTip>
#include <json11.hpp>

#include "api-reddit.hpp"
#include "qt-wrappers.hpp"
#include "window-basic-main.hpp"

#define PAGE_STATS 0
#define PAGE_OFFLINE 1

using namespace std;
using namespace json11;

namespace {

void on_frontend_event(enum obs_frontend_event event, void *param);

}

RedditStatsPanel::RedditStatsPanel(QWidget *parent)
	: QDockWidget(parent),
	  ui(new Ui::RedditStatsPanel)
{
	ui->setupUi(this);

	connect(ui->copyStreamLinkBtn, SIGNAL(clicked()), this,
	        SLOT(CopyURLToClipboard()));

	timer.reset(new QTimer(this));
	connect(timer.data(), &QTimer::timeout, this,
	        QOverload<>::of(&RedditStatsPanel::UpdateStats));

	countdownTimer.reset(new QTimer(this));
	connect(countdownTimer.data(), &QTimer::timeout, this,
	        QOverload<>::of(&RedditStatsPanel::CountdownTime));
	countdownTimer->start(1000);

	ui->upvotesLabel->setText("");
	ui->watchersLabel->setText("");
	ui->timeleftLabel->setText("");
	ui->awardsLabel->setText("");
	ui->copyStreamLinkBtn->setVisible(false);

	obs_frontend_add_event_callback(on_frontend_event, this);

	auto *main = OBSBasic::Get();
	if (main->StreamingActive()) {
		SetPage(PAGE_STATS);
	} else {
		SetPage(PAGE_OFFLINE);
	}
}

RedditStatsPanel::~RedditStatsPanel()
{
	obs_frontend_remove_event_callback(on_frontend_event, this);

	timer->stop();
	countdownTimer->stop();
	if (apiThread != nullptr) {
		apiThread->quit();
		apiThread->wait();
	}
}

void RedditStatsPanel::SetPage(int page)
{
	ui->stackedWidget->setCurrentIndex(page);

	switch (page) {
	case PAGE_STATS:
		UpdateStats();
		timer->start(5000);
		break;
	case PAGE_OFFLINE:
		timer->stop();
		timeRemaining = 0;
		break;
	}
}

void RedditStatsPanel::UpdateStats()
{
	auto *main = OBSBasic::Get();
	auto *broadcastId = config_get_string(main->Config(), "Reddit",
	                                      "BroadcastId");

	auto *thread = RedditApi::FetchStream(broadcastId);
	connect(thread, &RemoteTextThread::Result, this,
	        &RedditStatsPanel::OnLoadStream);
	if (apiThread != nullptr) {
		apiThread->quit();
		apiThread->wait();
	}
	apiThread.reset(thread);
	apiThread->start();
}

void RedditStatsPanel::CountdownTime()
{
	if (timeRemaining > 0) {
		timeRemaining--;
	}

	const QString timeRemainingStr = QDateTime::fromTime_t(timeRemaining)
	                                 .toUTC()
	                                 .toString("hh:mm:ss");
	ui->timeleftLabel->setText(timeRemainingStr);
}

void RedditStatsPanel::CopyURLToClipboard()
{
	if (!url.empty()) {
		QApplication::clipboard()->setText(QString::fromStdString(url));
		QToolTip::showText(QCursor::pos(),
		                   Str(
			                   "Reddit.Panel.Stats.Tooltip.Link.Copied"),
		                   this,
				   QRect(),
				   2000);

	}
}

void RedditStatsPanel::OnLoadStream(const QString &text,
                                    const QString &,
                                    const QStringList &resultHeaders)
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
	string newUrl = postData["url"].string_value();
	if(url != newUrl) {
		url = newUrl;
		blog(LOG_INFO, "Reddit: Loaded stream stats for URL: %s", url.c_str());
	}
	int score = postData["score"].int_value();
	int watchers = streamData["continuous_watchers"].int_value();
	timeRemaining = static_cast<int>(streamData["estimated_remaining_time"]
		.number_value());
	vector<Json> awards = postData["awardings"].array_items();
	int totalAwards = 0;
	for(auto it = awards.begin(); it != awards.end(); ++it) {
		totalAwards += (*it)["total"].int_value();
	}

	ui->upvotesLabel->setText(QString::number(score));
	ui->watchersLabel->setText(QString::number(watchers));
	ui->awardsLabel->setText(QString::number(totalAwards));
	ui->copyStreamLinkBtn->setVisible(true);
}

namespace {

void on_frontend_event(enum obs_frontend_event event, void *param)
{
	auto *panel = static_cast<RedditStatsPanel *>(param);

	switch (event) {
	case OBS_FRONTEND_EVENT_STREAMING_STARTED:
		panel->SetPage(PAGE_STATS);
		break;
	case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
		panel->SetPage(PAGE_OFFLINE);
		break;
	}
}

}
