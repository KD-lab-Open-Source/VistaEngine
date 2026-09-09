// HeadsDialog.h — Qt port of the Heads library editor (SurMap5/MainFrame.cpp
// OnEditHeads -> kdw::edit(Serializer(HeadLibrarySerialization()), ...)).
//
// The heads library is a plain list of head file names
// (GlobalAttributes::showHeadNames). The original used the kdw LibraryEditor;
// here a simple editable list dialog talks to the engine through the
// IWorldBridge (headNames/setHeadNames).

#pragma once

#include <QDialog>

#include "editor/EditorTool.h"   // IWorldBridge (engine-free)

class QListWidget;
class QPushButton;
class QLineEdit;

class HeadsDialog : public QDialog
{
	Q_OBJECT
public:
	explicit HeadsDialog(QWidget* parent = nullptr);

	void setBridge(IWorldBridge* bridge) { bridge_ = bridge; refresh(); }

private slots:
	void onAdd();
	void onRemove();
	void onSave();

private:
	void refresh();

	IWorldBridge* bridge_ = nullptr;
	QListWidget* list_ = nullptr;
	QLineEdit* edit_ = nullptr;
	QPushButton* btnAdd_ = nullptr;
	QPushButton* btnRemove_ = nullptr;
	QPushButton* btnSave_ = nullptr;
};