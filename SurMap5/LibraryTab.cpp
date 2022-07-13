#include "stdafx.h"
#include <set>
#include "Serialization.h"
#include "FileUtils.h"
#include "LibraryTab.h"

#include "LibraryTreeObject.h"

#include "Dictionary.h"
#include "mfc\PopupMenu.h"
#include "LibraryWrapper.h"
#include "LibraryEditorTree.h"
#include "LibraryEditorWindow.h"
#include "LibraryAttribEditor.h"
#include "LibrariesManager.h"

#include "EditArchive.h"

#include "CreateAttribDlg.h"

#ifdef __UNIT_ATTRIBUTE_H__
# error "UnitAttribute.h included!"
#endif

REGISTER_CLASS(LibraryTabBase, LibraryTabEditable, "LibraryTabEditable");
REGISTER_CLASS(LibraryTabBase, LibraryTabSearch, "LibraryTabSearch");

LibraryTreeObject* objectByElementName(const char* elementName, const char* subElementName, TreeObject* parent);

LibraryTabBase::LibraryTabBase(CLibraryEditorWindow* window, const LibraryBookmark& bookmark)
: window_(window)
, bookmark_(bookmark)
{
}

void LibraryTabBase::serialize(Archive& ar)
{
	ar.serialize(bookmark_, "bookmark", 0);
}

void LibraryTabBase::setBookmark(const LibraryBookmark& bookmark)
{
	bookmark_ = bookmark;
}

void LibraryTabBase::onSelect()
{
	xassert(window_);
	window_->attribEditor().signalElementChanged()	= bindMethod(*this, &Self::onAttribElementChanged);
	window_->attribEditor().signalElementSelected() = bindMethod(*this, &Self::onAttribElementSelected);
	window_->tree().signalElementSelected()			= bindMethod(*this, &Self::onElementSelected);
}


// ----------------------------------------------------------------------------

static bool isDigit(char c)/*{{{*/
{
	static bool initialized = false;
	static bool table[256];
	if(!initialized){
		for(int i = 0; i < 256; ++i)
			table[i] = false;
		table['0'] = table['1'] = table['2'] = table['3'] = table['4']
		= table['5'] = table['6'] = table['7'] = table['8'] = table['9'] = true;
		initialized = true;
	}
	return table[(unsigned char)c];
}/*}}}*/

std::string makeName(const char* reservedComboList, const char* nameBase)
{
	if(nameBase[0] == '\0')
		nameBase = "unnamed";

	if(indexInComboListString(reservedComboList, nameBase) < 0)
		return std::string(nameBase);
	std::string name_base = nameBase;

	// обрезаем циферки
	std::string::iterator ptr = name_base.end() - 1;
	if(name_base.size() > 1){
		while(ptr != name_base.begin() && isDigit(*ptr))
			ptr--;
		name_base = std::string(name_base.begin(), ptr + 1);
	}

	int index = 1;
	std::string default_name = name_base;
	while(indexInComboListString(reservedComboList, default_name.c_str()) >= 0){
		char num_buf[11];
		sprintf(num_buf, "%02i", index);

		XBuffer buf;
		buf < name_base.c_str();
		buf < num_buf;
		default_name = static_cast<const char*>(buf);
		++index;
	}
	return default_name;
}

// ----------------------------------------------------------------------------
LibraryGroupTreeObject::LibraryGroupTreeObject(LibraryCustomEditor* customEditor, const char* groupName)
: LibraryTreeObject(customEditor)
, groupName_(groupName)
{
	if(customEditor && !strcmp(groupName, ""))
		name_ = customEditor->library()->name();
	else
		name_ = groupName;
}
void LibraryGroupTreeObject::onRightClick()
{
	if(customEditor_){
		CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_);
		tree->popupMenu().clear();
		PopupMenuItem& root = tree->popupMenu().root();
		root.add(TRANSLATE("Добавить"))
		.connect(bindMethod(*this, &LibraryGroupTreeObject::onMenuCreate));

		onMenuConstruction(root);

		if(groupName_ == "" && customEditor_->library()->editorDynamicGroups()){
			root.addSeparator();
			root.add(TRANSLATE("Добавить группу"))
			.connect(bindMethod(*this, &LibraryGroupTreeObject::onMenuCreateGroup));
		}

		tree->spawnMenuAtObject(this);
	}
}

std::string LibraryGroupTreeObject::fullGroupName() const
{
	std::string result = groupName_;
	const LibraryGroupTreeObject* current = safe_cast<const LibraryGroupTreeObject*>(parent());
	while(current && current->parent()){
		result = std::string(current->groupName()) + "\\" + result;
		current = safe_cast<const LibraryGroupTreeObject*>(current->parent());
	}
	return result;
}

void LibraryGroupTreeObject::onMenuCreateGroup()
{
	if(customEditor_){
		const char* groupsComboList = customEditor_->library()->editorGroupsComboList();
		CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_);

		CCreateAttribDlg createDialog(false, false, makeName(groupsComboList, TRANSLATE("Новая группа")).c_str());
		int result = createDialog.DoModal();
		if(result == IDOK){
			std::string attribName = makeName(groupsComboList, createDialog.name());
			customEditor_->library()->editorAddGroup(attribName.c_str());
			LibraryTabEditable::buildLibraryTree(tree->rootObject(), customEditor_); // suicide
			focusObjectByGroupName(attribName.c_str(), tree->rootObject());
		}
		tree->UpdateWindow();

	}
}

