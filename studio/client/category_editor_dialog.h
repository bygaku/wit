#ifndef WIT_CATEGORY_EDITOR_DIALOG_H
#define WIT_CATEGORY_EDITOR_DIALOG_H

#include <QDialog>
#include <vector>

#include "build_artifacts.h"

class QTableWidget;
class QPushButton;

namespace wit::studio {

/**
 * @class CategoryEditorDialog
 * @brief Modal editor for the project's category list.
 */
class CategoryEditorDialog : public QDialog {
public:
	/**
	 * @param project Read for the current categories and for cue references.
	 * @param parent  Owning widget.
	 */
	explicit CategoryEditorDialog(const ProjectModel& project, QWidget* parent = nullptr);

	/**
	 * @brief The edited category list; valid only after exec() returned Accepted.
	 */
	[[nodiscard]] const std::vector<CategoryInfo>& Result() const { return categories_; }

private:
	void BuildLayout();
	void ReloadTable();
	void AddCategory();
	void RemoveSelectedCategory();

	/**
	 * @brief Validate names (non-empty, unique) before closing with Accepted.
	 */
	void OnAccept();

	/**
	 * @brief Whether any cue in the project references the given category id.
	 */
	[[nodiscard]] bool IsCategoryInUse(uint16_t category_id) const;

	const ProjectModel&       project_;
	std::vector<CategoryInfo> categories_;
	bool                      loading_ = false;

	QTableWidget* table_         = nullptr;
	QPushButton*  remove_button_ = nullptr;
};

}

#endif // WIT_CATEGORY_EDITOR_DIALOG_H