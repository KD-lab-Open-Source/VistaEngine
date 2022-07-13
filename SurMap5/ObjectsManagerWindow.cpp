#include "StdAfx.h"
#include "SurMap5.h"

#include "SurToolUnit.h"
#include "SurToolSource.h"
#include "SurToolAnchor.h"
#include "SurTool3DM.h"
#include "SurToolCameraEditor.h"

#include ".\ObjectsManagerWindow.h"
#include "ObjectsManagerTree.h"

#include "ToolsTreeWindow.h"
#include "MainFrame.h"

#include "..\AttribEditor\AttribEditorCtrl.h"
#include "..\Util\MFC\SizeLayoutManager.h"
#include "..\Environment\Environment.h"
#include "..\Environment\Anchor.h"

#include "..\Game\Universe.h"
#include "..\Units\Nature.h"
#include "..\Units\EnvironmentSimple.h"

#include "..\Game\CameraManager.h"
#include "NameComboDlg.h"

#include <algorithm>

#include "SelectionUtil.h"

#include "EditArchive.h"
#include "FileUtils.h"
#include "mfc\ObjectsTreeCtrl.h"
#include "mfc\PopupMenu.h"

typedef CObjectsTreeCtrl::ItemType ItemType;

//////////////////////////////////////////////////////////////////////////////

class WorldTreeObject : public TreeObject{/*{{{*/
public:
    virtual void kill() = 0;
    virtual bool isAlive() const = 0;
	virtual bool worldObjectSelected() const = 0;
    virtual void select() = 0;
	virtual void deselect() = 0;
	virtual Vect3f location() const = 0;
	virtual bool isSimilar(const WorldTreeObject& object) const { return false; };
	virtual void onMenuConstruction(PopupMenuItem& root); 
	virtual void getActions(UniverseObjectActionList& actions) {};
	void onRightClick();
	bool onDoubleClick();

	void onMenuLocate();
};/*}}}*/

static int playerIndex(Player* player)
{
	PlayerVect& players = universe()->Players;
	PlayerVect::iterator it = std::find(players.begin(), players.end(), player);
	if(it != players.end())
		return std::distance(players.begin(), it);
	else
		return -1;
}

namespace UniverseObjectActions{
	class EnvironmentApplyPreset : public UniverseObjectAction{
		bool canBeUsedOn(BaseUniverseObject& object) const{
			return object.objectClass() == UNIVERSE_OBJECT_ENVIRONMENT;
		}
		virtual const char* description() const{
			return TRANSLATE("Применить пресет");
		}
		void operator()(BaseUniverseObject& object){
			UnitEnvironment& unit = safe_cast_ref<UnitEnvironment&>(object);
			CSurToolEnvironment::loadPreset(&unit);
		}
	};
	REGISTER_CLASS_IN_FACTORY(Factory, typeid(EnvironmentApplyPreset).name(), EnvironmentApplyPreset);

	class EnvironmentSavePreset : public UniverseObjectAction{
		bool canBeUsedOn(BaseUniverseObject& object) const{
			return object.objectClass() == UNIVERSE_OBJECT_ENVIRONMENT;
		}
		virtual const char* description() const{
			return TRANSLATE("Сохранить как пресет по умолчанию");
		}
		void operator()(BaseUniverseObject& object){
			UnitEnvironment& unit = safe_cast_ref<UnitEnvironment&>(object);
			CSurToolEnvironment::savePreset(&unit);
		}
	};
	REGISTER_CLASS_IN_FACTORY(Factory, typeid(EnvironmentSavePreset).name(), EnvironmentSavePreset);
};

class UnitTreeObject  : public WorldTreeObject {/*{{{*/
public:
    UnitTreeObject(UnitBase& unit, const char* name)
    : unit_(unit)
    {
		XBuffer label;
		label < name;
		label < " - ";
		if(unit.player()->isWorld())
			label < "W";
		else
			label <= playerIndex(unit.player());
		name_ = label;
    }
    bool isAlive() const {
        return unit_.alive();
    }
	void kill() { unit_.Kill(); }
    void select() { unit_.setSelected(true); }
    void deselect() { unit_.setSelected(false); }
	Vect3f location() const{
		return unit_.position();
	}
	bool worldObjectSelected() const{ return unit_.selected(); }
	Serializeable getSerializeable(const char* name = "", const char* nameAlt = "") {
		return Serializeable(unit_, name, nameAlt);
	}

