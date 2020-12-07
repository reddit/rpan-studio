#include <QPainter>

#include "panel-reddit-ama.hpp"


#include "auth-reddit.hpp"
#include "obs-app.hpp"
#include "platform.hpp"
#include "window-basic-main.hpp"

namespace {
void on_frontend_event(enum obs_frontend_event event, void *param);
}

AMAModel *RedditAMAPanel::model;

#define PAGE_OFFLINE 0
#define PAGE_STREAMING 1

RedditAMAPanel::RedditAMAPanel(QWidget *parent)
	: QDockWidget(parent),
	  ui(new Ui::RpanAMAPanel)
{
	ui->setupUi(this);

	ui->usernameLabel->setText("");
	ui->questionLabel->setText("");
	ui->dismissButton->setEnabled(false);

	model = new AMAModel(ui->incomingQuestionTable);
	model->addQuestion("foo", "bar");

	ui->incomingQuestionTable->setModel(model);
	ui->incomingQuestionTable->horizontalHeader()->setSectionResizeMode(
		0, QHeaderView::Stretch);
	ui->incomingQuestionTable->horizontalHeader()->setSectionResizeMode(
		1, QHeaderView::Fixed);
	ui->incomingQuestionTable->horizontalHeader()->setSectionResizeMode(
		2, QHeaderView::Fixed);

	ui->incomingQuestionTable->setColumnWidth(1, 32);
	ui->incomingQuestionTable->setColumnWidth(2, 32);

	connect(ui->incomingQuestionTable->selectionModel(),
	        &QItemSelectionModel::currentChanged,
	        this,
	        &RedditAMAPanel::OnSelectionChanged);

	connect(ui->dismissButton,
	        &QPushButton::clicked,
	        this,
	        &RedditAMAPanel::OnDismissQuestion);

	DestroyOverlay();
	SetPage(PAGE_STREAMING);

	obs_frontend_add_event_callback(on_frontend_event, this);
}

RedditAMAPanel::~RedditAMAPanel()
{
	delete model;
	model = nullptr;
}

bool RedditAMAPanel::IsActive()
{
        auto config = OBSBasic::Get()->Config();
        return config_get_bool(config, "Reddit", "AMAMode");
}

void RedditAMAPanel::SetPage(int page)
{
	switch (page) {
	case PAGE_STREAMING:
		ClearPanel();
		model->clear();
		break;
	case PAGE_OFFLINE:
		HideOverlay();
		break;
	}
	ui->stackedWidget->setCurrentIndex(page);
}

void RedditAMAPanel::HideOverlay()
{
	OBSScene scene =
		obs_scene_from_source(obs_frontend_get_current_scene());
	if (!scene) {
		return;
	}
	
	OBSSceneItem overlay = obs_scene_find_source(
		scene, "#AMA_GROUP");

	if (overlay) {
		obs_sceneitem_set_visible(overlay, false);
	}
}

void RedditAMAPanel::DestroyOverlay()
{
	OBSScene scene = obs_scene_from_source(obs_frontend_get_current_scene());
	if (!scene) {
		return;
	}

	OBSSceneItem overlay =
		obs_scene_find_source(scene, "#AMA_GROUP");

	if (overlay) {
		obs_sceneitem_remove(overlay);
		ClearPanel();
	}
}

void RedditAMAPanel::RemoveUI()
{
	auto main = OBSBasic::Get();
	auto auth = static_cast<RedditAuth *>(main->GetAuth());
	auto panel = auth->amaPanel.get();
	auto menu = auth->amaMenu.get();
	panel->close();
	menu->setVisible(false);
}


void RedditAMAPanel::ClearPanel()
{
	ui->usernameLabel->setText("");
	ui->questionLabel->setText("");
	ui->dismissButton->setEnabled(false);
}

OBSSceneItem RedditAMAPanel::CreateOverlay()
{
	OBSScene scene =
		obs_scene_from_source(obs_frontend_get_current_scene());
	OBSSceneItem group =
		obs_scene_add_group2(scene, "#AMA_GROUP", true);

	OBSData settings = obs_data_create();
	obs_data_set_bool(settings, "hidden", true);
	obs_source_update(obs_sceneitem_get_source(group), settings);

	obs_sceneitem_set_visible(group, false);
	obs_sceneitem_set_locked(group, true);
	obs_sceneitem_set_alignment(group, OBS_ALIGN_BOTTOM | OBS_ALIGN_LEFT);

	CreateOverlayBackground(scene, group);
	CreateOverlayLogo(scene, group);
	CreateOverlayUsername(scene, group);
	CreateOverlayQuestion(scene, group);

	vec2 pos;
	pos.x = 0;   // TODO: scene width / 2
	pos.y = 854; // TODO: scene height
	obs_sceneitem_set_pos(group, &pos);

	return group;
}

