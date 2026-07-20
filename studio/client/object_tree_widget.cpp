#include "object_tree_widget.h"

#include <QContextMenuEvent>
#include <QHeaderView>
#include <QMenu>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QInputDialog>

namespace wit::studio {

namespace {
constexpr int ROLE_COLLECTION_INDEX = Qt::UserRole + 1;
constexpr int ROLE_CUE_INDEX        = Qt::UserRole + 2;
constexpr int ROLE_WAVEFORM_INDEX   = Qt::UserRole + 3;
}  // namespace

ObjectTreeWidget::ObjectTreeWidget(QWidget* parent) : QWidget(parent) {
	BuildLayout();
}

/* =====================================================================
 * UI construction
 * ===================================================================== */
void ObjectTreeWidget::BuildLayout() {
	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	{
		m_tree_widget_ = new QTreeWidget(this);
		m_tree_widget_->setHeaderLabel("プロジェクト");
		m_tree_widget_->setColumnCount(1);
		m_tree_widget_->header()->setStretchLastSection(true);
		m_tree_widget_->setSelectionMode(QAbstractItemView::SingleSelection);
		m_tree_widget_->setContextMenuPolicy(Qt::DefaultContextMenu);
		layout->addWidget(m_tree_widget_);
	}

	connect(m_tree_widget_, &QTreeWidget::currentItemChanged, this,
			[this](QTreeWidgetItem* current, QTreeWidgetItem*) {
		if (current == nullptr || !onItemSelected) return;
		onItemSelected(current->type(), current->text(0));
	});

	connect(m_tree_widget_, &QTreeWidget::itemChanged, this,
			[this](QTreeWidgetItem* item, int) {
		if (item == nullptr || !onItemRenamed) return;
		onItemRenamed(item, item->text(0));
	});
}

/* =====================================================================
 * Add item into tree
 * ===================================================================== */
QTreeWidgetItem* ObjectTreeWidget::AddCueCollectionIntoTree(const QString& name) {
	// Add a CC into TreeWidget
	auto* item = new QTreeWidgetItem(m_tree_widget_, ItemType::CueCollection);
	item->setText(0, name);
	item->setFlags(item->flags() | Qt::ItemIsEditable);

	const int collection_index = m_tree_widget_->indexOfTopLevelItem(item);
	item->setData(0, ROLE_COLLECTION_INDEX, collection_index);

	item->setExpanded(true);
	return item;
}

QTreeWidgetItem* ObjectTreeWidget::AddCueIntoCueCollection(QTreeWidgetItem* parent_collection, const QString& name) {
	if (parent_collection == nullptr) return nullptr;

	// Add a Cue into parent (CC)
	auto* item = new QTreeWidgetItem(parent_collection, ItemType::Cue);
	item->setText(0, name);
	item->setFlags(item->flags() | Qt::ItemIsEditable);

	const int collection_index = parent_collection->data(0, ROLE_COLLECTION_INDEX).toInt();
	const int cue_index = parent_collection->indexOfChild(item);
	item->setData(0, ROLE_COLLECTION_INDEX, collection_index);
	item->setData(0, ROLE_CUE_INDEX, cue_index);

	parent_collection->setExpanded(true);
	return item;
}

int ObjectTreeWidget::TopLevelIndexOf(QTreeWidgetItem* item) const {
	// indexOfTopLevelItem returns -1 for non-top-level items, which is exactly
	// the "not a collection" signal the host expects.
	return m_tree_widget_->indexOfTopLevelItem(item);
}

void ObjectTreeWidget::CurrentSelection(int& out_collection, int& out_cue) const {
	out_collection	= -1;
	out_cue			= -1;

	auto* item = m_tree_widget_->currentItem();
	if (item == nullptr) return;

	if (item->type() == ItemType::CueCollection) {
		out_collection = m_tree_widget_->indexOfTopLevelItem(item);
		return;
	}

	if (item->type() == ItemType::Cue) {
		QTreeWidgetItem* parent = item->parent();
		if (parent == nullptr) return;

		out_collection = m_tree_widget_->indexOfTopLevelItem(parent);
		out_cue        = parent->indexOfChild(item);
	}
}

void ObjectTreeWidget::Clear() {
	// Suppress currentItemChanged while tearing the tree down; otherwise the
	// host's selection handler runs against a model that is mid-rebuild.
	const QSignalBlocker blocker(m_tree_widget_);
	m_tree_widget_->clear();
}

void ObjectTreeWidget::ResolveItem(QTreeWidgetItem* item, int& out_collection, int& out_cue) const {
	out_collection	= -1;
	out_cue			= -1;

	if (item == nullptr) return;

	if (item->type() == ItemType::CueCollection) {
		out_collection = m_tree_widget_->indexOfTopLevelItem(item);
		return;
	}

	if (item->type() == ItemType::Cue) {
		QTreeWidgetItem* parent = item->parent();
		if (parent == nullptr) return;

		out_collection = m_tree_widget_->indexOfTopLevelItem(parent);
		out_cue        = parent->indexOfChild(item);
	}
}

void ObjectTreeWidget::SetItemTextSilently(QTreeWidgetItem* item, const QString& text) {
	if (item == nullptr) return;

	// Reverting a rejected edit must not re-enter the itemChanged handler.
	const QSignalBlocker blocker(m_tree_widget_);
	item->setText(0, text);
}

/* =====================================================================
 * Context menu
 * ===================================================================== */
void ObjectTreeWidget::contextMenuEvent(QContextMenuEvent* event) {
	QMenu menu(this);

	QTreeWidgetItem* item = m_tree_widget_->itemAt(m_tree_widget_->viewport()->mapFromGlobal(event->globalPos()));

	// On empty space
	if (item == nullptr) {
		QAction* add_collection = menu.addAction("キューコレクションを追加");
		connect(add_collection, &QAction::triggered, this, [this]() {
			if (onAddCueCollectionRequested) onAddCueCollectionRequested();
		});
		menu.exec(event->globalPos());
		return;
	}

	// On a collection
	if (item->type() == ItemType::CueCollection) {
		QAction* add_cue	  = menu.addAction("キューを追加");
		QAction* edit_cc_name = menu.addAction("コレクションの名前を変更");

		connect(add_cue, &QAction::triggered, this, [this, item]() {
			if (onAddCueRequested) onAddCueRequested(item);
		});

		connect(edit_cc_name, &QAction::triggered, this, [this, item]() {
			bool ok = false;

		  	auto current_name = item->text(0);
		  	auto new_name = QInputDialog::getText(this,
													   "名前を変更",
													   "新しい名前を入力してください:",
													   QLineEdit::Normal,
													   current_name,
													   &ok);
			// Call the callback only if the user clicks "OK" and the input is not empty.
		  	if (ok && !new_name.isEmpty()) {
				  if (onEditCueCollectionNameRequested) {
					  onEditCueCollectionNameRequested(item, new_name);
				  }
		  	}
		});
	}

	menu.exec(event->globalPos());
}

}