	const std::type_info& getTypeInfo () const {
		return typeid(unit_);
	}
    void* getPointer() const{ return reinterpret_cast<void*>(&unit_); }

	bool isSimilar(const WorldTreeObject& object) const{
		if(const UnitTreeObject* unitObject = dynamic_cast<const UnitTreeObject*>(&object)){
			if(&unit_.attr() == &unitObject->unit_.attr()){
				if(unit_.attr().isEnvironmentSimple() || unit_.attr().isEnvironmentBuilding()){
					const UnitEnvironment& unit1 = safe_cast_ref<const UnitEnvironment&>(unit_);
					const UnitEnvironment& unit2 = safe_cast_ref<const UnitEnvironment&>(unitObject->unit_);
					const char* model1 = unit1.modelName();
					const char* model2 = unit2.modelName();
					return unit1.environmentType() == unit2.environmentType() && ::compareFileName(model1, model2);
				}
				else
					return true;
			}
		}
		return false;
	};

	void onMenuSavePreset()
	{
		UnitEnvironment& unit = safe_cast_ref<UnitEnvironment&>(unit_);
		CSurToolEnvironment::savePreset(&unit);
	}

	void getActions(UniverseObjectActionList& actions)
	{
		__super::getActions(actions);
		actions.add<UniverseObjectActions::EnvironmentApplyPreset>();
		actions.add<UniverseObjectActions::EnvironmentSavePreset>();
	}
	
	void onMenuConstruction(PopupMenuItem& root)
	{
		__super::onMenuConstruction(root);
		if(unit_.attr().isEnvironment()){
			if(!root.empty())
				root.addSeparator();
			root.add(TRANSLATE("Применить пресет"))
				.connect(bindArgument(&CSurToolEnvironment::loadPreset, safe_cast<UnitEnvironment*>(&unit_)));
			root.add(TRANSLATE("Сохранить как пресет по умолчанию"))
				.connect(bindMethod(*this, &UnitTreeObject::onMenuSavePreset));
		}
	}

	UnitBase& unit() { return unit_; }
private:
    UnitBase& unit_;
};/*}}}*/

class SourceTreeObject  : public WorldTreeObject {/*{{{*/
public:
    SourceTreeObject(SourceBase& source, const char* name)
    : source_(source)
    {
		name_ = name;
    }
    bool isAlive() const{
        return source_.isAlive();
    }
	void kill(){ source_.kill(); }
    void select(){ source_.setSelected(true); }
    void deselect(){ source_.setSelected(false); }
	bool worldObjectSelected() const{ return source_.selected(); }
	Serializeable getSerializeable(const char* name = "", const char* nameAlt = ""){
		return Serializeable(source_, name, nameAlt);
	}
	const std::type_info& getTypeInfo () const{
		return typeid(source_);
	}
    void* getPointer()const{
		return reinterpret_cast<void*>(&source_);
	}
	SourceBase* source(){ 
		return &source_;
	}

	bool isSimilar(const WorldTreeObject& object) const{
		if(const SourceTreeObject* castedObject = dynamic_cast<const SourceTreeObject*>(&object)){
			if(source_.type() == castedObject->source_.type())
				return true;
		}
		return false;
	}

	Vect3f location() const{
		return source_.position();
	}
private:
    SourceBase& source_;
};/*}}}*/

class AnchorTreeObject  : public WorldTreeObject {/*{{{*/
public:
    AnchorTreeObject(Anchor& anchor, const char* name)
    : anchor_(anchor)
    {
        killed_ = false;
		name_ = name;
    }
    bool isAlive() const{ return !killed_; }
	void kill(){
        xassert(environment);
		Environment::Anchors::iterator it = std::find(environment->anchors().begin(),environment->anchors().end(), &anchor_);
		xassert(it != environment->anchors().end());
		environment->anchors().erase(it);
		killed_ = true;
    }
    void select(){
        xassert(!killed_);
		anchor_.setSelected(true);
    }
    void deselect(){
        xassert(!killed_);
		anchor_.setSelected(false);
    }
	bool worldObjectSelected() const{
		return !killed_ && anchor_.selected();
	}
	Serializeable getSerializeable(const char* name = "", const char* nameAlt = ""){
        xassert(!killed_);
		return Serializeable(anchor_, name, nameAlt);
	}
	const std::type_info& getTypeInfo () const{
        xassert(!killed_);
		return typeid(anchor_);
	}
    void* getPointer()const{
		return reinterpret_cast<void*>(&anchor_);
	}
	Anchor* source(){ 
		return &anchor_;
	}
	Vect3f location() const{
		return anchor_.position();
	}
private:
    bool killed_;
    Anchor& anchor_;
};/*}}}*/

