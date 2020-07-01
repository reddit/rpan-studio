#pragma once

#include "auth-oauth.hpp"

class QDockWidget;

class RedditAuth : public OAuthStreamKey {
Q_OBJECT

public:
	static std::shared_ptr<RedditAuth> Login(QWidget *parent);

public:
	static std::string userAgent;

public:
	RedditAuth(const Def &def);

public:
	bool RetryLogin() override { return false; };
	void SaveInternal() override;
	bool LoadInternal() override;
	void LoadUI() override;

	std::shared_ptr<std::string> Token();

	void SetKey(const std::string &key) { key_ = key; }
	std::shared_ptr<std::string> GetUsername() { return std::make_shared<std::string>(username); }

private:
	bool RefreshAccessToken();

private:
	bool requires2FA = false;
	bool uiLoaded = false;
	std::string cookie;
	std::string code;
	std::string username;

	QScopedPointer<QDockWidget> chatPanel;
	QScopedPointer<QDockWidget> statsPanel;
	QScopedPointer<QAction> chatMenu;
	QScopedPointer<QAction> statsMenu;
};
