#ifndef WIT_CUE_PROPERTIES_WIDGET_H
#define WIT_CUE_PROPERTIES_WIDGET_H

#include <QWidget>
#include <filesystem>
#include <functional>

#include "build_artifacts.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QLineEdit;
class QListWidget;

namespace wit::studio {

/**
 * @class CuePropertiesEditorWidget
 * @brief Property form for a single cue.
 */
class CuePropertiesEditorWidget : public QWidget {
public:
    explicit CuePropertiesEditorWidget(QWidget* parent = nullptr);

	std::function<void()> on_changed;
    std::function<std::filesystem::path()> project_dir_provider;

    /**
     * @brief Bind the form to a cue (nullptr clears and disables the form).
     * @param cue        The cue to edit; must outlive the binding.
     * @param categories Current category list for the combo box.
     */
    void Bind(CueModel* cue, const std::vector<CategoryInfo>& categories);

private:
    void BuildLayout();
    void LoadFromCue();
    void AddWaveformFiles();
    void RemoveSelectedWaveform();

    CueModel* cue_     = nullptr;
    bool      loading_ = false;

    QLineEdit*      name_edit_        = nullptr;
    QComboBox*      category_combo_   = nullptr;
    QComboBox*      cue_type_combo_  = nullptr;
    QCheckBox*      loop_check_       = nullptr;
    QComboBox*      streaming_combo_  = nullptr;
    QDoubleSpinBox* volume_spin_      = nullptr;
    QDoubleSpinBox* pitch_spin_       = nullptr;
    QGroupBox*      volume_random_group_ = nullptr;
    QDoubleSpinBox* volume_random_min_   = nullptr;
    QDoubleSpinBox* volume_random_max_   = nullptr;
    QGroupBox*      pitch_random_group_  = nullptr;
    QDoubleSpinBox* pitch_random_min_    = nullptr;
    QDoubleSpinBox* pitch_random_max_    = nullptr;
    QListWidget*    waveform_list_       = nullptr;
};

}

#endif // WIT_CUE_PROPERTIES_WIDGET_H