namespace UniverseObjectActions{
	class CameraSplinePlay : public UniverseObjectAction{
		bool canBeUsedOn(BaseUniverseObject& object) const{
			return object.objectClass() == UNIVERSE_OBJECT_CAMERA_SPLINE;
		}
		virtual const char* description() const{
			return TRANSLATE("Проиграть сплайн");
		}
		void operator()(BaseUniverseObject& object){
			CSurToolCameraEditor* editor = new CSurToolCameraEditor(&object, true);
			mainFrame().getToolWindow().pushEditorMode(editor);
		}
	};
	REGISTER_CLASS_IN_FACTORY(Factory, typeid(CameraSplinePlay).name(), CameraSplinePlay);
};

class CameraSplineTreeObject  : public WorldTreeObject {/*{{{*/
public:
    CameraSplineTreeObject(CameraSpline& spline, const char* name)
    : spline_(spline)
	, killed_(false)
    {
		name_ = name;
    }
    bool isAlive() const { return true; }
	void kill() {
		cameraManager->deleteSpline(&spline_);
		killed_ = true;
    }
    void select() {
		xassert(!killed_);
		spline_.setSelected(true);
    }
    void deselect() {
		xassert(!killed_);
		spline_.setSelected(false);
    }
	bool worldObjectSelected() const{ 
		xassert(!killed_);
		return spline_.selected();
	}
	Serializeable getSerializeable(const char* name = "", const char* nameAlt = "") {
		xassert(!killed_);
		return Serializeable(spline_, name, nameAlt);
	}
	const std::type_info& getTypeInfo () const {
		return typeid(CameraSpline);
	}
    void* getPointer() const{ return reinterpret_cast<void*>(&spline_); }
	Vect3f location() const{
		xassert(!killed_);
		return spline_.position();
	}

	void onMenuPlay()
	{
		CToolsTreeWindow& toolWindow = ((CMainFrame*)AfxGetMainWnd())->getToolWindow();

		CSurToolCameraEditor* editor = new CSurToolCameraEditor(&spline_, true);
		toolWindow.pushEditorMode(editor);
	}

	void getActions(UniverseObjectActionList& actions)
	{
		__super::getActions(actions);
		actions.add<UniverseObjectActions::CameraSplinePlay>();
	}

    void onMenuLocateLastPoint()
	{
		if(universe() && environment && !spline_.empty()){
			CameraCoordinate coord = spline_.spline().back();

			coord.position() += spline_.position();
			cameraManager->setCoordinate(coord);
		}
	}

	void onMenuConstruction(PopupMenuItem& root)
	{
		__super::onMenuConstruction(root);
		if(!spline_.empty()){
			if(!root.empty())
				root.addSeparator();
			root.add(TRANSLATE("Найти последнюю точку сплайна"))
				.connect(bindMethod(*this, &CameraSplineTreeObject::onMenuLocateLastPoint));
			root.add(TRANSLATE("Проиграть сплайн"))
				.connect(bindMethod(*this, &CameraSplineTreeObject::onMenuPlay));
		}
	}
private:
    CameraSpline& spline_;
	bool killed_;
};/*}}}*/

//////////////////////////////////////////////////////////////////////////////
void WorldTreeObject::onMenuConstruction(PopupMenuItem& root)
{
	root.add(TRANSLATE("Найти на карте"))
	.connect(bindMethod<void>(*this, &WorldTreeObject::onMenuLocate));
	root.add(TRANSLATE("Выделить похожие"))
	.connect(bindMethod<bool>(*this, &WorldTreeObject::onDoubleClick))
	.setDefault(true);
}; 
void WorldTreeObject::onMenuLocate()
{
	if(!universe() || !environment){
		xassert(0);
		return;
	}

	CameraCoordinate coord = cameraManager->coordinate();
	coord.setPosition(location());
	cameraManager->setCoordinate(coord);
}

void WorldTreeObject::onRightClick()
{
}

bool WorldTreeObject::onDoubleClick()
{
	CObjectsManagerWindow* window = safe_cast<CObjectsManagerWindow*>(tree_->GetParent());
	CObjectsManagerWindow::UpdateLock lock(*window);

	TreeObject::iterator it;
	FOR_EACH(*tree_->rootObject(), it){
		WorldTreeObject& treeObject = safe_cast_ref<WorldTreeObject&>(**it);

		if(isSimilar(treeObject))
			tree_->selectObject(&treeObject);
	}
	window->selectOnWorld();
	return true;
}
//////////////////////////////////////////////////////////////////////////////