void RedditAMAPanel::CreateOverlayBackground(OBSScene scene, OBSSceneItem group)
{
	OBSData settings = obs_data_create();
	obs_data_set_int(settings, "color", 0xFF4accff);
	obs_data_set_int(settings, "width", 480);
	obs_data_set_int(settings, "height", 192);
	obs_data_set_bool(settings, "hidden", true);
	OBSSource bg = obs_source_create("color_source", "#AMA_BG",
	                                 settings, nullptr);

	OBSData opacitySettings = obs_data_create();
	obs_data_set_int(opacitySettings, "opacity", 95);
	OBSSource colorFilter = obs_source_create(
		"color_filter", "Opacity", opacitySettings, nullptr);
	obs_source_filter_add(bg, colorFilter);

	OBSSceneItem bgItem = obs_scene_add(scene, bg);
	obs_sceneitem_group_add_item(group, bgItem);
}

void RedditAMAPanel::CreateOverlayUsername(OBSScene scene, OBSSceneItem group)
{
	OBSData settings = obs_data_create();
	obs_data_set_int(settings, "custom_width", 386);
	obs_data_set_bool(settings, "word_wrap", true);
	obs_data_set_int(settings, "color1", 0xff000000);
	obs_data_set_int(settings, "color2", 0xFF000000);
	obs_data_set_bool(settings, "hidden", true);
	OBSData fontSettings = obs_data_create();
	obs_data_set_string(fontSettings, "face", "Arial");
	obs_data_set_int(fontSettings, "size", 24);
	obs_data_set_string(fontSettings, "style", "Regular");
	obs_data_set_int(fontSettings, "flags", 0);
	obs_data_set_obj(settings, "font", fontSettings);
	OBSSource text = obs_source_create("text_ft2_source",
	                                   "#AMA_USERNAME", settings,
	                                   nullptr);

	OBSSceneItem textItem = obs_scene_add(scene, text);
	obs_sceneitem_group_add_item(group, textItem);

	vec2 pos;
	pos.x = 88;
	pos.y = 24;
	obs_sceneitem_set_pos(textItem, &pos);
	obs_sceneitem_set_order(textItem, OBS_ORDER_MOVE_TOP);
}

void RedditAMAPanel::CreateOverlayQuestion(OBSScene scene, OBSSceneItem group)
{
	OBSData settings = obs_data_create();
	obs_data_set_int(settings, "custom_width", 386);
	obs_data_set_bool(settings, "word_wrap", true);
	obs_data_set_int(settings, "color1", 0xFF000000);
	obs_data_set_int(settings, "color2", 0xFF000000);
	obs_data_set_bool(settings, "hidden", true);
	OBSData fontSettings = obs_data_create();
	obs_data_set_string(fontSettings, "face", "Arial");
	obs_data_set_int(fontSettings, "size", 26);
	obs_data_set_string(fontSettings, "style", "Regular");
	obs_data_set_int(fontSettings, "flags", 0);
	obs_data_set_obj(settings, "font", fontSettings);
	OBSSource text = obs_source_create("text_ft2_source",
	                                   "#AMA_QUESTION", settings,
	                                   nullptr);

	OBSSceneItem textItem = obs_scene_add(scene, text);
	obs_sceneitem_group_add_item(group, textItem);

	vec2 pos;
	pos.x = 88;
	pos.y = 58;
	obs_sceneitem_set_pos(textItem, &pos);
	obs_sceneitem_set_order(textItem, OBS_ORDER_MOVE_TOP);
}

void RedditAMAPanel::CreateOverlayLogo(OBSScene scene, OBSSceneItem group)
{
	std::string logoPath;
	GetDataFilePath("images/reddit.png", logoPath);

	OBSData settings = obs_data_create();
	obs_data_set_string(settings, "file", logoPath.c_str());
	obs_data_set_bool(settings, "hidden", true);
	OBSSource image = obs_source_create("image_source", "#AMA_REDDIT",
	                                    settings, nullptr);

	OBSSceneItem imageItem = obs_scene_add(scene, image);
	obs_sceneitem_group_add_item(group, imageItem);

	vec2 pos;
	pos.x = 16.0;
	pos.y = 16.0;
	obs_sceneitem_set_pos(imageItem, &pos);
	obs_sceneitem_set_order(imageItem, OBS_ORDER_MOVE_TOP);
}

