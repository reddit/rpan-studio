#pragma once

#include <QTimer>

#include "ui_RedditStartStreamDialog2.h"

class RedditStartStreamDialog2 : public QDialog {
Q_OBJECT

public:
	explicit RedditStartStreamDialog2(QWidget *parent = nullptr);

	~RedditStartStreamDialog2();

	QScopedPointer<Ui::RedditStartStreamDialog2> ui;

private:
	void SetPage(int page);

	void LoadSubreddits();
	void LoadDynamicConfig();

	QScopedPointer<QThread> loadSubredditsThread;
	QScopedPointer<QThread> createStreamThread;
	QScopedPointer<QThread> loadDynamicConfigThread;

	bool retried = false;

private slots:
	void CreateStream();

	void OnLoadSubreddits(const QString &text, const QString &error,
	                      const QStringList &responseHeaders);
	void OnCreateStream(const QString &text, const QString &error,
	                    const QStringList &responseHeaders);
	void OnDynamicConfig(const QString &text, const QString &error,
	                     const QStringList &responseHeaders);

	void UpdateButtons();
};
