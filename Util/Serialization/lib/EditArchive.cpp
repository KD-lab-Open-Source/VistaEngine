#include "StdAfx.h"
#include "EditArchive.h"
#include "Dictionary.h"

////////////////////////////////////////
EditIArchive::EditIArchive(const EditOArchive& eoa)
{
	currentNode_ = 0;
	setRootNode(eoa.rootNode());
	translatedOnly_ = true;
}

////////////////////////////////////////
EditArchive::EditArchive()
: hwnd_(0),
treeControlSetup_(0, 0, 800, 1000, "treeControlSetup")
{
	outputNode_ = 0;
}

EditArchive::EditArchive(HWND hwnd, const TreeControlSetup& treeControlSetup)
: hwnd_(hwnd),
treeControlSetup_(treeControlSetup)
{
	outputNode_ = 0;
}


EditArchive::~EditArchive()
{
	clear();
}

void EditArchive::setTranslatedOnly(bool translatedOnly)
{
	EditOArchive::setTranslatedOnly(translatedOnly);
	EditIArchive::setTranslatedOnly(translatedOnly);
}

AttribEditorInterface& attribEditorInterface();

bool EditArchive::edit()
{	
	outputNode_ = attribEditorInterface().edit(rootNode(), hwnd_, treeControlSetup_);
	setRootNode(outputNode_);
	return outputNode_;
}

void EditArchive::clear()
{
	AttribEditorInterface& attribEditorInterface();

	if(outputNode_)
		attribEditorInterface().free(outputNode_);
	outputNode_ = 0;
}


void EditIArchive::openNode (const char* name, const char* nameAlt, const char* typeName, bool container)
{
	if(!name)
		return;

	if(nameAlt)
		name = nameAlt;
	if(*name == '_')
		++name;

	ParserState state;
	state.container = container;
	state.exists    = false;

	xassert(currentNode_);
	if(isNodeExists(true) && !currentNode_->empty()){
		if(inContainer()){
            if(lastNode_ == currentNode_){
				xassert(!currentNode_->empty());
                currentNode_ = currentNode_->front ();
                lastNode_ = currentNode_;

				state.exists = true;
            }
			else{
				const TreeNode* node = lastNode_;
				while (node->parent() && currentNode_ != node->parent())
					node = node->parent();
				
                TreeNode::const_iterator iter = std::find (currentNode_->begin(),
															currentNode_->end(), node);
                if(iter != currentNode_->end()){
					++iter;

					if(iter != currentNode_->end()){
						currentNode_ = *iter;
						lastNode_ = currentNode_;
						state.exists = true;
					}
					else{
						// ...вот тут
						//currentNode_ = lastNode_;
						lastNode_ = currentNode_;
						currentNode_ = currentNode_->parent();
						state.exists = true;
						// ^^^^
					}
                }
				else{
					state.exists = false;
                    xassert(0);
                }
            }
		}
		else{
			const TreeNode* cand = currentNode_->find(name, 0);
			if(cand){
				currentNode_ = cand;
                lastNode_ = currentNode_;
				state.exists = true;
			}
			else
				state.exists = false;
		}
	}
	else
		state.exists = false;

	parserStack_.push_back (state);
}

void EditIArchive::closeNode (const char* name)
{
    if(name){
        if(isNodeExists(true)){
			xassert(currentNode_->parent());
            currentNode_ = currentNode_->parent();			
        }
        xassert(!parserStack_.empty());
        parserStack_.pop_back ();
    }
}

int EditIArchive::openPointer (const char* name, const char* nameAlt,
                               const char* baseName, const char* baseNameAlt,
                               const char* baseNameComboList, const char* baseNameComboListAlt)
{
    openNode (name, nameAlt, baseName);
    const char* value = currentNode_->value();
    int result = NULL_POINTER;
    if(value[0] != '\0'){
        result = indexInComboListString(baseNameComboListAlt, value);
        if (result == NULL_POINTER) {
            XBuffer msg;
            msg < "Error: No such class registered: ";
            msg < value;
            xassertStr (0, static_cast<const char*>(msg));
            return UNREGISTERED_CLASS;
        }
    }
    return result;
}

void EditIArchive::closePointer (const char* name, const char* typeName, const char* derivedName) {
    closeNode (name);
}


bool EditIArchive::processEnum(int& value, const EnumDescriptor& descriptor, const char* name, const char* nameAlt)
{
	bool result = false;
    openNode(name, nameAlt, descriptor.typeName());
    if(isNodeExists()){
		value = descriptor.keyByNameAlt(currentNode_->value());
		result = true;
    }
    closeNode(name);
    return result;

}

