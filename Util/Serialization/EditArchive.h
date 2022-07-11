#ifndef __EDIT_ARCHIVE_H__
#define __EDIT_ARCHIVE_H__

#include <iterator>
//#include <stdio.h>

#include "Serialization.h"
#include "TreeInterface.h"
#include "SafeCast.h"

class EditOArchive : public Archive
{
public:
	EditOArchive()	{
		rootNode_ = new TreeNode("");
		currentNode_ = rootNode_;
		translatedOnly_ = true;
		nesting_ = 0;
		ignoreNameDuplication_ = false;
	}
	~EditOArchive() {}

	void setTranslatedOnly(bool translatedOnly) {
		translatedOnly_ = translatedOnly;
	}

	void setIgnoreNameDuplication(bool ignoreNameDuplication) { 
		ignoreNameDuplication_ = ignoreNameDuplication; 
	}

	const TreeNode* rootNode() const { 
		xassert(!nesting_);
		return rootNode_;
	}

	virtual bool usesNameAlt () const {
		return true;
	}

	virtual bool isEdit () const {
        return true;
	}

    virtual bool isOutput () const {
        return true;
    }

	//static void dump(const TreeNode* _node, std::ostream& _stream, int _indent = 0) {
	//	std::string indent_str;
	//	for (int i = 0; i < _indent; ++i)
	//		indent_str += " ";
	//	_stream << indent_str;
	//	_stream << "[" << _node->name() << "]";
	//	_stream << " = ";
	//	_stream << _node->value() << std::endl;
 //       TreeNode::const_iterator it;
 //       FOR_EACH(*_node, it) {
 //           dump(*it, _stream, _indent + 1);
 //       }
	//}


	// To simulate sub-blocks
	void openBlock(const char* name, const char* nameAlt) {
		openNode(name, nameAlt, "");
	}

	void closeBlock() {
		closeNode("");
	}

    ////////////////////////////////////////
protected:
	bool processEnum(int& value, const EnumDescriptor& descriptor, const char* name, const char* nameAlt);
	bool processBitVector(int& flags, const EnumDescriptor& descriptor, const char* name, const char* nameAlt);

	bool processValue(std::string& value, const char* name, const char* nameAlt){
        openNode(name, nameAlt, typeid(std::string).name());
        currentNode_->setValue(value.c_str());
		currentNode_->setEditType(TreeNode::EDIT);
        closeNode(name);
        return true;
	}

    bool processValue(bool& value, const char* name, const char* nameAlt){
        openNode(name, nameAlt, typeid(bool).name());
        currentNode_->setValue(value ? "Да" : "Нет");
        currentNode_->setComboList(TreeNode::COMBO, "Да|Нет");
        closeNode(name);
        return true;
    }

    bool processValue(char& value, const char* name, const char* nameAlt){
        openNode(name, nameAlt, typeid(char).name());
        XBuffer buf; buf <= value; 
		currentNode_->setValue(buf);
		currentNode_->setEditType(TreeNode::EDIT);
        closeNode(name);
        return true;
    }
    
    // Signed types
    bool processValue(signed char& value, const char* name, const char* nameAlt) {
        openNode(name, nameAlt, typeid(signed char).name());
        XBuffer buf; buf <= value; 
		currentNode_->setValue(buf);
		currentNode_->setEditType(TreeNode::EDIT);
        closeNode(name);
        return true;
    }
    bool processValue(signed short& value, const char* name, const char* nameAlt) {
        openNode(name, nameAlt, typeid(signed short).name());
        XBuffer buf; buf <= value; 
		currentNode_->setValue(buf);
		currentNode_->setEditType(TreeNode::EDIT);
        closeNode(name);
        return true;
    }
    bool processValue(signed int& value, const char* name, const char* nameAlt) {
        openNode(name, nameAlt, typeid(signed int).name());
        XBuffer buf; buf <= value; 
		currentNode_->setValue(buf);
		currentNode_->setEditType(TreeNode::EDIT);
        closeNode(name);
        return true;
    }
    bool processValue(signed long& value, const char* name, const char* nameAlt) {
        openNode(name, nameAlt, typeid(signed long).name());
        XBuffer buf; buf <= value; 
		currentNode_->setValue(buf);
		currentNode_->setEditType(TreeNode::EDIT);
        closeNode(name);
        return true;
    }
    // Unsigned types
    bool processValue(unsigned char& value, const char* name, const char* nameAlt) {
        openNode(name, nameAlt, typeid(unsigned char).name());
        XBuffer buf; buf <= value; 
		currentNode_->setValue(buf);
		currentNode_->setEditType(TreeNode::EDIT);
        closeNode(name);
        return true;
    }
    bool processValue(unsigned short& value, const char* name, const char* nameAlt) {
        openNode(name, nameAlt, typeid(unsigned short).name());
        XBuffer buf; buf <= value; 
		currentNode_->setValue(buf);
		currentNode_->setEditType(TreeNode::EDIT);
        closeNode(name);
        return true;
    }
    bool processValue(unsigned int& value, const char* name, const char* nameAlt) {
        openNode(name, nameAlt, typeid(unsigned int).name());
        XBuffer buf; buf <= value; 
		currentNode_->setValue(buf);
		currentNode_->setEditType(TreeNode::EDIT);
        closeNode(name);
        return true;
    }
    bool processValue(unsigned long& value, const char* name, const char* nameAlt) {
        openNode(name, nameAlt, typeid(unsigned long).name());
        XBuffer buf; buf <= value; 
		currentNode_->setValue(buf);
		currentNode_->setEditType(TreeNode::EDIT);
        closeNode(name);
        return true;
    }