BOOL CObjectsManagerWindow::Create(DWORD style, const CRect& rect, CWnd* parentWnd)
{
	return CWnd::Create(className(), 0, style, rect, parentWnd, 0);
}

CObjectsManagerWindow::CObjectsManagerWindow(CMainFrame* mainFrame)
: sortByName_(true)
, skipSelChanged_(false)
, popupMenu_(new PopupMenu(32))
, tree_(new CObjectsManagerTree(this))
{
	WNDCLASS wndclass;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if(!::GetClassInfo (hInst, className(), &wndclass)){
        wndclass.style			= CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW|WS_TABSTOP;
		wndclass.lpfnWndProc	= ::DefWindowProc;
		wndclass.cbClsExtra		= 0;
		wndclass.cbWndExtra		= 0;
		wndclass.hInstance		= hInst;
		wndclass.hIcon			= NULL;
		wndclass.hCursor		= AfxGetApp ()->LoadStandardCursor (IDC_ARROW);
		wndclass.hbrBackground	= reinterpret_cast<HBRUSH> (COLOR_WINDOW);
		wndclass.lpszMenuName	= NULL;
		wndclass.lpszClassName	= className();

		if (!AfxRegisterClass (&wndclass))
			AfxThrowResourceException ();
	}
}

CObjectsManagerWindow::~CObjectsManagerWindow()
{
}

BEGIN_MESSAGE_MAP(CObjectsManagerWindow, CWnd)
	ON_NOTIFY(TLN_BEFORESELCHANGED, 0, OnObjectsTreeBeforeSelChanged)
	ON_NOTIFY(TLN_AFTERSELCHANGED, 0, OnObjectsTreeAfterSelChanged)

	ON_NOTIFY(TCN_SELCHANGE,  0, OnTabChanged)
	ON_NOTIFY(TLN_KEYDOWN,    0, OnObjectsTreeKeyDown)

	ON_NOTIFY(NM_CLICK, 0, OnObjectsTreeClick)
	ON_NOTIFY(NM_RCLICK, 0, OnObjectsTreeRClick)
	
	ON_NOTIFY(TLN_BEGINLABELEDIT, 0, OnObjectsTreeBeginLabelEdit)

	ON_WM_SHOWWINDOW()
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()


int CObjectsManagerWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	DWORD treeStyle = 
		TLC_TREELIST		                    // TreeList or Listn
		| TLC_MULTIPLESELECT                    // single or multiple select
		| TLC_SHOWSELACTIVE	                    // show active column of selected item
		| TLC_SHOWSELALWAYS	                    // show selected item always
		| TLC_SHOWSELFULLROWS                   // show selected item in fullrow mode
//		| TLC_HEADER		                    // show header
		| TLC_READONLY
		;

    CRect treeRect;
    GetClientRect(&treeRect);
	treeRect.top = 24;
    VERIFY(tree_->Create(WS_VISIBLE | WS_CHILD | WS_BORDER, treeRect, this, 0));
	tree_->SetFont(CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT)));
	tree_->SetStyle(treeStyle);

	CRect rect;
	tree_->GetClientRect(&rect);
	tree_->InsertColumn(TRANSLATE("Объекты"), TLF_DEFAULT_LEFT,
		rect.Width () - GetSystemMetrics (SM_CXVSCROLL) - GetSystemMetrics (SM_CXBORDER) * 2);
	tree_->SetColumnModify(0, TLM_REQUEST);

	VERIFY(typeTabs_.Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, treeRect.right, treeRect.top), this, 0));

	typeTabs_.SetFont(CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT)));
	typeTabs_.InsertItem (TAB_SOURCES,		 TRANSLATE("Источники"));
	typeTabs_.InsertItem (TAB_ENVIRONMENT,   TRANSLATE("Окружение"));
	typeTabs_.InsertItem (TAB_UNITS,         TRANSLATE("Юниты"));
	typeTabs_.InsertItem (TAB_CAMERA,        TRANSLATE("Камеры"));
	typeTabs_.InsertItem (TAB_ANCHORS,       TRANSLATE("Якори"));
	typeTabs_.SetCurSel (TAB_SOURCES);

	if(!layout_.isInitialized()){
		layout_.init(this);
		layout_.add(true, true, true, false, &typeTabs_);
		layout_.add(true, true, true, true, tree_);
	}

	LRESULT result;
	OnTabChanged(0, &result);

	eventMaster().eventWorldChanged().registerListener(this);
	eventMaster().eventObjectChanged().registerListener(this);
	eventMaster().eventSelectionChanged().registerListener(this);
	return 0;
}

