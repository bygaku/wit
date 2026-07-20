#ifndef WIT_ISSUE_LIST_WIDGET_H
#define WIT_ISSUE_LIST_WIDGET_H

#include <QWidget>
#include <vector>

#include "build_artifacts.h"

class QTableWidget;

namespace wit::studio {

/**
 * @class IssueListWidget
 * @brief Bottom pane listing validation / build issues.
 *
 * @note Default: Place it at the bottom of the main window.
 */
class IssueListWidget : public QWidget {
public:
	explicit IssueListWidget(QWidget* parent = nullptr);

	/**
	 * @brief Replace the list contents with the given issues.
	 */
	void ShowIssues(const std::vector<Issue>& issues);

	/**
	 * @brief Remove every row.
	 */
	void Clear();

private:
	void BuildLayout();

	QTableWidget* m_table_widget_ = nullptr;
};

}

#endif // WIT_ISSUE_LIST_WIDGET_H