#include "api-reddit.hpp"

#include <QMessageAuthenticationCode>
#include <QUuid>

#include <ui-config.h>

#include "auth-reddit.hpp"
#include "obf.h"
#include "window-basic-main.hpp"
#include <json11.hpp>

#define REDDIT_BASE_URL "https://strapi.reddit.com/"
#define REDDIT_AUTH_BASE_URL "https://old.reddit.com/api/v1/"
#define REDDIT_DESKTOP_BASE_URL "https://gateway.reddit.com/desktopapi/v1/"

using namespace std;
using namespace json11;

namespace {

const char *API_CONFIG = REDDIT_BASE_URL "config";
const char *API_SUBREDDIT_LIST = REDDIT_BASE_URL
	"recommended_broadcaster_prompts";
const char *API_CREATE_STREAM_PREFIX = REDDIT_BASE_URL "r/";
const char *API_CREATE_STREAM_SUFFIX = "/broadcasts";
const char *API_VIDEOS_PREFIX = REDDIT_BASE_URL "videos/";
const char *API_STOP_STREAM_SUFFIX = "/end";
const char *API_ADD_COMMENT_SUFFIX = "/comment";

const char *API_POSTCOMMENTS = REDDIT_DESKTOP_BASE_URL "postcomments/";

const char *API_AUTH_LOGIN = REDDIT_AUTH_BASE_URL "login";
const char *API_AUTH_AUTHORIZE = REDDIT_AUTH_BASE_URL "authorize";
const char *API_AUTH_TOKEN = REDDIT_AUTH_BASE_URL "access_token";
const char *API_AUTH_REDIRECT_URL = "http://localhost:65010/callback";

const char *CONTENT_JSON = "application/json";
const char *CONTENT_FORM = "application/x-www-form-urlencoded";

const char *REDDIT_VENDOR_ID = "c3204ce9-ce8a-4939-a576-2fb15799d46b";
const char *REDDIT_DEVICE_ID = "12d22735-6220-415d-a117-c6687eb17de4";

string state = QUuid::createUuid().toString().toStdString().substr(1, 36);

QScopedPointer<RemoteTextThread> stopStreamThread;

vector<string> AuthHeaders()
{
	auto *main = OBSBasic::Get();
	auto *auth = dynamic_cast<RedditAuth *>(main->GetAuth());

	if (auth == nullptr) {
		return vector<string>();
	}

	string bearer = "Authorization: Bearer ";
	const shared_ptr<string> token = auth->Token();
	if (token != nullptr) {
		bearer += *token;
	}

	vector<string> headers;
	headers.push_back(bearer);

	return headers;
}

vector<string> LoginHeaders(const std::string &postData,
                            const vector<string> *extraHeaders = nullptr)
{
	vector<string> headers;

	const time_t now = time(nullptr);
	const string nowStr = to_string(now);

	string hmacSecret = REDDIT_SECRET;
	deobfuscate_str(&hmacSecret[0], REDDIT_HASH);

	const string hmacBody = "Epoch:" + nowStr +
	                        "|Body:" + postData;
	const string hmacBodySha = QMessageAuthenticationCode::hash(
		                           hmacBody.c_str(), hmacSecret.c_str(),
		                           QCryptographicHash::Sha256)
	                           .toHex()
	                           .toStdString();

	const string hmacHeader = "Epoch:" + nowStr +
	                          "|User-Agent:" + RedditAuth::userAgent +
	                          "|Client-Vendor-ID:" + string(
		                          REDDIT_VENDOR_ID);
	const string hmacHeaderSha = QMessageAuthenticationCode::hash(
		                             hmacHeader.c_str(),
		                             hmacSecret.c_str(),
		                             QCryptographicHash::Sha256)
	                             .toHex()
	                             .toStdString();

	const string hmacPrefix = string(REDDIT_HMAC_GLOBAL_VERSION) + ":" +
	                          REDDIT_HMAC_PLATFORM + ":" +
	                          REDDIT_HMAC_TOKEN_VERSION + ":"
	                          + nowStr + ":";

	memset((void *)hmacSecret.data(), 0, hmacSecret.length());

	headers.emplace_back("Client-Vendor-ID: " + string(REDDIT_VENDOR_ID));
	headers.emplace_back("X-hmac-signed-body: " +
	                     hmacPrefix +
	                     hmacBodySha);
	headers.emplace_back("X-hmac-signed-result: " +
	                     hmacPrefix +
	                     hmacHeaderSha);

	if (extraHeaders != nullptr) {
		for (auto iter = extraHeaders->begin();
		     iter != extraHeaders->end(); ++iter) {
			headers.emplace_back(*iter);
		}
	}

	return headers;
}

}

RemoteTextThread *RedditApi::FetchConfig()
{
	auto *thread = new RemoteTextThread(
		API_CONFIG,
		AuthHeaders(),
		CONTENT_JSON);
	thread->setUserAgent(RedditAuth::userAgent);
	return thread;
}

RemoteTextThread *RedditApi::FetchSubreddits()
{
	auto *thread = new RemoteTextThread(
		API_SUBREDDIT_LIST,
		AuthHeaders(),
		CONTENT_JSON);
	thread->setUserAgent(RedditAuth::userAgent);
	return thread;
}

RemoteTextThread *RedditApi::CreateStream(const string &title,
                                          const string &subreddit)
{
	string url = API_CREATE_STREAM_PREFIX + subreddit +
	             API_CREATE_STREAM_SUFFIX + "?title=" + title;

	// POST data is required to send a POST request
	string postData = "title=" + title;

	auto *thread = new RemoteTextThread(
		url,
		AuthHeaders(),
		CONTENT_FORM,
		postData);
	thread->setUserAgent(RedditAuth::userAgent);
	return thread;
}

