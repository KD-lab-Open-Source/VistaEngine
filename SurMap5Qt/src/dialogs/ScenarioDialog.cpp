// ScenarioDialog.cpp — see header.

#include "ScenarioDialog.h"

#include "panels/PropertyTree.h"
#include "editor/PropertyRow.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

ScenarioDialog::ScenarioDialog(const QString& title, editor::PropertyRow* root, QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(title);
	resize(560, 640);

	tree_ = new PropertyTree(this);
	tree_->setRoot(root);

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(tree_, 1);
	layout->addWidget(buttons);
}

editor::PropertyRow* ScenarioDialog::root() const
{
	return tree_ ? tree_->root() : nullptr;
}
