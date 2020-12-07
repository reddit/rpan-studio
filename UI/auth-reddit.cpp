#include "auth-reddit.hpp"

#include <QInputDialog>

#include <qt-wrappers.hpp>

#include "api-reddit.hpp"
#include "panel-reddit-chat.hpp"
#include "panel-reddit-stats.hpp"
#include "panel-reddit-ama.hpp"
#include "remote-text.hpp"
#include "window-basic-main.hpp"
#include "window-reddit-login-dialog2.hpp"

using namespace std;
using namespace json11;

namespace {

Auth::Def redditDef = {"Reddit", Auth::Type::OAuth_StreamKey};

}

string RedditAuth::userAgent;

std::shared_ptr<RedditAuth> RedditAuth::Login(QWidget *parent)
{
	RedditLoginDialog2 dlg(parent);
	int result = dlg.exec();

	if (!result) {
		return nullptr;
	}

	shared_ptr<RedditAuth> auth = make_shared<RedditAuth>(redditDef);
	auth->cookie = *dlg.Cookie();
	auth->currentScopeVer = 1;
	auth->code = *dlg.Code();
	auth->username = *dlg.Username();
	if (!auth->RefreshAccessToken()) {
		return nullptr;
	}

	return auth;
}

RedditAuth::RedditAuth(const Def &def)
	: OAuthStreamKey(def)
{
}

void RedditAuth::SaveInternal()
{
	auto main = OBSBasic::Get();
	config_set_string(main->Config(), service(), "Cookie", cookie.c_str());
	config_set_string(main->Config(), service(), "Username",
	                  username.c_str());
	OAuthStreamKey::SaveInternal();
}

bool RedditAuth::LoadInternal()
{
	auto main = OBSBasic::Get();
	cookie = config_get_string(main->Config(), service(), "Cookie");
	username = config_get_string(main->Config(), service(), "Username");
	firstLoad = false;
	OAuth::LoadInternal();
	return true;
}

shared_ptr<string> RedditAuth::Token()
{
	if ((!token.empty() && !TokenExpired()) || RefreshAccessToken()) {
		return make_shared<string>(token);
	}
	return nullptr;
}

bool RedditAuth::RefreshAccessToken()
{
	string newToken;
	string newRefreshToken;
	long newExpireTime;

	if (!RedditApi::AuthToken(code,
	                          refresh_token,
	                          cookie,
	                          &newToken,
	                          &newRefreshToken,
	                          &newExpireTime)) {
		return false;
	}

	token = newToken;
	if (!newRefreshToken.empty()) {
		refresh_token = newRefreshToken;
	}
	expire_time = newExpireTime;

	Save();

	return true;
}

void RedditAuth::LoadUI()
{
	if (uiLoaded) {
		return;
	}

	auto *main = OBSBasic::Get();

	chatPanel.reset(new RedditChatPanel());
	main->addDockWidget(Qt::RightDockWidgetArea, chatPanel.data());
	chatMenu.reset(main->AddDockWidget(chatPanel.data()));

	statsPanel.reset(new RedditStatsPanel());
	main->addDockWidget(Qt::RightDockWidgetArea, statsPanel.data());
	statsMenu.reset(main->AddDockWidget(statsPanel.data()));

	amaPanel.reset(new RedditAMAPanel());
	amaPanel->setAllowedAreas(Qt::LeftDockWidgetArea);
	amaPanel->close();
	amaMenu.reset(main->AddDockWidget(amaPanel.data()));
	amaMenu->setVisible(false);

	uiLoaded = true;
}


static shared_ptr<Auth> CreateRedditAuth()
{
	return make_shared<RedditAuth>(redditDef);
}

static void DeleteCookies()
{
}

void RegisterRedditAuth()
{
	QSysInfo sysInfo;
	RedditAuth::userAgent = "RPAN Studio/Version " OBS_VERSION "/" +
	                        sysInfo.prettyProductName().toStdString();

	OAuth::RegisterOAuth(redditDef,
	                     CreateRedditAuth,
	                     RedditAuth::Login,
	                     DeleteCookies);
}
