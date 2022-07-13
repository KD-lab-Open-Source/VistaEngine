#include "StdAfx.h"
#include "ObjectsManagerTree.h"
#include "ObjectsManagerWindow.h"
#include "WorldTreeObjects.h"
#include "kdw/Clipboard.h"
#include "kdw/ObjectsTree.h"

#include "SurMap5\SelectionUtil.h"
#include "Units\EnvironmentSimple.h"

ObjectsManagerTree::ObjectsManagerTree()
: tab_(TAB_SOURCES)
, sortByName_(true)
{

}

void ObjectsManagerTree::setTab(ObjectsManagerTab tab)
{
	bool changed = tab_ != tab;
	tab_ = tab;
	if(changed)
		rebuild();
}

void ObjectsManagerTree::removeDead()
{
	kdw::TreeObjects deadOnes;

	kdw::TreeObject::iterator it;
	FOR_EACH(*root(), it){
		WorldTreeObject* object = safe_cast<WorldTreeObject*>(&**it);
		xassert(object);
		if(!object->isAlive())
			deadOnes.push_back(object);
	}

	kdw::TreeObjects::iterator oit;
	FOR_EACH(deadOnes, oit){
		(*oit)->remove();
	}
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
const char* getObjectLabel(ShareHandle<Anchor>& anchor){ return anchor->label(); }

template<class T>
WorldTreeObject* makeTreeObject(T& obj){ return 0; }
WorldTreeObject* makeTreeObject(SourceBase* source, const char* name) { return new SourceTreeObject(*source, name); }
WorldTreeObject* makeTreeObject(ShareHandle<CameraSpline>& spline, const char* name) { return new CameraSplineTreeObject(*spline, name); }
WorldTreeObject* makeTreeObject(ShareHandle<Anchor>& anchor, const char* name) { return new AnchorTreeObject(*anchor, name); }

template<class Container>
static void addObjectsToTree(std::vector<WorldTreeObject*>& dest, Container& _container, const char* _namePrefix) {
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

static bool compareTreeObjects(const WorldTreeObject* lhs, const WorldTreeObject* rhs)
{
	char str1[MAX_PATH];
	strcpy(str1, lhs->text());
	char str2[MAX_PATH];
	strcpy(str2, rhs->text());

    return strcmp(CharUpper(str1), CharUpper(str2)) < 0;
}

void ObjectsManagerTree::rebuild()
{
	std::vector<WorldTreeObject*> treeObjects;

	root()->clear();
	if(tab_ == TAB_SOURCES && sourceManager){
		sourceManager->flushNewSources();
		for(int isource=0; isource < SOURCE_MAX; isource++){
			SourceType type=(SourceType)isource;
			SourceManager::Sources cur;
			sourceManager->getTypeSources(type,cur);
			addObjectsToTree(treeObjects, cur, SourceBase::getDisplayName(type).c_str());
		}
	}
	else if(tab_ == TAB_CAMERA){
		if(cameraManager)
			addObjectsToTree(treeObjects, cameraManager->splines(), TRANSLATE("Камера"));
	}
	else if(tab_ == TAB_ANCHORS){
		if(sourceManager)
            addObjectsToTree(treeObjects, sourceManager->anchors(), TRANSLATE("Якорь"));
	}
	else if(tab_ == TAB_ENVIRONMENT){
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
	else if(tab_ == TAB_UNITS){
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

	WorldTreeObject* lastObject = 0;
	for(int i = 0; i < treeObjects.size(); ++i){
		WorldTreeObject* object = treeObjects[i];
		root()->add(object);
		if(object->worldObjectSelected()){
			selectObject(object, true, false, false);
			lastObject = object;
		}
	}
	if(lastObject && lastObject->selected())
		ensureVisible(lastObject);

	update();
	redraw();
}

bool ObjectsManagerTree::onRowKeyDown(kdw::TreeRow* row, sKey key)
{
	Model::Selection selection = model()->selection();
	if(key == sKey(VK_DELETE)){
		onMenuDelete();
		update();
		return true;
	}
	if(key == sKey('V')){
		onMenuPaste();
		return true;
	}
	return false;
}

void ObjectsManagerTree::onRowSelected(kdw::TreeRow* row)
{
	kdw::TreeObject* lastObject = 0;
	kdw::TreeObject* startObject = 0;
	kdw::TreeObject* rowObj = safe_cast<kdw::TreeObject*>(row);
	if(::isPressed(VK_SHIFT))
	{
		kdw::TreeObject::iterator it;
		FOR_EACH(*root(), it){
			WorldTreeObject& treeObject = safe_cast_ref<WorldTreeObject&>(**it);
			if(!startObject){
				if(treeObject.selected()){
					startObject = &treeObject;
					lastObject = rowObj;
					break;
				}
			}
			else
				if(treeObject.selected()){
					lastObject = &treeObject;
					break;
				}
			if(&treeObject == rowObj)
				startObject = &treeObject;
		}
	}

	bool deselect = false;
	if(!::isPressed(VK_CONTROL))
		::deselectAll();
	else{
		deselect = rowObj->selected();
		selectObject(rowObj, !deselect, true, false);
		update();
	}

	if(::isPressed(VK_SHIFT))
	{
		bool selectStart = false;
		kdw::TreeObject::iterator it;
		FOR_EACH(*root(), it){
			WorldTreeObject& treeObject = safe_cast_ref<WorldTreeObject&>(**it);

			if(!selectStart && &treeObject == startObject)
				selectStart = true;
			
			if(selectStart)
			{
				selectObject(&treeObject, true, false, false);
				if(&treeObject == lastObject){
					selectObject(lastObject, true, false, true);
					break;
				}
			}
		}
		update();
	}

	if(!deselect)
		__super::onRowSelected(row);
	else
		__super::onRowDeselected(row);

	selectOnWorld();
}

bool ObjectsManagerTree::onRowLMBDoubleClick(kdw::TreeRow* row, const Recti& rowRect, Vect2i point)
{
	::deselectAll();
	bool res = __super::onRowLMBDoubleClick(row, rowRect, point);
	selectOnWorld();
	return res;
}

void ObjectsManagerTree::onRowRMBDown(kdw::TreeRow* row, const Recti& rowRect, Vect2i point)
{
	kdw::PopupMenu menu(100);
	kdw::PopupMenuItem& root = menu.root();

	kdw::TreeModel::Selection selection = model()->selection();

	if(selection.empty())
		return;

	kdw::TreeRow* firstSelectedRow = model()->rowFromPath(selection.front());
	if(!firstSelectedRow)
		return;

	WorldTreeObject* worldObject = dynamic_cast<WorldTreeObject*>(firstSelectedRow);
	if(!worldObject)
		return;

	if(selection.size() == 1)
		worldObject->onMenuConstruction(root, this);
	if(UnitTreeObject* unitObject = dynamic_cast<UnitTreeObject*>(worldObject)){
		if(unitObject->isEnvironment()){
			if(selection.size() > 1){
				if(!root.empty())
					root.addSeparator();
				root.add(TRANSLATE("Применить пресет"))
					.connect(this, &ObjectsManagerTree::onMenuApplyPreset);
			}
		}
		else{
			kdw::PopupMenuItem& menu = root.add(TRANSLATE("Сменить игрока"));
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
				menu.add(playerLabel, player)
					.connect(this, &ObjectsManagerTree::onMenuChangePlayer);
				++index;
			}
		}
	}

	if(!root.empty())
		root.addSeparator();
	root.add(TRANSLATE("Сортировать по имени"))
		.connect(this, &ObjectsManagerTree::onMenuSortByName)
		.check(sortByName());
	root.add(TRANSLATE("Сортировать по времени создания"))
		.connect(this, &ObjectsManagerTree::onMenuSortByTime)
		.check(!sortByName());

	root.addSeparator();
	root.add(TRANSLATE("Вставить"))
		.connect(this, &ObjectsManagerTree::onMenuPaste)
		.setHotkey(sKey('V'));

	root.addSeparator();
	root.add(TRANSLATE("Удалить"))
		.connect(this, &ObjectsManagerTree::onMenuDelete)
		.setHotkey(sKey(VK_DELETE));

	menu.spawn(this);
}

void ObjectsManagerTree::onMenuChangePlayer(Player* player)
{
	Model::Selection selection = model()->selection();
	Model::Selection::iterator it;
	FOR_EACH(selection, it){
		UnitTreeObject* object = safe_cast<UnitTreeObject*>(model()->rowFromPath(*it));
		UnitBase& unit = object->unit();
		if(unit.player() != player)
			unit.changeUnitOwner(player);
	}
	rebuild();
}

void ObjectsManagerTree::onMenuSortByTime()
{
	sortByName_ = false;
	rebuild();
}

void ObjectsManagerTree::onMenuSortByName()
{
	sortByName_ = true;
	rebuild();
}

void ObjectsManagerTree::onMenuApplyPreset()
{
	Model::Selection selection = model()->selection();
	Model::Selection::iterator it;

	FOR_EACH(selection, it){
		WorldTreeObject* object = static_cast<WorldTreeObject*>(model()->rowFromPath(*it));
		if(UnitTreeObject* unitObject = dynamic_cast<UnitTreeObject*>(object)){
			UnitBase& unitBase = unitObject->unit();
			if(unitBase.attr().isEnvironment()){
				UnitEnvironment& unit = safe_cast_ref<UnitEnvironment&>(unitBase);
				CSurToolEnvironment::loadPreset(&unit);
			}
		}
	}
}

void ObjectsManagerTree::updateSelectFromWorld()
{
	kdw::TreeRow::iterator it;
	FOR_EACH(*root(), it){
	    WorldTreeObject* treeObject = safe_cast<WorldTreeObject*>(&**it);
		bool worldObjectSelected = treeObject->worldObjectSelected();
		if(treeObject->selected() != worldObjectSelected){
			model()->selectRow(treeObject, worldObjectSelected);
			model()->setFocusedRow(treeObject);
			ensureVisible(treeObject, false);
		}	
	}
	update();
}


void ObjectsManagerTree::onMenuDelete()
{
	Model::Selection selection = model()->selection();
	Model::Selection::iterator it;

	FOR_EACH(selection, it){
		kdw::TreeRow* row = model()->rowFromPath(*it);
		if(!row)
			continue;
		WorldTreeObject* object = safe_cast<WorldTreeObject*>(row);
		if(object && object->worldObjectSelected())
			object->kill();
	}
}

void ObjectsManagerTree::onMenuPaste()
{
	Model::Selection selection = model()->selection();
	Model::Selection::iterator it;

	int counter = 0;
	FOR_EACH(selection, it){
		WorldTreeObject& object = *safe_cast<WorldTreeObject*>(model()->rowFromPath(*it));

		kdw::Clipboard clipboard(this);
		Serializer serializeable = object.getSerializer();

		int changed = clipboard.smartPaste(serializeable);
		if(changed)
			++counter;
	}

	if(counter > 0){
		eventMaster().signalObjectChanged().emit(this);
		CString str;
		str.Format(TRANSLATE("Было изменено %i объекта(ов)"), counter);
		MessageBox(0, str, 0, MB_OK|MB_ICONINFORMATION);
	}
}

void ObjectsManagerTree::selectOnWorld()
{
	eventMaster().signalSelectionChanged().emit(this);
}

/*
void ObjectsManagerTree::selectObject(TreeObject* object, bool select)
{
	ensureVisible(object);
	model()->selectRow(object, true);
	model()->setFocusedRow(object);
	onRowSelected(object);
	update();
}
*/

//------------------------------------------------------------------------------------------------

CObjectsManagerTree::CObjectsManagerTree()
: CObjectsTreeCtrl(new ObjectsManagerTree())
{

}

