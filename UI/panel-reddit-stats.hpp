#pragma once

#include <QDockWidget>


#include "obs.hpp"
#include "ui_RedditStatsPanel.h"

class RedditStatsPanel : public QDockWidget {
Q_OBJECT

public:
	explicit RedditStatsPanel(QWidget *parent = nullptr);
	~RedditStatsPanel();

	void SetPage(int page);

	QScopedPointer<Ui::RedditStatsPanel> ui;

private:
	QScopedPointer<QTimer> timer;
	QScopedPointer<QTimer> countdownTimer;
	QScopedPointer<QThread> apiThread;

	int timeRemaining = 0;
	std::string url;

private slots:
	void UpdateStats();
	void CountdownTime();
	void CopyURLToClipboard();
	void OnLoadStream(const QString &text, const QString &error,
	                  const QStringList &resultHeaders);
};
