// LibraryEditorDialog.cpp — see header.

#include "LibraryEditorDialog.h"

#include "../panels/LibraryEditor.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

LibraryEditorDialog::LibraryEditorDialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Library Editor"));
	setMinimumSize(640, 480);

	editor_ = new LibraryEditor(this);

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(editor_, 1);
	layout->addWidget(buttons);
}

void LibraryEditorDialog::openLibrary(const std::string& libraryName)
{
	if(!bridge_)
		return;
	setWindowTitle(tr("Library Editor — %1").arg(QString::fromStdString(libraryName)));
	editor_->openLibrary(bridge_, libraryName);
	exec();
}