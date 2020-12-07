#pragma once

#include "auth-oauth.hpp"

class QDockWidget;
class RedditStartStreamDialog2;

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
	friend class RedditStartStreamDialog2;
	friend class RedditAMAPanel;
	
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
	QScopedPointer<QDockWidget> amaPanel;
	QScopedPointer<QAction> chatMenu;
	QScopedPointer<QAction> statsMenu;
	QScopedPointer<QAction> amaMenu;
};