    bool processValue(float& value, const char* name, const char* nameAlt) {
        openNode(name, nameAlt, typeid(float).name());
        //XBuffer buf; buf.SetDigits(7); buf <= value; 
		static char buf[255] = {0};
		sprintf(buf, "%.6g", value);
		currentNode_->setValue(buf);
		currentNode_->setEditType(TreeNode::EDIT);
        closeNode(name);
        return true;
    }
    bool processValue(double& value, const char* name, const char* nameAlt) {
        openNode(name, nameAlt, typeid(double).name());
        //XBuffer buf; buf.SetDigits(7); buf <= value; 
		static char buf[255] = {0};
		sprintf(buf, "%.6g", value);
		currentNode_->setValue(buf);
		currentNode_->setEditType(TreeNode::EDIT);
        closeNode(name);
        return true;
    }

	bool processValue(PrmString& t, const char* name, const char* nameAlt) {
        openNode(name, nameAlt, typeid (PrmString).name());
		currentNode_->setValue(t ? t : "\\0");
		currentNode_->setEditType(TreeNode::EDIT);
        closeNode(name);
        return true;
	}

	bool processValue(ComboListString& t, const char* name, const char* nameAlt) {
        openNode(name, nameAlt, typeid(ComboListString).name());
		currentNode_->setValue(t);
		currentNode_->setComboList(TreeNode::COMBO, t.comboList());
        closeNode(name);
        return true;
	}

    virtual bool openContainer(const char* name, const char* nameAlt, const char* typeName, const char* elementTypeName, int& count, bool readOnly) {
        openNode(name, nameAlt, typeName);
        currentNode_->setEditType(TreeNode::VECTOR);
		currentNode_->setExpanded(!readOnly);
		currentNode_->setValue(elementTypeName);
        return true;
    }

    virtual void closeContainer (const char* name) {
        closeNode(name);
    }
    
	Archive* openDefaultArchive(const char* typeName, const char* derivedTypeName, const char* derivedTypeNameAlt);
    void closeDefaultArchive(ShareHandle<Archive> base_ar, const char* typeName, const char* derivedTypeName, const char* derivedTypeNameAlt);

	bool baseRegistered(const char* baseName) const {
		AttribEditorInterface& attribEditor = attribEditorInterface();
		TreeNodeStorage& storage = attribEditor.treeNodeStorage();
		return storage.baseRegistered(baseName);
	}



	bool openNode(const char* name, const char* nameAlt, const char* typeName);
	void closeNode(const char* name);
    bool openStruct(const char* name, const char* nameAlt, const char* typeName);
    void closeStruct(const char* name);

    int openPointer (const char* name, const char* nameAlt,
					 const char* baseName, const char* baseNameAlt,
                     const char* derivedName, const char* derivedNameAlt);
    void closePointer (const char* name, const char* typeName, const char* derivedName);

    bool needDefaultArchive(const char* baseName) const {
        return !baseRegistered(baseName);
    }

private:
	ShareHandle<TreeNode> rootNode_;
    std::list<ShareHandle <TreeNode> > defaultNodes_;
	TreeNode* currentNode_;
	bool translatedOnly_;
	bool ignoreNameDuplication_;
	int nesting_;
};


