#include "main_window.h"

#include <QWidget>
#include <QCloseEvent>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QStackedWidget>
#include <QProgressDialog>

#include "project_serializer.h"
#include "project_builder.h"

#include "object_tree_widget.h"
#include "cue_properties_editor_widget.h"
#include "cue_collection_properties_editor_widget.h"
#include "project_properties_editor_widget.h"
#include "category_editor_dialog.h"
#include "issue_list_widget.h"

namespace {
constexpr auto	WINDOW_WIDTH  		= 1400;
constexpr auto	WINDOW_HEIGHT 		= 1050;
constexpr auto	LEFT_WIDTH_RATIO	= 2;
constexpr auto	RIGHT_WIDTH_RATIO 	= 5;
constexpr auto	TOP_HEIGHT_RATIO	= 3;


#ifdef VERSION_RELEASE_1_0
constexpr auto	BOTTOM_HEIGHT_RATIO = 3;
#else
constexpr auto	BOTTOM_HEIGHT_RATIO = 1;
#endif

constexpr auto	PROJECT_FILE_FILTER = "WitStudio Project (*.wsp)";
constexpr auto	PROJECT_FILE_SUFFIX = "wsp";
constexpr auto	BUILD_OUTPUT_DIR    = "wit_assets";

/**
 * @brief Inspector stack page order.
 */
enum InspectorPage {
	PAGE_EMPTY      = 0,	///< Nothing selected on the tree
	PAGE_PROJECT    = 1,	///< Project selected
	PAGE_COLLECTION = 2,	///< Cue collection selected
	PAGE_CUE        = 3,	///< Cue selected
};
}

namespace wit::studio {

MainWindow::MainWindow() {
    resize(WINDOW_WIDTH, WINDOW_HEIGHT);

	project_ = ProjectSerializer::MakeNewProject("新規プロジェクト");

	BuildMenus();
	BuildBodyLayout();
	RebuildTreeFromProject();
	UpdateWindowTitle();
}

/* =====================================================================
 * UI construction
 * ===================================================================== */
void MainWindow::BuildMenus() {
	auto* file_menu = menuBar()->addMenu("ファイル(&F)");

	auto* new_action = file_menu->addAction("新しいプロジェクト(&N)");
	new_action->setShortcut(QKeySequence::New);
	connect(new_action, &QAction::triggered, this, [this]() { NewProject(); });

	auto* open_action = file_menu->addAction("プロジェクトを開く(&O)...");
	open_action->setShortcut(QKeySequence::Open);
	connect(open_action, &QAction::triggered, this, [this]() { OpenProject(); });

	file_menu->addSeparator();

	auto* save_action = file_menu->addAction("上書き保存(&S)");
	save_action->setShortcut(QKeySequence::Save);
	connect(save_action, &QAction::triggered, this, [this]() { SaveProject(); });

	auto* save_as_action = file_menu->addAction("名前を付けて保存(&A)...");
	save_as_action->setShortcut(QKeySequence::SaveAs);
	connect(save_as_action, &QAction::triggered, this, [this]() { SaveProjectAs(); });

	file_menu->addSeparator();

	auto* quit_action = file_menu->addAction("終了(&Q)");
	quit_action->setShortcut(QKeySequence::Quit);
	connect(quit_action, &QAction::triggered, this, [this]() { close(); });

	auto* edit_menu = menuBar()->addMenu("編集(&E)");

	auto* delete_action = edit_menu->addAction("選択項目を削除(&D)");
	connect(delete_action, &QAction::triggered, this, [this]() { DeleteSelectedItem(); });

	edit_menu->addSeparator();

	auto* category_action = edit_menu->addAction("カテゴリを編集(&C)...");
	connect(category_action, &QAction::triggered, this, [this]() { OpenCategoryEditor(); });

	auto* build_menu = menuBar()->addMenu("ビルド(&B)");

	auto* build_action = build_menu->addAction("プロジェクトをビルド(&B)");
	build_action->setShortcut(QKeySequence("F7"));
	connect(build_action, &QAction::triggered, this, [this]() { RunBuild(); });
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

		object_tree_->onItemRenamed = [this](QTreeWidgetItem* item, const QString& new_name) {
			OnItemRenamed(item, new_name);
		};

		object_tree_->onDeleteItemRequested = [this](QTreeWidgetItem* item) {
			OnDeleteItemRequested(item);
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
			object_tree_->SetProjectLabel(QString::fromStdString(project_.project_name));
			MarkDirty();
			UpdateWindowTitle();
		};
		inspector_view_->addWidget(project_properties_editor_);

		cue_collection_properties_editor_ = new CueCollectionPropertiesEditorWidget(inspector_view_);
		cue_collection_properties_editor_->on_changed = [this]() {
			MarkDirty();
		};
		cue_collection_properties_editor_->name_change_requester = [this](const QString& new_name) -> bool {
			return RenameSelectedCollection(new_name);
		};
		inspector_view_->addWidget(cue_collection_properties_editor_);

		cue_properties_editor_ = new CuePropertiesEditorWidget(inspector_view_);
		cue_properties_editor_->on_changed = [this]() {
			MarkDirty();
		};
		cue_properties_editor_->project_dir_provider = [this]() -> std::filesystem::path {
			return current_path_.empty() ? std::filesystem::path() : current_path_.parent_path();
		};
		cue_properties_editor_->name_change_requester = [this](const QString& new_name) -> bool {
			return RenameSelectedCue(new_name);
		};
		inspector_view_->addWidget(cue_properties_editor_);

		inspector_view_->setCurrentIndex(PAGE_EMPTY);

		horizontal_split->addWidget(inspector_view_);
		horizontal_split->setStretchFactor(0, LEFT_WIDTH_RATIO);
		horizontal_split->setStretchFactor(1, RIGHT_WIDTH_RATIO);
	}

