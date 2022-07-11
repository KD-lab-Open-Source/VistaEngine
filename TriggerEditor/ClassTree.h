#pragma once

#include "kdw/ObjectsTree.h"

class ClassTreeRow : public kdw::TreeObject
{
public:
	ClassTreeRow(const char* nameAlt, int index = -1);

	ClassTreeRow* findChild(string& comboNameAlt);

	void onDrag(kdw::DragManager& dragManager);

private:
	int index_;
};

class ClassTree : public kdw::ObjectsTree
{
public:
	ClassTree(const ComboStrings& comboStrings, const ComboStrings& comboStringsAlt, const ComboStrings& ignoredTypeIDs = ComboStrings());

	ClassTreeRow* root() { return safe_cast<ClassTreeRow*>(model()->root()); }
	const ClassTreeRow* root() const{ return safe_cast<ClassTreeRow*>(model()->root()); }

};