class EditIArchive : public Archive
{
public:
	EditIArchive() {
		currentNode_ = 0;
		lastNode_ = 0;
		translatedOnly_ = true;
	}
	EditIArchive(const TreeNode* node) {
		currentNode_ = 0;
		setRootNode(node);
		translatedOnly_ = true;
	}

	EditIArchive(const EditOArchive& eoa);
	~EditIArchive() {}

	void setRootNode(const TreeNode* node) { currentNode_ = node; }
	void setTranslatedOnly(bool translatedOnly) { translatedOnly_ = translatedOnly; }

	bool processEnum(int& value, const EnumDescriptor& comboList, const char* name, const char* nameAlt);
	bool processBitVector(int& flags, const EnumDescriptor& comboList, const char* name, const char* nameAlt);
	
	bool isEdit() const{ return true; }

	void openBlock(const char* name, const char* nameAlt) {
		openNode(name, nameAlt, "");
	}

	void closeBlock() {
		closeNode("");
	}


protected:
    virtual bool openContainer(const char* name, const char* nameAlt, const char* typeName, const char* elementTypeName, int& count, bool readOnly) {
        openNode(name, nameAlt, typeName, true);
		if(isNodeExists()) {
			count = (int)currentNode_->size ();
			return true;
		} else {
			count = 0;
			return false;
		}
    }
    virtual void closeContainer (const char* name) {
        closeNode(name);
    }
	virtual bool usesNameAlt () const {
		return true;
	}

    virtual bool openStruct (const char* name, const char* nameAlt, const char* typeName) {
        openNode(name, nameAlt, typeName);
		return(isNodeExists());
    }

    virtual void closeStruct (const char* name) {
        closeNode(name);
    }

    virtual int openPointer (const char* name, const char* nameAlt,
							 const char* baseName, const char* baseNameAlt,
                             const char* baseNameComboList, const char* baseNameComboListAlt);
	virtual void closePointer (const char* name, const char* typeName, const char* derivedName);

	void openNode(const char* name, const char* nameAlt = 0, const char* typeName = "", bool container = false);
    void closeNode(const char* name);

private:
	struct ParserState {
		bool container;
		bool exists;
	};

	inline bool inContainer () const {
		if(parserStack_.empty())
			return false;
		return parserStack_.back().container;
	}
	inline bool isNodeExists(bool ignore_unedited = false) const {
		if(parserStack_.empty())
			return true;
		if(!parserStack_.back().exists)
			return false;
		if(!ignore_unedited && currentNode_ && currentNode_->unedited())
			return false;
		return true;
	}

	std::vector<ParserState> parserStack_;

	const TreeNode* currentNode_;
    const TreeNode* lastNode_;
	bool translatedOnly_;

	/////////////////////////////////////

    bool processValue(bool& value, const char* name, const char* nameAlt) {
        bool result;
        openNode(name, nameAlt);
		if(result = isNodeExists())
			value = strcmp(currentNode_->value(), "Да") == 0;
        closeNode(name);
        return result;
    }

    bool processValue(char& value, const char* name, const char* nameAlt) {
        bool result;
        openNode(name, nameAlt);
		if(result = isNodeExists()) {
            signed short val;
            currentNode_->valueBuffer () >= val;
            value = char(val);
        }
        closeNode(name);
        return result;
    }
    
    // Signed types
    bool processValue(signed char& value, const char* name, const char* nameAlt) {
        bool result;
        openNode(name, nameAlt);
        if(result = isNodeExists()) {
            signed short val;
            currentNode_->valueBuffer () >= val;
            value = signed char(val);
        }
        closeNode(name);
        return result;
    }
    bool processValue(signed short& value, const char* name, const char* nameAlt) {
        bool result;
        openNode(name, nameAlt);
        if(result = isNodeExists())
            currentNode_->valueBuffer () >= value;
        closeNode(name);
        return result;
    }
    bool processValue(signed int& value, const char* name, const char* nameAlt) {
        bool result;
        openNode(name, nameAlt);
        if(result = isNodeExists())
            currentNode_->valueBuffer () >= value;
        closeNode(name);
        return result;
    }
    bool processValue(signed long& value, const char* name, const char* nameAlt) {
        bool result;
        openNode(name, nameAlt);
        if(result = isNodeExists())
            currentNode_->valueBuffer () >= value;
        closeNode(name);
        return result;
    }

