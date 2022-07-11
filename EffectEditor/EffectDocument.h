#ifndef __EFFECT_DOCUMENT_H_INCLUDED__
#define __EFFECT_DOCUMENT_H_INCLUDED__

#include "Render/src/CurveWrapper.h"
#include "Render/src/NParticle.h"
#include "kdw/Navigator.h"
#include "kdw/Sandbox.h"
#include "kdw/Tool.h"

namespace kdw{
	class PopupMenuItem;
	class Sandbox;
	class Toy;
}

class cScene;
class cTileMap;

class Environment;
class EffectDocument;
class EffectWrapper;
extern EffectDocument* globalDocument;

class NavBase : public kdw::NavigatorNode{
public:
	EffectDocument* document() { return globalDocument; }
};

class EmitterWrapper;
class NodeEmitter;
class NodeCurve : public kdw::ModelNode{
public:
	explicit NodeCurve(CurveWrapperBase*, bool visible = false);

	void onSelect();
	void setVisible(bool visible);
	bool visible() const{ return visible_; }
	NodeEmitter* parent();
	const NodeEmitter* parent() const;

	int selectedPoint() const;
	void setSelectedPoint(int index);

	float currentValue() const;
	void setCurrentValue(float value);

	CurveWrapperBase* wrapper(){ return wrapper_; }
protected:
	ShareHandle<CurveWrapperBase> wrapper_;
	bool visible_;
};

class NodeEmitter : public kdw::ModelNode{
public:
	explicit NodeEmitter(EmitterKeyInterface* emitterKey);
	~NodeEmitter();

	void update();
	void updateToys();
	NodeCurve* curveByName(const char* name);

	void set(EmitterKeyInterface* emitter);
	EmitterKeyInterface* get(){ return emitterKey_; }
	const EmitterKeyInterface* get() const{ return emitterKey_; }
	void onSelect();
	void onDeselect();
	void onAdd();
	void onRemove();
	void onContextMenu(kdw::PopupMenuItem& root, kdw::Widget* widget);
	void onMenuRemoveEmitter();

	void onCurveChanged(bool significantChange);

	void onToyEmitterPositionBeforeMove(kdw::Toy* toy);
	void onToyEmitterPositionMove(kdw::Toy* toy);
	void onToyEmitterPositionMoved(kdw::Toy* toy);

	void onToySplinePointActivate(kdw::Toy* toy);
	void onToySplinePointMove(kdw::Toy* toy);
	void onToySplinePointBeforeMove(kdw::Toy* toy);
	void onEffectCenterChanged();

	int selectedGenerationPoint()const{ return selectedGenerationPoint_; }
	void setSelectedGenerationPoint(int index);

	int selectedPoint() const{ return selectedPoint_; }
	void setSelectedPoint(int index);

	void buildKey();

	const char* name() const;
	Serializer serializeable();
	void serialize(Archive& ar);

	bool browseable() const{ return false; }
protected:
	void createToys();

	EmitterKeyInterface* emitterKey_;

	ShareHandle<kdw::Toy> toyEffectPosition_;
	kdw::Toys toyEmitterPosition_;
	kdw::Toys toySplinePoints_;

	int selectedGenerationPoint_;
	int selectedPoint_;
};

typedef std::vector<ShareHandle<EmitterWrapper> > EmitterWrappers;
class cObject3dx;

class NodeEffect : public kdw::ModelNode{
public:
	NodeEffect(EffectKey* key, const char* fileName);
	~NodeEffect();
	void serialize(Archive& ar);
	EffectKey* get(){ return effectKey_; }
	void set(EffectKey* effect);
	const char* fileName() const{ return fileName_.c_str(); }
	
	void onRenderDeviceInitialize();
	
	Serializer serializeable();
	void onSelect();
	void onContextMenu(kdw::PopupMenuItem& root, kdw::Widget* widget);

	void onMenuAddEmitter();
	void onMenuSave(kdw::Widget* widget);
	void onMenuSaveAs(kdw::Widget* widget);
	void onMenuUnload();

	void saveOld(const char* fileName);
	bool saved()const{ return !fileName_.empty(); }
	void rebuild();

	NodeEmitter* addEmitter();

	NodeEmitter* child(int index){ return safe_cast<NodeEmitter*>(kdw::ModelNode::child(index)); }
	NodeEmitter* child(const char* name){ return safe_cast<NodeEmitter*>(kdw::ModelNode::child(name)); }
	void remove(NodeEmitter* emitter);

	void loadModel(const char* fileName);
	void setModelVisibilityGroupIndex(int index);
	void setModelAnimationChainIndex(int index);
	void setModelNodeIndex(int index);
	float calculateModelScale(cObject3dx* model);
	void updateModelPosition();
	void onTimeChanged(ModelObserver* changer);
protected:
	void updateEffectNodePose();
	std::string fileName_;