void LibraryGroupTreeObject::onMenuCreate()
{
	if(customEditor_){
		CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_);

		std::string groupName = fullGroupName();
		bool canBePasted = true;

		CCreateAttribDlg createDialog(canBePasted, true, TRANSLATE("Новый элемент"));
		int result = createDialog.DoModal();
		if(result == IDOK){
			std::string attribName = makeName(customEditor_->library()->editorComboList(), createDialog.name());

			std::string newName = customEditor_->library()->editorAddElement(attribName.c_str(), groupName.c_str());
			xassert(newName != "");

			LibraryTabEditable::buildLibraryTree(tree->rootObject(), customEditor_); // suicide
			if(LibraryElementTreeObject* object = safe_cast<LibraryElementTreeObject*>(objectByElementName(newName.c_str(), "", tree->rootObject()))){
				object->setElementName(newName.c_str());
				if(canBePasted && createDialog.paste())
					object->onMenuPaste();
				object->rebuild();
				object->focus();
			}
			else
				xassert(0);
		}
		tree->UpdateWindow();
	}
}

void LibraryGroupTreeObject::onSelect()
{
	if(customEditor_->library()){
		__super::onSelect();
		CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_);
		if(tree->signalElementSelected())
			tree->signalElementSelected()("", Serializeable());
	}
}

// ----------------------------------------------------------------------------
LibraryElementTreeObject::LibraryElementTreeObject(LibraryCustomEditor* customEditor, const char* elementName)
: LibraryTreeObject(customEditor)
, elementName_(elementName)
, elementIndex_(-1)
{
	setElementName(elementName);
}


void LibraryElementTreeObject::onAttribElementChanged()
{
	int index = customEditor_->library()->editorFindElement(elementName_.c_str());
	if(index != elementIndex_){
		const char* name = customEditor_->library()->editorElementName(elementIndex_);

		setElementName(name);
		updateLabel();
	}
}


void LibraryElementTreeObject::onRightClick()
{
	CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_);
	tree->popupMenu().clear();

	PopupMenuItem& root = tree->popupMenu().root();

	onMenuConstruction(root);

	if(!root.empty())
		root.addSeparator();

	if(customEditor_->library()->editorAllowRename()){
		root.add(TRANSLATE("Переименновать"))
		.connect(bindMethod(*this, &LibraryElementTreeObject::onMenuRename));
	}

	root.add(TRANSLATE("Копировать"))
	.connect(bindMethod(*this, &LibraryElementTreeObject::onMenuCopy));


	const char* typeName = customEditor_->library()->editorElementSerializeable(elementName_.c_str(), "", "", true).typeName();
	std::string groupName = customEditor_->library()->editorElementGroup(elementName_.c_str());

	bool canBePasted = typeName ? TreeNodeClipboard::instance().typeNameInClipboard(typeName, tree_->GetSafeHwnd()) : false;

	root.add(TRANSLATE("Вставить"))
	.connect(bindMethod(*this, &LibraryElementTreeObject::onMenuPaste))
	.enable(canBePasted);

	root.addSeparator();
	root.add(TRANSLATE("Искать ссылку"))
	.connect(bindMethod(*this, &LibraryElementTreeObject::onMenuSearch));

	root.addSeparator();
	root.add(TRANSLATE("Удалить"))
	.connect(bindMethod(*this, &LibraryElementTreeObject::onMenuDelete));

	tree->spawnMenuAtObject(this);
}


void LibraryElementTreeObject::onMenuDelete()
{
	CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_);
	customEditor_->library()->editorElementErase(elementName_.c_str());
	LibraryTabEditable::buildLibraryTree(tree->rootObject(), customEditor_); // suicide
}

void LibraryElementTreeObject::setElementName(const char* elementName)
{
	xassert(!customEditor_ || customEditor_->library()->editorFindElement(elementName) != -1);
	elementName_ = elementName;
	name_ = elementName;
}

Serializeable LibraryElementTreeObject::getSerializeable()
{
	return customEditor_->library()->editorElementSerializeable(elementName_.c_str(), "", "", false);
}

void LibraryElementTreeObject::onMenuPaste()
{
	CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_);

	if(!TreeNodeClipboard::instance().empty()){

		Serializeable ser(customEditor_->library()->editorElementSerializeable(elementName(), "", "", true));
		TreeNodeClipboard::instance().get(ser, tree->GetSafeHwnd());
		rebuild();
		tree->signalElementSelected_(elementName(), getSerializeable());
	}
}

void LibraryElementTreeObject::onMenuCopy()
{
	CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_);

	EditorLibraryInterface* library = customEditor_->library();
	int index = library->editorFindElement(elementName_.c_str());
	Serializeable serializeable = library->editorElementSerializeable(index, "", "", true);
	TreeNodeClipboard::instance().set(serializeable, tree->GetSafeHwnd());
}

void LibraryElementTreeObject::onMenuRename()
{
	EditorLibraryInterface* library = customEditor_->library();

	CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_);	
	CCreateAttribDlg dlg(false, false, TRANSLATE("Переименновать"), elementName_.c_str(), tree->GetParent());
	if(dlg.DoModal() == IDOK){
		std::string newName = makeName(library->editorComboList(), dlg.name());
		library->editorElementSetName(elementName_.c_str(), newName.c_str());
		elementName_ = newName;
		name_ = elementName_;
		updateLabel();
		tree->signalElementSelected_(elementName(), getSerializeable());
	}	
}

void LibraryElementTreeObject::onMenuSearch()
{
	xassert(customEditor_->library());
	if(customEditor_->library()){
		CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_);
		LibraryTabEditable* editable = safe_cast<LibraryTabEditable*>(tree->tab());
		editable->onSearchLibraryElement(customEditor_->library()->name(), elementName_.c_str());
	}
}

