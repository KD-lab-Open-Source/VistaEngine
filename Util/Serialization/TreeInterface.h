#ifndef __TREE_INTERFACE_H__
#define __TREE_INTERFACE_H__

#include <my_stl.h>
#include <vector>
#include <list>
#include <string>

#include "StaticMap.h"
#include "Handle.h"
#include "XUtil.h"

#include "TreeEditor.h"
#include "AttribEditorInterface.h"

#ifdef EXPORT_ATTRIB_EDITOR
# define ATTRIB_EDITOR_API __declspec(dllexport)
#else
# ifdef IMPORT_ATTRIB_EDITOR
#  define ATTRIB_EDITOR_API __declspec(dllimport)
# else
#  define ATTRIB_EDITOR_API
# endif
#endif

class TreeNode;
typedef std::vector<std::string> TreeNodePath;

typedef std::vector<std::string> ComboStrings;

AttribEditorInterface& attribEditorInterface();

class TreeNodeClipboard;
class TreeNodeStorage{
public:
    virtual const TreeNode* findDerivedByIndex(const char* baseName, int typeIndex) = 0;
	virtual const TreeNode* defaultTreeNode(const char* typeName) = 0;
    virtual bool baseRegistered (const char* baseName) const = 0;
    virtual void registerBaseType(const char* typeName) = 0;
	virtual bool registerType(const char* typeName) = 0;
	virtual const TreeNode* assignType(const char* typeName, const TreeNode* treeNode) = 0;
	virtual bool registerDerivedType(const char* baseName, const char* derivedName, const char* derivedNameAlt) = 0;
    virtual void assignDerivedType(const char* baseName, const char* derivedName, const TreeNode* treeNode) = 0;
    virtual const char* derivedComboList(const char* baseName) const = 0;
    virtual const char* derivedComboListAlt(const char* baseName) const = 0;
    virtual std::size_t calculateSize() const = 0;
};

class TreeNode : public ShareHandleBase
{
public:
	enum EditType {
		STATIC,		// Просто текст. Редактировать нельзя
		EDIT,		// Просто текст. Редактировать можно
		COMBO,		// Одно из значений из comboList(), разделенных |
		COMBO_MULTI, // Несколько значений из comboList(), также разделенных |, может быть пусто
		VECTOR,		 // value не меняется, но можно удалять и 
					//	добавлять в список, инициализируя значением defaultTreeNode()
		POLYMORPHIC, // Инициализировать defaultTreeNode(value from comboList())
	};

	typedef std::list<ShareHandle<TreeNode> > List;
	typedef List::iterator iterator;
	typedef List::const_iterator const_iterator;
	typedef std::string string;

	//-----------------------------------------------------
	explicit TreeNode(const char* name = "") 
		: name_(name)
		, value_("")
		, type_("")
		, editType_(STATIC)
		, defaultTreeNode_(0)
		, nodeEditor_(0)
		, parent_(0)
		, expanded_(false)
		, unedited_ (false)
		, color_(0)
		, inDigest_(false)
		, hidden_(false) {}

	TreeNode(const TreeNode& original)
	{
		*this = original;
	}

	TreeNode& operator=(const TreeNode& node)
	{
		editType_ = node.editType();
		name_ = node.name();
		value_ = node.value();
		comboList_ = node.comboList();
		type_ = node.type();
		defaultTreeNode_ = node.defaultTreeNode_;
		defaultTreeNodes_ = node.defaultTreeNodes_;
		unedited_ = node.unedited_;
		nodeEditor_ = 0;
		expanded_ = node.expanded();
		hidden_ = node.hidden();
		color_ = node.color();
		inDigest_ = node.inDigest();

		children_.clear();
		List::const_iterator i = node.children().begin(), e = node.children().end();
		for(; i != e; ++i)
		{
			ShareHandle<TreeNode> ptr(new TreeNode());
			push_back(ptr);
			*(ptr.get()) = *((*i).get());
		}
		return *this;
	}

	~TreeNode(){
		if(nodeEditor_){
			nodeEditor_->free();
			nodeEditor_ = 0;
		}
	}

