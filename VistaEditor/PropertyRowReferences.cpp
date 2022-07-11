#include "StdAfx.h"
#include "Units\AbnormalStateAttribute.h"
#include "Units\IronBullet.h"
#include "Units\IronLegion.h"
#include "Units\CommandsQueue.h"
#include "UserInterface\UI_References.h"
#include "UserInterface\UserInterface.h"
#include "Terra\terTools.h"
#include "Serialization/StringTableImpl.h"
#include "Serialization/SerializationFactory.h"
#include "kdw/PropertyRowReference.h"

class UIControlReferenceTreeBuilder : public kdw::TreeBuilder {
public:
	UIControlReferenceTreeBuilder(UI_ControlReference& reference)
	: reference_(reference)
	{}
	class ControlObject : public Object{
	public:
		ControlObject(const UI_ControlBase* control, const char* name)
		: Object(name, name)
		, control_(control)
		{
			selectable_ = control != 0;
		}

		const UI_ControlBase* control() { return control_; }

	protected:
		const UI_ControlBase* control_;
	};
protected:
	UI_ControlReference& reference_;

	Object* buildControlSubtree(Object* parent, const UI_ControlContainer* container, const UI_ControlContainer* selected){
		UI_ControlContainer::ControlList::const_iterator it;
		const char* name = container->name();
		Object* containerObj = parent->add(new ControlObject(0, name));

		Object* result = selected == container ? containerObj : 0;
		FOR_EACH(container->controlList(), it){
			if(Object* o = buildControlSubtree(containerObj, &**it, selected))
				result = o;
		}
		return result;
	}
	Object* buildControlSubtree(Object* parent, const UI_ControlBase* container, const UI_ControlContainer* selected){
		UI_ControlContainer::ControlList::const_iterator it;
		const char* name = container->name();
		Object* containerObj = parent->add(new ControlObject(container, name));

		Object* result = selected == container ? containerObj : 0;
		FOR_EACH(container->controlList(), it){
			if(Object* o = buildControlSubtree(containerObj, &**it, selected))
				result = o;
		}
		return result;
	}

    Object* buildTree(Object* root){
		typedef StaticMap<std::string, Object*> Groups;
		Object* result = 0;
		UI_Dispatcher::ScreenContainer& screens = UI_Dispatcher::instance().screens();
		UI_Dispatcher::ScreenContainer::iterator sit;
		UI_ControlBase* selected = reference_.control();
		FOR_EACH(screens, sit){
			UI_Screen* screen = &*sit;
			if(Object* o = buildControlSubtree(root, screen, selected))
				result = o;
		}
		return result;
    }
	bool select(Object* object){
		ControlObject* controlObject = safe_cast<ControlObject*>(object);
		if(controlObject->selectable()){
			reference_ = UI_ControlReference(controlObject->control());
			return true;
		}
		else
			return false;
	}
};

DECLARE_SEGMENT(PropertyRowReference)
typedef kdw::PropertyRowReference<UI_ControlReference, UIControlReferenceTreeBuilder> PropertyRowUIControlReference;
REGISTER_PROPERTY_ROW(UI_ControlReference, PropertyRowUIControlReference) 

REGISTER_REFERENCE(AbnormalStateTypeReference)
REGISTER_REFERENCE(AttributeBuildingReference)
REGISTER_REFERENCE(AttributeItemInventoryReference)
REGISTER_REFERENCE(AttributeItemReference)
REGISTER_REFERENCE(AttributeItemResourceReference)
REGISTER_REFERENCE(AttributePadReference)
REGISTER_REFERENCE(AttributePlayerReference)
REGISTER_REFERENCE(AttributeProjectileReference)
REGISTER_REFERENCE(AttributeReference)
REGISTER_REFERENCE(AttributeUnitActingReference)
REGISTER_REFERENCE(AttributeUnitObjectiveReference)
REGISTER_REFERENCE(AttributeUnitOrBuildingReference)
REGISTER_REFERENCE(AttributeUnitReference)
REGISTER_REFERENCE(BodyPartType)
REGISTER_REFERENCE(WeaponAnimationType)
REGISTER_REFERENCE(Difficulty)
REGISTER_REFERENCE(EffectReference)
REGISTER_REFERENCE(ParameterFormulaReference)
REGISTER_REFERENCE(ParameterGroupReference)
REGISTER_REFERENCE(ParameterTypeReference)
REGISTER_REFERENCE(ParameterTypeReferenceZero)
REGISTER_REFERENCE(ParameterValueReference)
REGISTER_REFERENCE(PlacementZone)
REGISTER_REFERENCE(Race)
REGISTER_REFERENCE(RigidBodyPrmReference)
REGISTER_REFERENCE(SoundReference)
REGISTER_REFERENCE(SourceReference)
REGISTER_REFERENCE(TerToolReference)
REGISTER_REFERENCE(UI_CursorReference)
REGISTER_REFERENCE(UI_FontReference)
REGISTER_REFERENCE(UI_MessageTypeReference)
REGISTER_REFERENCE(UI_ShowModeUnitSpriteReference)
REGISTER_REFERENCE(UI_SpriteReference)
REGISTER_REFERENCE(UI_TextureReference)
REGISTER_REFERENCE(UnitFormationTypeReference)
REGISTER_REFERENCE(WeaponGroupTypeReference)
REGISTER_REFERENCE(WeaponPrmReference)
REGISTER_REFERENCE(CommandsQueueReference)
REGISTER_REFERENCE(FormationPatternReference)
