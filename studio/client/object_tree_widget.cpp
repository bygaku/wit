#include "object_tree_widget.h"

#include <QContextMenuEvent>
#include <QHeaderView>
#include <QMenu>
#include <QShortcut>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QInputDialog>

namespace wit::studio {

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
		m_tree_widget_->setHeaderLabel("プロジェクトツリー");
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

	auto* delete_shortcut = new QShortcut(QKeySequence::Delete, this);
	delete_shortcut->setContext(Qt::WidgetWithChildrenShortcut);
	connect(delete_shortcut, &QShortcut::activated, this, [this]() {
		QTreeWidgetItem* item = m_tree_widget_->currentItem();
		if (item != nullptr && onDeleteItemRequested) {
			onDeleteItemRequested(item);
		}
	});
}

/* =====================================================================
 * Add item into tree
 * ===================================================================== */
QTreeWidgetItem* ObjectTreeWidget::AddCueCollectionIntoProject(const QString& name) {
	if (m_project_item_ == nullptr) return nullptr;

	// Add a CC under the project root
	auto* item = new QTreeWidgetItem(m_project_item_, ItemType::CueCollection);
	item->setText(0, name);
	item->setFlags(item->flags() | Qt::ItemIsEditable);

	m_project_item_->setExpanded(true);
	item->setExpanded(true);
	return item;
}

QTreeWidgetItem* ObjectTreeWidget::AddCueIntoCueCollection(QTreeWidgetItem* parent_collection, const QString& name) {
	if (parent_collection == nullptr) return nullptr;

	// Add a Cue into parent (CC)
	auto* item = new QTreeWidgetItem(parent_collection, ItemType::Cue);
	item->setText(0, name);
	item->setFlags(item->flags() | Qt::ItemIsEditable);

	parent_collection->setExpanded(true);
	return item;
}

QTreeWidgetItem* ObjectTreeWidget::ResetWithProject(const QString& project_name) {
	const QSignalBlocker blocker(m_tree_widget_);
	m_tree_widget_->clear();

	m_project_item_ = new QTreeWidgetItem(m_tree_widget_, ItemType::Project);
	m_project_item_->setText(0, project_name);
	m_project_item_->setFlags(m_project_item_->flags() & ~Qt::ItemIsEditable);
	m_project_item_->setExpanded(true);

	return m_project_item_;
}

void ObjectTreeWidget::SetProjectLabel(const QString& project_name) {
	if (m_project_item_ == nullptr) return;
	SetItemTextSilently(m_project_item_, project_name);
}

QTreeWidgetItem* ObjectTreeWidget::CollectionItemAt(int index) const {
	if (m_project_item_ == nullptr) return nullptr;
	if (index < 0 || index >= m_project_item_->childCount()) return nullptr;
	return m_project_item_->child(index);
}

QTreeWidgetItem* ObjectTreeWidget::CueItemAt(int collection_index, int cue_index) const {
	QTreeWidgetItem* collection = CollectionItemAt(collection_index);
	if (collection == nullptr) return nullptr;
	if (cue_index < 0 || cue_index >= collection->childCount()) return nullptr;
	return collection->child(cue_index);
}

void ObjectTreeWidget::RemoveItem(QTreeWidgetItem* item) {
	if (item == nullptr || item == m_project_item_) return;	///< the root is permanent
	const QSignalBlocker blocker(m_tree_widget_);

	if (QTreeWidgetItem* parent = item->parent()) {
		parent->removeChild(item);
	} else {
		const int index = m_tree_widget_->indexOfTopLevelItem(item);
		if (index >= 0) m_tree_widget_->takeTopLevelItem(index);
	}

	delete item;
}

int ObjectTreeWidget::CollectionIndexOf(QTreeWidgetItem* item) const {
	if (m_project_item_ == nullptr || item == nullptr) return -1;
	return m_project_item_->indexOfChild(item);
}

void ObjectTreeWidget::CurrentSelection(int& out_collection, int& out_cue) const {
	ResolveItem(m_tree_widget_->currentItem(), out_collection, out_cue);
}

void ObjectTreeWidget::ResolveItem(QTreeWidgetItem* item, int& out_collection, int& out_cue) const {
	out_collection	= -1;
	out_cue			= -1;

	if (item == nullptr) return;

	if (item->type() == ItemType::CueCollection) {
		out_collection = CollectionIndexOf(item);
		return;
	}

	if (item->type() == ItemType::Cue) {
		QTreeWidgetItem* parent = item->parent();
		if (parent == nullptr) return;

		out_collection = CollectionIndexOf(parent);
		out_cue        = parent->indexOfChild(item);
	}
}

void ObjectTreeWidget::SetItemTextSilently(QTreeWidgetItem* item, const QString& text) {
	if (item == nullptr) return;
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

	// On the project root
	if (item->type() == ItemType::Project) {
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
		menu.addSeparator();
		QAction* delete_cc    = menu.addAction("コレクションを削除");

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

		connect(delete_cc, &QAction::triggered, this, [this, item]() {
			if (onDeleteItemRequested) onDeleteItemRequested(item);
		});
	}

	// On a cue
	if (item->type() == ItemType::Cue) {
		QAction* delete_cue = menu.addAction("キューを削除");

		connect(delete_cue, &QAction::triggered, this, [this, item]() {
			if (onDeleteItemRequested) onDeleteItemRequested(item);
		});
	}

	menu.exec(event->globalPos());
}

}