void CObjectsManagerWindow::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	layout_.onSize(cx, cy);

	CRect treeRect;
	tree_->GetWindowRect(&treeRect);
	tree_->SetColumnWidth(0, treeRect.Width() - GetSystemMetrics(SM_CXVSCROLL) - GetSystemMetrics(SM_CXBORDER) * 2);
}


namespace{

template<class T>bool isVisibleInObjectsManager(T& obj){ return true; }
bool isVisibleInObjectsManager(ShareHandle<SourceBase> source){
	return source != CSurToolSource::sourceOnMouse() && source->isAlive();
}
bool isVisibleInObjectsManager(ShareHandle<Anchor> source){
	return source != CSurToolAnchor::anchorOnMouse();
}

template<class T>
const char* getObjectLabel(T& obj) { return ""; }
const char* getObjectLabel(SourceBase* source){ return source->label(); }
const char* getObjectLabel(ShareHandle<CameraSpline>& spline){ return spline->name(); }
const char* getObjectLabel(ShareHandle<Anchor>& anchor){ return anchor->c_str(); }

template<class T>
WorldTreeObject* makeTreeObject(T& obj){ return 0; }
WorldTreeObject* makeTreeObject(SourceBase* source, const char* name) { return new SourceTreeObject(*source, name); }
WorldTreeObject* makeTreeObject(ShareHandle<CameraSpline>& spline, const char* name) { return new CameraSplineTreeObject(*spline, name); }
WorldTreeObject* makeTreeObject(ShareHandle<Anchor>& anchor, const char* name) { return new AnchorTreeObject(*anchor, name); }

template<class Container>
void addObjectsToTree(std::vector<WorldTreeObject*>& dest, Container& _container, const char* _namePrefix) {
    Container::iterator it;
    int index = 0;
    FOR_EACH(_container, it){
        CString str;
		if(isVisibleInObjectsManager(*it)){
			if(strlen(getObjectLabel(*it)) == 0)
				str.Format("%s #%i", _namePrefix, index++);
			else
				str.Format("%s #%i - %s", _namePrefix, index++, getObjectLabel(*it));
			dest.push_back(makeTreeObject(*it, str));
		}
    }
}

}

bool compareTreeObjects(const WorldTreeObject* lhs, const WorldTreeObject* rhs)
{
    return strcmp(lhs->name(), rhs->name()) < 0;
}

void CObjectsManagerWindow::rebuildList()/*{{{*/
{
    UpdateLock lock(*this);

	std::vector<WorldTreeObject*> treeObjects;
	int selection = typeTabs_.GetCurSel ();

	Serializeable holder;

	tree_->clear ();
	//attribEditor_.detachData ();

	if (selection == TAB_SOURCES && environment) {
		environment->flushNewSources();
		for(int isource=0; isource < SOURCE_MAX; isource++){
			SourceType type=(SourceType)isource;
			Environment::Sources cur;
			environment->getTypeSources(type,cur);
			addObjectsToTree(treeObjects, cur, SourceBase::getDisplayName(type).c_str());
		}
	}
	else if(selection == TAB_CAMERA){
		if(cameraManager)
			addObjectsToTree(treeObjects, cameraManager->splines(), TRANSLATE("Камера"));
	}
	else if(selection == TAB_ANCHORS){
		if(environment)
            addObjectsToTree(treeObjects, environment->anchors(), TRANSLATE("Якорь"));
	}
	else if(selection == TAB_ENVIRONMENT){
        if(universe()){
			int index = 0;
			Player* player = universe()->worldPlayer ();
			UnitList& units = const_cast<UnitList&>(player->units ());
			UnitList::iterator it;
			FOR_EACH (units, it) {
				UnitBase*& unit = (*it).unit();

				if(unit->auxiliary() || !unit->alive())
					continue;
				
				if(UnitEnvironment* unit_environment = dynamic_cast<UnitEnvironment*>(unit)){
					std::string modelName = unit_environment->modelName();
					std::string::size_type pos = modelName.rfind ("\\") + 1;
					std::string name (modelName.begin() + pos, modelName.end());
                    treeObjects.push_back(new UnitTreeObject(*unit, name.c_str()));
				}
				else if(UnitEnvironmentSimple* unit_environment = dynamic_cast<UnitEnvironmentSimple*>(unit)){
					std::string modelName = unit_environment->modelName();
					std::string::size_type pos = modelName.rfind ("\\") + 1;
					std::string name (modelName.begin() + pos, modelName.end());
                    treeObjects.push_back(new UnitTreeObject(*unit, name.c_str()));
				}
			}
        }
	}
	else if(selection == TAB_UNITS){
        if(universe()){
			PlayerVect::iterator it;
			FOR_EACH (universe ()->Players, it) {
				Player* player = *it;
				UnitList& units = const_cast<UnitList&> (player->units ());
				UnitList::iterator it;
				FOR_EACH (units, it) {
					UnitBase*& unit = (*it).unit ();
					
					if(!unit->attr().internal && !unit->auxiliary() && unit->alive()){
                        treeObjects.push_back(new UnitTreeObject(*unit, unit->attr().libraryKey()));
					}
				}
			}
        }
	}

    if(sortByName_)
        std::sort(treeObjects.begin(), treeObjects.end(), &compareTreeObjects);

	for(int i = 0; i < treeObjects.size(); ++i){
		WorldTreeObject* object = treeObjects[i];
		CObjectsTreeCtrl::ItemType item = tree_->addObject(object);
		if(object->worldObjectSelected())
			tree_->SelectItem(item, 0, CObjectsTreeCtrl::SI_SELECT, false);
	}
}/*}}}*/