bool LibraryElementTreeObject::onBeginDrag()
{
	if(!customEditor_->library()->editorDynamicGroups())
		return false;
	return true;
}

bool LibraryElementTreeObject::onDragOver(const TreeObjects& objects)
{
	if(!customEditor_->library()->editorDynamicGroups())
		return false;
	xassert(!objects.empty());
	if(objects.front() == this)
		return false;
	else
		return true;
}

void LibraryElementTreeObject::onDrop(const TreeObjects& objects)
{
	xassert(!objects.empty());
	LibraryTreeObject* treeObject = safe_cast<LibraryTreeObject*>(objects.front());
	if(treeObject->isElement()){
		LibraryElementTreeObject* source = safe_cast<LibraryElementTreeObject*>(treeObject);

		std::string sourceName = source->elementName_.c_str();

		LibraryGroupTreeObject* group = safe_cast<LibraryGroupTreeObject*>(parent());
		LibraryGroupTreeObject* sourceGroup = safe_cast<LibraryGroupTreeObject*>(source->parent());

		if(strcmp(sourceGroup->groupName(), group->groupName()))
			customEditor_->library()->editorElementSetGroup(sourceName.c_str(), group->groupName());

		customEditor_->library()->editorElementMoveBefore(sourceName.c_str(), elementName_.c_str());

		CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_);
		LibraryTabEditable::buildLibraryTree(tree->rootObject(), customEditor_); // suicide
		focusObjectByElementName(sourceName.c_str(), "", tree->rootObject());
	}
}

void LibraryElementTreeObject::onSelect()
{
	__super::onSelect();
	elementIndex_ = customEditor_->library()->editorFindElement(elementName_.c_str());

	if(CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_)){
		if(tree->signalElementSelected())
			tree->signalElementSelected()(elementName(), getSerializeable());
	}
}

// ----------------------------------------------------------------------------

void LibraryCustomEditorFactory::queueRegistration(CreatorBase& creator_op, LibraryInstanceFunc func)
{
	registrationQueue_[func] = &creator_op;
}

void LibraryCustomEditorFactory::add(const char* libraryName, CreatorBase& creator_op)
{
	if(creators_.find(libraryName) != creators_.end())
		xassertStr(0 && "Повторная регистрация в фабрике", libraryName);

	creators_[libraryName] = &creator_op;
}

const LibraryCustomEditorFactory::CreatorBase* LibraryCustomEditorFactory::find(const char* libraryName, bool silent)
{
	Creators::const_iterator it = creators_.find(libraryName);
	if(it != creators_.end())
		return it->second;

	xassert(silent && "Неопознанный идентификатор класса");
	return 0;
}

LibraryCustomEditor* LibraryCustomEditorFactory::create(const char* libraryName, bool silent)
{
	EditorLibraryInterface* library = LibrariesManager::instance().find(libraryName);
	if(library){
		if(const CreatorBase* creator = find(libraryName, true))
			return creator->create();
		else
			return new LibraryCustomEditor(library);
	}
	else
		return 0;
}

void LibraryCustomEditorFactory::registerQueued()
{
	if(!registrationQueue_.empty()){
		RegistrationQueue::iterator it;
		FOR_EACH(registrationQueue_, it){
			EditorLibraryInterface& library = it->first();
			add(library.name(), *it->second);
		}
		registrationQueue_.clear();
	}
}

bool fixDuplicates(EditorLibraryInterface* library)
{
	static std::vector<LibraryWrapperBase*> fixedLibraries;
	if(std::find(fixedLibraries.begin(), fixedLibraries.end(), library) != fixedLibraries.end())
		return false;

	bool result = false;
	typedef std::vector<std::string> Names;
	Names names;
	int count = library->editorSize();
	for(int i = 0; i < count; ++i){
		const char* name = library->editorElementName(i);
		Names::iterator it = std::find(names.begin(), names.end(), name);
		if(it != names.end()){
			std::string newName = makeName(library->editorComboList(), name);
			library->editorElementSetName(i, newName.c_str());
			result = true;
			names.push_back(newName);
		}
		else{
			names.push_back(name);
		}
	}

	fixedLibraries.push_back(library);
	return result;
}

// ----------------------------------------------------------------------------

void LibraryTabEditable::buildLibraryTree(TreeObject* rootObject, LibraryCustomEditor* customEditor)/*{{{*/
{
	rootObject->clear();

	typedef StaticMap<std::string, TreeObject*> Groups;
	Groups groups;


	if(customEditor->library()){
		//if(fixDuplicates(library))
		//	::AfxMessageBox(TRANSLATE("Библиотека содержала дубликаты, некоторые элементы были переименнованы"), MB_ICONWARNING | MB_OK);

		TreeObject* root = groups[""] = rootObject->add(customEditor->createGroupTreeObject(""));

		TreeObject* parent = root;

		const char* groupsComboList = customEditor->library()->editorGroupsComboList();
		if(groupsComboList){
			ComboStrings groupStrings;
			ComboStrings::iterator it;
			splitComboList(groupStrings, groupsComboList);
			FOR_EACH(groupStrings, it){
				const char* groupString = it->c_str();
				ComboStrings groupPath;
				ComboStrings::iterator pit;
				splitComboList(groupPath, groupString, '\\');

				std::string currentPath = "";
				TreeObject* lastObject = parent;
				FOR_EACH(groupPath, pit){
					if(!currentPath.empty())
						currentPath += "\\";

					currentPath += pit->c_str();
					TreeObject*& groupObject = groups[currentPath.c_str()];
					if(!groupObject){

						lastObject = lastObject->add(customEditor->createGroupTreeObject(pit->c_str()));
						groupObject = lastObject;
					}
					else
						lastObject = groupObject;
				}
			}
		}

		int count = customEditor->library()->editorSize();
		for(int index = 0; index < count; ++index){
			std::string name = customEditor->library()->editorElementName(index);
			if(name != ""){
				std::string group = customEditor->library()->editorElementGroup(index);
				parent = root;

				Groups::iterator it = groups.find(group);
				TreeObject*& groupObject = groups[group];
				if(it != groups.end())
					parent = groupObject;
				else{
					xassert(!groupsComboList);
					parent = groupObject = parent->add(customEditor->createGroupTreeObject(group.c_str()));
				}

				LibraryElementTreeObject* object = 0;
				object = customEditor->createTreeObject(name.c_str());
				parent->add(object);
				object->rebuild();
			}
		}
		groups[""]->expand();
	}
}/*}}}*/

