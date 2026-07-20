#include "cue_properties_editor_widget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace wit::studio {

namespace {

/* =====================================================================
 * Recommended randomize ranges
 * ===================================================================== */
constexpr float RECOMMENDED_VOLUME_MIN = 0.8f;
constexpr float RECOMMENDED_VOLUME_MAX = 1.0f;
constexpr float RECOMMENDED_PITCH_MIN  = 0.75f;
constexpr float RECOMMENDED_PITCH_MAX  = 1.25f;

/**
 * @brief Whether a range still holds the flat default (no variation).
 */
bool IsFlatDefault(const RandomizeRange& range) {
	return range.min == 1.0f && range.max == 1.0f;
}

/**
 * @brief Attach a hover explanation to a widget in the user's own terms.
 */
void SetHelp(QWidget* widget, const QString& text) {
	widget->setToolTip(text);
}

}  // namespace

CuePropertiesEditorWidget::CuePropertiesEditorWidget(QWidget* parent) : QWidget(parent) {
	BuildLayout();
	Bind(nullptr, {});
}

/* =====================================================================
 * UI construction
 * ===================================================================== */
void CuePropertiesEditorWidget::BuildLayout() {
	auto* root = new QVBoxLayout(this);

	/* =====================================================================
  	 * Basic Properties
  	 * ===================================================================== */
	auto* basic_group = new QGroupBox("基本設定", this);
	auto* basic_form  = new QFormLayout(basic_group);
	{
		name_edit_ = new QLineEdit(basic_group);
		SetHelp(name_edit_, "ゲーム側からこのキューを呼び出すときの登録名です。\n"
							"注意：コレクション内で重複しない名前を付けてください。");
		basic_form->addRow("キュー名", name_edit_);

		category_combo_ = new QComboBox(basic_group);
		SetHelp(category_combo_, "このキューが属するカテゴリです。\n"
							"カテゴリごとの音量やダッキング設定がまとめて適用されます。");
		basic_form->addRow("カテゴリ", category_combo_);

		cue_type_combo_ = new QComboBox(basic_group);
		cue_type_combo_->addItem("同時再生（ポリフォニック）",	static_cast<int>(CueType::POLYPHONIC));
		cue_type_combo_->addItem("シャッフル再生",				static_cast<int>(CueType::SHUFFLE));
		cue_type_combo_->addItem("順再生（シーケンシャル）",		static_cast<int>(CueType::SEQUENTIAL));
		SetHelp(cue_type_combo_, "複数の波形を登録したときの鳴らし方です。\n"
				"・同時再生：重ねて鳴らせます（足音などの連打向き）\n"
				"・シャッフル：毎回ランダムに 1 つの波形を選びます（同じ音の繰り返しを避けたいとき）\n"
				"・順再生：登録順に上から 1 つずつ鳴らします");
		basic_form->addRow("再生ルール", cue_type_combo_);

		loop_check_ = new QCheckBox("ループ再生", basic_group);
		SetHelp(loop_check_, "オンにすると、停止するまで繰り返し再生します。BGM や 環境音 向けです。");
		basic_form->addRow("", loop_check_);

		streaming_combo_ = new QComboBox(basic_group);
		streaming_combo_->addItem("メモリ常駐（短い音向け）", static_cast<int>(StreamingMode::MEMORY_RESIDENT));
		streaming_combo_->addItem("ストリーミング（BGM / 環境音 向け）", static_cast<int>(StreamingMode::STREAMING));
		SetHelp(streaming_combo_, "音データのロード方法です。\n"
				"・メモリ常駐：初期化時にメモリにロードします。効果音など短い音に向いています。\n"
				"・ストリーミング：再生に必要な分だけ、再生しながら少しずつロードします。BGM など長い音に向いています。");
		basic_form->addRow("ロード方式", streaming_combo_);

		root->addWidget(basic_group);
	}

	/* =====================================================================
	  * Volume & Pitch
	  * ===================================================================== */
	auto* level_group = new QGroupBox("音量・ピッチ", this);
	auto* level_form  = new QFormLayout(level_group);
	{
		volume_spin_ = new QDoubleSpinBox(level_group);
		volume_spin_->setRange(0.0, 1.0);
		volume_spin_->setSingleStep(0.05);
		volume_spin_->setDecimals(2);
		SetHelp(volume_spin_, "このキューの基本音量です。1.00 が原音のままの大きさです。\n"
				"注意：すでに音が大きいファイルを読み込んだ場合、1.00以上の値を入れるとクリッピングする可能性があります。");
		level_form->addRow("音量", volume_spin_);

		pitch_spin_ = new QDoubleSpinBox(level_group);
		pitch_spin_->setRange(0.25, 4.0);
		pitch_spin_->setSingleStep(0.05);
		pitch_spin_->setDecimals(2);
		SetHelp(pitch_spin_, "このキューの基本ピッチ（再生速度）です。1.00 が原音のままの高さです。");
		level_form->addRow("ピッチ", pitch_spin_);

		root->addWidget(level_group);
	}

	/* =====================================================================
 	 * Volume Randomize
 	 * ===================================================================== */
	{
		volume_random_group_ = new QGroupBox("音量ランダマイズ", this);
		volume_random_group_->setCheckable(true);
		volume_random_group_->setChecked(false);
		SetHelp(volume_random_group_, "オンにすると、再生するたびに音量がランダムに変化します。\n"
				"TIPS: 同じ効果音の連続再生が単調に聞こえるのを防げます。");

		auto* form = new QFormLayout(volume_random_group_);

		auto* vmin = new QDoubleSpinBox(volume_random_group_);
		vmin->setObjectName("volume_random_min");
		vmin->setRange(0.0, 1.0);
		vmin->setSingleStep(0.05);
		vmin->setDecimals(2);
		SetHelp(vmin, "ランダムに選ばれる音量下限。");
		form->addRow("最小", vmin);

		auto* vmax = new QDoubleSpinBox(volume_random_group_);
		vmax->setObjectName("volume_random_max");
		vmax->setRange(0.0, 1.0);
		vmax->setSingleStep(0.05);
		vmax->setDecimals(2);
		SetHelp(vmax, "ランダムに選ばれる音量上限。");
		form->addRow("最大", vmax);

		root->addWidget(volume_random_group_);
	}

	/* =====================================================================
  	 * Pitch Randomize
  	 * ===================================================================== */
	{
		pitch_random_group_ = new QGroupBox("ピッチランダマイズ", this);
		pitch_random_group_->setCheckable(true);
		pitch_random_group_->setChecked(false);
		SetHelp(pitch_random_group_, "オンにすると、再生するたびにピッチがランダムに変化します。\n"
				"TIPS: 足音や打撃音などに軽い揺らぎを加えられます。");
		auto* form = new QFormLayout(pitch_random_group_);

		auto* pmin = new QDoubleSpinBox(pitch_random_group_);
		pmin->setObjectName("pitch_random_min");
		pmin->setRange(0.25, 4.0);
		pmin->setSingleStep(0.05);
		pmin->setDecimals(2);
		SetHelp(pmin, "ランダムに選ばれるピッチの下限です。");
		form->addRow("最小", pmin);

		auto* pmax = new QDoubleSpinBox(pitch_random_group_);
		pmax->setObjectName("pitch_random_max");
		pmax->setRange(0.25, 4.0);
		pmax->setSingleStep(0.05);
		pmax->setDecimals(2);
		SetHelp(pmax, "ランダムに選ばれるピッチの上限です。");
		form->addRow("最大", pmax);

		root->addWidget(pitch_random_group_);
	}

	/* =====================================================================
	 * Waveform authoring
	 * ===================================================================== */
	auto* wf_group   = new QGroupBox("音声ファイル", this);
	auto* wf_layout  = new QVBoxLayout(wf_group);
	auto* wf_buttons = new QVBoxLayout();

	auto* add_btn	 = new QPushButton("ファイルを追加…", wf_group);
	auto* remove_btn = new QPushButton("選択を削除", wf_group);
	{
		waveform_list_ = new QListWidget(wf_group);
		SetHelp(waveform_list_, "このキューで鳴らす音声ファイルの一覧です。");
		wf_layout->addWidget(waveform_list_);

		SetHelp(add_btn,    "音声ファイルをこのキューに追加します。");
		wf_buttons->addWidget(add_btn);
		SetHelp(remove_btn, "一覧で選択したファイルをこのキューから削除します。");
		wf_buttons->addWidget(remove_btn);

		wf_layout->addLayout(wf_buttons);

		root->addWidget(wf_group);
		root->addStretch(1);
	}

	/* =====================================================================
	 * Signal
	 * ===================================================================== */
	auto notify = [this]() {
		if (loading_ || cue_ == nullptr) return;
		LoadFromCue();
	};

	connect(name_edit_, &QLineEdit::editingFinished, this, [this]() {
		if (loading_ || cue_ == nullptr) return;

		const QString proposed = name_edit_->text().trimmed();
		if (proposed.toStdString() == cue_->cue_name) return;	///< no change

		bool accepted = false;
		if (name_change_requester) accepted = name_change_requester(proposed);

		if (!accepted) {
			loading_ = true;
			name_edit_->setText(QString::fromStdString(cue_->cue_name));
			loading_ = false;
		}
	});

	connect(category_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
		if (loading_ || cue_ == nullptr) return;
		cue_->category_id = static_cast<uint16_t>(category_combo_->currentData().toUInt());
		if (on_changed) on_changed();
	});

	connect(cue_type_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
		if (loading_ || cue_ == nullptr) return;
		cue_->cue_type = static_cast<CueType>(cue_type_combo_->currentData().toInt());
		if (on_changed) on_changed();
	});

	connect(loop_check_, &QCheckBox::toggled, this, [this](bool on) {
		if (loading_ || cue_ == nullptr) return;
		cue_->loop_enabled = on;
		if (on_changed) on_changed();
	});

	connect(streaming_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
		if (loading_ || cue_ == nullptr) return;
		cue_->streaming_mode = static_cast<StreamingMode>(streaming_combo_->currentData().toInt());
		if (on_changed) on_changed();
	});

	connect(volume_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double v) {
		if (loading_ || cue_ == nullptr) return;
		cue_->base_volume = static_cast<float>(v);
		if (on_changed) on_changed();
	});

	connect(pitch_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double v) {
		if (loading_ || cue_ == nullptr) return;
		cue_->base_pitch = static_cast<float>(v);
		if (on_changed) on_changed();
	});

	// Randomize toggles: seed a recommended range the first time only.
	connect(volume_random_group_, &QGroupBox::toggled, this, [this](bool on) {
		if (loading_ || cue_ == nullptr) return;
		cue_->volume_random.enabled = on;
		if (on && IsFlatDefault(cue_->volume_random)) {
			cue_->volume_random.min = RECOMMENDED_VOLUME_MIN;
			cue_->volume_random.max = RECOMMENDED_VOLUME_MAX;
		}
		LoadFromCue();
		if (on_changed) on_changed();
	});

	connect(pitch_random_group_, &QGroupBox::toggled, this, [this](bool on) {
		if (loading_ || cue_ == nullptr) return;
		cue_->pitch_random.enabled = on;
		if (on && IsFlatDefault(cue_->pitch_random)) {
			cue_->pitch_random.min = RECOMMENDED_PITCH_MIN;
			cue_->pitch_random.max = RECOMMENDED_PITCH_MAX;
		}
		LoadFromCue();
		if (on_changed) on_changed();
	});

	// Range spin boxes, resolved by object name from the group children.
	if (auto* vmin = volume_random_group_->findChild<QDoubleSpinBox*>("volume_random_min")) {
		connect(vmin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double v) {
			if (loading_ || cue_ == nullptr) return;
			cue_->volume_random.min = static_cast<float>(v);
			if (on_changed) on_changed();
		});
	}

	if (auto* vmax = volume_random_group_->findChild<QDoubleSpinBox*>("volume_random_max")) {
		connect(vmax, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double v) {
			if (loading_ || cue_ == nullptr) return;
			cue_->volume_random.max = static_cast<float>(v);
			if (on_changed) on_changed();
		});
	}

	if (auto* pmin = pitch_random_group_->findChild<QDoubleSpinBox*>("pitch_random_min")) {
		connect(pmin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double v) {
			if (loading_ || cue_ == nullptr) return;
			cue_->pitch_random.min = static_cast<float>(v);
			if (on_changed) on_changed();
		});
	}

	if (auto* pmax = pitch_random_group_->findChild<QDoubleSpinBox*>("pitch_random_max")) {
		connect(pmax, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double v) {
			if (loading_ || cue_ == nullptr) return;
			cue_->pitch_random.max = static_cast<float>(v);
			if (on_changed) on_changed();
		});
	}

	connect(add_btn,    &QPushButton::clicked, this, [this]() {
		AddWaveformFiles();
	});

	connect(remove_btn, &QPushButton::clicked, this, [this]() {
		RemoveSelectedWaveform();
	});

	(void)notify;
}