void RedditAMAPanel::OnSelectionChanged(const QModelIndex &current,
                                        const QModelIndex &)
{
	QSharedPointer<Question> question = model->getQuestion(current.row());
	if (question.isNull()) {
		return;
	}

	OBSScene scene = obs_scene_from_source(
		obs_frontend_get_current_scene());
	OBSSceneItem overlay = obs_scene_find_source(scene, "#AMA_GROUP");

	switch (current.column()) {
	case 1: {
		ui->usernameLabel->setText(question->username);
		ui->questionLabel->setText(question->question);
		ui->dismissButton->setEnabled(true);
		ui->incomingQuestionTable->selectColumn(0);
		model->removeQuestion(current.row());

		// TODO: save timecode for TOC

		if (!overlay) {
			overlay = CreateOverlay();
		}
		obs_sceneitem_set_order(overlay, OBS_ORDER_MOVE_TOP);
		obs_sceneitem_set_visible(overlay, true);

		OBSScene overlayGroup =
			obs_sceneitem_group_get_scene(overlay);

		OBSSceneItem usernameItem = obs_scene_find_source(
			overlayGroup, "#AMA_USERNAME");
		if (usernameItem) {
			OBSSource username = obs_sceneitem_get_source(
				usernameItem);
			OBSData settings =
				obs_source_get_settings(username);
			obs_data_set_string(settings, "text",
			                    QString("u/%1")
			                    .arg(question->username)
			                    .toUtf8());
			obs_source_update(username, settings);
		}
		OBSSceneItem questionItem = obs_scene_find_source(
			overlayGroup, "#AMA_QUESTION");
		if (questionItem) {
			OBSSource q = obs_sceneitem_get_source(
				questionItem);
			OBSData settings =
				obs_source_get_settings(q);
			obs_data_set_string(settings, "text",
			                    question->question.toUtf8());
			obs_source_update(q, settings);
			obs_source_update_properties(q);
		}

		break;
	}
	case 2:
		ui->incomingQuestionTable->selectColumn(0);
		model->removeQuestion(current.row());
		if (overlay) {
			obs_sceneitem_set_visible(overlay, false);
		}
		break;
	}
}

void RedditAMAPanel::OnDismissQuestion()
{
	ClearPanel();
	// save timestamp and question
	HideOverlay();
}

AMAModel::AMAModel(QWidget *parent)
	: QAbstractTableModel(parent)
{
	acceptIcon.reset(new QPixmap(":/res/images/ok_32.png"));
	removeIcon.reset(new QPixmap(":/res/images/invalid_32.png"));
}

QVariant AMAModel::data(const QModelIndex &index, int role) const
{
	if (index.isValid() &&
	    index.row() < questions.size() &&
	    index.row() >= 0) {
		switch (role) {
		case Qt::DisplayRole:
			if (index.column() == 0) {
				auto q = questions[index.row()];
				auto cell = QString("%1\n%2")
				            .arg(q->username)
				            .arg(q->question);
				return cell;
			}
			break;
		case Qt::DecorationRole:
			switch (index.column()) {
			case 1:
				return *acceptIcon;
			case 2:
				return *removeIcon;
			}
		case Qt::SizeHintRole:
			if (index.column() != 0) {
				return acceptIcon->size();
			}
			break;
		}
	}
	return QVariant();
}

QVariant AMAModel::headerData(int section, Qt::Orientation orientation,
                              int role) const
{
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
		switch (section) {
		case 0:
			return QString("Question");
		case 1:
			return QString("Accept");
		case 2:
			return QString("Reject");
		}
	}
	return QVariant();
}

void AMAModel::addQuestion(const QString &question, const QString &username)
{
	int endIndex = questions.length();
	beginInsertRows(QModelIndex(), endIndex, endIndex);

	questions.push_back(
		QSharedPointer<Question>(new Question(question, username)));
	endInsertRows();
}

QSharedPointer<Question> AMAModel::getQuestion(int row)
{
	if (row < questions.length() && row >= 0) {
		return QSharedPointer<Question>(questions[row]);
	}
	return nullptr;
}

void AMAModel::removeQuestion(int row)
{
	if (questions.length() == 0) {
		return;
	}
	beginRemoveRows(QModelIndex(), row, row);
	questions.removeAt(row);
	endRemoveRows();
}

void AMAModel::clear()
{
	if (questions.length() == 0) {
		return;
	}
	beginRemoveRows(QModelIndex(), 0, questions.length() - 1);
	questions.clear();
	endRemoveRows();
}

namespace {

void on_frontend_event(enum obs_frontend_event event, void *param)
{
	auto *panel = static_cast<RedditAMAPanel *>(param);
	if (!panel->IsActive()) {
		return;
	}

	switch (event) {
	case OBS_FRONTEND_EVENT_STREAMING_STARTED:
		panel->SetPage(PAGE_STREAMING);
		panel->CreateOverlay();
		break;
	case OBS_FRONTEND_EVENT_STREAMING_STOPPING:
		// TODO: post TOC comment
	case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
		panel->SetPage(PAGE_OFFLINE);
		panel->RemoveUI();
	case OBS_FRONTEND_EVENT_EXIT:
		panel->DestroyOverlay();
		break;
	case OBS_FRONTEND_EVENT_SCENE_CHANGED:
		panel->DestroyOverlay();
		break;
	}
}

}
