#include "StdAfx.h"
#include "TreeNodeStorageImpl.h"

// --------------------------------------------------------------------------

TreeNodeStorageImpl& TreeNodeStorageImpl::instance()
{
	static TreeNodeStorageImpl impl;
	return impl;
}

TreeNodeStorageImpl::~TreeNodeStorageImpl()
{
	defaultTreeNodes_.clear();
}

TreeNodeStorageImpl::DerivedClass::~DerivedClass()
{
}

const TreeNode* TreeNodeStorageImpl::findDerivedByIndex (const char* baseName, int typeIndex)
{
    Map::iterator it = defaultTreeNodes_.find(baseName);
    if(it == defaultTreeNodes_.end ()) {
        xassert(0 && "No such base registered!");
    }
    DerivedVector& vec = it->second;
    return vec[typeIndex].treeNode.get ();
}

bool TreeNodeStorageImpl::baseRegistered(const char* baseName) const
{
    Map::const_iterator it;
    it = defaultTreeNodes_.find (baseName);
    return (it != defaultTreeNodes_.end ());
}

void TreeNodeStorageImpl::registerBaseType(const char* typeName){
	DerivedVector& vect = defaultTreeNodes_[typeName];
    vect.clear();
}

bool TreeNodeStorageImpl::registerType(const char* typeName)
{
	ShareHandle<TreeNode>& node = defaultTypeTreeNodes_[typeName];
	if(node)
		return false;
	else{
		node = new TreeNode("");
		return true;
	}
}

const TreeNode* TreeNodeStorageImpl::assignType(const char* typeName, const TreeNode* treeNode)
{
	ShareHandle<TreeNode>& node = defaultTypeTreeNodes_[typeName];
	xassert(node && "registerType is not called before assignType!");
	*node = *treeNode;
	return node;
}

const TreeNode* TreeNodeStorageImpl::defaultTreeNode(const char* typeName)
{
	return defaultTypeTreeNodes_[typeName];
}

bool TreeNodeStorageImpl::registerDerivedType(const char* baseName, const char* derivedName, const char* derivedNameAlt)
{
    DerivedVector& derivedVector = defaultTreeNodes_[baseName];
	DerivedVector::iterator it;
	for(it = derivedVector.begin(); it != derivedVector.end(); ++it)
		if(it->name == derivedName)
			return false; 

	DerivedClass derived;
	derived.name = derivedName;
	derived.nameAlt = derivedNameAlt;
	derived.treeNode = new TreeNode("");
	derivedVector.push_back(derived); // DerivedClass(derivedName, derivedNameAlt, treeNodeCopy);
	return true;
}


void TreeNodeStorageImpl::assignDerivedType(const char* baseName, const char* derivedName, const TreeNode* treeNode)
{
    DerivedVector& derivedVector = defaultTreeNodes_[baseName];
	DerivedVector::iterator it;
	for(it = derivedVector.begin(); it != derivedVector.end(); ++it){
		if(it->name == derivedName){
			*it->treeNode = *treeNode;
			return; 
		}
	}
	xassert(0 && "registerDerivedType shall be called before!");
}

const char* TreeNodeStorageImpl::derivedComboList(const char* baseName) const
{
    Map::const_iterator mit = defaultTreeNodes_.find(baseName);
	xassert(!defaultTreeNodes_.empty());
    if(mit != defaultTreeNodes_.end())
		return mit->second.comboList();
    else{
        XBuffer buf;
        buf < "Unregistered base class: " < baseName;
        xxassert(0, buf);
        return "";
    }
}
const char* TreeNodeStorageImpl::derivedComboListAlt(const char* baseName) const
{
    Map::const_iterator mit = defaultTreeNodes_.find(baseName);
	xassert(!defaultTreeNodes_.empty());
    if(mit != defaultTreeNodes_.end())
		return mit->second.comboListAlt();
    else{
        XBuffer buf;
        buf < "Unregistered base class: " < baseName;
        xxassert(0, buf);
        return "";
    }
}

std::size_t TreeNodeStorageImpl::calculateSize() const
{
    size_t totalSize = 0;
    Map::const_iterator map_it;
    DerivedVector::const_iterator it;
    FOR_EACH(defaultTreeNodes_, map_it){
        const DerivedVector& derivedVector = map_it->second;
        FOR_EACH(derivedVector, it){
            const TreeNode* node = it->treeNode;
            totalSize += node->calculateSize(true);
        }
    }
    return totalSize;
}
