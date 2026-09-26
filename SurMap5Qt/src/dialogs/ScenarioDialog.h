// ScenarioDialog.h — a modal PropertyTree over one Serializer, the Qt port of
// the kdw::edit(...) dialog CMainFrame::OnEditMap / OnEditGameScenario opened
// (Scripts\TreeControlSetups\EditMapState / GameScenarioState).
//
// The dialog owns the tree it is given and hands it back on OK so the caller
// can write it into the engine through PropertyIArchive (IWorldBridge::
// mapScenarioSetTree / gameScenarioSetTree) and persist it (...Save).
//
// Qt-clean: only touches the engine-free editor::PropertyRow model.

#pragma once

#include <QDialog>

#include <QString>

namespace editor { class PropertyRow; }
class PropertyTree;

class ScenarioDialog : public QDialog
{
	Q_OBJECT
public:
	// Take ownership of `root` (the bridge's fresh heap tree). Null shows an
	// empty form.
	ScenarioDialog(const QString& title, editor::PropertyRow* root, QWidget* parent = nullptr);

	// The (possibly edited) tree, valid until the dialog is destroyed.
	editor::PropertyRow* root() const;

private:
	PropertyTree* tree_ = nullptr;
};