bool EditIArchive::processBitVector(int& flags, const EnumDescriptor& descriptor, const char* name, const char* nameAlt) {
    openNode(name, nameAlt, descriptor.typeName());
    bool result = false;
	if(isNodeExists()){
		ComboStrings values;
		splitComboList(values, currentNode_->value(), '|');
		
		flags = 0;
		int size = values.size();
		for(int i = 0; i < size; ++i)
			flags |= descriptor.keyByNameAlt(values[i].c_str());
		result = true;
	} 
    closeNode(name);
    return result;
}


//////////////////////////////////////////////////
const char* EnumDescriptor::name(int key) const 
{
	KeyToName::const_iterator i = keyToName_.find(key);
	if(i != keyToName_.end())
		return i->second;

	if(!ignoreErrors_)
		xassert(0 && "Unregistered enum key");
	return "";
}

const char* EnumDescriptor::nameAlt(int key) const 
{
	KeyToName::const_iterator i = keyToNameAlt_.find(key);
	if(i != keyToNameAlt_.end())
		return i->second;

	if(!ignoreErrors_)
		xassert(0 && "Unregistered enum key");
	return "";
}

int EnumDescriptor::keyByName(const char* name) const 
{
	NameToKey::const_iterator i = nameToKey_.find(name);
	if(i != nameToKey_.end())
		return i->second;

	if(!ignoreErrors_)
		xassertStr(!strlen(name) && "Unknown enum identificator will be ignored: ",(typeName_ + "::" + name).c_str());
	return 0;
}

int EnumDescriptor::keyByNameAlt(const char* nameAlt) const 
{
	NameToKey::const_iterator i = nameAltToKey_.find(nameAlt);
	if(i != nameAltToKey_.end())
		return i->second;

	if(!ignoreErrors_)
		xassertStr(!strlen(nameAlt) && "Unknown enum identificator will be ignored: ",(typeName_ + "::" + nameAlt).c_str());
	return 0;
}

bool EnumDescriptor::nameExists(const char* name) const
{
    return nameToKey_.find(name) != nameToKey_.end();
}

bool EnumDescriptor::nameAltExists(const char* nameAlt) const
{
    return nameAltToKey_.find(nameAlt) != nameAltToKey_.end();
}

string EnumDescriptor::nameCombination(int bitVector, const char* separator) const 
{
	string value;
	for(KeyToName::const_iterator i = keyToName_.begin(); i != keyToName_.end(); ++i)
		if((bitVector & i->first) == i->first){
			bitVector &= ~i->first;
			if(!value.empty())
				value += separator;
			value += i->second;
		}
	return value;
}

ComboStrings EnumDescriptor::nameCombinationStrings(int bitVector) const 
{
	ComboStrings result;
	for(KeyToName::const_iterator i = keyToName_.begin(); i != keyToName_.end(); ++i)
		if((bitVector & i->first) == i->first){
			bitVector &= ~i->first;
			result.push_back(i->second.c_str());
		}
	return result;
}

string EnumDescriptor::nameAltCombination(int bitVector, const char* separator) const 
{
	string value;
	for(KeyToName::const_iterator i = keyToNameAlt_.begin(); i != keyToNameAlt_.end(); ++i)
		if((bitVector & i->first) == i->first){
			bitVector &= ~i->first;
			if(!value.empty())
				value += separator;
			value += i->second;
		}
	return value;
}

int EnumDescriptor::defaultValue() const 
{ 
	xassert(!keyToName_.empty()); 
	return (int)keyToName_.begin()->first; 
}

void EnumDescriptor::add(int key, const char* name, const char* nameAlt)
{
//	xassert(name && strlen(name) && stlren(nameAlt));
	keyToName_[key] = name;
	keyToNameAlt_[key] = nameAlt;
	nameToKey_[name] = key;
	nameAltToKey_[nameAlt] = key;
	if(!comboList_.empty()){
		comboList_ += "|";
		comboListAlt_ += "|";
	}
	comboList_ += name;
	comboListAlt_ += nameAlt;
	comboStrings_.push_back(name);
	comboStringsAlt_.push_back(nameAlt);
}

EnumDescriptor::Key::Key(int value) 
: value_(value) 
{
	value =(value & 0x55555555) +((value >> 1) & 0x55555555);
	value =(value & 0x33333333) +((value >> 2) & 0x33333333);
	value =(value & 0x0f0f0f0f) +((value >> 4) & 0x0f0f0f0f);
	value =(value & 0x00FF00FF) +((value >> 8) & 0x00FF00FF);
	value =(value & 0x0000FFFF) +((value >> 16) & 0x0000FFFF);
	bitsNumber_ = value;
}

bool EnumDescriptor::Key::operator<(const Key& rhs) const 
{
	return bitsNumber_ == rhs.bitsNumber_ ? value_ < rhs.value_ : bitsNumber_ > rhs.bitsNumber_;
}


