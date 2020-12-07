#pragma once

#include <QDialog>

#include "ui_RedditErrorLogDialog.h"

class RedditErrorLogDialog : public QDialog {
Q_OBJECT

public:
	RedditErrorLogDialog(QWidget *parent, const QString &step,
	                     const QString &log);

	QScopedPointer<Ui::ErrorLogDialog> ui;
};
