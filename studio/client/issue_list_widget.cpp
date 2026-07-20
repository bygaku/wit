#include "issue_list_widget.h"

#include <QBrush>
#include <QColor>
#include <QHeaderView>
#include <QTableWidget>
#include <QVBoxLayout>

namespace wit::studio {

namespace {

/**
 * @brief Issue table columns.
 */
enum IssueColumn {
	COL_SEVERITY = 0,
	COL_LOCATION,
	COL_MESSAGE,
	COL_COUNT,
};

}  // namespace

IssueListWidget::IssueListWidget(QWidget* parent) : QWidget(parent) {
	BuildLayout();
}

/* =====================================================================
 * UI construction
 * ===================================================================== */
void IssueListWidget::BuildLayout() {
	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	m_table_widget_ = new QTableWidget(this);
	m_table_widget_->setColumnCount(COL_COUNT);
	m_table_widget_->setHorizontalHeaderLabels({ "種類", "場所", "内容" });
	m_table_widget_->horizontalHeader()->setSectionResizeMode(COL_MESSAGE, QHeaderView::Stretch);
	m_table_widget_->verticalHeader()->setVisible(false);
	m_table_widget_->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_table_widget_->setSelectionBehavior(QAbstractItemView::SelectRows);
	layout->addWidget(m_table_widget_);
}

/* =====================================================================
 * Contents
 * ===================================================================== */
void IssueListWidget::ShowIssues(const std::vector<Issue>& issues) {
	m_table_widget_->setRowCount(static_cast<int>(issues.size()));

	for (int row = 0; row < static_cast<int>(issues.size()); ++row) {
		const Issue& issue = issues[row];

		const bool is_error = (issue.severity == Severity::ERROR);
		auto* severity_item = new QTableWidgetItem(is_error ? "エラー" : "警告");
		severity_item->setForeground(is_error ? QBrush(QColor("#d32f2f"))
											  : QBrush(QColor("#f57f17")));
		m_table_widget_->setItem(row, COL_SEVERITY, severity_item);

		m_table_widget_->setItem(row, COL_LOCATION,
			new QTableWidgetItem(QString::fromStdString(issue.location)));
		m_table_widget_->setItem(row, COL_MESSAGE,
			new QTableWidgetItem(QString::fromStdString(issue.message)));
	}
}

void IssueListWidget::Clear() {
	m_table_widget_->setRowCount(0);
}

}