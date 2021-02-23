#include <qt-wrappers.hpp>

#include "window-reddit-login-dialog2.hpp"

#include <QDesktopServices>
#include <QUrlQuery>
#include <json11.hpp>

#include "api-reddit.hpp"
#include "window-basic-main.hpp"
#include "window-reddit-error-log.hpp"

using namespace std;
using namespace json11;

#define PAGE_SIGNIN 0
#define PAGE_OTP 1
#define PAGE_SPINNER 2

RedditLoginDialog2::RedditLoginDialog2(QWidget *parent)
	: QDialog(parent),
	  ui(new Ui::RedditLoginDialog2)
{
	ui->setupUi(this);

	setWindowFlag(Qt::WindowContextHelpButtonHint, false);

	ui->forgotPasswordLink->setText(
		QString("<a href='https://www.reddit.com/password'>%1</a>").arg(
			QTStr("Reddit.Login.Links.ForgotPassword")));
	ui->forgotUsernameLink->setText(
		QString("<a href='https://www.reddit.com/username'>%1</a>").arg(
			QTStr("Reddit.Login.Links.ForgotUsername")));
	ui->signupLink->setText(
		QString("<a href='https://www.reddit.com/register/'>%1</a>").
		arg(
			QTStr("Reddit.Login.Links.Signup")));

	ui->errorMoreInfo->setText(
		QString("<a href='#'>%1</a>").arg(
			ui->errorMoreInfo->text()));
	ui->errorMoreInfo->setVisible(false);
	ui->otpErrorMoreInfo->setText(
		QString("<a href='#'>%1</a>").arg(
			ui->otpErrorMoreInfo->text()));
	ui->otpErrorMoreInfo->setVisible(false);

	ui->errorLabel->setText("");
	ui->otpErrorLabel->setText("");
	ui->errorLabel->setProperty("themeID", "error");
	ui->otpErrorLabel->setProperty("themeID", "error");
	ui->stack->setCurrentIndex(PAGE_SIGNIN);

	connect(ui->signinButton, SIGNAL(clicked()), this, SLOT(SignIn()));
	connect(ui->errorMoreInfo, SIGNAL(clicked()), this,
	        SLOT(ErrorMoreInfo()));
	connect(ui->otpErrorMoreInfo, SIGNAL(clicked()), this,
	        SLOT(ErrorMoreInfo()));
}

void RedditLoginDialog2::SetPage(int page)
{
	ui->stack->setCurrentIndex(page);

	switch (page) {
	case PAGE_SIGNIN:
		ui->subtitle->setText(Str("Reddit.Login.SignIn"));
		ui->signinButton->setEnabled(true);
		ui->signinButton->setText(Str("Reddit.Login.SignIn"));
		break;
	case PAGE_OTP:
		ui->subtitle->setText(Str("Reddit.Login.2FA.Subtitle"));
		ui->signinButton->setEnabled(true);
		ui->signinButton->setText(Str("Reddit.Login.2FA.Button.Check"));
		break;
	case PAGE_SPINNER:
		ui->signinButton->setEnabled(false);
		break;
	}
}

void RedditLoginDialog2::ErrorMoreInfo()
{
	QString logStr =
		QString("Server response:\n%1").arg(*serverTextResponse);
	if (!serverErrorResponse->isEmpty()) {
		logStr.append(QString("\n\nServer error response:\n%1")
			.arg(*serverErrorResponse));
	}

	RedditErrorLogDialog dlg(this, errorStep, logStr);
	dlg.exec();
}

