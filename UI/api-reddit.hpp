#pragma once

#include "remote-text.hpp"

class RedditApi {
public:
	static RemoteTextThread *FetchConfig();
	static RemoteTextThread *FetchSubreddits();
	static RemoteTextThread *CreateStream(const std::string &title, const std::string &subreddit);
	static RemoteTextThread *StopStream(const std::string &broadcastId);
	static RemoteTextThread *FetchStream(const std::string &broadcastId);
	static RemoteTextThread *PostComment(const std::string &broadcastId, const std::string &comment);

	static RemoteTextThread *FetchPostComments(const std::string &postId);

	static RemoteTextThread *AuthLogin(const std::string &username, const std::string &password, const std::string &otp = "");
	static RemoteTextThread *AuthAuthorize(const std::string &modhash, const std::string &cookie);
	static bool AuthToken(const std::string &code, const std::string &refreshToken, const std::string &cookie, std::string *tokenOut, std::string *refreshTokenOut, long *expireTimeOut);
};
