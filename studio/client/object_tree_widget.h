#ifndef WIT_OBJECT_TREE_WIDGET_H
#define WIT_OBJECT_TREE_WIDGET_H

#include <QWidget>
#include <QTreeWidget>
#include <functional>

#include "build_artifacts.h"

class QComboBox;
class QLineEdit;
class QSpinBox;

namespace wit::studio {
/**
 * @class ObjectTreeWidget
 * @brief Display all objects added to the project in a tree view.
 *
 * @note Default: Place it in the upper left side.
 * Allows right clicks within the widget.
 */
class ObjectTreeWidget : public QWidget {
public:
	explicit ObjectTreeWidget(QWidget* parent = nullptr);

	/* =====================================================================
	 * Event
	 * ===================================================================== */
	std::function<void()> onProjectSelected;
	std::function<void()> onAddCueCollectionRequested;
	std::function<void(QTreeWidgetItem* parent_collection)> onAddCueRequested;
	std::function<void(int item_type, const QString& name)> onItemSelected;
	std::function<void(QTreeWidgetItem* target_item, const QString& name)> onItemRenamed;
	std::function<void(QTreeWidgetItem* target_item, const QString& name)> onEditCueCollectionNameRequested;
	std::function<void(QTreeWidgetItem* target_item)> onDeleteItemRequested;

	enum ItemType {
		CueCollection	= QTreeWidgetItem::UserType + 1,
		Cue				= QTreeWidgetItem::UserType + 2,
		Waveform		= QTreeWidgetItem::UserType + 3,
		Project			= QTreeWidgetItem::UserType + 4,
	};

	/**
	 *
	 * @param name
	 * @return
	 */
	QTreeWidgetItem* AddCueCollectionIntoProject(const QString& name);

	/**
	 *
	 * @param parent_collection
	 * @param name
	 * @return
	 */
	QTreeWidgetItem* AddCueIntoCueCollection(QTreeWidgetItem* parent_collection, const QString& name);

	/**
	 *
	 * @param parent_collection
	 * @param name
	 * @return
	 */
	// QTreeWidgetItem* AddWaveformIntoCue(QTreeWidgetItem* parent_collection, const QString& name); ///< TODO

	/**
	 * @brief Remove every item and recreate the project root.
	 * @param project_name Label shown on the root item.
	 * @return The project root item.
	 */
	QTreeWidgetItem* ResetWithProject(const QString& project_name);

	/**
	 * @brief Update the project root label without emitting rename callbacks.
	 */
	void SetProjectLabel(const QString& project_name);

	/**
	 * @brief Collection item at the given row under the project root, or nullptr.
	 */
	[[nodiscard]] QTreeWidgetItem* CollectionItemAt(int index) const;

	/**
	 * @brief Cue item at (collection_index, cue_index), or nullptr.
	 */
	[[nodiscard]] QTreeWidgetItem* CueItemAt(int collection_index, int cue_index) const;

	/**
	 * @brief Remove a single item (and its children) from the tree.
	 */
	void RemoveItem(QTreeWidgetItem* item);

	/**
 	 * @brief Row index of a collection item under the project root, or -1.
 	 */
	[[nodiscard]] int CollectionIndexOf(QTreeWidgetItem* item) const;

	/**
	 * @brief Resolve the current selection to model coordinates.
	 * @param out_collection Receives the collection index, or -1.
	 * @param out_cue        Receives the cue index within the collection, or -1
	 *                       when a collection (not a cue) is selected.
	 */
	void CurrentSelection(int& out_collection, int& out_cue) const;

	/**
	 * @brief Resolve an arbitrary item to model coordinates.
	 * @param item           The item to resolve.
	 * @param out_collection Receives the collection index, or -1.
	 * @param out_cue        Receives the cue index, or -1 for a collection.
	 */
	void ResolveItem(QTreeWidgetItem* item, int& out_collection, int& out_cue) const;

	/**
	 * @brief Set an item's text without emitting onItemRenamed.
	 */
	void SetItemTextSilently(QTreeWidgetItem* item, const QString& text);

protected:
	/**
	 * @note Accepts right-clicks and displays a menu.
	 */
	void contextMenuEvent(QContextMenuEvent* event) override;


private:
	void BuildLayout();


	QTreeWidget*     m_tree_widget_;
	QTreeWidgetItem* m_project_item_ = nullptr;
};

}

#endif // WIT_OBJECT_TREE_WIDGET_H