bool focusObjectByGroupName(const char* name, TreeObject* parent)/*{{{*/
{
	TreeObject::iterator it;
	FOR_EACH(*parent, it){
		LibraryTreeObject* object = safe_cast<LibraryTreeObject*>(*it);
		if(object->isGroup()){
			if(!strcmp(((LibraryGroupTreeObject*)object)->groupName(), name)){
				object->focus();
				return true;
			}
		}
		if(focusObjectByGroupName(name, *it))
			return true;
	}
	return false;
}/*}}}*/

LibraryTreeObject* objectByElementName(const char* elementName, const char* subElementName, TreeObject* parent)
{
	TreeObject::iterator it;
	FOR_EACH(*parent, it){
		LibraryTreeObject* object = safe_cast<LibraryTreeObject*>(*it);
		if(object->isElement()){
			if(!strcmp(((LibraryElementTreeObject*)object)->elementName(), elementName)){
				if(LibraryTreeObject* subElement = object->subElementByName(subElementName))
					return subElement;
				else
					return object;
			}
		}
		if(TreeObject* object = objectByElementName(elementName, "", *it))
			return safe_cast<LibraryTreeObject*>(object);
	}
	return 0;
}

bool focusObjectByElementName(const char* elementName, const char* subElementName, TreeObject* parent)/*{{{*/
{
	if(LibraryTreeObject* object = objectByElementName(elementName, subElementName, parent)){
		object->focus();
		return true;
	}
	else 
		return false;
}/*}}}*/

bool updateObjectByElementName(const char* elementName, TreeObject* parent)/*{{{*/
{
	if(LibraryTreeObject* object = objectByElementName(elementName, "", parent)){
		object->updateLabel();
		return true;
	}
	else 
		return false;
}/*}}}*/

// ----------------------------------------------------------------------------
bool LibraryGroupTreeObject::onBeginDrag()
{
	//xassert(library_);
	//if(!library_->editorAllowDrag())
	//	return false;
	return false;
}

bool LibraryGroupTreeObject::onDragOver(const TreeObjects& objects)
{
	if(!customEditor_->library()->editorDynamicGroups())
		return false;

	xassert(!objects.empty());
	LibraryTreeObject* object = safe_cast<LibraryTreeObject*>(objects.front());
	if(object->isElement() || (object->isGroup() && object->parent() != 0))
		return true;
	else
		return false;
}

void LibraryGroupTreeObject::onDrop(const TreeObjects& objects)
{
	xassert(!objects.empty());
	LibraryTreeObject* treeObject = safe_cast<LibraryTreeObject*>(objects.front());
	if(treeObject->isElement()){
		LibraryElementTreeObject* source = safe_cast<LibraryElementTreeObject*>(treeObject);

		std::string sourceName = source->elementName();
		customEditor_->library()->editorElementSetGroup(sourceName.c_str(), groupName_.c_str());
		CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_);
		LibraryTabEditable::buildLibraryTree(tree->rootObject(), customEditor_); // suicide
		focusObjectByElementName(sourceName.c_str(), "", tree->rootObject());
	}
	else
		xassert(0);
}




LibraryTabEditable::LibraryTabEditable(CLibraryEditorWindow* window, const LibraryBookmark& bookmark)
: LibraryTabBase(window, bookmark)
, firstSelect_(true)
{
	if(window)
		setLibrary(bookmark_.libraryName());
}


void LibraryTabEditable::onMenuFindUnused()
{
	CWaitCursor waitCursor;
	
	LibraryTabSearch* tab = new LibraryTabSearch(window_);
	window_->addTab(tab);

	tab->findUnused(library()->name(), bindMethod(*window_, &CLibraryEditorWindow::onProgress));

	window_->setCurrentTab(tab, true);
	window_->updateTabs();
}

void LibraryTabEditable::onSearchLibraryElement(const char* libraryName, const char* elementName)
{
	CWaitCursor waitCursor;

	LibraryTabSearch* tab = new LibraryTabSearch(window_);
	window_->addTab(tab);
	tab->findLibraryReference(libraryName, elementName, bindMethod(*window_, &CLibraryEditorWindow::onProgress));
	window_->setCurrentTab(tab, true);
	window_->updateTabs();
}