	EffectKey* effectKey_;

	int modelVisibilitySetIndex_;
	int modelAnimationChainIndex_;
	int modelNodeIndex_;
	std::string modelFileName_;
	cObject3dx* model_;
	cObject3dx* modelLogic_;
};

typedef std::vector<ShareHandle<EffectWrapper> > EffectWrappers;

class NodeEffectRoot : public kdw::ModelNode{
public:
	NodeEffectRoot(EffectDocument* document);

	void onContextMenu(kdw::PopupMenuItem& root, kdw::Widget* widget);
	void onMenuNewEffect();
	void onMenuOpenEffect(kdw::Widget* widget);
protected:
	EffectDocument* document_;
};

class HistoryItem : public ShareHandleBase, public kdw::ModelObserver{
public:
	virtual ~HistoryItem() {}
	virtual void act() = 0;
	virtual HistoryItem* createRedo() = 0;
};

class HistoryManager{
public:
	static const int HISTORY_STEPS = 50;
	struct RedoItem{
		ShareHandle<HistoryItem> undo;
		ShareHandle<HistoryItem> redo;
	};
	HistoryManager();

	void pushEffect();
	void pushEmitter();
	void pushNewEffect(int index);
	void pushRemoveEffect(int index);

	void undo();
	void redo();
	bool canBeUndone();
	bool canBeRedone();
	sigslot::signal0& signalChanged(){ return signalChanged_; }
protected:
	void push(HistoryItem* historyItem);

	sigslot::signal0 signalChanged_;
	typedef std::vector<ShareHandle<HistoryItem> > HistoryItems;
	HistoryItems historyItems_;
	typedef std::vector<RedoItem> RedoItems;
	RedoItems redoItems_;
};

class EffectDocument : public kdw::ModelTimed{
public:
	EffectDocument();
	~EffectDocument();

	HistoryManager& history(){ return history_; }

	void serialize(Archive& ar);

	void clear();
	NodeEffect* add(EffectKey* effectKey = 0);
	NodeEffect* add(const char* fileName);
	void close(EffectWrapper* effect);

	void onRenderDeviceInitialize();
	//void onRenderDeviceFinalize();

	void recreateEffect();

	NodeEffect* activeEffect(){ return activeEffect_; }
	void setActiveEffect(NodeEffect* nodeEffect);
	NodeEmitter* activeEmitter(){ return activeEmitter_; }
	void setActiveEmitter(NodeEmitter* nodeEmitter);
	NodeCurve* activeCurve(){ return activeCurve_; }
	void setActiveCurve(NodeCurve* nodeCurve);

	cScene* scene();
	cEffect* effect();
	NodeEffectRoot* createRoot();

	void quant(float deltaTime);
	void setTime(float time, kdw::ModelObserver* changer);
	void queueSetTime(float time, kdw::ModelObserver* changer);

	void onSelectionChanged();

	void onCurveChanged(bool significantChange);

	void setWorldName(const char* spgFileName);
	const char* worldName() const{ return worldName_.c_str(); }
	void createFlatTerrain();
	const char* worldsDirectory() const{ return worldsDirectory_.c_str(); }

	sigslot::signal0& signalCurveToggled(){ return signalCurveToggled_; }
	sigslot::signal1<bool>& signalCurveChanged(){ return signalCurveChanged_; }
	void setEffectOrigin(const Vect3f& origin);
	void setEffectNodePose(const Se3f& pose);

	void updateEffectPose();	
	Se3f effectPose() const{ return effectPose_; }
	Vect3f effectOrigin() const{ return effectOrigin_; }
	Vect2i worldCenter() const{ return worldCenter_; }
	void onEffectOriginChanged(bool recreate = true);
	sigslot::signal0& signalEffectCenterChanged(){ return signalEffectCenterChanged_; }
	void setParticleRate(float particleRate);
protected:
	void createEffect();
	void updateEffectPosition();
	bool loadWorld(const char* worldName);
	void createEnvironment();

	HistoryManager history_;

	sigslot::signal0 signalCurveToggled_;
	sigslot::signal1<bool> signalCurveChanged_;
	sigslot::signal0 signalEffectCenterChanged_;
    
	std::string worldsDirectory_;
	std::string worldName_;
	ShareHandle<NodeEffect> activeEffect_;
	ShareHandle<NodeEmitter> activeEmitter_;
	ShareHandle<NodeCurve> activeCurve_;
	cEffect* effect_;
	cScene* scene_;
	int randomSeed_;
	Vect2i worldCenter_; 
	Vect3f effectOrigin_;
	Se3f effectNodePose_;
	Se3f effectPose_;
	float particleRate_;
};

#endif