	void generateComboList(){
        if(editType() == POLYMORPHIC){
            std::string derivedComboListAlt = attribEditorInterface().treeNodeStorage().derivedComboListAlt(type_);
            comboList_ = derivedComboListAlt.c_str();
        }
    }

	void intersect(const TreeNode* node);
	//-----------------------------------------------------
	const List& children() const { return children_; }

	void push_back(TreeNode* treeNode){ 
		children_.push_back(treeNode); 
		treeNode->setParent(this); 
	}
	void insert(iterator before, TreeNode* ptr){
		children_.insert(before, ptr);
		ptr->setParent(this);
	}

	void erase(iterator i){
		(*i)->setParent(NULL);
		children_.erase(i);
	}
	void erase(iterator first, iterator last){
		for (iterator it = first; it != last; ++it) {
			(*it)->setParent(NULL);
		}
		children_.erase(first, last);
	}

	iterator begin(){ return children_.begin(); }
	iterator end(){ return children_.end(); }
	const_iterator begin() const{ return children_.begin(); }
	const_iterator end() const{ return children_.end(); }
	TreeNode* front() const { return children_.front(); }
	TreeNode* back() const { return children_.back(); }
	bool empty() const{ return children_.empty(); }
	size_t size() const{ return children_.size(); }

	void clear(){
		iterator i = begin(), e = end();
		for(; i != e; ++i)
			(*i)->setParent(NULL);
		children_.clear();
	}

	const TreeNode* parent() const { return parent_; }
	TreeNode* parent() { return parent_; }
	void setParent(TreeNode* parent) { parent_ = parent; }

	//-----------------------------------------------------
	const char* name() const{ return name_; }
	void setName(const char* name){
		name_ = name;
	}

	const char* value() const { return value_.c_str(); }
	void setValue(const char* value) { 
		value_ = value; 
	}

	const char* type() const { // Для проверки при копировании
		return type_;
	} 
	void setType(const char* type) {
		type_ = type;
	}

    void setEditor (TreeEditor* editor) {
		if(nodeEditor_){
			nodeEditor_->free();
			nodeEditor_ = 0;
		}
        nodeEditor_ = editor;
    }

    TreeEditor* editor(){
        return nodeEditor_;
    }
	const TreeEditor* editor() const{
        return nodeEditor_;
	}

	EditType editType() const { return editType_; }
	void setEditType(EditType editType) { editType_ = editType; }

	const char* comboList() const {
        return comboList_.c_str();
    }

	void setComboList(EditType editType, const ComboStrings& comboList){
		editType_ = editType; 

        comboList_ = "";
        ComboStrings::const_iterator it;
        FOR_EACH (comboList, it) {
            if (comboList_ != "")
                comboList_ += "|";
            comboList_ += *it;
        }
	}

	void setComboList(EditType editType, const char* comboList){
		editType_ = editType; 
		comboList_ = comboList; 
	}

	void setTreeNode(const TreeNode* treeNode){
		editType_ = VECTOR;
		defaultTreeNode_ = new TreeNode();
		*defaultTreeNode_ = *treeNode;
	}

	void setInDigest(bool value = true) { inDigest_ = value; }
	bool inDigest()const { return inDigest_; }
	void setHidden() { hidden_ = true; }
	bool hidden() const { return hidden_; }
    	
	const TreeNode* defaultTreeNode() const { return defaultTreeNode_; }
	const TreeNode* defaultTreeNode(int typeIndex) const { 
		return attribEditorInterface().treeNodeStorage().findDerivedByIndex(type_, typeIndex);
	}
	std::string nameAltToName (const char* nameAlt) const {
		DefaultTreeNodeMap::const_iterator it = defaultTreeNodes_.find (nameAlt);
		if (it == defaultTreeNodes_.end ()) {
			xassert (0);
		}
		return it->second.first;
	}
	void addDefaultTreeNode(const char* typeName, const char* typeNameAlt, TreeNode* treeNode) {
		defaultTreeNodes_[typeNameAlt] = std::make_pair(typeName, treeNode);
	}
    
	XBuffer valueBuffer() const { return XBuffer((void*)value_.c_str(), (int)value_.size()); }

	//-----------------------------------------------------
	bool expanded() const { return expanded_; }
	void setExpanded(bool expanded) { expanded_ = expanded;	}

