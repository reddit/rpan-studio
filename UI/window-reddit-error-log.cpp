#include "window-reddit-error-log.hpp"

RedditErrorLogDialog::RedditErrorLogDialog(QWidget *parent, const QString &step,
                                           const QString &log)
	: QDialog(parent),
	  ui(new Ui::ErrorLogDialog)
{
	ui->setupUi(this);

	QFont monospace("monospace");
	monospace.setStyleHint(QFont::Monospace);
	ui->logOutput->setFont(monospace);

	ui->failureStep->setText(step);
	ui->logOutput->setText(log);
}