void RedditLoginDialog2::SignIn()
{
	const string newUsername = ui->usernameEdit->text().toStdString();
	const string password = ui->passwordEdit->text().toStdString();

	if (newUsername.empty()) {
		ui->errorLabel->setText(Str("Reddit.Login.Error.Username"));
		return;
	}
	if (password.empty()) {
		ui->errorLabel->setText(Str("Reddit.Login.Error.Password"));
		return;
	}
	string otpCode;
	if (needsOtp) {
		otpCode = ui->otpEdit->text().trimmed().toStdString();
		if (otpCode.empty() || otpCode.length() < 6) {
			ui->otpErrorLabel->setText(
				Str("Reddit.Login.2FA.Error.Required"));
			return;
		}
	}

	switch (ui->stack->currentIndex()) {
	case PAGE_SIGNIN:
		ui->subtitle->setText(QTStr("Reddit.Login.Subtitle.SigningIn"));
		break;
	case PAGE_OTP:
		ui->subtitle->setText(
			Str("Reddit.Login.Subtitle.CheckingCode"));
		break;
	}
	ui->stack->setCurrentIndex(PAGE_SPINNER);
	ui->errorLabel->setText("");
	ui->otpErrorLabel->setText("");

	username = make_shared<string>(newUsername);

	// Step 1: Login
	auto *thread = RedditApi::AuthLogin(newUsername, password, otpCode);
	connect(thread, &RemoteTextThread::Result, this,
	        &RedditLoginDialog2::LoginResult);

	loginThread.reset(thread);
	loginThread->start();
}

void RedditLoginDialog2::LoginResult(const QString &text,
                                     const QString &errorText,
                                     const QStringList &responseHeaders)
{
	string error;
	const Json loginJson = Json::parse(text.toStdString(), error);
	const Json loginJsonData = loginJson["json"]["data"];
	if (!loginJsonData.is_object()) {
		const Json errors = loginJson["json"]["errors"];
		string msg;
		if (errors.is_array() && errors.array_items().size() > 0) {
			for (const Json item : errors.array_items()) {
				msg = item.array_items()[1]
					.string_value();
				break;
			}
		}

		if (msg.empty()) {
			msg = Str("Reddit.Error.Unknown");
		}

		blog(LOG_INFO, "Reddit: Login failure: %s",
		     text.toStdString().c_str());

		string err = Str("Reddit.Login.Error.Generic.Message") + msg;

		if (needsOtp) {
			ui->otpErrorLabel->setText(QString::fromStdString(err));
			ui->otpErrorMoreInfo->setVisible(true);
			SetPage(PAGE_OTP);
		} else {
			ui->errorLabel->setText(QString::fromStdString(err));
			ui->errorMoreInfo->setVisible(true);
			SetPage(PAGE_SIGNIN);
		}

		serverTextResponse.reset(new QString(text));
		serverErrorResponse.reset(new QString(errorText));
		errorStep = QTStr("Reddit.ErrorLog.Step.Login");

		return;
	}
	const Json details = loginJsonData["details"];
	if (details.is_string() &&
	    details.string_value() == "TWO_FA_REQUIRED") {
		needsOtp = true;
		blog(LOG_INFO, "Reddit: Account has 2FA enabled");
		SetPage(PAGE_OTP);
		return;
	}

	const string modhash = loginJsonData["modhash"].string_value();

	QString newCookie;
	for (const QString &header : responseHeaders) {
		if (header.left(12) == "set-cookie: ") {
			auto val = header.mid(12);
			if (!newCookie.isEmpty()) {
				newCookie += ";";
			}
			newCookie += val.left(val.indexOf(';'));
		}
	}

	cookie = make_shared<string>(newCookie.toStdString());

	// Step 2: Authorize
	auto *thread = RedditApi::AuthAuthorize(modhash,
	                                        newCookie.toStdString());
	connect(thread, &RemoteTextThread::Result, this,
	        &RedditLoginDialog2::AuthorizeResult);

	authThread.reset(thread);
	authThread->start();
}

void RedditLoginDialog2::AuthorizeResult(const QString &text,
                                         const QString &errorText,
                                         const QStringList &responseHeaders)
{
	string error;
	Json authJson = Json::parse(text.toStdString(), error);

	QString newCode;
	for (const QString &header : responseHeaders) {
		if (header.left(10) == "location: ") {
			QString val = header.mid(10);
			QUrl location(val);
			QUrlQuery query(location);

			if(!query.hasQueryItem("code")) {
                                SetPage(PAGE_SIGNIN);
                                serverTextResponse.reset(new QString(text));
                                serverErrorResponse.reset(new QString(errorText));
                                errorStep = QTStr("Reddit.ErrorLog.Step.Authorization");
				return;
			}

			newCode = query.queryItemValue("code");
			break;
		}
	}

	code = make_shared<string>(newCode.toStdString());

	accept();
}
