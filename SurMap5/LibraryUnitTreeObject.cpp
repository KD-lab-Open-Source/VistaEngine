#include "StdAfx.h"

#include "..\AttribEditor\AttribEditorCtrl.h" // Для TreeNodeClipboard
#include "UnitAttribute.h"

#include "LibraryEditorTree.h"
#include "LibraryTreeObject.h"
#include "mfc\PopupMenu.h"

#include "Dictionary.h"

class LibraryUnitTreeObject : public LibraryElementTreeObject{
    typedef LibraryUnitTreeObject Self;
public:
    LibraryUnitTreeObject(LibraryCustomEditor* customEditor, const char* elementName);

    void onMenuAddChain(int index);
    void onMenuConstruction(PopupMenuItem& root);
	void onMenuCopyChains();
	void onMenuPasteChains();

    void onMenuDeleteChain(const char* chainName);

	LibraryTreeObject* subElementByName(const char* subElementName);
	AttributeBase* attribute() { return attribute_; }

	AnimationChain* chainByName(const char* name);
private:
    void rebuild();

    AttributeBase* attribute_;
};

class LibraryUnitChainTreeObject: public LibraryTreeObject{
public:
    LibraryUnitChainTreeObject(AttributeBase* attribute, AnimationChain* chain)
	: LibraryTreeObject(0)
	, attribute_(attribute)
	, chain_(0)
    {
		if(chain)
			setChainName(chain->name());
    }

	void onAttribElementChanged();
    void onRightClick();
	void onSelect();

	void setChainName(const char* name);
	const char* chainName() const{ return chainName_.c_str(); }
private:
    AttributeBase* attribute_;
	AnimationChain* chain_;
	std::string chainName_;
};

LibraryUnitTreeObject::LibraryUnitTreeObject(LibraryCustomEditor* customEditor, const char* elementName)
: LibraryElementTreeObject(customEditor, elementName)
, attribute_(0)
{
    xassert(customEditor->library() == &AttributeLibrary::instance());
    AttributeReference ref(elementName);
    attribute_ = const_cast<AttributeBase*>(&*ref);
}

void LibraryUnitTreeObject::rebuild()
{
	clear();
	if(attribute_){
		attribute_->refreshChains(true);

		AnimationChains::iterator it;
		FOR_EACH(attribute_->animationChains, it)
			add(new LibraryUnitChainTreeObject(attribute_, &*it));
	}
}

LibraryTreeObject* LibraryUnitTreeObject::subElementByName(const char* subElementName)
{
	iterator it;
	FOR_EACH(*this, it){
		LibraryUnitChainTreeObject* chainTreeObject = safe_cast<LibraryUnitChainTreeObject*>(*it);
		if(strcmp(chainTreeObject->chainName(), subElementName) == 0){
			return chainTreeObject;
		}
	}
	return 0;
}

void LibraryUnitTreeObject::onMenuAddChain(int index)
{
	if(attribute_){
		AttributeBase* attr = attribute_;

		const ComboStrings& chainNames = getEnumDescriptor(CHAIN_NONE).comboStrings();
		xassert(index >= 0 && index < chainNames.size());
		const char* chainName = chainNames[index].c_str();


		ComboStrings oldNames;
		AnimationChains::iterator cit;
		FOR_EACH(attribute_->animationChains, cit)
			oldNames.push_back(cit->name());

		AnimationChain newChain;
		newChain.chainID = ChainID(getEnumDescriptor(CHAIN_NONE).keyByName(chainName));
		attribute_->animationChains.push_back(newChain);
		attribute_->createModel(attribute_->modelName.c_str());
		attribute_->refreshChains(true);

		rebuild();


		TreeObject::iterator it;
		FOR_EACH(*this, it){
			LibraryUnitChainTreeObject* object = safe_cast<LibraryUnitChainTreeObject*>(*it);
			if(std::find(oldNames.begin(), oldNames.end(), object->chainName()) == oldNames.end()){
				object->focus();
				break;
			}
		}
	}
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

void LibraryUnitTreeObject::onMenuDeleteChain(const char* chainName)
{
	if(attribute_){
		AnimationChains::iterator it;
		FOR_EACH(attribute_->animationChains, it){
			if(strcmp(it->name(), chainName) == 0){
				attribute_->animationChains.erase(it);
				focus();
				rebuild();
				return;
			}
		}
		xassert(0);
	}
}

void LibraryUnitTreeObject::onMenuCopyChains()
{
	if(attribute_){
		CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_);	

		Serializeable serializeable(attribute_->animationChains);
		xassert(tree_ && ::IsWindow(tree_->GetSafeHwnd()));
		TreeNodeClipboard::instance().set(serializeable, tree_->GetSafeHwnd());
	}
}