void LibraryTabEditable::onSearchTreeNode(const TreeNode* node)
{
	CWaitCursor waitCursor;

	TreeNode root;
	root.push_back(new TreeNode);
	*const_cast<TreeNode*>(&*root.children().front()) = *node;

	LibraryTabSearch* tab = new LibraryTabSearch(window_);
	window_->addTab(tab);
	tab->findTreeNode(root, false, bindMethod(*window_, &CLibraryEditorWindow::onProgress));
	window_->setCurrentTab(tab, true);
	window_->updateTabs();
}


void LibraryTabEditable::onMenuSort()
{
	xassert(customEditor_->library());
	customEditor_->library()->editorSort();
	buildLibraryTree(window_->tree().rootObject(), customEditor_);
	setBookmark(bookmark_);
}

void LibraryTabEditable::onMenuConstruction(PopupMenuItem& root)
{
	root.add(TRANSLATE("Отсортировать"))
		.connect(bindMethod(*this, &Self::onMenuSort));
	root.add(TRANSLATE("Найти неиспользуемые..."))
		.connect(bindMethod(*this, &Self::onMenuFindUnused));
}

EditorLibraryInterface* LibraryTabEditable::library() const
{
	xassert(customEditor_ && customEditor_->library());
	return customEditor_->library();
}

void LibraryTabEditable::setLibrary(const char* libraryName)
{
	customEditor_ = LibraryCustomEditorFactory::instance().create(libraryName);
}

void LibraryTabEditable::onSelect()
{
	firstSelect_ = true;
	__super::onSelect();

    window_->attribEditor().signalSearchTreeNode() = bindMethod(*this, &Self::onSearchTreeNode);
    window_->attribEditor().signalFollowReference() = bind2nd(bindMethod(*window_, &CLibraryEditorWindow::openBookmark), true);

	if(&window_->attribEditor() && &window_->tree()){
		CWaitCursor waitCursor;
		window_->attribEditor().detachData();
		buildLibraryTree(window_->tree().rootObject(), customEditor_);
		loadTabState();
		setBookmark(bookmark_);
	}
}

void LibraryTabEditable::loadTabState()
{
	if(library() && library()->editorFindElement(bookmark_.elementName()) >= 0){

		std::string group = library()->editorElementGroup(bookmark_.elementName());
		AttribEditorStates::iterator it = attribEditorStates_.find(group.c_str());
		if(it != attribEditorStates_.end()){
			AttribEditorState& state = it->second;
			XBuffer buf(&state[0], state.size());
			window_->attribEditor().loadExpandState(buf);
		}
	}
}

void LibraryTabEditable::saveTabState()
{
	if(library() && library()->editorFindElement(bookmark_.elementName()) >= 0){
		std::string group = library()->editorElementGroup(bookmark_.elementName());

		XBuffer buf(64, 1);
		window_->attribEditor().saveExpandState(buf);
		AttribEditorState& state = attribEditorStates_[group];
		state.resize(buf.size());
		memcpy(&state[0], buf.buffer(), buf.size());
	}
}

void LibraryTabEditable::onClose()
{
	saveTabState();
}

void LibraryTabEditable::serialize(Archive& ar)
{
	LibraryTabBase::serialize(ar);
	ar.serialize(attribEditorStates_, "attribEditorStates", 0);
	if(ar.isInput())
		setLibrary(bookmark_.libraryName());
}


void LibraryTabEditable::onAttribElementChanged(const TreeNode* node)
{
	xassert(library());

	LibraryTreeObject* object = safe_cast<LibraryTreeObject*>(window_->tree().selected());
	if(object){
		object->onAttribElementChanged();
		if(object->isElement()){
			LibraryElementTreeObject* elementObject = safe_cast<LibraryElementTreeObject*>(object);
			bookmark_.setElementName(elementObject->elementName());
		}
	}
	else
		xassert(0);
}

void LibraryTabEditable::onElementSelected(const char* elementName, Serializeable& serializeable)
{
	xassert(library());
	if(!firstSelect_)
		saveTabState();
	else
		firstSelect_ = false;
	xassert(window_);
	bookmark_.setElementName(elementName);
	if(strcmp(elementName, "")){
		window_->attribEditor().attachSerializeable(serializeable);
		loadTabState();
		window_->attribEditor().selectItemByPath(bookmark_.attribEditorPath());
	}
	else
		window_->attribEditor().detachData();
}

void LibraryTabEditable::onAttribElementSelected(const ComboStrings& path)
{
	bookmark_.setAttribEditorPath(path);
}

bool LibraryTabEditable::isValid() const
{
	return library() != 0;
}

const char* LibraryTabEditable::title() const
{
	if(library()){
		return TRANSLATE(library()->editName());
	}
	else{
		return bookmark_.libraryName();
	}
}

void LibraryTabEditable::setBookmark(const LibraryBookmark& bookmark)
{
	focusObjectByElementName(bookmark.elementName(), bookmark.subElementName(), window_->tree().rootObject());
	window_->attribEditor().selectItemByPath(bookmark.attribEditorPath());
	LibraryTabBase::setBookmark(bookmark);
}

// ----------------------------------------------------------------------------

class LibraryBookmarkTreeObject : public LibraryTreeObject
{
public:
	LibraryBookmarkTreeObject(CLibraryEditorWindow* window, const LibraryBookmark& bookmark, const char* name, int index = -1)
	: LibraryTreeObject(0)
	, bookmark_(bookmark)
	, window_(window)
	, index_(index)
	{
		name_ = name;
		customEditor_ = LibraryCustomEditorFactory::instance().create(bookmark.libraryName());
	}
	bool isBookmark() const{ return true; }

