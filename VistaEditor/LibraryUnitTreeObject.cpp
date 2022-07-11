#include "StdAfx.h"

#include "Serialization/Dictionary.h"
#include "Serialization/StringTable.h"
#include "Serialization/BinaryArchive.h"

#include "kdw/Clipboard.h"
#include "kdw/Win32Proxy.h"
#include "kdw/LibraryTree.h"
#include "kdw/LibraryTreeObject.h"
#include "kdw/PopupMenu.h"
#include "kdw/LibraryTab.h"

#include "Units/UnitAttribute.h"

class LibraryUnitChainTreeObject;
class LibraryUnitTreeObject : public kdw::LibraryElementTreeObject{
    typedef LibraryUnitTreeObject Self;
public:
	LibraryUnitTreeObject(kdw::LibraryCustomEditor* customEditor, const char* elementName);

	void onMenuAddChain(kdw::ObjectsTree* tree, int index);
	void onMenuConstruction(kdw::PopupMenuItem& root, kdw::ObjectsTree* tree);
	void onMenuCopyChains(kdw::ObjectsTree* tree);
	void onMenuPasteChains(kdw::ObjectsTree* tree);
	void onMenuDelete(kdw::ObjectsTree* tree);
    void onMenuDeleteChain(kdw::ObjectsTree* tree, const char* chainName);

	LibraryTreeObject* subElementByName(const char* subElementName);
	AttributeBase* attribute() { return attribute_; }

	AnimationChain* chainByName(const char* name);
	LibraryUnitChainTreeObject* chainObjectByEditorIndex(int editorIndex);
	LibraryUnitChainTreeObject* chainObjectByName(const char* name);
	string makeChainNameUnique(AnimationChain* chain);
	Serializer getSerializer(const char* name, const char* nameAlt);

    void rebuild();
	kdw::LibraryTreeObject* findGroup(const char* groupName);

private:

    AttributeBase* attribute_;
};

class LibraryUnitChainTreeObject: public kdw::LibraryTreeObject, public sigslot::has_slots
{
public:
    LibraryUnitChainTreeObject(AttributeBase* attribute, AnimationChain* chain, const char* groupName)
	: kdw::LibraryTreeObject()
	, attribute_(attribute)
	, chain_(0)
	, groupName_(groupName)
    {
		if(chain)
			setChainName(chain->name());
    }

	kdw::LibraryTreeObject* onAttribElementChanged();
	void onRightClick(kdw::ObjectsTree* tree);
	void onSelect(kdw::ObjectsTree* tree);
	void onCopy();
	void onPaste();

	void setChainName(const char* name);
	const char* chainName() const{ return chainName_.c_str(); }

	Serializer getSerializer(const char* name, const char* nameAlt);
	void update(AnimationChain* chain);

	LibraryUnitTreeObject* parentUnit();

private:
    AttributeBase* attribute_;
	AnimationChain* chain_;
	string chainName_;
	string groupName_;
	
	static ShareHandle<BinaryOArchive> copiedChain_;
};

ShareHandle<BinaryOArchive> LibraryUnitChainTreeObject::copiedChain_;

LibraryUnitTreeObject::LibraryUnitTreeObject(kdw::LibraryCustomEditor* customEditor, const char* elementName)
: kdw::LibraryElementTreeObject(customEditor, elementName)
, attribute_(0)
{
    xassert(customEditor->library() == &AttributeLibrary::instance());
    AttributeReference ref(elementName);
    attribute_ = const_cast<AttributeBase*>(&*ref);
}

void LibraryUnitTreeObject::rebuild()
{
	if(attribute_){
		clear();
		attribute_->refreshChains();

		AnimationChains::iterator it;
		FOR_EACH(attribute_->animationChains, it){
			AnimationChain* chain = &*it;
			if(strlen(chain->groupName()))
				findGroup(chain->groupName());
		}

		FOR_EACH(attribute_->animationChains, it){
			AnimationChain* chain = &*it;
			if(!strlen(chain->groupName()))
				add(new LibraryUnitChainTreeObject(attribute_, chain, ""));
			else
				findGroup(chain->groupName())->add(new LibraryUnitChainTreeObject(attribute_, chain, chain->groupName()));
		}
	}
}