void CObjectsManagerWindow::OnTabChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
    rebuildList();

	*pResult = 0;
}

void CObjectsManagerWindow::selectInList()
{
	UpdateLock lock(*this);

    
	TreeObject* root = tree_->rootObject();
	TreeObject::iterator it;
	
	FOR_EACH(*root, it){
	    WorldTreeObject* treeObject = safe_cast<WorldTreeObject*>(*it);
		bool worldObjectSelected = treeObject->worldObjectSelected();
		if(treeObject->selected() != worldObjectSelected){
			if(worldObjectSelected)
				treeObject->select();
			else
				treeObject->deselect();
			treeObject->ensureVisible();
		}
	}
}

void CObjectsManagerWindow::selectOnWorld()
{
	int tab = typeTabs_.GetCurSel ();
	
	::deselectAll();

	POSITION pos = tree_->GetFirstSelectedItemPosition ();
	while(ItemType current_item = tree_->GetNextSelectedItem (pos)) {
		WorldTreeObject& object = static_cast<WorldTreeObject&>(*tree_->objectByItem(current_item));
        object.select();
	}

	eventMaster().eventSelectionChanged().emit();
}

void CObjectsManagerWindow::OnObjectsTreeBeforeSelChanged(NMHDR* nm, LRESULT* result)
{
	NMTREELIST* nmTreeList = reinterpret_cast<NMTREELIST*>(nm);
    
    skipSelChanged_ = true;

    *result = 0;
}

void CObjectsManagerWindow::onObjectsTreeRightClick()
{
	CPoint cursorPosition;
	GetCursorPos(&cursorPosition);

	popupMenu_->clear();
	PopupMenuItem& root = popupMenu_->root();

	CPoint treePoint = cursorPosition;
	tree_->ScreenToClient(&treePoint);

	TreeObjects selection = tree_->selection();

	if(selection.size() == 1)
		safe_cast<WorldTreeObject*>(selection.front())->onMenuConstruction(root);
	else{

		if(currentTab() == TAB_ENVIRONMENT){
			if(!root.empty())
				root.addSeparator();
			root.add(TRANSLATE("Применить пресет"))
				.connect(bindMethod(*this, &Self::onMenuApplyPreset));
		}
	}
	if(currentTab() == TAB_UNITS){
		PopupMenuItem& menu = root.add(TRANSLATE("Сменить игрока"));
		PlayerVect& players = universe()->Players;
		PlayerVect::iterator it;
		int index = 0;
		FOR_EACH(players, it){
			Player* player = *it;
			XBuffer playerLabel;
			if(!player->isWorld())
				playerLabel <= index;
			else
				playerLabel < "W";
			playerLabel < " ";
			playerLabel < player->race().c_str();
			menu.add(playerLabel)
				.connect(bindArgument(bindMethod(*this, &Self::onMenuChangePlayer), player));
			++index;
		}
	}


	if(!root.empty())
		root.addSeparator();
	root.add(TRANSLATE("Сортировать по имени"))
		.connect(bindMethod<void>(*this, &Self::onMenuSortByName))
		.check(sortByName());
	root.add(TRANSLATE("Сортировать по времени создания"))
		.connect(bindMethod<void>(*this, &Self::onMenuSortByTime))
		.check(!sortByName());

	root.addSeparator();
	root.add(TRANSLATE("Вставить"))
		.connect(bindMethod<void>(*this, &Self::onMenuPaste));

    popupMenu_->spawn(cursorPosition, GetSafeHwnd());
}