	void onSelect(){
		if(customEditor_){
			__super::onSelect();
			CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_);
			XBuffer buf;
			buf <= index_;
 			if(tree->signalElementSelected_)
				tree->signalElementSelected_(buf, Serializeable());
		}
	}

	void onMenuRemoveLibraryElement()
	{
		CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_);

		std::string elementName = bookmark_.elementName();
		if(customEditor_->library()){
			LibraryTabSearch* tab = safe_cast<LibraryTabSearch*>(window_->currentTab());
			SearchResultItems& items = tab->items();
			SearchResultItems::iterator it = std::find(items.begin(), items.end(), bookmark_);
			if(it != items.end()){
				items.erase(it);
			}
			else{
				xassert(0);
			}

			customEditor_->library()->editorElementErase(elementName.c_str());
			erase();
		}
	}

	void onRightClick(){
		CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_);
		tree->popupMenu().clear();

		PopupMenuItem& menu = tree->popupMenu().root();

		menu.add(TRANSLATE("Следовать по ссылке..."))
			.connect(bindMethod(*this, LibraryBookmarkTreeObject::onDoubleClick))
			.setDefault();		
		menu.addSeparator();
		menu.add(TRANSLATE("Удалить элемент библиотеки"))
			.connect(bindMethod(*this, LibraryBookmarkTreeObject::onMenuRemoveLibraryElement));

		tree->spawnMenuAtObject(this);
	}

	bool onDoubleClick(){
		window_->openBookmark(bookmark_, true);
		return true;
	}

	const LibraryBookmark& bookmark() const{ return bookmark_; }
	int elementIndex() const{ return index_; }
protected:
	LibraryBookmark bookmark_;
	CLibraryEditorWindow* window_;
	int index_;
};

// ----------------------------------------------------------------------------
struct TreePathLeaf{
	TreePathLeaf(const char* _name = "", int _index = -1)
	: name(_name)
	, index(_index)
	{
	}
	const char* name;
	int index;
};

typedef std::vector<TreePathLeaf> TreePath;
typedef std::vector<TreePath> TreePathes;


static bool compareTreeNodes(const TreeNode& lhs, const TreeNode& rhs)
{
	if(strcmp(lhs.type(), rhs.type()) != 0)
		return false;

	if(!lhs.editor() && !rhs.editor() && strcmp(lhs.value(), rhs.value()) != 0 && strlen(lhs.type()) && strlen(rhs.type()))
		return false;

	if(strlen(lhs.type()) == 0)
		return false;

	if(lhs.size() != rhs.size())
		return false;

	TreeNode::const_iterator lit = lhs.begin();
	TreeNode::const_iterator rit = rhs.begin();
	for(; lit != lhs.end(); ++lit, ++rit){
		if(strcmp((*lit)->name(), (*rit)->name()) != 0)
			return false;
		if(!compareTreeNodes(**lit, **rit))
			return false;
	}

	return true;
}

static void treeNodeFindElementReferences(TreePathes& result, const TreeNode* const root,
										  const char* libraryName, const char* elementName, int index = 0, const TreePath& pathBase = TreePath())
{
	if(root->hidden())
		return;

	TreeEditorFactory& factory = TreeEditorFactory::instance();

	TreePath path(pathBase);
	if(root->parent() && root->parent()->editType() == TreeNode::VECTOR)
		path.push_back(TreePathLeaf(0, index));
	else
		path.push_back(TreePathLeaf(root->name(), index));

	TreeNode::const_iterator nit;
	bool found = false;
	const char* referencedLibrary = factory.findReferencedLibrary(root->type());
	if(referencedLibrary[0] && strcmp(referencedLibrary, libraryName) == 0){
		//result.push_back(path);

		TreeEditor* editor = factory.create(root->type());
		if(editor){
			LibraryBookmark bookmark;
			editor->onChange(*root);
			if(editor->getLibraryBookmark(&bookmark)){
				if(strcmp(bookmark.elementName(), elementName) == 0){
					result.push_back(path);
					found = true;
				}
			}
			delete editor;
		}
	}
	if(!found){
		TreeNode::const_iterator it;
		int i = 0;
		FOR_EACH(*root, it){
			treeNodeFindElementReferences(result, *it, libraryName, elementName, i++, path);
		}
	}

}

static void treeNodeFindTreeNode(TreePathes& result, const TreeNode* const root,
							   const TreeNode& node, bool sameName, int index = 0, const TreePath& pathBase = TreePath())
{
	if(root->hidden())
		return;

	TreePath path(pathBase);
	if(root->parent() && root->parent()->editType() == TreeNode::VECTOR)
		path.push_back(TreePathLeaf(0, index));
	else
		path.push_back(TreePathLeaf(root->name(), index));

	TreeNode::const_iterator nit;
	bool found = false;
	FOR_EACH(node, nit){
		if(compareTreeNodes(*root, **nit)){
			result.push_back(path);
			found = true;
			break;
		}
	}
	if(!found){
		TreeNode::const_iterator it;
		int i = 0;
		FOR_EACH(*root, it)
		treeNodeFindTreeNode(result, *it, node, sameName, i++, path);
	}
}

typedef std::set<std::string> LibraryElements;

static void treeNodeFindLibraryReferences(LibraryElements& result, const TreeNode* root, const char* libraryName, int index = 0)
{
	TreeEditorFactory& factory = TreeEditorFactory::instance();

	TreeNode::const_iterator nit;
	bool found = false;
	const char* referencedLibrary = factory.findReferencedLibrary(root->type());
	if(referencedLibrary[0] && strcmp(referencedLibrary, libraryName) == 0){
		TreeEditor* editor = factory.create(root->type());
		if(editor){
			LibraryBookmark bookmark;
			editor->onChange(*root);
			if(editor->getLibraryBookmark(&bookmark)){
				result.insert(bookmark.elementName());
			}
			delete editor;
		}
	}
	if(!found){
		TreeNode::const_iterator it;
		int i = 0;
		FOR_EACH(*root, it){
			if(!(*it)->hidden())
				treeNodeFindLibraryReferences(result, *it, libraryName, i++);
		}
	}

}

