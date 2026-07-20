#include "cue_collection_properties_editor_widget.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

#include "binary/uuid.h"

namespace wit::studio {

CueCollectionPropertiesEditorWidget::CueCollectionPropertiesEditorWidget(QWidget* parent) : QWidget(parent) {
	BuildLayout();
	Bind(nullptr);
}

/* =====================================================================
 * UI construction
 * ===================================================================== */
void CueCollectionPropertiesEditorWidget::BuildLayout() {

	{
		auto* root  = new QVBoxLayout(this);
		auto* group = new QGroupBox("キューコレクション設定", this);
		auto* form  = new QFormLayout(group);

		name_edit_ = new QLineEdit(group);
		name_edit_->setToolTip("キューコレクションの名前です。\n"
						"ビルドすると、この名前で .wccb / .wwb が出力されます。");
		form->addRow("コレクション名", name_edit_);

		uuid_label_ = new QLabel(group);
		uuid_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
		uuid_label_->setToolTip("このコレクションを一意に識別する UUID です。変更できません。");
		form->addRow("UUID", uuid_label_);

		cue_count_label_ = new QLabel(group);
		form->addRow("キュー数", cue_count_label_);

		root->addWidget(group);
		root->addStretch(1);
	}

	connect(name_edit_, &QLineEdit::editingFinished, this, [this]() {
		if (loading_ || collection_ == nullptr) return;

		const QString proposed = name_edit_->text().trimmed();
		if (proposed.toStdString() == collection_->name) return;	///< no change

		bool accepted = false;
		if (name_change_requester) accepted = name_change_requester(proposed);

		if (!accepted) {
			loading_ = true;
			name_edit_->setText(QString::fromStdString(collection_->name));
			loading_ = false;
			return;
		}

		if (on_changed) on_changed();
	});
}

/* =====================================================================
 * Binding
 * ===================================================================== */
void CueCollectionPropertiesEditorWidget::Bind(CueCollectionModel* collection) {
	collection_ = collection;
	setEnabled(collection_ != nullptr);
	LoadFromCollection();
}

void CueCollectionPropertiesEditorWidget::LoadFromCollection() {
	loading_ = true;

	if (collection_ == nullptr) {
		name_edit_->clear();
		uuid_label_->clear();
		cue_count_label_->clear();
		loading_ = false;
		return;
	}

	name_edit_->setText(QString::fromStdString(collection_->name));
	uuid_label_->setText(QString::fromStdString(collection_->wccb_uuid.ToHexString()));
	cue_count_label_->setText(QString::number(collection_->cues.size()));

	loading_ = false;
}

}