void CObjectsManagerWindow::OnObjectsTreeAfterSelChanged(NMHDR* nm, LRESULT* result)
{
	NMTREELIST* nmTreeList = reinterpret_cast<NMTREELIST*>(nm);

    skipSelChanged_ = false;

	CToolsTreeWindow& toolsWindow = ((CMainFrame*)AfxGetMainWnd())->getToolWindow();
	
	if(toolsWindow.isToolFromTreeSelected())
        toolsWindow.replaceEditorMode(0);

    selectOnWorld();
	if(tree_->selection().size() == 1)
		tree_->selected()->ensureVisible();

    *result = 0;
}

void CObjectsManagerWindow::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CWnd::OnShowWindow(bShow, nStatus);

	if(IsWindow(typeTabs_.GetSafeHwnd())){
		LRESULT result;
		OnTabChanged(0, &result);
	}
}

void CObjectsManagerWindow::onSelectionChanged()
{
	selectInList();
}

void CObjectsManagerWindow::OnObjectsTreeKeyDown(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMTREELISTKEYDOWN* pNM = (NMTREELISTKEYDOWN*)(pNMHDR);
	TreeObjects selection = tree_->selection();
	if(pNM->wVKey == VK_DELETE){
        UpdateLock lock(*this);
		TreeObjects::iterator it;

		FOR_EACH(selection, it){
			WorldTreeObject& object = static_cast<WorldTreeObject&>(**it);

			if(object.worldObjectSelected()){
				object.kill();
				tree_->deleteObject(&object);
			}
		}
	}

	if(pNM->wVKey == 'V'){
		GetAsyncKeyState(VK_SHIFT);
		onMenuPaste();
	}
	*pResult = 0;
}

void CObjectsManagerWindow::OnObjectsTreeRClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	if(!universe() || !(environment))
		return;

	CPoint cursorPosition;
	GetCursorPos(&cursorPosition);

	if(pNMHDR->hwndFrom == tree_->GetSafeHwnd()){
        onObjectsTreeRightClick();
	}
	if(pNMHDR->hwndFrom == typeTabs_.GetSafeHwnd()){
		popupMenu_->clear();
		PopupMenuItem& root = popupMenu_->root();

		root.add(TRANSLATE("Выделить все в текущей вкладке"))
			.connect(bindMethod(*this, &Self::onMenuTabSelectAll));
		root.add(TRANSLATE("Снять выделение в текущей вкладке"))
			.connect(bindMethod(*this, &Self::onMenuTabDeselectAll));
		root.add(TRANSLATE("Снять выделение с других вкладок"))
			.connect(bindMethod(*this, &Self::onMenuTabDeselectOthers));


		popupMenu_->spawn(cursorPosition, GetSafeHwnd());
		//menu.GetSubMenu(1)->TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y, this);
		*pResult = 1;
	}
	*pResult = 0;
}

void CObjectsManagerWindow::OnObjectsTreeClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMTREELIST* notify = (NMTREELIST*)(pNMHDR);
	
	if(notify->pItem && !::isControlPressed() && !isShiftPressed()){
		deselectAll();
	}
	
	*pResult = 0;
}

void CObjectsManagerWindow::OnObjectsTreeBeginLabelEdit (NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 1;
}

void CObjectsManagerWindow::onMenuChangePlayer(Player* player)
{
	TreeObjects selection = tree_->selection();
	TreeObjects::iterator it;
	FOR_EACH(selection, it){
		UnitTreeObject* object = safe_cast<UnitTreeObject*>(*it);
		UnitBase& unit = object->unit();
		if(unit.player() != player)
			unit.changeUnitOwner(player);
	}
	rebuildList();
}

void CObjectsManagerWindow::onMenuSortByTime()
{
	sortByName_ = false;
	rebuildList();
}

