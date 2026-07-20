#include "project_properties_editor_widget.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

namespace wit::studio {

namespace {

/**
 * @brief Attach a hover explanation to a widget in the user's own terms.
 */
void SetHelp(QWidget* widget, const QString& text) {
	widget->setToolTip(text);
}

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

ProjectPropertiesEditorWidget::ProjectPropertiesEditorWidget(QWidget* parent) : QWidget(parent) {
	BuildLayout();
	Bind(nullptr);
}

/* =====================================================================
 * UI construction
 * ===================================================================== */
void ProjectPropertiesEditorWidget::BuildLayout() {
	auto* root = new QVBoxLayout(this);

	/* =====================================================================
	 * Main properties
	 * ===================================================================== */
	{
		auto*basic_group = new QGroupBox("プロジェクト設定", this);
		auto*basic_form  = new QFormLayout(basic_group);

		name_edit_       = new QLineEdit(basic_group);
		SetHelp(name_edit_, "プロジェクトの名前です。\n"
						 "プロジェクトをビルドすると、この名前で .wpb が出力されます。");
		basic_form->addRow("プロジェクト名", name_edit_);

		sample_rate_combo_ = new QComboBox(basic_group);
		sample_rate_combo_->addItem("44100 Hz", 44100);
		sample_rate_combo_->addItem("48000 Hz", 48000);
		SetHelp(sample_rate_combo_, "プロジェクト全体のサンプリングレートです。\n"
						"すべての音声はこのレートに変換されて取り込まれます。"
						"後から変えると全波形の再ビルドが必要です。");
		basic_form->addRow("サンプリングレート", sample_rate_combo_);

		bit_depth_combo_ = new QComboBox(basic_group);
		bit_depth_combo_->addItem("16 bit PCM", 16);
		bit_depth_combo_->addItem("32 bit float", 32);
		SetHelp(bit_depth_combo_, "音データのビット深度です。\n"
						"・16 bit：ファイルが小さく、ほとんどの用途に十分な音質です。*オススメ\n"
						"・32 bit：より高精度ですが、容量が倍になります。");
		basic_form->addRow("ビット深度", bit_depth_combo_);

		channels_combo_ = new QComboBox(basic_group);
		channels_combo_->addItem("ステレオ", 2);
		SetHelp(channels_combo_, "チャンネル数です。\n"
						"v1.0 ではステレオのみに対応しています。");
		basic_form->addRow("チャンネル", channels_combo_);

		root->addWidget(basic_group);
	}

	/* =====================================================================
	 * Sharp details
	 * ===================================================================== */
	detail_group_ = new QGroupBox("詳細設定", this);
	detail_group_->setCheckable(true);
	detail_group_->setChecked(false);
	SetHelp(detail_group_, "ダッキングやカテゴリごとの音量などをこだわりたいときの設定です。");
	{
		auto* detail_layout = new QVBoxLayout(detail_group_);
		auto* ducking_group = new QGroupBox("ダッキング", detail_group_);
		auto* ducking_form  = new QFormLayout(ducking_group);

		fade_in_spin_ = new QSpinBox(ducking_group);
		fade_in_spin_->setRange(0, 10000);
		fade_in_spin_->setSuffix(" ms");
		SetHelp(fade_in_spin_, "ダッキングで音量を下げ始めてから、下がりきるまでの時間です。");
		ducking_form->addRow("フェードイン", fade_in_spin_);

		fade_out_spin_ = new QSpinBox(ducking_group);
		fade_out_spin_->setRange(0, 10000);
		fade_out_spin_->setSuffix(" ms");
		SetHelp(fade_out_spin_, "ダッキングが終わって、音量が元に戻るまでの時間です。");
		ducking_form->addRow("フェードアウト", fade_out_spin_);

		detail_layout->addWidget(ducking_group);

		auto* category_label = new QLabel("カテゴリごとの音量・ダッキング", detail_group_);
		detail_layout->addWidget(category_label);

		category_table_ = new QTableWidget(detail_group_);
		category_table_->setColumnCount(COL_COUNT);
		category_table_->setHorizontalHeaderLabels({ "カテゴリ", "音量 (dB)", "ダッキング減衰 (dB)", "ダッカー" });
		category_table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		category_table_->verticalHeader()->setVisible(false);
		category_table_->setEditTriggers(QAbstractItemView::AllEditTriggers);
		SetHelp(category_table_, "カテゴリごとの基準音量と、ダッキング時にどれだけ下げるかを設定します。\n"
						"「ダッカー」を入れたカテゴリが鳴っている間、他のカテゴリが下がります。");
		detail_layout->addWidget(category_table_);

		root->addWidget(detail_group_);
		root->addStretch(1);
	}

	/* =====================================================================
	 * Signals
	 * ===================================================================== */
	connect(name_edit_, &QLineEdit::textEdited, this, [this](const QString& text) {
		if (loading_ || project_ == nullptr) return;
		project_->project_name = text.toStdString();
		if (on_changed) on_changed();
	});

	connect(sample_rate_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
		if (loading_ || project_ == nullptr) return;
		project_->audio_format.sample_rate = static_cast<uint32_t>(sample_rate_combo_->currentData().toUInt());
		if (on_changed) on_changed();
	});