static void comboStringsFromTreePath(XBuffer& buf, ComboStrings& comboStrings, const TreePath& path)
{
	TreePath::const_iterator cit;
	FOR_EACH(path, cit){
		if(cit != path.begin()){
			if(cit->name){
				buf < "/" < cit->name;
				comboStrings.push_back(cit->name);
			}
			else{
				XBuffer buf2(12, 1);
				buf2 <= cit->index;
				buf < "/" < buf2;
				comboStrings.push_back(static_cast<const char*>(buf2));
			}
		}
	}
};

void LibraryTabSearch::onSelect()
{
	window_->tree().clear();
	SearchResultItems& items = this->items();
	SearchResultItems::iterator it;

	int index = 0;
	FOR_EACH(items, it){
		LibraryBookmark& bookmark = *it;

		XBuffer buf(256, 1);

		buf < bookmark.libraryName() < ", " < bookmark.elementName() < ": ";
		ComboStrings::const_iterator it;
		FOR_EACH(bookmark.attribEditorPath(), it){
			buf < "/";
			buf < it->c_str();
		}
		TreeObject* object = window_->tree().rootObject()->add(new LibraryBookmarkTreeObject(window_, bookmark, buf, index));
		if(index == selectedIndex_)
			object->focus();
		++index;
	}
}

void LibraryTabSearch::onMenuDeleteAll()
{
	SearchResultItems::iterator it;
	FOR_EACH(searchResultItems_, it){
		LibraryBookmark& bookmark = *it;
		EditorLibraryInterface* library = LibrariesManager::instance().find(bookmark.libraryName());
		if(int index = library->editorFindElement(bookmark.elementName()))
			library->editorElementErase(index);
	}
	searchResultItems_.clear();
    onSelect();
}

void LibraryTabSearch::onMenuConstruction(PopupMenuItem& root)
{
	root.add(TRANSLATE("Сохранить список как текст..."))
		.connect(bindMethod(*this, &LibraryTabSearch::onMenuSaveAsText));
	root.addSeparator();
	root.add(TRANSLATE("Удалить все найденные элементы"))
		.connect(bindMethod(*this, &LibraryTabSearch::onMenuDeleteAll));
}

void LibraryTabSearch::onElementSelected(const char* elementName, Serializeable& serializeable)
{
	selectedIndex_ = atoi(elementName);
	bookmark_.setElementName(elementName);
}

static void libraryFindTreeNode(EditorLibraryInterface& library, SearchResultItems& items, const TreeNode& tree, int& index, int totalCount, SearchProgressCallback progressCallback)
{
	int count = library.editorSize();
	for(int i = 0; i < count; ++i){
		Serializeable element = library.editorElementSerializeable(i, "", "", true);
		EditOArchive oa;
		element.serialize(oa);

		TreePathes pathes;
		if(!oa.rootNode()->empty()){
			treeNodeFindTreeNode(pathes, oa.rootNode(), tree, false);

			TreePathes::iterator pit;
			FOR_EACH(pathes, pit){
				XBuffer buf(256, 1);
				buf < library.editorElementName(i);
				buf < ": ";


				ComboStrings path;
				comboStringsFromTreePath(buf, path, *pit);

				LibraryBookmark bookmark(library.name(), library.editorElementName(i), path);
				items.push_back(bookmark);
			}
			float percent = float(index) / float(totalCount);
			progressCallback(percent);
		}
		++index;
	}
}

static void libraryFindLibraryReferences(EditorLibraryInterface& library, LibraryElements& elements, const char* libraryName, int& index, int totalCount, SearchProgressCallback progressCallback)
{	
	int count = library.editorSize();

	for(int i = 0; i < count; ++i){
		Serializeable element = library.editorElementSerializeable(i, "", "", true);
		EditOArchive oa;
		element.serialize(oa);

		if(!oa.rootNode()->empty()){
			treeNodeFindLibraryReferences(elements, oa.rootNode(), libraryName);
			float percent = float(index) / float(totalCount);
			progressCallback(percent);
		}
		++index;
	}
}

static void libraryFindElementReferences(EditorLibraryInterface& library, SearchResultItems& items, const char* libraryName, const char* elementName, int& index, int totalCount, SearchProgressCallback progressCallback)
{
	int count = library.editorSize();
	for(int i = 0; i < count; ++i){
		Serializeable element = library.editorElementSerializeable(i, "", "", true);
		EditOArchive oa;
		element.serialize(oa);

		TreePathes pathes;
		if(!oa.rootNode()->empty()){
			treeNodeFindElementReferences(pathes, oa.rootNode(), libraryName, elementName);

			TreePathes::iterator pit;
			FOR_EACH(pathes, pit){
				XBuffer buf(256, 1);
				buf < library.editorElementName(i);
				buf < ": ";

				ComboStrings path;
				comboStringsFromTreePath(buf, path, *pit);

				LibraryBookmark bookmark(library.name(), library.editorElementName(i), path);
				items.push_back(bookmark);
			}
			float percent = float(index) / float(totalCount);
			progressCallback(percent);
		}
		++index;
	}
}

// ---------------------------------------------------------------------------