void CObjectsManagerWindow::onMenuSortByName()
{
	sortByName_ = true;
	rebuildList();
}

void CObjectsManagerWindow::onMenuApplyPreset()
{
	TreeObjects selection = tree_->selection();
	TreeObjects::iterator it;

	FOR_EACH(selection, it){
		WorldTreeObject* object = static_cast<WorldTreeObject*>(*it);
		if(UnitTreeObject* unitObject = dynamic_cast<UnitTreeObject*>(object)){
			UnitBase& unitBase = unitObject->unit();
			if(unitBase.attr().isEnvironment()){
				UnitEnvironment& unit = safe_cast_ref<UnitEnvironment&>(unitBase);
				CSurToolEnvironment::loadPreset(&unit);
				//TreeNode
			}
		}
	}
}

void CObjectsManagerWindow::updateList()
{
	ItemType current_item = tree_->GetChildItem(TLI_ROOT);
	while(current_item){
		WorldTreeObject& object = static_cast<WorldTreeObject&>(*tree_->objectByItem(current_item));
		xassert(&object);
		ItemType last_item = current_item;
		
		current_item = tree_->GetNextSiblingItem(current_item);
		if(!object.isAlive())
			object.erase();
	}

	TreeObjects deadOnes;

	TreeObject::iterator it;
	FOR_EACH(*tree_->rootObject(), it){
		WorldTreeObject* object = static_cast<WorldTreeObject*>(*it);
		xassert(object);
		if(!object->isAlive())
			deadOnes.push_back(object);
	}

	TreeObjects::iterator oit;
	FOR_EACH(deadOnes, oit){
		tree_->deleteObject(*oit);
	}
}

CObjectsManagerWindow::UpdateLock::UpdateLock(CObjectsManagerWindow& objectsManager)
: objectsManager_(objectsManager)
{
    objectsManager_.skipSelChanged_ = true;
    objectsManager_.tree_->AllowRedraw(FALSE);
}

CObjectsManagerWindow::UpdateLock::~UpdateLock()
{
    objectsManager_.skipSelChanged_ = false;
    objectsManager_.tree_->AllowRedraw(TRUE);
    objectsManager_.tree_->Invalidate(FALSE);
}

void CObjectsManagerWindow::onMenuTabDeselectOthers()
{
	selectOnWorld();
}

void CObjectsManagerWindow::onMenuTabSelectAll()
{
	//UpdateLock lock(*this);
	ItemType current_item = tree_->GetChildItem(TLI_ROOT);
    while(current_item != 0){
        tree_->SelectItem(current_item, 0, CObjectsTreeCtrl::SI_SELECT, false);
        current_item = tree_->GetNextSiblingItem (current_item);
    }
	eventMaster().eventSelectionChanged().emit();
}

void CObjectsManagerWindow::onMenuTabDeselectAll()
{
	//UpdateLock lock(*this);
	ItemType current_item = tree_->GetChildItem(TLI_ROOT);
    while(current_item != 0){
        tree_->SelectItem(current_item, 0, CObjectsTreeCtrl::SI_DESELECT, false);
        current_item = tree_->GetNextSiblingItem (current_item);
    }
	eventMaster().eventSelectionChanged().emit();
}

void CObjectsManagerWindow::onWorldChanged()
{
	tree_->clear();
}

void CObjectsManagerWindow::onObjectChanged()
{
	LRESULT result;
	OnTabChanged (0, &result);
}

BOOL CObjectsManagerWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
	popupMenu_->onCommand(wParam, lParam);

	return CWnd::OnCommand(wParam, lParam);
}

void CObjectsManagerWindow::onMenuPaste()
{
	TreeObjects selection = tree_->selection();

	int counter = 0;

	TreeObjects::iterator it;
	FOR_EACH(selection, it){
		WorldTreeObject& object = static_cast<WorldTreeObject&>(**it);

		Serializeable serializeable = object.getSerializeable();

		xassert(::IsWindow(GetSafeHwnd()));
		int changed = TreeNodeClipboard::instance().smartPaste(serializeable, GetSafeHwnd());
		if(changed)
			++counter;
	}

	if(counter > 0){
		eventMaster().eventObjectChanged().emit();
		CString str;
		str.Format(TRANSLATE("Было изменено %i объекта(ов)"), counter);
		tree_->MessageBox(str, 0, MB_OK|MB_ICONINFORMATION);
	}
}

int CObjectsManagerWindow::currentTab()
{
	return typeTabs_.GetCurSel();
}