/////////////////////////////////////////////////////
string cutTokenFromComboList(string& comboList) {
	int pos = comboList.find("|");
	if(pos == string::npos){
		string name = comboList;
		comboList = "";
		return name;
	}
	else{
		string name(comboList, 0, pos);
		comboList.erase(0, pos + 1);
		return name;
	}
}

string getEnumToken(const char*& buffer) {
	while(*buffer == ' ' || *buffer == '|')
		++buffer;
	const char* marker = buffer;
	while(*buffer != '|' && *buffer != 0)
		++buffer;
	string str(marker, buffer - marker);
	while(str.size() && str[str.size() - 1] == ' ')
		str.pop_back();
	return str;
}

ComboInts makeComboInts(const ComboStrings& values, const ComboStrings& list) {
    ComboInts result;
    ComboStrings::const_iterator it;
    FOR_EACH(values, it) {
        int index = std::distance(list.begin(),
                                   std::find(list.begin(), list.end(), *it)
                                   );
        result.push_back(index);
    }
    return result;
}

void joinComboList(std::string& out, const ComboStrings& strings, char delimeter)
{
	ComboStrings::const_iterator it;
	FOR_EACH(strings, it){
		out += *it;
		out.push_back(delimeter);
	}
	if(!out.empty())
		out.pop_back();
}

void splitComboList(ComboStrings& combo_array, const char* ptr, char delimeter)
{
	combo_array.clear();

	const char* start = ptr;

	for(; *ptr; ++ptr)
		if(*ptr == delimeter){
			combo_array.push_back(std::string(start, ptr));
			start = ptr + 1;
		}
	combo_array.push_back(std::string(start, ptr));
}

int indexInComboListString(const char* combo_string, const char* value)
{
    if(!combo_string || !value)
		return -1;

	if(*combo_string == 0 && *value == 0)
		return 0;

	size_t value_len = strlen(value);
	const char* ptr = combo_string;

	const char* token_start = ptr;
	const char* token_end = ptr;

	int index = 0;
	while(*ptr != '\0') {
		if(*ptr != '|') {
			token_end = ptr+1;
			if(!token_start)
				token_start = ptr;
		}
		if(*ptr == '|' || *(ptr + 1) == '\0') {
			size_t len = token_end - token_start;
			if(len == value_len && strncmp(token_start, value, len) == 0)
				return index;
			++index;
			token_start = ptr+1;
			token_end = ptr+1;
		}
		++ptr;
	}
	return -1;
}

string getStringTokenByIndex(const char* worker, int number)
{
	int current = 0;
	const char* begin = worker;
	while(*worker){
		if(*worker == '|')
			if(current == number)
				break;
			else{
				++current;
				begin = worker + 1;
			}
		++worker;
	}
	if(current == number && worker > begin)
		return string(begin, worker);
	return string("");
}

//////////////////////////////////////////////////////////////////////////////

bool EditOArchive::processEnum(int& value, const EnumDescriptor& descriptor, const char* name, const char* nameAlt)
{
    openNode(name, nameAlt, descriptor.typeName());
    currentNode_->setValue(descriptor.nameAlt(value));
    currentNode_->setType(descriptor.typeName());

	currentNode_->setComboList(TreeNode::COMBO, descriptor.comboListAlt());
    closeNode(name);
	return true;
}

bool EditOArchive::processBitVector(int& flags, const EnumDescriptor& descriptor, const char* name, const char* nameAlt)
{
    openNode(name, nameAlt, descriptor.typeName());
    std::string value = descriptor.nameAltCombination(flags);
    currentNode_->setValue(value.c_str());
    currentNode_->setComboList(TreeNode::COMBO_MULTI, descriptor.comboListAlt());
    closeNode(name);
    return true;
}

Archive* EditOArchive::openDefaultArchive(const char* typeName, const char* derivedTypeName, const char* derivedTypeNameAlt)
{
	if(derivedTypeName){
		if(attribEditorInterface().treeNodeStorage().registerDerivedType(typeName, derivedTypeName, derivedTypeNameAlt))
			return new EditOArchive();
	}
	else{
		if(attribEditorInterface().treeNodeStorage().registerType(typeName))
			return new EditOArchive();
	}
	return 0;
}