kdw::LibraryTreeObject* LibraryUnitTreeObject::findGroup(const char* groupName)
{
	iterator it;
	FOR_EACH(*this, it){
		kdw::LibraryTreeObject* object = safe_cast<kdw::LibraryTreeObject*>(&**it);
		if(dynamic_cast<LibraryUnitChainTreeObject*>(object))
			continue;
		if(!strcmp(object->text(), groupName))
			return object;
	}
	kdw::LibraryTreeObject* object = new kdw::LibraryTreeObject;
	object->setText(groupName);
	return add(object);
}

kdw::LibraryTreeObject* LibraryUnitTreeObject::subElementByName(const char* subElementName)
{
	iterator it;
	FOR_EACH(*this, it){
		LibraryUnitChainTreeObject* chainTreeObject = dynamic_cast<LibraryUnitChainTreeObject*>(&**it);
		if(chainTreeObject && !strcmp(chainTreeObject->chainName(), subElementName)){
			return chainTreeObject;
		}
	}
	return 0;
}

void LibraryUnitTreeObject::onMenuAddChain(kdw::ObjectsTree* tree, int indexAlt)
{
	if(attribute_){
		const ComboStrings& chainNamesAlt = getEnumDescriptor(CHAIN_NONE).comboStringsAlt();
		xassert(indexAlt >= 0 && indexAlt < chainNamesAlt.size());
		const char* chainNameAlt = chainNamesAlt[indexAlt].c_str();

		ComboStrings oldNames;
		AnimationChains::iterator cit;
		FOR_EACH(attribute_->animationChains, cit)
			oldNames.push_back(cit->name());

		AnimationChain newChain;
		newChain.chainID = ChainID(getEnumDescriptor(CHAIN_NONE).keyByNameAlt(chainNameAlt));
		attribute_->animationChains.push_back(newChain);
		attribute_->setCurrentAttribute(attribute_);
		attribute_->createModel(attribute_->modelName.c_str());
		rebuild();

		TreeObject::iterator it;
		FOR_EACH(*this, it){
			LibraryUnitChainTreeObject* object = dynamic_cast<LibraryUnitChainTreeObject*>(&**it);
			if(object && std::find(oldNames.begin(), oldNames.end(), object->chainName()) == oldNames.end()){
				tree->selectObject(object);
				break;
			}
		}
	}
}

LibraryUnitChainTreeObject* LibraryUnitTreeObject::chainObjectByEditorIndex(int editorIndex)
{
	TreeObject::iterator it;
	FOR_EACH(*this, it){
		LibraryUnitChainTreeObject* object = dynamic_cast<LibraryUnitChainTreeObject*>(&**it);
		if(object && chainByName(object->chainName())->editorIndex() == editorIndex)
			return object;
	}
	return 0;
}

LibraryUnitChainTreeObject* LibraryUnitTreeObject::chainObjectByName(const char* name)
{
	TreeObject::iterator it;
	FOR_EACH(*this, it){
		LibraryUnitChainTreeObject* object = dynamic_cast<LibraryUnitChainTreeObject*>(&**it);
		if(object && strcmp(object->chainName(), name) == 0)
			return object;
	}
	return 0;
}

AnimationChain* LibraryUnitTreeObject::chainByName(const char* chainName)
{
	if(attribute_){
		AnimationChains::iterator it;
		FOR_EACH(attribute_->animationChains, it)
			if(strcmp(it->name(), chainName) == 0)
				return &*it;
	}
	return 0;
}

string LibraryUnitTreeObject::makeChainNameUnique(AnimationChain* chain)
{
	string comboList;
	if(attribute_){
		AnimationChains::iterator it;
		FOR_EACH(attribute_->animationChains, it){
			if(chain != &*it && strlen(it->name()) > 0){
				comboList += "|";
				comboList += it->name();
			}
		}
		return kdw::makeName(comboList.c_str(), chain->name_.c_str());
	}
	xassert(0);
	return "";
}

Serializer LibraryUnitTreeObject::getSerializer(const char* name, const char* nameAlt)
{
	return Serializer(*attribute_, name, nameAlt);
}