	bool unedited() const { return unedited_; }
	void setUnedited(bool unedited) { unedited_ = unedited; }
	
	unsigned int color() const { return color_;	}
	void setColor(unsigned int color) { color_ = color;	}

	TreeNode* find(const char* name, const char* type){
		iterator it;
		FOR_EACH(children_, it)
			if(strcmp((**it).name(), name) == 0 && (type == 0 || strcmp((*it)->type(), type) == 0))
				return *it;

		return 0;
	}

	const TreeNode* find(const char* name, const char* type) const {
		const_iterator it;
		FOR_EACH(children_, it)
			if(strcmp((**it).name(), name) == 0 && (type == 0 || strcmp((*it)->type(), type) == 0))
				return *it;

		return 0;
	}


	TreeNode* findChild(const char* childName, const char* childType, const char* parentType) {
		if(strcmp(name(), childName) == 0 &&
			strcmp(type(), childType) == 0 &&
			(parentType == 0 || (parent() && (strcmp(parent()->type(), parentType) == 0))))
			return this;
		iterator i;
		FOR_EACH(children_, i){
			TreeNode* node = (*i)->findChild(childName, childType, parentType);
			if(node)
				return node;
		}
		return 0;
	}

	//-----------------------------------------------------
	static bool isExtensible(EditType type){ return type == VECTOR; }

	std::size_t calculateSize(bool withDefaultTreeNodes) const{
		std::size_t totalSize = sizeof(*this);
		if(withDefaultTreeNodes){
			totalSize += strlen(name()) + 1;
			totalSize += strlen(type()) + 1;
		}
		totalSize += strlen(value()) + 1;
		totalSize += strlen(comboList()) + 1;

		const_iterator it;
		FOR_EACH(*this, it){
			const TreeNode* node = *it;
			totalSize += node->calculateSize(withDefaultTreeNodes);
		}
		if(true){
			if(defaultTreeNode_)
				totalSize += defaultTreeNode_->calculateSize(withDefaultTreeNodes);
		}
		return totalSize;
	}

private:
	typedef StaticMap<const char*, std::pair<const char*, TreeNode*> > DefaultTreeNodeMap;
    DefaultTreeNodeMap defaultTreeNodes_;

	const char* name_;
	const char* type_;
	EditType editType_;
	TreeNode* parent_;
	List children_;
	std::string value_;
	std::string comboList_;
	ShareHandle<TreeNode> defaultTreeNode_;
    TreeEditor* nodeEditor_;
	unsigned int color_;
	bool unedited_;
	bool expanded_;
	bool hidden_;
	bool inDigest_;
};


struct TreeControlSetup{
	RECT window;
	bool expandAllNodes_;
	bool showRootNode_;

	TreeControlSetup(int left, int top, int right, int bottom, const char* configName, bool expandAllNodes = false, bool showRootNode = true) {
		showRootNode_ = showRootNode;
		expandAllNodes_ = expandAllNodes;
		window.left = left; 
		window.top = top; 
		window.right = right; 
		window.bottom = bottom; 
		configName_ = configName;
		assert(configName_);
	}

	~TreeControlSetup() {}

	char const* getConfigName() const{ return configName_; }
private:
	const char* configName_;
};


#ifndef _FINAL_VERSION_
extern ATTRIB_EDITOR_API AttribEditorInterface& attribEditorInterfaceExternal();

#else // _FINAL_VERSION_

class DummyAttribEditorInterface : public AttribEditorInterface{
public:
	TreeEditor* createTreeEditor(const char* name){ return 0; }
	void free(TreeNode const* node) {}
	bool isTreeEditorRegistered(const char*) { return false; }
	TreeNodeStorage& treeNodeStorage() { return *((TreeNodeStorage*)(0)); }
	TreeNodeClipboard& clipboard() { return *((TreeNodeClipboard*)(0)); }

	TreeNode* edit(const TreeNode* node, HWND wnd, TreeControlSetup& treeControlSetup) { return 0; }
};

inline AttribEditorInterface& attribEditorInterfaceExternal(){
	static DummyAttribEditorInterface editor;
	return editor;
}

#endif // _FINAL_VERSION_

#endif // __TREE_INTERFACE_H__