void EditOArchive::closeDefaultArchive(ShareHandle<Archive> base_ar, const char* typeName, const char* derivedTypeName, const char* derivedTypeNameAlt)
{
    EditOArchive& ar = safe_cast_ref<EditOArchive&>(*base_ar.get ());
    TreeNode* treeNode = const_cast<TreeNode*>(ar.rootNode ());

    if(!treeNode->empty()){
        TreeNode* newNode = treeNode->front();
        if(derivedTypeName == 0){ // не полиморфный
			attribEditorInterface().treeNodeStorage().assignType(typeName, newNode);
        }
        else{
            newNode->setName("");
            newNode->setType(typeName);
            newNode->setValue(derivedTypeNameAlt);
            newNode->setEditType(TreeNode::POLYMORPHIC);
			attribEditorInterface().treeNodeStorage().assignDerivedType(typeName, derivedTypeName, newNode);

			std::string comboListAlt = attribEditorInterface().treeNodeStorage().derivedComboListAlt(typeName);
			newNode->setComboList(TreeNode::POLYMORPHIC, comboListAlt.c_str());
        }
    }
    else{
		/*
        TreeNode* newNode = treeNode;
        if(typeName == 0)
            currentNode_->setTreeNode (newNode);
        else{
            newNode->setType(typeName);
            newNode->setValue(derivedTypeNameAlt);
            newNode->setEditType(TreeNode::POLYMORPHIC);
			attribEditorInterface().treeNodeStorage().assignDerivedType(typeName, derivedTypeName, newNode);
        }
		*/
    }
}

//////////////////////////////////////////////////////////////////////////////

bool EditOArchive::openNode(const char* name, const char* nameAlt, const char* typeName)
{
    ++nesting_;
    if(name){
        if(nameAlt)
            name = nameAlt;
        if(*name == '_')
            ++name;
        TreeNode* node = new TreeNode(name);
        if((!nameAlt || *nameAlt == '_') && translatedOnly_)
            node->setHidden();
        
        if(strcmp(name, "@") != 0 && currentNode_->find(name, 0) != 0)
            xassertStr(ignoreNameDuplication_, XBuffer() < "Ёлемент с именем \"" < name < "\" повтор€етс€ дважды на одном уровне дерева редактора.");

        currentNode_->push_back(node);
        currentNode_ = node;
        currentNode_->setType (typeName);
        // TODO: decide what to do
        //currentNode_->setEditor (TreeEditorFactory::instance ().create (typeName, true));
        return true;
    }
    return false;
}

void EditOArchive::closeNode(const char* name) 
{
    --nesting_;
    if(name && currentNode_->parent())
        currentNode_ = const_cast<TreeNode*>(currentNode_->parent());
}

bool EditOArchive::openStruct(const char* name, const char* nameAlt, const char* typeName)
{
    openNode(name, nameAlt, typeName);
    return true;
}

void EditOArchive::closeStruct(const char* name)
{
    closeNode(name);
}

int EditOArchive::openPointer(const char* name, const char* nameAlt,
                              const char* baseName, const char* baseNameAlt,
                              const char* derivedName, const char* derivedNameAlt)
{
    if(!name)
        name = "[?]";
    if(!nameAlt)
        nameAlt = "[?]";

    openNode(name, nameAlt, baseName);
    
    currentNode_->setEditType (TreeNode::POLYMORPHIC);

    if(baseName) {
		if(!baseRegistered(baseName)){
			AttribEditorInterface& attribEditor = attribEditorInterface();
			TreeNodeStorage& storage = attribEditor.treeNodeStorage();
            storage.registerBaseType(baseName);
		}
        currentNode_->setType(baseName);
        currentNode_->setValue(derivedNameAlt ? derivedNameAlt : (derivedName ? derivedName : ""));
		currentNode_->setEditType(TreeNode::POLYMORPHIC);
    }
    else{
        currentNode_->setValue ("");
    }
    return NULL_POINTER;
}

void EditOArchive::closePointer(const char* name, const char* typeName, const char* derivedName)
{
	std::string derivedComboListAlt = attribEditorInterface().treeNodeStorage().derivedComboListAlt(currentNode_->type());
	currentNode_->setComboList(TreeNode::POLYMORPHIC, derivedComboListAlt.c_str());

    if(!name)
        name = "[?]";

    closeNode(name);
}

// --------------------------------------------------------------------------

void TreeNode::intersect(const TreeNode* node)
{
    if (empty ())
        return;

    for (iterator it = children_.begin(); it != children_.end(); ) {
        if((*it)->hidden()){
            it = children_.erase (it);
        }
        else{
            TreeNode* node_it = *it;
            const TreeNode* node_sub = node->find (node_it->name(), node_it->type());
            if(node_sub){
                if(node_sub->empty () && node_sub->editType() != TreeNode::VECTOR){
                    if(strcmp (node_sub->value(), node_it->value()) != 0){
                        node_it->setValue ("");
                        node_it->setUnedited (true);
                    }
                    ++it;
                }
                else{
                    if(node_sub->editor() == 0 && node_sub->editType() != TreeNode::VECTOR && node_sub->hidden() == false){
                        node_it->intersect (node_sub);
                        ++it;
                    }
                    else{
                        it = children_.erase(it);
                    }
                }
            }
            else{
                it = children_.erase(it);
            }
        }
    }
    return;
}