void LibraryUnitTreeObject::onMenuDelete(kdw::ObjectsTree* _tree)
{
	kdw::LibraryTree* tree = safe_cast<kdw::LibraryTree*>(_tree);
	kdw::LibraryTreeObject* parent = safe_cast<kdw::LibraryTreeObject*>(this->parent());
	TreeObject::iterator foundIt = std::find(parent->begin(), parent->end(), this);
	xassert(foundIt != parent->end());
	TreeObject::iterator it = foundIt;
	++it;

	string elementName;
	if(it != parent->end()){
		kdw::LibraryElementTreeObject* nextElement = safe_cast<kdw::LibraryElementTreeObject*>(&**it);
		elementName = nextElement->elementName();
	}
	else if(foundIt != parent->begin()){
		--foundIt;
		kdw::LibraryElementTreeObject* previousElement = safe_cast<kdw::LibraryElementTreeObject*>(&**foundIt);
		elementName = previousElement->elementName();
	}
	customEditor_->library()->editorElementErase(this->elementName());
	
	kdw::LibraryTabEditable::buildLibraryTree(safe_cast<kdw::LibraryTree*>(tree), tree->root(), customEditor_); // suicide
	if(!elementName.empty())
		focusObjectByElementName(tree, elementName.c_str(), "", tree->root());
}

void LibraryUnitTreeObject::onMenuDeleteChain(kdw::ObjectsTree* _tree, const char* chainName)
{
	kdw::LibraryTree* tree = safe_cast<kdw::LibraryTree*>(_tree);
	if(attribute_){
		AnimationChains::iterator it;
		FOR_EACH(attribute_->animationChains, it){
			if(strcmp(it->name(), chainName) == 0){
				string elementName = this->elementName();
				attribute_->animationChains.erase(it);
				rebuild();
				focusObjectByElementName(tree, elementName.c_str(), "", tree->root());
				tree->update();
				return;
			}
		}
		xassert(0);
	}
}

void LibraryUnitTreeObject::onMenuCopyChains(kdw::ObjectsTree* tree)
{
	if(attribute_){
		Serializer serializeable(attribute_->animationChains, "animationChains");
		kdw::Clipboard clipboard(tree);
		clipboard.copy(serializeable);
	}
}

void LibraryUnitTreeObject::onMenuPasteChains(kdw::ObjectsTree* tree)
{
	Serializer serializeable(attribute_->animationChains, "animationChains");
	kdw::Clipboard clipboard(tree);
	clipboard.paste(serializeable);

	rebuild();
	__super::onSelect(tree);
}

void LibraryUnitTreeObject::onMenuConstruction(kdw::PopupMenuItem& root, kdw::ObjectsTree* _tree)
{
	if(attribute_){
		if(!root.empty())
			root.addSeparator();

		kdw::PopupMenuItem& chains = root.add(TRANSLATE("Добавить цепочку"));

		const ComboStrings& chainNames = getEnumDescriptor(CHAIN_NONE).comboStringsAlt();
		ComboStrings::const_iterator it;
		int index = 0;
		FOR_EACH(chainNames, it)
			chains.add(TRANSLATE(it->c_str()), _tree, index++)
				.connect(this, &Self::onMenuAddChain);

		root.add(TRANSLATE("Копировать все цепочки"), _tree)
		.connect(this, &LibraryUnitTreeObject::onMenuCopyChains);

		kdw::Clipboard clipboard(_tree);

		const char* animationChainsTypeName = typeid(attribute_->animationChains).name();
		root.add(TRANSLATE("Вставить(заменить) цепочки"), _tree)
			.connect(this, &LibraryUnitTreeObject::onMenuPasteChains)
			.enable(clipboard.canBePastedOn(animationChainsTypeName));
	}
}

// --------------------------------------------------------------------------

