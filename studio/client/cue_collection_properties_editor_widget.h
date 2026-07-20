#ifndef WIT_CUE_COLLECTION_PROPERTIES_EDITOR_WIDGET_H
#define WIT_CUE_COLLECTION_PROPERTIES_EDITOR_WIDGET_H

#include <QWidget>
#include <functional>

#include "build_artifacts.h"

class QLabel;
class QLineEdit;

namespace wit::studio {

/**
 * @class CueCollectionPropertiesEditorWidget
 * @brief Property form for a single cue collection.
 */
class CueCollectionPropertiesEditorWidget : public QWidget {
public:
	explicit CueCollectionPropertiesEditorWidget(QWidget* parent = nullptr);

	std::function<void()> on_changed;
	std::function<bool(const QString& new_name)> name_change_requester;

	/**
	 * @brief Bind the form to a collection (nullptr clears and disables it).
	 * @param collection The collection to edit; must outlive the binding.
	 */
	void Bind(CueCollectionModel* collection);

private:
	void BuildLayout();
	void LoadFromCollection();

	QLineEdit* name_edit_  		= nullptr;
	QLabel*    uuid_label_ 		= nullptr;
	QLabel*    cue_count_label_ = nullptr;

	CueCollectionModel* collection_ = nullptr;
	bool                loading_    = false;
};

}

#endif // WIT_CUE_COLLECTION_PROPERTIES_EDITOR_WIDGET_H
