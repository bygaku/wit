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
	std::function<void()> onAddCueCollectionRequested;
	std::function<void(QTreeWidgetItem* parent_collection)> onAddCueRequested;
	std::function<void(int item_type, const QString& name)> onItemSelected;
	std::function<void(int item_type, const QString& name)> onItemChanged;
	std::function<void(QTreeWidgetItem* target_item, const QString& name)> onEditCueCollectionNameRequested;

	enum ItemType {
		CueCollection	= QTreeWidgetItem::UserType + 1,
		Cue				= QTreeWidgetItem::UserType + 2,
		Waveform		= QTreeWidgetItem::UserType + 3,
	};

	/**
	 *
	 * @param name
	 * @return
	 */
	QTreeWidgetItem* AddCueCollectionIntoTree(const QString& name);

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
 	 * @brief Top-level row index of a collection item, or -1 if it is not one.
 	 *
 	 * Lets the host map a collection item back to its position in
 	 * ProjectModel::cue_collections without exposing the tree internals.
 	 */
	[[nodiscard]] int TopLevelIndexOf(QTreeWidgetItem* item) const;

protected:
	/**
	 * @note Accepts right-clicks and displays a menu.
	 */
	void contextMenuEvent(QContextMenuEvent* event) override;


private:
	void BuildLayout();


	QTreeWidget* m_tree_widget_;
};

}

#endif // WIT_OBJECT_TREE_WIDGET_H
