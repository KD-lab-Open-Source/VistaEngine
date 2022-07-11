#include "StdAfx.h"
#include "ClassTree.h"
#include "kdw/DragManager.h"

ClassTreeRow::ClassTreeRow(const char* nameAlt, int index)
: kdw::TreeObject(nameAlt)
, index_(index)
{
}

ClassTreeRow* ClassTreeRow::findChild(string& comboNameAlt)
{
	int pos = comboNameAlt.find("\\");
	if(pos == string::npos)
		return this;
	string nameAlt(comboNameAlt.begin(), comboNameAlt.begin() + pos);
	comboNameAlt.erase(comboNameAlt.begin(), comboNameAlt.begin() + pos + 1);

	Rows::iterator i;
	FOR_EACH(children_, i){
		ClassTreeRow& row = (ClassTreeRow&)**i;
		if(nameAlt == row.text())
			return row.findChild(comboNameAlt);
	}

	ClassTreeRow* child = new ClassTreeRow(nameAlt.c_str());
	add(child);
	return child;
}

void ClassTreeRow::onDrag(kdw::DragManager& dragManager)
{
	dragManager.beginDrag(index_);
}

//////////////////////////////////////////////////////

ClassTree::ClassTree(const ComboStrings& comboStrings, const ComboStrings& comboStringsAlt, const ComboStrings& ignoredTypeIDs)
{
	model()->setRoot(new ClassTreeRow(""));

	for(int i = 0; i < comboStrings.size(); i++){
		if(find(ignoredTypeIDs.begin(), ignoredTypeIDs.end(), comboStrings[i]) != ignoredTypeIDs.end())
			continue;
		string nameAlt = comboStringsAlt[i];
		ClassTreeRow* parent = root()->findChild(nameAlt);
		parent->add(new ClassTreeRow(nameAlt.c_str(), i));
	}
	update();
}


