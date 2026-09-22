// LibraryEditor.cpp — see header.

#include "LibraryEditor.h"

#include "LibraryTree.h"
#include "PropertyTree.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

LibraryEditor::LibraryEditor(QWidget* parent)
	: QWidget(parent)
{
	tree_ = new LibraryTree(this);
	propertyTree_ = new PropertyTree(this);
	saveButton_ = new QPushButton(tr("Save"), this);

	auto* layout = new QHBoxLayout(this);
	layout->addWidget(tree_, 1);
	layout->addWidget(propertyTree_, 2);

	auto* buttonRow = new QVBoxLayout;
	buttonRow->addWidget(saveButton_);
	layout->addLayout(buttonRow);

	connect(tree_, &LibraryTree::elementSelected, this, &LibraryEditor::onElementSelected);
	connect(saveButton_, &QPushButton::clicked, this, &LibraryEditor::onSave);
}

void LibraryEditor::openLibrary(IWorldBridge* bridge, const std::string& libraryName)
{
	bridge_ = bridge;
	libraryName_ = libraryName;
	currentElement_ = -1;
	tree_->setLibrary(bridge, libraryName);
	// setLibrary selects the first element, which fires elementSelected and
	// loads it. If the library is empty, nothing is shown.
}

void LibraryEditor::onElementSelected(int elementIndex)
{
	loadElement(elementIndex);
}

void LibraryEditor::loadElement(int elementIndex)
{
	if(!bridge_ || elementIndex < 0)
		return;
	currentElement_ = elementIndex;
	// PropertyTree takes ownership of the bridge's heap tree; the rows stay
	// alive while displayed, so in-place edits land in this tree and onSave
	// can write it back.
	propertyTree_->setRoot(bridge_->libraryElementTree(libraryName_, elementIndex, true));
}

void LibraryEditor::onSave()
{
	if(!bridge_ || currentElement_ < 0)
		return;
	// Write the (possibly edited) tree back into the library element, then
	// persist the library — kdw::LibraryEditor::onSave did the same through
	// PropertyIArchive + saveLibrary.
	if(editor::PropertyRow* root = propertyTree_->root())
		bridge_->libraryElementSetTree(libraryName_, currentElement_, root);
	bridge_->librarySave(libraryName_);
}
