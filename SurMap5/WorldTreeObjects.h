#ifndef __SURMAP_WORLD_TREE_OBJECTS_H_INCLUDED__
#define __SURMAP_WORLD_TREE_OBJECTS_H_INCLUDED__

#include "mfc/ObjectsTreeCtrl.h"
#include "kdw/PopupMenu.h"
#include "FileUtils\FileUtils.h"

#include "UniverseObjectAction.h"
#include "SurMap5\SurTool3DM.h"
#include "SurMap5\SurToolCameraEditor.h"
#include "SurMap5\SurToolSource.h"
#include "SurMap5\SurToolAnchor.h"
#include "SurMap5\ToolsTreeWindow.h"
#include "SurMap5\MainFrame.h"

#include "Game\Player.h"
#include "Game\Universe.h"
#include "Units\UnitEnvironment.h"

#include "Environment\SourceManager.h"
#include "Game\CameraManager.h"

//////////////////////////////////////////////////////////////////////////////

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

class WorldTreeObject : public kdw::TreeObject, public sigslot::has_slots{/*{{{*/
public:
	virtual void kill() = 0;
	virtual bool isAlive() const = 0;
	virtual bool worldObjectSelected() const = 0;
	virtual void select() = 0;
	virtual void deselect() = 0;
	virtual Vect3f location() const = 0;
	virtual bool isSimilar(const WorldTreeObject& object) const { return false; };
	virtual void onMenuConstruction(kdw::PopupMenuItem& root, kdw::ObjectsTree* tree); 
	virtual void getActions(UniverseObjectActionList& actions) {};
	void onRightClick(kdw::ObjectsTree* tree);
	bool onDoubleClick(kdw::ObjectsTree* tree);

	void onMenuSelectSimilar(kdw::ObjectsTree* tree);
	void onMenuLocate();

	void onSelect(kdw::ObjectsTree* tree);
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

class UnitTreeObject  : public WorldTreeObject{/*{{{*/
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
		setText(label);
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
	Serializer getSerializer(const char* name = "", const char* nameAlt = "") {
		return Serializer(unit_, name, nameAlt);
	}

	bool isEnvironment() const{ return unit_.attr().isEnvironment(); }

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
					return unit1.environmentType() == unit2.environmentType() && compareFileName(unit1.modelName(), unit2.modelName());
				}
				else
					return true;
			}
		}
		return false;
	};

	void onMenuLoadPreset()
	{
		UnitEnvironment& unit = safe_cast_ref<UnitEnvironment&>(unit_);
		CSurToolEnvironment::loadPreset(&unit);
	}

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
	
	void onMenuConstruction(kdw::PopupMenuItem& root, kdw::ObjectsTree* tree)
	{
		__super::onMenuConstruction(root, tree);
		if(unit_.attr().isEnvironment()){
			if(!root.empty())
				root.addSeparator();
			root.add(TRANSLATE("Применить пресет"))
				.connect(this, &UnitTreeObject::onMenuLoadPreset);
			root.add(TRANSLATE("Сохранить как пресет по умолчанию"))
				.connect(this, &UnitTreeObject::onMenuSavePreset);
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
		setText(name);
	}
	bool isAlive() const{
		return source_.isAlive();
	}
	void kill(){ source_.kill(); }
	void select(){ source_.setSelected(true); }
	void deselect(){ source_.setSelected(false); }
	bool worldObjectSelected() const{ return source_.selected(); }
	Serializer getSerializer(const char* name = "", const char* nameAlt = ""){
		return Serializer(source_, name, nameAlt);
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
		setText(name);
	}
	bool isAlive() const{ return !killed_; }
	void kill(){
		xassert(sourceManager);
		SourceManager::Anchors::iterator it = std::find(sourceManager->anchors().begin(),sourceManager->anchors().end(), &anchor_);
		xassert(it != sourceManager->anchors().end());
		sourceManager->anchors().erase(it);
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
	Serializer getSerializer(const char* name = "", const char* nameAlt = ""){
		xassert(!killed_);
		return Serializer(anchor_, name, nameAlt);
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

class CameraSplineTreeObject : public WorldTreeObject {/*{{{*/
public:
	CameraSplineTreeObject(CameraSpline& spline, const char* name)
	: spline_(spline)
	, killed_(false)
	{
		setText(name);
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
	Serializer getSerializer(const char* name = "", const char* nameAlt = "") {
		xassert(!killed_);
		return Serializer(spline_, name, nameAlt);
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
		if(universe() && sourceManager && !spline_.empty()){
			CameraCoordinate coord = spline_.spline().back();

			coord.position() += spline_.position();
			cameraManager->setCoordinate(coord);
		}
	}

	void onMenuConstruction(kdw::PopupMenuItem& root, kdw::ObjectsTree* tree)
	{
		__super::onMenuConstruction(root, tree);
		if(!spline_.empty()){
			if(!root.empty())
				root.addSeparator();
			root.add(TRANSLATE("Найти последнюю точку сплайна"))
				.connect(this, &CameraSplineTreeObject::onMenuLocateLastPoint);
			root.add(TRANSLATE("Проиграть сплайн"))
				.connect(this, &CameraSplineTreeObject::onMenuPlay);
		}
	}
private:
	CameraSpline& spline_;
	bool killed_;
};/*}}}*/

//////////////////////////////////////////////////////////////////////////////
void WorldTreeObject::onMenuConstruction(kdw::PopupMenuItem& root, kdw::ObjectsTree* tree)
{
	root.add(TRANSLATE("Найти на карте"))
		.connect(this, &WorldTreeObject::onMenuLocate);
	root.add(TRANSLATE("Выделить похожие"), tree)
		.connect(this, &WorldTreeObject::onMenuSelectSimilar);
		//.setDefault(true);
}; 

void WorldTreeObject::onSelect(kdw::ObjectsTree* tree)
{
	if(selected())
		select();
	else
		deselect();

	CToolsTreeWindow& toolsWindow = ((CMainFrame*)AfxGetMainWnd())->getToolWindow();
	if(!toolsWindow.isToolFromTreeSelected())
        toolsWindow.replaceEditorMode(0);
}

void WorldTreeObject::onMenuLocate()
{
	if(!universe() || !sourceManager){
		xassert(0);
		return;
	}

	CameraCoordinate coord = cameraManager->coordinate();
	coord.setPosition(location());
	cameraManager->setCoordinate(coord);
}

void WorldTreeObject::onRightClick(kdw::ObjectsTree* tree)
{
}

void WorldTreeObject::onMenuSelectSimilar(kdw::ObjectsTree* tree)
{
	onDoubleClick(tree);
}

bool WorldTreeObject::onDoubleClick(kdw::ObjectsTree* tree)
{
	tree->model()->deselectAll();
	TreeObject* lastObject = 0;
	TreeObject::iterator it;
	FOR_EACH(*tree->root(), it){
		WorldTreeObject& treeObject = safe_cast_ref<WorldTreeObject&>(**it);

		if(isSimilar(treeObject)){
			tree->selectObject(&treeObject, true, false, false);
			lastObject = &treeObject;
		}
	}
	if(lastObject)
		tree->selectObject(lastObject, true, false, true);
	tree->update();
	return false;
}
#endif