void LibraryUnitTreeObject::onMenuPasteChains()
{
    CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_);	

	Serializeable serializeable(attribute_->animationChains);
	xassert(tree_ && ::IsWindow(tree_->GetSafeHwnd()));
	TreeNodeClipboard::instance().get(serializeable, tree_->GetSafeHwnd());

	rebuild();
	tree->signalElementSelected()(elementName(), getSerializeable());
}

void LibraryUnitTreeObject::onMenuConstruction(PopupMenuItem& root)
{
	if(attribute_){
		CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_);

		if(!root.empty())
			root.addSeparator();

		PopupMenuItem& chains = root.add(TRANSLATE("Добавить цепочку"));

		const ComboStrings& chainNames = getEnumDescriptor(CHAIN_NONE).comboStringsAlt();
		ComboStrings::const_iterator it;
		int index = 0;
		FOR_EACH(chainNames, it){
			chains.add(TRANSLATE(it->c_str()))
			.connect(bindArgument(bindMethod(*this, &Self::onMenuAddChain), index++));
		}

		root.add(TRANSLATE("Копировать все цепочки"))
		.connect(bindMethod(*this, &LibraryUnitTreeObject::onMenuCopyChains));

		const char* animationChainsTypeName = typeid(attribute_->animationChains).name();
		root.add(TRANSLATE("Вставить(заменить) цепочки"))
			.connect(bindMethod(*this, &LibraryUnitTreeObject::onMenuPasteChains))
			.enable(TreeNodeClipboard::instance().typeNameInClipboard(animationChainsTypeName, tree_->GetSafeHwnd()));
	}
}

// --------------------------------------------------------------------------

void LibraryUnitChainTreeObject::onRightClick()
{
	LibraryUnitTreeObject* unitObject = safe_cast<LibraryUnitTreeObject*>(parent());

    CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_);
	PopupMenu& menu = tree->popupMenu();
	menu.clear();
	PopupMenuItem& root = menu.root();
	
	root.add(TRANSLATE("Удалить цепочку"))
		.connect(bindArgument(bindMethod(*unitObject, &LibraryUnitTreeObject::onMenuDeleteChain), chainName_.c_str()));
	
	tree->spawnMenuAtObject(this);
	/* buildUnitNode (attribute_base, tree_.GetParentItem (item)); */
}

void LibraryUnitChainTreeObject::onSelect()
{
    __super::onSelect();
    CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_);

	attribute_->createModel(attribute_->modelName.c_str());
	attribute_->refreshChains(true);

	LibraryUnitTreeObject* unitObject = safe_cast<LibraryUnitTreeObject*>(parent());
	chain_ = unitObject->chainByName(chainName_.c_str());
	xassert(chain_);

	std::string elementName = unitObject->elementName();
	elementName += "\\";
	elementName += chainName_;
	if(tree->signalElementSelected())
		tree->signalElementSelected()(elementName.c_str(), Serializeable(*chain_, "chain", "chain"));
}

void LibraryUnitChainTreeObject::setChainName(const char* name)
{
	name_ = name;
	chainName_ = name;
}

void LibraryUnitChainTreeObject::onAttribElementChanged()
{
	xassert(chain_);
	setChainName(chain_->name());
	updateLabel();
}

class LibraryCustomUnitEditor : public LibraryCustomEditor{
public:
	LibraryCustomUnitEditor(EditorLibraryInterface* library);
	LibraryElementTreeObject* createTreeObject(const char* elementName);
};

LibraryCustomUnitEditor::LibraryCustomUnitEditor(EditorLibraryInterface* library)
: LibraryCustomEditor(library)
{
	xassert(library == &AttributeLibrary::instance());
}

LibraryElementTreeObject* LibraryCustomUnitEditor::createTreeObject(const char* elementName)
{
	return new LibraryUnitTreeObject(this, elementName);
}

REGISTER_LIBRARY_CUSTOM_EDITOR(&AttributeLibrary::instance, LibraryCustomUnitEditor);