/* =====================================================================
 * Binding
 * ===================================================================== */
void CuePropertiesEditorWidget::Bind(CueModel* cue, const std::vector<CategoryInfo>& categories) {
	cue_ = cue;

	loading_ = true;
	category_combo_->clear();
	for (const auto& cate : categories) {
		category_combo_->addItem(QString::fromStdString(cate.name), cate.id);
	}

	loading_ = false;

	setEnabled(cue_ != nullptr);
	LoadFromCue();
}

void CuePropertiesEditorWidget::LoadFromCue() {
	loading_ = true;

	if (cue_ == nullptr) {
		name_edit_->clear();
		waveform_list_->clear();
		loading_ = false;
		return;
	}

	/* =====================================================================
	 * Basic properties
	 * ===================================================================== */
	{
		name_edit_->setText(QString::fromStdString(cue_->cue_name));

		const int cate_index = category_combo_->findData(cue_->category_id);
		if (cate_index >= 0) category_combo_->setCurrentIndex(cate_index);

		const int cue_type_index = cue_type_combo_->findData(static_cast<int>(cue_->cue_type));
		if (cue_type_index >= 0) cue_type_combo_->setCurrentIndex(cue_type_index);

		loop_check_->setChecked(cue_->loop_enabled);

		const int stream_index = streaming_combo_->findData(static_cast<int>(cue_->streaming_mode));
		if (stream_index >= 0) streaming_combo_->setCurrentIndex(stream_index);
	}

	/* =====================================================================
	 * Volume & Pitch
	 * ===================================================================== */
	volume_spin_->setValue(cue_->base_volume);
	pitch_spin_->setValue(cue_->base_pitch);

	/* =====================================================================
	 * Randamize
	 * ===================================================================== */
	{
		volume_random_group_->setChecked(cue_->volume_random.enabled);
		if (auto* vmin = volume_random_group_->findChild<QDoubleSpinBox*>("volume_random_min")) {
			vmin->setValue(cue_->volume_random.min);
		}
		if (auto* vmax = volume_random_group_->findChild<QDoubleSpinBox*>("volume_random_max")) {
			vmax->setValue(cue_->volume_random.max);
		}

		pitch_random_group_->setChecked(cue_->pitch_random.enabled);
		if (auto* pmin = pitch_random_group_->findChild<QDoubleSpinBox*>("pitch_random_min")) {
			pmin->setValue(cue_->pitch_random.min);
		}
		if (auto* pmax = pitch_random_group_->findChild<QDoubleSpinBox*>("pitch_random_max")) {
			pmax->setValue(cue_->pitch_random.max);
		}
	}

	/* =====================================================================
	 * Waveform
	 * ===================================================================== */
	{
		waveform_list_->clear();
		for (const auto& wf : cue_->waveforms) {
			waveform_list_->addItem(QString::fromStdString(
				wf.display_name.empty() ? wf.file_path : wf.display_name));
		}
	}

	loading_ = false;
}

