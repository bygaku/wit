#include "main_window.h"

#include <QWidget>
#include <QSplitter>
#include <QVBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QStackedWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>

#include "project_serializer.h"

#include "object_tree_widget.h"
#include "cue_properties_editor_widget.h"
#include "project_properties_editor_widget.h"

namespace {
constexpr auto	WINDOW_WIDTH  		= 1080;
constexpr auto	WINDOW_HEIGHT 		=  720;
constexpr auto	LEFT_WIDTH_RATIO	= 2;
constexpr auto	RIGHT_WIDTH_RATIO 	= 5;
constexpr auto	TOP_HEIGHT_RATIO	= 1;
constexpr auto	BOTTOM_HEIGHT_RATIO = 3;

constexpr auto	PROJECT_FILE_FILTER = "WitStudio Project (*.wsp)";
constexpr auto	PROJECT_FILE_SUFFIX = "wsp";

/**
 * @brief Inspector stack page order.
 */
enum InspectorPage {
	PAGE_EMPTY   = 0,	///< Nothing selected on the tree
	PAGE_PROJECT = 1,	///< Project / collection selected
	PAGE_CUE     = 2,	///< Cue selected
};
}

namespace wit::studio {

MainWindow::MainWindow() {
    resize(WINDOW_WIDTH, WINDOW_HEIGHT);

	project_ = ProjectSerializer::MakeNewProject("新規プロジェクト");

	BuildBodyLayout();
}

/* =====================================================================
 * UI construction
 * ===================================================================== */
void MainWindow::BuildMenus() {
}

void MainWindow::BuildBodyLayout() {
	auto* vertical_split	= new QSplitter(Qt::Vertical, this);
	auto* horizontal_split	= new QSplitter(Qt::Horizontal, vertical_split);

	auto palette = QPalette();

	// Upper left side
	object_tree_ = new ObjectTreeWidget(this);
	{
		object_tree_->onAddCueCollectionRequested = [this]() {
			AddCollection();
		};

		object_tree_->onAddCueRequested = [this](QTreeWidgetItem* parent_collection) {
			AddCue(parent_collection);
		};

		object_tree_->onEditCueCollectionNameRequested = [this](QTreeWidgetItem* item, const QString& new_name) {
			RenameCollection(item, new_name);
		};

		object_tree_->onItemSelected = [this](int item_type, const QString& name) {
			OnTreeSelectionChanged(item_type, name);
		};

		horizontal_split->addWidget(object_tree_);
	}

	// Upper right side
	inspector_view_ = new QStackedWidget(this);
	{
		auto* empty_page	= new QWidget(inspector_view_);
		auto* empty_layout	= new QVBoxLayout(empty_page);
		auto* hint			= new QLabel("左のツリーから項目を選ぶと、ここに設定が表示されます。", empty_page);

		hint->setAlignment(Qt::AlignCenter);
		hint->setWordWrap(true);
		empty_layout->addWidget(hint);
		inspector_view_->addWidget(empty_page);

		project_properties_editor_ = new ProjectPropertiesEditorWidget(inspector_view_);
		project_properties_editor_->on_changed = [this]() {
			/* mark dirty when wired */
		};
		inspector_view_->addWidget(project_properties_editor_);

		cue_properties_editor_ = new CuePropertiesEditorWidget(inspector_view_);
		cue_properties_editor_->on_changed = [this]() {
			/* mark dirty when wired */
		};
		inspector_view_->addWidget(cue_properties_editor_);

		inspector_view_->setCurrentIndex(PAGE_EMPTY);

		horizontal_split->addWidget(inspector_view_);
		horizontal_split->setStretchFactor(0, LEFT_WIDTH_RATIO);
		horizontal_split->setStretchFactor(1, RIGHT_WIDTH_RATIO);
	}

	// Body bottom
	auto bottom_widget = new QWidget(this);
	{
		bottom_widget->setAutoFillBackground(true);
		palette = bottom_widget->palette();
		palette.setColor(QPalette::Window, QColor("#0000ff"));
		bottom_widget->setPalette(palette);
	}

	{
		vertical_split->addWidget(horizontal_split);
		vertical_split->addWidget(bottom_widget);
		vertical_split->setStretchFactor(0, TOP_HEIGHT_RATIO);
		vertical_split->setStretchFactor(1, BOTTOM_HEIGHT_RATIO);
	}

	auto width  = WINDOW_WIDTH  / (LEFT_WIDTH_RATIO + RIGHT_WIDTH_RATIO);
	auto height = WINDOW_HEIGHT / (TOP_HEIGHT_RATIO + BOTTOM_HEIGHT_RATIO);

	vertical_split->setSizes({height * TOP_HEIGHT_RATIO, height * BOTTOM_HEIGHT_RATIO});
	horizontal_split->setSizes({width * LEFT_WIDTH_RATIO, width * RIGHT_WIDTH_RATIO});

	// Register as the central widget in the main window
	setCentralWidget(vertical_split);
}

/* =====================================================================
 * File actions
 * ===================================================================== */

void MainWindow::NewProject() {
	if (!ConfirmDiscardChanges()) return;

	ReplaceProject(ProjectSerializer::MakeNewProject("新規プロジェクト"), {});
}

void MainWindow::OpenProject() {
	if (!ConfirmDiscardChanges()) return;

	const auto selected = QFileDialog::getOpenFileName(
		this, "プロジェクトを開く", QString(), PROJECT_FILE_FILTER);
	if (selected.isEmpty()) return;	///< user cancelled

	const std::filesystem::path path = selected.toStdString();

	ProjectModel loaded;
	std::string  err;
	if (!ProjectSerializer::Load(path, loaded, err)) {
		QMessageBox::critical(this, "読み込みエラー", QString::fromStdString(err));
		return;
	}

	ReplaceProject(std::move(loaded), path);
}

bool MainWindow::SaveProject() {
	// Fall back to Save As when the project has never been written to disk.
	if (current_path_.empty()) {
		return SaveProjectAs();
	}

	std::string err;
	if (!ProjectSerializer::SaveToFile(current_path_, project_, err)) {
		QMessageBox::critical(this, "保存エラー", QString::fromStdString(err));
		return false;
	}

	dirty_ = false;
	UpdateWindowTitle();
	return true;
}

bool MainWindow::SaveProjectAs() {
	QString selected = QFileDialog::getSaveFileName(
		this, "名前を付けて保存", QString(), PROJECT_FILE_FILTER);
	if (selected.isEmpty()) return false;	///< user cancelled

	std::filesystem::path path = selected.toStdString();
	if (path.extension().empty()) {
		path.replace_extension(PROJECT_FILE_SUFFIX);
	}

	std::string err;
	if (!ProjectSerializer::SaveToFile(path, project_, err)) {
		QMessageBox::critical(this, "保存エラー", QString::fromStdString(err));
		return false;
	}

	current_path_ = path;
	dirty_        = false;
	UpdateWindowTitle();

	return true;
}

bool MainWindow::ConfirmDiscardChanges() {
	if (!dirty_) return true;	///< nothing to lose

	const auto choice = QMessageBox::warning(
		this, "確認",
		"保存されていない変更があります。保存しますか？",
		QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
		QMessageBox::Save);

	switch (choice) {
		case QMessageBox::Save:
			return SaveProject();
		case QMessageBox::Discard:
			return true;
		default:
			return false;
	}
}

void MainWindow::closeEvent(QCloseEvent* event) {
	if (ConfirmDiscardChanges()) {
		event->accept();
	} else {
		event->ignore();
	}
}

/* =====================================================================
 * Edit actions
 * ===================================================================== */

void MainWindow::AddCollection() {
	if (object_tree_ == nullptr) return;

	std::vector<std::string> taken;
	taken.reserve(project_.cue_collections.size());

	for (const auto& col : project_.cue_collections) {
		taken.push_back(col.name);
	}

	const std::string name = MakeUniqueName("新規CueCollection", taken);

	project_.cue_collections.push_back(ProjectSerializer::MakeNewCollection(name));

	object_tree_->AddCueCollectionIntoTree(QString::fromStdString(name));
	MarkDirty();
}

void MainWindow::AddCue(QTreeWidgetItem* parent_collection) {
	if (object_tree_ == nullptr || parent_collection == nullptr) return;	///< CueCollection does not exist

	const int collection_index = object_tree_->TopLevelIndexOf(parent_collection);
	if (collection_index < 0 ||	collection_index >= static_cast<int>(project_.cue_collections.size())) return;

	CueCollectionModel& collection = project_.cue_collections[collection_index];

	std::vector<std::string> taken;
	taken.reserve(collection.cues.size());
	for (const auto& cue : collection.cues) {
		taken.push_back(cue.cue_name);
	}

	CueModel cue;
	cue.cue_name = MakeUniqueName("新規Cue", taken);
	cue.cue_id   = project_.next_cue_id++;
	collection.cues.push_back(std::move(cue));

	object_tree_->AddCueIntoCueCollection(parent_collection, QString::fromStdString(collection.cues.back().cue_name));
	MarkDirty();
}

void MainWindow::RenameCollection(QTreeWidgetItem* item, const QString& new_name) {
	if (object_tree_ == nullptr || item == nullptr) return;
	if (new_name.isEmpty()) return;

	const int collection_index = object_tree_->TopLevelIndexOf(item);
	if (collection_index < 0 ||	collection_index >= static_cast<int>(project_.cue_collections.size())) return;

	project_.cue_collections[collection_index].name = new_name.toStdString();
	item->setText(0, new_name);
	MarkDirty();
}

std::string MainWindow::MakeUniqueName(const std::string& base, const std::vector<std::string>& taken) {
	auto is_taken = [&taken](const std::string& candidate) {
		for (const auto& name : taken) {
			if (name == candidate) return true;
		}
		return false;
	};

	if (!is_taken(base)) return base;

	for (int suffix = 2; ; ++suffix) {
		const std::string candidate = base + " " + std::to_string(suffix);
		if (!is_taken(candidate)) return candidate;
	}
}

void MainWindow::OnTreeSelectionChanged(int item_type, const QString& /*name*/) {
	if (inspector_view_ == nullptr) return;

	int collection_index;
	int cue_index;
	object_tree_->CurrentSelection(collection_index, cue_index);	///< Current Selection on the tree

	if (item_type == ObjectTreeWidget::Cue) {
		// A cue is selected: resolve it and bind the cue editor.
		if (collection_index < 0 || collection_index >= static_cast<int>(project_.cue_collections.size())) {
			inspector_view_->setCurrentIndex(PAGE_EMPTY);
			return;
		}

		CueCollectionModel& collection = project_.cue_collections[collection_index];
		if (cue_index < 0 || cue_index >= static_cast<int>(collection.cues.size())) {
			inspector_view_->setCurrentIndex(PAGE_EMPTY);
			return;
		}

		cue_properties_editor_->Bind(&collection.cues[cue_index], project_.categories);
		inspector_view_->setCurrentIndex(PAGE_CUE);

		return;
	}

	// A collection (or anything else) shows the project-wide settings.
	project_properties_editor_->Bind(&project_);
	inspector_view_->setCurrentIndex(PAGE_PROJECT);
}

void MainWindow::DeleteSelectedItem() {

}

void MainWindow::OpenCategoryEditor() {

}

/* =====================================================================
 * Build
 * ===================================================================== */

void MainWindow::RunBuild() {
}

/* =====================================================================
 * Project lifecycle
 * ===================================================================== */

void MainWindow::ReplaceProject(ProjectModel&& project, const std::filesystem::path& path) {
	project_      = std::move(project);
	current_path_ = path;
	dirty_        = false;

	if (inspector_view_ != nullptr) {
		inspector_view_->setCurrentIndex(PAGE_EMPTY);
	}

	RebuildTreeFromProject();
	UpdateWindowTitle();
}

void MainWindow::RebuildTreeFromProject() {
	if (object_tree_ == nullptr) return;

	object_tree_->Clear();

	for (const auto& collection : project_.cue_collections) {
		QTreeWidgetItem* collection_item =
			object_tree_->AddCueCollectionIntoTree(QString::fromStdString(collection.name));

		for (const auto& cue : collection.cues) {
			object_tree_->AddCueIntoCueCollection(
				collection_item, QString::fromStdString(cue.cue_name));
		}
	}
}

void MainWindow::MarkDirty() {
	if (dirty_) return;

	dirty_ = true;
	UpdateWindowTitle();
}

void MainWindow::UpdateWindowTitle() {
	const QString name = current_path_.empty()
		? QString::fromStdString(project_.project_name)
		: QString::fromStdString(current_path_.filename().string());

	QString title = name;
	if (dirty_) title.prepend('*');
	title += " - WitStudio";

	setWindowTitle(title);
}

}