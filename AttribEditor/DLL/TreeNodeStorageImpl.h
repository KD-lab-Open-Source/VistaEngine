#ifndef __TREE_NODE_STORAGE_IMPL_H_INCLUDED__
#define __TREE_NODE_STORAGE_IMPL_H_INCLUDED__

#include "TreeInterface.h"

class TreeNodeStorageImpl : public TreeNodeStorage{
public:
	static TreeNodeStorageImpl& instance();
	// virtuals:
	~TreeNodeStorageImpl();

	bool registerType(const char* typeName);
	const TreeNode* assignType(const char* typeName, const TreeNode* treeNode);
	const TreeNode* defaultTreeNode(const char* typeName);

	const TreeNode* findDerivedByIndex (const char* baseName, int typeIndex);
	bool baseRegistered(const char* baseName) const;
	


	void registerBaseType(const char* typeName);
	bool registerDerivedType(const char* baseName, const char* derivedName, const char* derivedNameAlt);
	void assignDerivedType(const char* baseName, const char* derivedName, const TreeNode* treeNode);
	const char* derivedComboList(const char* baseName) const;
	const char* derivedComboListAlt(const char* baseName) const;
	std::size_t calculateSize() const;
	//
private:
	struct DerivedClass {
		DerivedClass(){
		}
		DerivedClass(const char* _name, const char* _nameAlt, TreeNode* _treeNode)
		: name(_name), nameAlt(_nameAlt), treeNode(_treeNode)
		{}
		~DerivedClass();
		std::string name;
		std::string nameAlt;
		ShareHandle<TreeNode> treeNode;
	};
	class DerivedVector : public std::vector<DerivedClass>{
	public:
		typedef std::vector<DerivedClass> Super;
		void push_back(const DerivedClass& derivedClass){
			Super::push_back(derivedClass);
			if(!comboList_.empty())
				comboList_ += "|";
			if(!comboListAlt_.empty())
				comboListAlt_ += "|";
			comboList_ += derivedClass.name;
			comboListAlt_ += derivedClass.nameAlt;
		}
		void clear(){
			Super::clear();
			comboList_ = "";
			comboListAlt_ = "";
		}
		const char* comboList() const{ return comboList_.c_str(); }
		const char* comboListAlt() const{ return comboListAlt_.c_str(); }
	protected:
		std::string comboList_;
		std::string comboListAlt_;
	};

	// для полиморфных типов
	typedef StaticMap<std::string, DerivedVector> Map;
	Map defaultTreeNodes_;

	// для простых типов
	typedef StaticMap<std::string, ShareHandle<TreeNode> > TypesMap;
	TypesMap defaultTypeTreeNodes_;
};

#endif