void LibraryUnitChainTreeObject::onRightClick(kdw::ObjectsTree* tree)
{
	LibraryUnitTreeObject* unitObject = parentUnit();

	kdw::PopupMenu menu(100);
	kdw::PopupMenuItem& root = menu.root();
	
	root.add(TRANSLATE("Удалить цепочку"), tree, chainName_.c_str())
		.connect(unitObject, &LibraryUnitTreeObject::onMenuDeleteChain);
	
	root.addSeparator();

	root.add(TRANSLATE("Копировать цепочку"))
		.connect(this, &LibraryUnitChainTreeObject::onCopy);

	if(copiedChain_){
		kdw::LibraryTree* libraryTree = safe_cast<kdw::LibraryTree*>(tree);
		root.add(TRANSLATE("Вставить цепочку"))
			.connect(this, &LibraryUnitChainTreeObject::onPaste)
			.connect((kdw::Tree*)tree, &kdw::Tree::update)
			.connect(libraryTree->tab()->propertyTree(), &kdw::PropertyTree::revert);
	}
				
	menu.spawn(tree);
	/* buildUnitNode (attribute_base, tree_.GetParentItem (item)); */
}

void LibraryUnitChainTreeObject::onCopy()
{	
	if(!chain_)
		return;
	copiedChain_ = new BinaryOArchive;
	copiedChain_->serialize(*chain_, "chain", "chain");
 }

void LibraryUnitChainTreeObject::onPaste()
{
	if(!copiedChain_ || !chain_)
		return;
	BinaryIArchive ia(*copiedChain_);
	ia.serialize(*chain_, "chain", "chain");
	setChainName(chain_->name());
}

void LibraryUnitChainTreeObject::onSelect(kdw::ObjectsTree* tree)
{
	LibraryUnitTreeObject* unitObject = parentUnit();
	if(chain_)
		update(chain_);
	chain_ = unitObject->chainByName(chainName_.c_str());
	xassert(chain_);

	attribute_->setCurrentAttribute(attribute_);
	attribute_->createModel(attribute_->modelName.c_str());
	
	string elementName = unitObject->elementName();
	elementName += "\\";
	elementName += chainName_;
	bookmark_.setElementName(elementName.c_str());
	__super::onSelect(tree);
}

Serializer LibraryUnitChainTreeObject::getSerializer(const char* name, const char* nameAlt)
{
	LibraryUnitTreeObject* unitObject = parentUnit();
	chain_ = unitObject->chainByName(chainName_.c_str());

	if(chain_)
		return Serializer(*chain_, "", "");
	else
		return Serializer();
}

void LibraryUnitChainTreeObject::update(AnimationChain* chain)
{
	chainName_ = chain->name();
	setText(chainName_.c_str());
	chain_ = 0;
}

void LibraryUnitChainTreeObject::setChainName(const char* name)
{
	setText(name);
	chainName_ = name;
}

LibraryUnitTreeObject* LibraryUnitChainTreeObject::parentUnit() 
{
	return safe_cast<LibraryUnitTreeObject*>(dynamic_cast<LibraryUnitTreeObject*>(parent()) ? parent() : parent()->parent());
}

kdw::LibraryTreeObject* LibraryUnitChainTreeObject::onAttribElementChanged()
{
	xassert(chain_);
	if(chain_){
		LibraryUnitTreeObject* unitObject = parentUnit();
		if(unitObject->attribute()){
			AnimationChains::iterator it;
			int index = 0;
			FOR_EACH(unitObject->attribute()->animationChains, it){
				it->setEditorIndex(index++);
			}
		}
		int chainIndex = chain_->editorIndex();

		unitObject->rebuild();
		return unitObject->chainObjectByEditorIndex(chainIndex);
	}
	return this;
}

class LibraryCustomUnitEditor : public kdw::LibraryCustomEditor{
public:
	LibraryCustomUnitEditor(EditorLibraryInterface* library);
	kdw::LibraryElementTreeObject* createTreeObject(const char* elementName);
};

LibraryCustomUnitEditor::LibraryCustomUnitEditor(EditorLibraryInterface* library)
: LibraryCustomEditor(library)
{
	xassert(library == &AttributeLibrary::instance());
}

kdw::LibraryElementTreeObject* LibraryCustomUnitEditor::createTreeObject(const char* elementName)
{
	return new LibraryUnitTreeObject(this, elementName);
}

DECLARE_SEGMENT(LibraryCustomUnitEditor)
REGISTER_LIBRARY_CUSTOM_EDITOR(&AttributeLibrary::instance, LibraryCustomUnitEditor);
