#ifndef WIT_PROJECT_PROPERTIES_EDITOR_WIDGET_H
#define WIT_PROJECT_PROPERTIES_EDITOR_WIDGET_H

#include <QWidget>
#include <functional>

#include "build_artifacts.h"

class QComboBox;
class QGroupBox;
class QLineEdit;
class QSpinBox;
class QTableWidget;

namespace wit::studio {

/**
 * @class ProjectPropertiesEditorWidget
 * @brief Property form for the project itself.
 *
 * The essentials (name, audio format) are always shown. Everything a user can
 * safely leave alone (ducking fade times, per-category detail) lives behind a
 * "詳細設定" toggle with working defaults, so a project is usable without ever
 * opening it.
 */
class ProjectPropertiesEditorWidget : public QWidget {
public:
	explicit ProjectPropertiesEditorWidget(QWidget* parent = nullptr);

	std::function<void()> on_changed;

	/**
	 * @brief Bind the form to a project (nullptr clears and disables the form).
	 * @param project The project to edit; must outlive the binding.
	 */
	void Bind(ProjectModel* project);

private:
	void BuildLayout();
	void LoadFromProject();
	void RebuildCategoryTable();

	ProjectModel* project_ = nullptr;
	bool          loading_ = false;

	QLineEdit*    name_edit_       = nullptr;
	QComboBox*    sample_rate_combo_ = nullptr;
	QComboBox*    bit_depth_combo_   = nullptr;
	QComboBox*    channels_combo_    = nullptr;

	QGroupBox*    detail_group_    = nullptr;
	QSpinBox*     fade_in_spin_    = nullptr;
	QSpinBox*     fade_out_spin_   = nullptr;
	QTableWidget* category_table_  = nullptr;
};

}

#endif //WIT_PROJECT_PROPERTIES_EDITOR_WIDGET_H