	connect(bit_depth_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
		if (loading_ || project_ == nullptr) return;
		const int depth = bit_depth_combo_->currentData().toInt();
		project_->audio_format.bit_depth = static_cast<uint16_t>(depth);
		project_->audio_format.sample_format =
			(depth == 32) ? SampleFormat::FLOAT : SampleFormat::INT;
		if (on_changed) on_changed();
	});

	connect(channels_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
		if (loading_ || project_ == nullptr) return;
		project_->audio_format.channels = static_cast<uint8_t>(channels_combo_->currentData().toUInt());
		if (on_changed) on_changed();
	});

	connect(fade_in_spin_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) {
		if (loading_ || project_ == nullptr) return;
		project_->ducking.fade_in_ms = static_cast<uint32_t>(v);
		if (on_changed) on_changed();
	});

	connect(fade_out_spin_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) {
		if (loading_ || project_ == nullptr) return;
		project_->ducking.fade_out_ms = static_cast<uint32_t>(v);
		if (on_changed) on_changed();
	});

	connect(category_table_, &QTableWidget::cellChanged, this, [this](int row, int column) {
		if (loading_ || project_ == nullptr) return;
		if (row < 0 || row >= static_cast<int>(project_->categories.size())) return;

		CategoryInfo& cate = project_->categories[row];
		QTableWidgetItem* item = category_table_->item(row, column);
		if (item == nullptr) return;

		switch (column) {
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
			default:
				return;
		}

		if (on_changed) on_changed();
	});
}

/* =====================================================================
 * Binding
 * ===================================================================== */
void ProjectPropertiesEditorWidget::Bind(ProjectModel* project) {
	project_ = project;
	setEnabled(project_ != nullptr);
	LoadFromProject();
}

void ProjectPropertiesEditorWidget::LoadFromProject() {
	loading_ = true;

	if (project_ == nullptr) {
		name_edit_->clear();
		if (category_table_) category_table_->setRowCount(0);
		loading_ = false;
		return;
	}

	name_edit_->setText(QString::fromStdString(project_->project_name));

	const int rate_index = sample_rate_combo_->findData(project_->audio_format.sample_rate);
	if (rate_index >= 0) sample_rate_combo_->setCurrentIndex(rate_index);

	const int depth_index = bit_depth_combo_->findData(project_->audio_format.bit_depth);
	if (depth_index >= 0) bit_depth_combo_->setCurrentIndex(depth_index);

	const int ch_index = channels_combo_->findData(project_->audio_format.channels);
	if (ch_index >= 0) channels_combo_->setCurrentIndex(ch_index);

	fade_in_spin_->setValue(static_cast<int>(project_->ducking.fade_in_ms));
	fade_out_spin_->setValue(static_cast<int>(project_->ducking.fade_out_ms));

	RebuildCategoryTable();

	loading_ = false;
}

void ProjectPropertiesEditorWidget::RebuildCategoryTable() {
	if (category_table_ == nullptr || project_ == nullptr) return;

	const bool prev_loading = loading_;
	loading_ = true;

	category_table_->setRowCount(static_cast<int>(project_->categories.size()));
	for (int row = 0; row < static_cast<int>(project_->categories.size()); ++row) {
		const CategoryInfo& cate = project_->categories[row];

		auto* name_item = new QTableWidgetItem(QString::fromStdString(cate.name));
		name_item->setFlags(name_item->flags() & ~Qt::ItemIsEditable);
		category_table_->setItem(row, COL_NAME, name_item);

		category_table_->setItem(row, COL_VOLUME_DB,
			new QTableWidgetItem(QString::number(cate.static_volume_db, 'f', 1)));
		category_table_->setItem(row, COL_DUCK_DB,
			new QTableWidgetItem(QString::number(cate.ducking_attenuation_db, 'f', 1)));

		auto* ducker_item = new QTableWidgetItem();
		ducker_item->setFlags((ducker_item->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEditable);
		ducker_item->setCheckState(cate.is_ducker ? Qt::Checked : Qt::Unchecked);
		category_table_->setItem(row, COL_IS_DUCKER, ducker_item);
	}

	loading_ = prev_loading;
}

}