	// Body bottom
	issue_list_ = new IssueListWidget(this);

	{
		vertical_split->addWidget(horizontal_split);
		vertical_split->addWidget(issue_list_);
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

	const auto name = MakeUniqueName("新規CueCollection", taken);

	project_.cue_collections.push_back(ProjectSerializer::MakeNewCollection(name));

	object_tree_->AddCueCollectionIntoProject(QString::fromStdString(name));
	MarkDirty();
}

void MainWindow::AddCue(QTreeWidgetItem* parent_collection) {
	if (object_tree_ == nullptr || parent_collection == nullptr) return;	///< CueCollection does not exist

	const int collection_index = object_tree_->CollectionIndexOf(parent_collection);
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

	const int collection_index = object_tree_->CollectionIndexOf(item);
	ApplyCollectionRename(collection_index, new_name.trimmed().toStdString());
}

bool MainWindow::RenameSelectedCollection(const QString& new_name) {
	if (object_tree_ == nullptr) return false;

	int collection_index;
	int cue_index;
	object_tree_->CurrentSelection(collection_index, cue_index);
	if (cue_index >= 0) return false;

	return ApplyCollectionRename(collection_index, new_name.trimmed().toStdString());
}

bool MainWindow::RenameSelectedCue(const QString& new_name) {
	if (object_tree_ == nullptr) return false;

	int collection_index;
	int cue_index;
	object_tree_->CurrentSelection(collection_index, cue_index);
	if (cue_index < 0) return false;	///< no cue is selected

	return ApplyCueRename(collection_index, cue_index, new_name.trimmed().toStdString());
}

bool MainWindow::ApplyCollectionRename(int collection_index, const std::string& new_name) {
	if (collection_index < 0 || collection_index >= static_cast<int>(project_.cue_collections.size())) {
		return false;
	}

	CueCollectionModel& collection = project_.cue_collections[collection_index];
	if (new_name == collection.name) return true;	///< no change
	if (new_name.empty()) return false;

	for (std::size_t itr = 0; itr < project_.cue_collections.size(); ++itr) {
		if (static_cast<int>(itr) == collection_index) continue;
		if (project_.cue_collections[itr].name == new_name) {
			QMessageBox::warning(this, "名前を変更",
				"同じ名前のキューコレクションが既に存在します。");
			return false;
		}
	}

	collection.name = new_name;

	if (QTreeWidgetItem* item = object_tree_->CollectionItemAt(collection_index)) {
		object_tree_->SetItemTextSilently(item, QString::fromStdString(new_name));
	}

	MarkDirty();
	return true;
}

bool MainWindow::ApplyCueRename(int collection_index, int cue_index, const std::string& new_name) {
	if (collection_index < 0 || collection_index >= static_cast<int>(project_.cue_collections.size())) {
		return false;
	}

	CueCollectionModel& collection = project_.cue_collections[collection_index];
	if (cue_index < 0 || cue_index >= static_cast<int>(collection.cues.size())) return false;

	CueModel& cue = collection.cues[cue_index];
	if (new_name == cue.cue_name) return true;	///< no change
	if (new_name.empty()) return false;

	for (std::size_t itr = 0; itr < collection.cues.size(); ++itr) {
		if (static_cast<int>(itr) == cue_index) continue;
		if (collection.cues[itr].cue_name == new_name) {
			QMessageBox::warning(this, "名前を変更",
				"同じ名前のキューが既に存在します。");
			return false;
		}
	}

	cue.cue_name = new_name;

	if (QTreeWidgetItem* item = object_tree_->CueItemAt(collection_index, cue_index)) {
		object_tree_->SetItemTextSilently(item, QString::fromStdString(new_name));
	}

	MarkDirty();
	return true;
}

void MainWindow::OnItemRenamed(QTreeWidgetItem* item, const QString& new_name) {
	if (rebuilding_ || object_tree_ == nullptr || item == nullptr) return;

	int collection_index;
	int cue_index;
	object_tree_->ResolveItem(item, collection_index, cue_index);

	if (collection_index < 0 || collection_index >= static_cast<int>(project_.cue_collections.size())) {
		return;
	}

	const auto trimmed = new_name.trimmed().toStdString();

	if (cue_index >= 0) {
		if (cue_index >= static_cast<int>(project_.cue_collections[collection_index].cues.size())) return;
		if (!ApplyCueRename(collection_index, cue_index, trimmed)) {
			const auto& current = project_.cue_collections[collection_index].cues[cue_index].cue_name;
			object_tree_->SetItemTextSilently(item, QString::fromStdString(current));
		}
		return;
	}

	if (!ApplyCollectionRename(collection_index, trimmed)) {
		const auto& current = project_.cue_collections[collection_index].name;
		object_tree_->SetItemTextSilently(item, QString::fromStdString(current));
	}
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

	// Cue properties editor.
	if (item_type == ObjectTreeWidget::Cue) {
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

	// CC properties editor.
	if (item_type == ObjectTreeWidget::CueCollection) {
		// A collection is selected: bind its dedicated page.
		if (collection_index < 0 || collection_index >= static_cast<int>(project_.cue_collections.size())) {
			inspector_view_->setCurrentIndex(PAGE_EMPTY);
			return;
		}

		cue_collection_properties_editor_->Bind(&project_.cue_collections[collection_index]);
		inspector_view_->setCurrentIndex(PAGE_COLLECTION);
		return;
	}

	// Project properties editor.
	if (item_type == ObjectTreeWidget::Project) {
		project_properties_editor_->Bind(&project_);
		inspector_view_->setCurrentIndex(PAGE_PROJECT);
		return;
	}

	inspector_view_->setCurrentIndex(PAGE_EMPTY);
}

void MainWindow::RefreshInspector() {
	if (inspector_view_ == nullptr) return;

	int collection_index;
	int cue_index;
	object_tree_->CurrentSelection(collection_index, cue_index);

	switch (inspector_view_->currentIndex()) {
		case PAGE_PROJECT: {
			project_properties_editor_->Bind(&project_);
			break;
		}
		case PAGE_COLLECTION: {
			if (collection_index >= 0 && collection_index < static_cast<int>(project_.cue_collections.size())) {
				cue_collection_properties_editor_->Bind(&project_.cue_collections[collection_index]);
			} else {
				inspector_view_->setCurrentIndex(PAGE_EMPTY);
			}
			break;
		}
		case PAGE_CUE: {
			if (collection_index >= 0 && collection_index < static_cast<int>(project_.cue_collections.size())) {
				CueCollectionModel& collection = project_.cue_collections[collection_index];
				if (cue_index >= 0 && cue_index < static_cast<int>(collection.cues.size())) {
					cue_properties_editor_->Bind(&collection.cues[cue_index], project_.categories);
					break;
				}
			}
			inspector_view_->setCurrentIndex(PAGE_EMPTY);
			break;
		}
		default:
			break;
	}
}

void MainWindow::DeleteSelectedItem() {
	if (object_tree_ == nullptr) return;

	int collection_index;
	int cue_index;
	object_tree_->CurrentSelection(collection_index, cue_index);
	if (collection_index < 0) return;

	QTreeWidgetItem* item = (cue_index >= 0)
		? object_tree_->CueItemAt(collection_index, cue_index)
		: object_tree_->CollectionItemAt(collection_index);

	OnDeleteItemRequested(item);
}

void MainWindow::OnDeleteItemRequested(QTreeWidgetItem* item) {
	if (object_tree_ == nullptr || item == nullptr) return;

	int collection_index;
	int cue_index;
	object_tree_->ResolveItem(item, collection_index, cue_index);
	if (collection_index < 0 || collection_index >= static_cast<int>(project_.cue_collections.size())) return;

	CueCollectionModel& collection = project_.cue_collections[collection_index];

	// Delete cue.
	if (cue_index >= 0) {
		if (cue_index >= static_cast<int>(collection.cues.size())) return;

		const auto cue_name = QString::fromStdString(collection.cues[cue_index].cue_name);
		const auto choice = QMessageBox::question(
			this, "キューを削除",
			QString("キュー「%1」を削除しますか？\nこの操作は元に戻せません。").arg(cue_name),
			QMessageBox::Yes | QMessageBox::No,
			QMessageBox::No);
		if (choice != QMessageBox::Yes) return;

		inspector_view_->setCurrentIndex(PAGE_EMPTY);

		collection.cues.erase(collection.cues.begin() + cue_index);
		object_tree_->RemoveItem(item);
		MarkDirty();

		return;
	}

	// Delete CC.
	{
		const auto col_name = QString::fromStdString(collection.name);
		const auto choice = QMessageBox::question(
			this, "コレクションを削除",
			QString("コレクション「%1」と、その中のすべてのキュー (%2 個) を削除しますか？\n"
					"この操作は元に戻せません。")
				.arg(col_name)
				.arg(collection.cues.size()),
			QMessageBox::Yes | QMessageBox::No,
			QMessageBox::No);
		if (choice != QMessageBox::Yes) return;
	}

	inspector_view_->setCurrentIndex(PAGE_EMPTY);

	project_.cue_collections.erase(project_.cue_collections.begin() + collection_index);
	object_tree_->RemoveItem(item);
	MarkDirty();
}

void MainWindow::OpenCategoryEditor() {
	CategoryEditorDialog dialog(project_, this);
	if (dialog.exec() != QDialog::Accepted) return;

	project_.categories = dialog.Result();
	MarkDirty();
	RefreshInspector();
}

/* =====================================================================
 * Build
 * ===================================================================== */

void MainWindow::RunBuild() {
	if (current_path_.empty()) {
		QMessageBox::information(this, "ビルド",
			"ビルドの前にプロジェクトを保存してください。");
		if (!SaveProjectAs()) return;
	} else if (dirty_) {
		if (!SaveProject()) return;
	}

	const std::filesystem::path project_dir = current_path_.parent_path();
	const std::filesystem::path output_dir  = project_dir / BUILD_OUTPUT_DIR;

	QProgressDialog progress("ビルドしています...", "中断不可", 0, 0, this);
	progress.setWindowModality(Qt::WindowModal);
	progress.setCancelButton(nullptr);	///< The builder has no cancel path in v1.0
	progress.setMinimumDuration(0);
	progress.show();

	const BuildResult res = ProjectBuilder::Build(
		project_, project_dir, output_dir,
		[&progress](const BuildProgress& p) {
			progress.setMaximum(static_cast<int>(p.total_waveforms));
			progress.setValue(static_cast<int>(p.processed_waveforms));
			progress.setLabelText(QString("ビルドしています...\n%1")
				.arg(QString::fromStdString(p.current_item)));
			QCoreApplication::processEvents();
		});

	progress.close();

	issue_list_->ShowIssues(res.issues);

	if (res.succeeded) {
		QMessageBox::information(this, "ビルド",
			QString("ビルドが完了しました。\n出力先: %1")
				.arg(QString::fromStdString(output_dir.string())));
	} else {
		QMessageBox::critical(this, "ビルド",
			"ビルドに失敗しました。下部の一覧でエラー内容を確認してください。");
	}
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

	rebuilding_ = true;

	object_tree_->ResetWithProject(QString::fromStdString(project_.project_name));

	for (const auto& collection : project_.cue_collections) {
		QTreeWidgetItem* collection_item =
			object_tree_->AddCueCollectionIntoProject(QString::fromStdString(collection.name));

		for (const auto& cue : collection.cues) {
			object_tree_->AddCueIntoCueCollection(
				collection_item, QString::fromStdString(cue.cue_name));
		}
	}

	rebuilding_ = false;
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