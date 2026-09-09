// TerrainTypeDialog.h — Qt port of the Terrain Type Name editor
// (SurMap5/MainFrame.cpp OnLibrariesTerrraintypename ->
// TerrainTypeDescriptor::instance().editLibrary()).
//
// Shows the 16 terrain type names + their service colours. The original used
// the kdw LibraryEditor; here a read-only list dialog reads the engine data
// through the IWorldBridge (terrainTypeNames).

#pragma once

#include <QDialog>

#include "editor/EditorTool.h"   // IWorldBridge (engine-free)

class QListWidget;

class TerrainTypeDialog : public QDialog
{
	Q_OBJECT
public:
	explicit TerrainTypeDialog(QWidget* parent = nullptr);

	void setBridge(IWorldBridge* bridge) { bridge_ = bridge; refresh(); }

private:
	void refresh();

	IWorldBridge* bridge_ = nullptr;
	QListWidget* list_ = nullptr;
};