void LibraryTabSearch::findUnused(const char* libraryName, SearchProgressCallback progress)
{
	EditorLibraryInterface* library = LibrariesManager::instance().find(libraryName);
	if(!library)
		return;

	int totalCount = 0;
	items().clear();

	LibrariesManager::Libraries& libraries = LibrariesManager::instance().libraries();
	LibrariesManager::Libraries::iterator it;

	FOR_EACH(libraries, it){
		EditorLibraryInterface& library = it->second();
		totalCount += library.editorSize();
	}

	LibraryElements usedElements;

	int index = 0;
	FOR_EACH(libraries, it){
		EditorLibraryInterface& library = it->second();
		libraryFindLibraryReferences(library, usedElements, libraryName, index, totalCount, progress);
	}

	int count = library->editorSize();
	for(int i = 0; i < count; ++i){
		const char* name = library->editorElementName(i);
		LibraryElements::iterator it = usedElements.find(name);
		if(it == usedElements.end()){
			XBuffer buf(256, 1);
			buf < library->editorElementName(i);
			buf < ": ";

			LibraryBookmark bookmark(libraryName, name, ComboStrings());
			items().push_back(bookmark);
		}
	}
	if(progress)
		progress(0.0f);
}

void LibraryTabSearch::findLibraryReference(const char* libraryName, const char* elementName, SearchProgressCallback progress)
{
	int totalCount = 0;
	items().clear();

	LibrariesManager::Libraries& libraries = LibrariesManager::instance().libraries();
	LibrariesManager::Libraries::iterator it;

	FOR_EACH(libraries, it){
		if(it->second){
			EditorLibraryInterface& library = it->second();
			totalCount += library.editorSize();
		}
	}

	int index = 0;
	FOR_EACH(libraries, it){
		if(it->second){
			EditorLibraryInterface& library = it->second();
			libraryFindElementReferences(library, items(), libraryName, elementName, index, totalCount, progress);
		}
	}
	if(progress)
		progress(0.0f);
}

void LibraryTabSearch::findTreeNode(const TreeNode& tree, bool sameName, SearchProgressCallback progress)
{
	int totalCount = 0;
	items().clear();

	LibrariesManager::Libraries& libraries = LibrariesManager::instance().libraries();
	LibrariesManager::Libraries::iterator it;

	FOR_EACH(libraries, it){
		EditorLibraryInterface& library = it->second();
		totalCount += library.editorSize();
	}

	int index = 0;
	FOR_EACH(libraries, it){
		EditorLibraryInterface& library = it->second();
		libraryFindTreeNode(library, items(), tree, index, totalCount, progress);
	}
	if(progress)
		progress(0.0f);
}

void LibraryTabSearch::serialize(Archive& ar)
{
	ar.serialize(searchResultItems_, "searchResultItems", 0);
}

LibraryTabSearch::LibraryTabSearch(CLibraryEditorWindow* window, const LibraryBookmark& bookmark)
: LibraryTabBase(window, bookmark)
, selectedIndex_(-1)
{
	setBookmark(bookmark);
	title_ = "Результаты поиска";
	XBuffer buf;
	buf < "Search_" <= reinterpret_cast<unsigned int>(this);
	bookmark_.setLibraryName(buf);
}

void LibraryTabSearch::setBookmark(const LibraryBookmark& bookmark)
{
	selectedIndex_ = atoi(bookmark.elementName());
	LibraryTabBase::setBookmark(bookmark);
}

void LibraryTabSearch::onMenuSaveAsText()
{
	string result;
	const char* filter = "(*.txt)|*.txt||";
	CFileDialog dlg(FALSE, filter, "search_results.txt", OFN_LONGNAMES|OFN_HIDEREADONLY|OFN_NOCHANGEDIR, filter);
	dlg.m_ofn.lpstrTitle = TRANSLATE("Сохранить результаты поиска...");
	dlg.m_ofn.lpstrInitialDir = ".";
	int response = dlg.DoModal();
	if(response == IDOK){
		std::string fileName = dlg.GetPathName();
		XStream file(fileName.c_str(), XS_OUT);

		SearchResultItems::iterator it;
		FOR_EACH(items(), it){
			LibraryBookmark& bookmark = *it;
			file.write(bookmark.elementName(), strlen(bookmark.elementName()));
			file.write("\n");
		}
		file.close();
	}
	else{
	}
}

// ----------------------------------------------------------------------------

void LibraryBookmark::serialize(Archive& ar)
{
	ar.serialize(libraryName_, "libraryName", 0);
	ar.serialize(elementName_, "elementName", 0);
	ar.serialize(subElementName_, "elementName", 0);
	ar.serialize(attribEditorPath_, "attribEditorPath", 0);
}


// ----------------------------------------------------------------------------

LibraryCustomEditor::LibraryCustomEditor(EditorLibraryInterface* library)
: library_(library)
{

}

LibraryElementTreeObject* LibraryCustomEditor::createTreeObject(const char* elementName)
{
	xassert(library_);
	return new LibraryElementTreeObject(this, elementName);
}


LibraryGroupTreeObject* LibraryCustomEditor::createGroupTreeObject(const char* groupName)
{
	xassert(library_);
	return new LibraryGroupTreeObject(this, groupName);
}


Serializeable LibraryCustomEditor::serializeableByBookmark(const LibraryBookmark& bookmark, bool editOnly)
{
	xassert(library_);
	return library_->editorElementSerializeable(bookmark.elementName(), "element", "element", editOnly);
}

LibraryCustomEditor::~LibraryCustomEditor()
{

}
