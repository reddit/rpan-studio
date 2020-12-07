#pragma once

#include <QTimer>

#include "ui_RedditStartStreamDialog2.h"

namespace json11 {
class Json;
}

class RedditStartStreamDialog2 : public QDialog {
Q_OBJECT

public:
	explicit RedditStartStreamDialog2(QWidget *parent = nullptr);

	~RedditStartStreamDialog2();

	QScopedPointer<Ui::RedditStartStreamDialog2> ui;

private:
	void SetPage(int page);
	void UpdateEncoderSettings();
	json11::Json LoadEncoderSettings();
	void SaveStreamSettings();

	void LoadSubreddits();
	void LoadDynamicConfig();

	QScopedPointer<QThread> loadSubredditsThread;
	QScopedPointer<QThread> createStreamThread;
	QScopedPointer<QThread> loadDynamicConfigThread;

	bool retried = false;
	bool fixAudioTracks = false;
	bool fixVideoTracks = false;
	int maxAudioBitrate = 2048;
	int maxVideoBitrate = 128;
	int maxKeyframeInterval = 2;
	int currentAudioBitrate = 0;
	int currentVideoBitrate = 0;

	QScopedPointer<QString> serverTextResponse;
	QScopedPointer<QString> serverErrorResponse;
	QString errorStep;

private slots:
	void ErrorMoreInfo();
	
	void CreateStream();
	void ValidateConfig();

	void OnLoadSubreddits(const QString &text, const QString &error,
	                      const QStringList &responseHeaders);
	void OnCreateStream(const QString &text, const QString &error,
	                    const QStringList &responseHeaders);
	void OnDynamicConfig(const QString &text, const QString &error,
	                     const QStringList &responseHeaders);

	void UpdateButtons();
};
