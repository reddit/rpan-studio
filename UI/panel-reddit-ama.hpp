#pragma once

#include <QStyledItemDelegate>

#include "obs.hpp"
#include "ui_RedditAMAPanel.h"

class AMAModel;

class RedditAMAPanel : public QDockWidget {
Q_OBJECT

public:
	static AMAModel *Model() { return model; }

public:
	explicit RedditAMAPanel(QWidget *parent = nullptr);

	~RedditAMAPanel();

	void SetPage(int page);
	OBSSceneItem CreateOverlay();
	void DestroyOverlay();
	void RemoveUI();
	bool IsActive();

	QScopedPointer<Ui::RpanAMAPanel> ui;

private:
	void HideOverlay();
	void ClearPanel();
	void CreateOverlayBackground(OBSScene scene, OBSSceneItem group);
	void CreateOverlayUsername(OBSScene scene, OBSSceneItem group);
	void CreateOverlayQuestion(OBSScene scene, OBSSceneItem group);
	void CreateOverlayLogo(OBSScene scene, OBSSceneItem group);

	static AMAModel *model;

private slots:
	void OnSelectionChanged(const QModelIndex &current,
	                        const QModelIndex &previous);
	void OnDismissQuestion();
};

class Question {
public:
	Question(const QString &q, const QString &u)
		: question(q),
		  username(u)
	{
	}

	QString question;
	QString username;
};

class AMAModel : public QAbstractTableModel {
Q_OBJECT

public:
	AMAModel(QWidget *parent = nullptr);

	int rowCount(const QModelIndex &) const override
	{
		return questions.length();
	}

	int columnCount(const QModelIndex &) const override { return 3; }

	QVariant data(const QModelIndex &index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation,
	                    int role) const override;

	void addQuestion(const QString &question, const QString &username);
	QSharedPointer<Question> getQuestion(int row);
	void removeQuestion(int row);
	void clear();

private:
	QList<QSharedPointer<Question>> questions;
	QScopedPointer<QPixmap> acceptIcon;
	QScopedPointer<QPixmap> removeIcon;
};
