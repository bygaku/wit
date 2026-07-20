#include "category_editor_dialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include <set>

namespace wit::studio {

namespace {

/**
 * @brief Category table columns.
 */
enum CategoryColumn {
	COL_NAME = 0,
	COL_VOLUME_DB,
	COL_DUCK_DB,
	COL_IS_DUCKER,
	COL_COUNT,
};

}  // namespace

CategoryEditorDialog::CategoryEditorDialog(const ProjectModel& project, QWidget* parent)
	: QDialog(parent)
	, project_(project)
	, categories_(project.categories) {
	setWindowTitle("カテゴリを編集");
	resize(560, 360);

	BuildLayout();
	ReloadTable();
}

/* =====================================================================
 * UI construction
 * ===================================================================== */
void CategoryEditorDialog::BuildLayout() {
	auto* root = new QVBoxLayout(this);

	auto* hint = new QLabel("カテゴリごとの基準音量と、ダッキング時の減衰量を設定します。\n"
							"プリセットカテゴリは名前の変更と削除ができません。", this);
	hint->setWordWrap(true);
	root->addWidget(hint);

	table_ = new QTableWidget(this);
	table_->setColumnCount(COL_COUNT);
	table_->setHorizontalHeaderLabels({ "カテゴリ", "音量 (dB)", "ダッキング減衰 (dB)", "ダッカー" });
	table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	table_->verticalHeader()->setVisible(false);
	table_->setSelectionBehavior(QAbstractItemView::SelectRows);
	table_->setSelectionMode(QAbstractItemView::SingleSelection);
	root->addWidget(table_);

	auto* button_row = new QHBoxLayout();
	auto* add_button = new QPushButton("追加", this);
	remove_button_   = new QPushButton("削除", this);
	button_row->addWidget(add_button);
	button_row->addWidget(remove_button_);
	button_row->addStretch(1);
	root->addLayout(button_row);

	auto* dialog_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	root->addWidget(dialog_buttons);

	/* =====================================================================
	 * Signals
	 * ===================================================================== */
	connect(add_button, &QPushButton::clicked, this, [this]() {
		AddCategory();
	});

	connect(remove_button_, &QPushButton::clicked, this, [this]() {
		RemoveSelectedCategory();
	});

	connect(dialog_buttons, &QDialogButtonBox::accepted, this, [this]() {
		OnAccept();
	});

	connect(dialog_buttons, &QDialogButtonBox::rejected, this, [this]() {
		reject();
	});

	connect(table_, &QTableWidget::cellChanged, this, [this](int row, int column) {
		if (loading_) return;
		if (row < 0 || row >= static_cast<int>(categories_.size())) return;

		CategoryInfo& cate = categories_[row];
		QTableWidgetItem* item = table_->item(row, column);
		if (item == nullptr) return;

		switch (column) {
			case COL_NAME: {
				cate.name = item->text().trimmed().toStdString();
				break;
			}
			case COL_VOLUME_DB: {
				cate.static_volume_db = item->text().toFloat();
				break;
			}
			case COL_DUCK_DB: {
				cate.ducking_attenuation_db = item->text().toFloat();
				break;
			}
			case COL_IS_DUCKER: {
				cate.is_ducker = (item->checkState() == Qt::Checked);
				break;
			}
			default: return;
		}
	});
}

void CategoryEditorDialog::ReloadTable() {
	loading_ = true;

	table_->setRowCount(static_cast<int>(categories_.size()));
	for (int row = 0; row < static_cast<int>(categories_.size()); ++row) {
		const CategoryInfo& cate = categories_[row];

		auto* name_item = new QTableWidgetItem(QString::fromStdString(cate.name));
		if (cate.is_preset) name_item->setFlags(name_item->flags() & ~Qt::ItemIsEditable);

		table_->setItem(row, COL_NAME, name_item);

		table_->setItem(row, COL_VOLUME_DB,
			new QTableWidgetItem(QString::number(cate.static_volume_db, 'f', 1)));
		table_->setItem(row, COL_DUCK_DB,
			new QTableWidgetItem(QString::number(cate.ducking_attenuation_db, 'f', 1)));

		auto* ducker_item = new QTableWidgetItem();
		ducker_item->setFlags((ducker_item->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEditable);
		ducker_item->setCheckState(cate.is_ducker ? Qt::Checked : Qt::Unchecked);
		table_->setItem(row, COL_IS_DUCKER, ducker_item);
	}

	loading_ = false;
}

/* =====================================================================
 * Actions
 * ===================================================================== */
void CategoryEditorDialog::AddCategory() {
	uint16_t next_id = 0;
	for (const auto& cate : categories_) {
		if (cate.id >= next_id) next_id = static_cast<uint16_t>(cate.id + 1);
	}

	// Unique default name.
	std::string name;
	for (int suffix = 1; ; ++suffix) {
		name = "新規カテゴリ " + std::to_string(suffix);
		bool taken = false;
		for (const auto& cate : categories_) {
			if (cate.name == name) { taken = true; break; }
		}
		if (!taken) break;
	}

	CategoryInfo cate;
	cate.id                     = next_id;
	cate.name                   = name;
	cate.is_preset              = false;
	cate.static_volume_db       = 0.0f;
	cate.ducking_attenuation_db = -6.0f;
	cate.is_ducker              = false;
	categories_.push_back(std::move(cate));

	ReloadTable();
	table_->setCurrentCell(static_cast<int>(categories_.size()) - 1, COL_NAME);
}

void CategoryEditorDialog::RemoveSelectedCategory() {
	const int row = table_->currentRow();
	if (row < 0 || row >= static_cast<int>(categories_.size())) return;

	const CategoryInfo& cate = categories_[row];

	if (cate.is_preset) {
		QMessageBox::warning(this, "カテゴリを削除",
			"プリセットカテゴリは削除できません。");
		return;
	}

	if (IsCategoryInUse(cate.id)) {
		QMessageBox::warning(this, "カテゴリを削除",
			QString("カテゴリ「%1」は 1 つ以上のキューから使用されているため削除できません。\n"
					"先にキュー側のカテゴリを変更してください。")
				.arg(QString::fromStdString(cate.name)));
		return;
	}

	categories_.erase(categories_.begin() + row);
	ReloadTable();
}

void CategoryEditorDialog::OnAccept() {
	std::set<std::string> seen;
	for (const auto& cate : categories_) {
		if (cate.name.empty()) {
			QMessageBox::warning(this, "カテゴリを編集",
				"名前が空のカテゴリがあります。");
			return;
		}
		if (!seen.insert(cate.name).second) {
			QMessageBox::warning(this, "カテゴリを編集",
				QString("カテゴリ名「%1」が重複しています。")
					.arg(QString::fromStdString(cate.name)));
			return;
		}
	}

	accept();
}

bool CategoryEditorDialog::IsCategoryInUse(uint16_t category_id) const {
	for (const auto& collection : project_.cue_collections) {
		for (const auto& cue : collection.cues) {
			if (cue.category_id == category_id) return true;
		}
	}
	return false;
}

}