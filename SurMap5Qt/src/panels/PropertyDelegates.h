// PropertyDelegates.h — Qt in-place editors for PropertyTree rows.
//
// Qt port of kdw's per-row widgets (Entry, ComboBox, CheckComboBox,
// ColorChooserDialog, sliders, file dialogs): a QStyledItemDelegate over
// the Value column dispatching on editor::RowKind —
//   Text    QLineEdit
//   Number  QSpinBox / QDoubleSpinBox
//   Enum    QComboBox over the descriptor entries
//   Combo   editable QComboBox over the string list
//   Ranged  QSlider + spinbox pair over [min, max]
//   File    QLineEdit + "..." file-dialog button
// Bool rows toggle through the item checkbox (see PropertyTree), Color and
// Flags rows open their dialogs on double-click (no inline editor).
//
// Qt-clean: only touches the engine-free editor::PropertyRow model.

#pragma once

#include <QStyledItemDelegate>

#include "editor/PropertyRow.h"   // editor::PropertyRow (engine-free model)

namespace editor {
class PropertyRowEnum;
class PropertyRowCombo;
class PropertyRowRanged;
class PropertyRowFile;
class PropertyRowColor;
class PropertyRowBitVector;
}

class PropertyRowDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	explicit PropertyRowDelegate(QObject* parent = nullptr);

	// The row behind an index (null when none).
	static editor::PropertyRow* row(const QModelIndex& index);

	// Modal checklist editor for a flags row (kdw::CheckComboBox).
	// Returns true when accepted (the row's flags were updated).
	static bool editFlags(editor::PropertyRowBitVector* row, QWidget* parent);

	// Color dialog editor for a color row (kdw::ColorChooserDialog).
	// Returns true when accepted (the row's color was updated).
	static bool editColor(editor::PropertyRowColor* row, QWidget* parent);

	QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
	                      const QModelIndex& index) const override;
	void setEditorData(QWidget* editor, const QModelIndex& index) const override;
	void setModelData(QWidget* editor, QAbstractItemModel* model,
	                  const QModelIndex& index) const override;
	void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
	                          const QModelIndex& index) const override;
	void paint(QPainter* painter, const QStyleOptionViewItem& option,
	           const QModelIndex& index) const override;
};