/* =====================================================================
 * Waveform actions
 * ===================================================================== */
void CuePropertiesEditorWidget::AddWaveformFiles() {
	if (cue_ == nullptr) return;

	const QStringList files = QFileDialog::getOpenFileNames(
		this, "音声ファイルを追加", QString(),
		"音声ファイル (*.wav);;すべてのファイル (*.*)");
	if (files.isEmpty()) return;

	std::filesystem::path base;
	if (project_dir_provider) base = project_dir_provider();

	for (const QString& file : files) {
		WaveformModel wf;
		std::filesystem::path abs = file.toStdString();
		std::error_code err;
		std::filesystem::path rel = base.empty() ? abs : std::filesystem::relative(abs, base, err);
		wf.file_path    = (err || rel.empty() ? abs : rel).generic_string();
		wf.display_name = QFileInfo(file).fileName().toStdString();
		cue_->waveforms.push_back(std::move(wf));
	}

	LoadFromCue();
	if (on_changed) on_changed();
}

void CuePropertiesEditorWidget::RemoveSelectedWaveform() {
	if (cue_ == nullptr) return;

	const int row = waveform_list_->currentRow();
	if (row < 0 || row >= static_cast<int>(cue_->waveforms.size())) return;

	cue_->waveforms.erase(cue_->waveforms.begin() + row);

	LoadFromCue();
	if (on_changed) on_changed();
}

}
