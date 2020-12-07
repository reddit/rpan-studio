#pragma once

#include <QDialog>

#include "ui_RedditLoginDialog2.h"

class RedditLoginDialog2 : public QDialog {
Q_OBJECT

public:
	explicit RedditLoginDialog2(QWidget *parent = nullptr);

	std::unique_ptr<Ui::RedditLoginDialog2> ui;

	std::shared_ptr<std::string> Username() { return username; }

	std::shared_ptr<std::string> Cookie() { return cookie; }

	std::shared_ptr<std::string> Code() { return code; }

	std::shared_ptr<std::string> RefreshToken() { return refreshToken; }

private:
	void SetPage(int page);

	std::shared_ptr<std::string> username;
	std::shared_ptr<std::string> cookie;
	std::shared_ptr<std::string> code;
	std::shared_ptr<std::string> refreshToken;
	bool needsOtp = false;

	QScopedPointer<QThread> loginThread;
	QScopedPointer<QThread> authThread;
	QScopedPointer<QString> serverTextResponse;
	QScopedPointer<QString> serverErrorResponse;
	QString errorStep;

private slots:
	void ErrorMoreInfo();
	
	void SignIn();

	void LoginResult(const QString &text, const QString &error,
	                 const QStringList &responseHeaders);

	void AuthorizeResult(const QString &text, const QString &error,
	                     const QStringList &responseHeaders);
};