    // Unsigned  types
    bool processValue(unsigned char& value, const char* name, const char* nameAlt) {
        bool result;
        openNode(name, nameAlt);
        if(result = isNodeExists())
            currentNode_->valueBuffer () >= value;
        closeNode(name);
        return result;
    }
    bool processValue(unsigned short& value, const char* name, const char* nameAlt) {
        bool result;
        openNode(name, nameAlt);
        if(result = isNodeExists())
            currentNode_->valueBuffer () >= value;
        closeNode(name);
        return result;
    }
    bool processValue(unsigned int& value, const char* name, const char* nameAlt) {
        bool result;
        openNode(name, nameAlt);
        if(result = isNodeExists())
            currentNode_->valueBuffer () >= value;
        closeNode(name);
        return result;
    }
    bool processValue(unsigned long& value, const char* name, const char* nameAlt) {
        bool result;
        openNode(name, nameAlt);
        if(result = isNodeExists())
            currentNode_->valueBuffer () >= value;
        closeNode(name);
        return result;
    }

    bool processValue(float& value, const char* name, const char* nameAlt) {
        bool result;
        openNode(name, nameAlt);
        if(result = isNodeExists())
            currentNode_->valueBuffer () >= value;
        closeNode(name);
        return result;
    }
    bool processValue(double& value, const char* name, const char* nameAlt) {
        bool result;
        openNode(name, nameAlt);
        if(result = isNodeExists())
            currentNode_->valueBuffer () >= value;
        closeNode(name);
        return result;
    }

	bool processValue(ComboListString& t, const char* name, const char* nameAlt) {
        bool result;
        openNode(name, nameAlt);
        if(result = isNodeExists())
            t = currentNode_->value();
        closeNode(name);
        return result;
	}

	bool processValue(std::string& value, const char* name, const char* nameAlt) {
        bool result;
        openNode(name, nameAlt);
        if(result = isNodeExists())
            value = currentNode_->value();
        closeNode(name);
        return result;
	}

	bool processValue(PrmString& t, const char* name, const char* nameAlt) {
        bool result;
        openNode(name, nameAlt);
        if(result = isNodeExists()) {
            if(currentNode_->value() != "\\0")
                t = currentNode_->value();
            else
                t = 0;
        }
        closeNode(name);
        return result;
	}
};

class EditArchive : public EditOArchive, public EditIArchive
{
public:
	EditArchive();
	EditArchive(HWND hwnd, const TreeControlSetup& treeControlSetup);
	~EditArchive();

	void setTranslatedOnly(bool translatedOnly);
    
	bool edit();
	void clear();

	template<class T>
	bool edit(T& t, const char* name = 0) {
		static_cast<EditOArchive&>(*this).serialize(t, 0, 0);
		TreeNode* node = const_cast<TreeNode*>(rootNode());
		if(name) // если редактируется указатель, то имя конфликтует с его типом
			node->setValue(name);
		
		node->setEditor(TreeEditorFactory::instance().create(typeid(T).name(), true));
		if(node->editor())
			node->editor()->onChange(*node);
		
		if((node->editor() && node->editor()->invokeEditor (*node, hwnd_))) {
			setRootNode (rootNode());
			static_cast<EditIArchive&>(*this).serialize(t, 0, 0);
			clear();
			return true;
		} 
		else {
			bool editConfirmed = edit();
			if(editConfirmed){
				static_cast<EditIArchive&>(*this).serialize(t, 0, 0);
				clear();
				return true;
			}
		}
		return false;
	}

private:
	const TreeNode* outputNode_;

	HWND hwnd_;
	TreeControlSetup treeControlSetup_;
};

template<class T>
inline void loadStructFromNode (T& data, const TreeNode& node) {
	ShareHandle<TreeNode> parent(new TreeNode (""));

	parent->push_back(new TreeNode(""));
	*parent->front() = node;
	
	EditIArchive iar(parent);
    iar.serialize(data, node.name(), node.name());
}

template<class T>
inline void saveStructToNode (const T& data, TreeNode& node) {
    EditOArchive oar;
	oar.serialize (data, "", "");
	node.clear ();
	TreeNode::const_iterator it;

	FOR_EACH (*oar.rootNode ()->front (), it) {
		node.push_back (*it);
	}
}

#endif //__EDIT_ARCHIVE_H__