RemoteTextThread *RedditApi::StopStream(const string &broadcastId)
{
	string url = API_VIDEOS_PREFIX + broadcastId + API_STOP_STREAM_SUFFIX;

	stopStreamThread.reset(new RemoteTextThread(
		url,
		AuthHeaders(),
		CONTENT_JSON,
		" "));
	stopStreamThread->setUserAgent(RedditAuth::userAgent);
	return stopStreamThread.data();
}

RemoteTextThread *RedditApi::FetchStream(const string &broadcastId)
{
	string url = API_VIDEOS_PREFIX + broadcastId;

	auto *thread = new RemoteTextThread(
		url,
		AuthHeaders(),
		CONTENT_JSON);
	thread->setUserAgent(RedditAuth::userAgent);
	return thread;
}

RemoteTextThread *RedditApi::PostComment(const std::string &broadcastId,
                                         const std::string &comment)
{
	string url = API_VIDEOS_PREFIX + broadcastId + API_ADD_COMMENT_SUFFIX;

	Json json = Json::object{
		{"text", comment}
	};
	string postData = json.dump();

	auto *thread = new RemoteTextThread(
		url,
		AuthHeaders(),
		CONTENT_JSON,
		postData);
	thread->setUserAgent(RedditAuth::userAgent);
	return thread;
}

RemoteTextThread *RedditApi::FetchPostComments(const std::string &postId)
{
	string url = API_POSTCOMMENTS + postId +
	             "?rtj=only" +
	             "&emotes_as_images=true" +
	             "&sort=chat";

	auto *thread = new RemoteTextThread(
		url,
		AuthHeaders());
	thread->setUserAgent(RedditAuth::userAgent);
	return thread;
}


RemoteTextThread *RedditApi::AuthLogin(const std::string &username,
                                       const std::string &password,
                                       const std::string &otp)
{
	string postData = "user=" + string(username) +
	                  "&passwd=" + QUrl::toPercentEncoding(
		                  QString::fromStdString(password)).
	                  toStdString() +
	                  "&rem=true" +
	                  "&grant_type=password" +
	                  "&duration=permanent" +
	                  "&api_type=json" +
	                  "&profile_img=true";

	if (!otp.empty()) {
		postData += "&otp=" + otp;
	}

	auto *thread = new RemoteTextThread(
		API_AUTH_LOGIN,
		LoginHeaders(postData),
		CONTENT_FORM,
		postData);
	thread->setUserAgent(RedditAuth::userAgent);
	return thread;
}

RemoteTextThread *RedditApi::AuthAuthorize(const std::string &modhash,
                                           const std::string &cookie)
{
	string clientId = REDDIT_CLIENTID;
	deobfuscate_str(&clientId[0], REDDIT_HASH);

	string postData = "state=" + state +
	                  "&scope=" +
	                  QUrl::toPercentEncoding("*").toStdString() +
	                  "&authorize=allow" +
	                  "&duration=permanent" +
	                  "&response_type=code" +
	                  "&client_id=" + clientId +
	                  "&redirect_uri=" +
	                  QUrl::toPercentEncoding(API_AUTH_REDIRECT_URL).
	                  toStdString();

	vector<string> extraHeaders;
	extraHeaders.emplace_back("X-Modhash: " + modhash);
	extraHeaders.emplace_back("Cookie: " + cookie);

	auto *thread = new RemoteTextThread(
		API_AUTH_AUTHORIZE,
		LoginHeaders(postData, &extraHeaders),
		CONTENT_FORM,
		postData);
	thread->setUserAgent(RedditAuth::userAgent);
	return thread;
}

bool RedditApi::AuthToken(const std::string &code,
                          const std::string &refreshToken,
                          const std::string &cookie,
                          std::string *tokenOut,
                          std::string *refreshTokenOut,
                          long *expireTimeOut)
{
	string deviceId = REDDIT_DEVICE_ID;
	string postData = "device_id=" + deviceId +
	                  "&redirect_uri=" +
	                  QUrl::toPercentEncoding(API_AUTH_REDIRECT_URL).
	                  toStdString();
	if (!code.empty()) {
		postData += string("&grant_type=authorization_code") +
			"&code=" + code;
	} else if (!refreshToken.empty()) {
		postData += string("&grant_type=refresh_token") +
			"&refresh_token=" + refreshToken;
	} else {
		return false;
	}

	vector<string> headers = LoginHeaders(postData);
	headers.emplace_back("Cookie: " + cookie);
	headers.emplace_back("X-Reddit-Device-Id: " + deviceId);

	vector<string> responseHeaders;

	string clientId = REDDIT_CLIENTID;
	deobfuscate_str(&clientId[0], REDDIT_HASH);
	const string userPassAuth = clientId + ":";

	string output;
	string error;
	long responseCode;

	bool success = GetRemoteFile(
		API_AUTH_TOKEN,
		output,
		error,
		&responseCode,
		"application/x-www-form-urlencoded",
		postData.c_str(),
		headers,
		nullptr,
		0,
		userPassAuth.c_str(),
		RedditAuth::userAgent.c_str(),
		&responseHeaders);

	if (!success || responseCode != 200) {
		return false;
	}

	Json tokenJson = Json::parse(output, error);

	string accessToken = tokenJson["access_token"].string_value();
	string newRefreshToken = tokenJson["refresh_token"].string_value();
	int expireTime = tokenJson["expires_in"].int_value();

	tokenOut->swap(accessToken);
	if (!newRefreshToken.empty()) {
		refreshTokenOut->swap(newRefreshToken);
	}
	*expireTimeOut = (uint64_t)time(nullptr) + expireTime;

	return true;
}
