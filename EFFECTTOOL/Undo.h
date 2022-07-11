
#pragma once

class CGroupData;
class CEffectData;
class CEmitterData;

#include "D3DScene.h"

enum TypeUndoCopy
{
	EMITTER_COPY, 
	EFFECT_COPY, 
	GROUP_COPY, 
};
class BaseUndo 
{
	int step;
public:
	bool saved;
	bool insert;
	virtual void Apply(bool undo)=0;
	virtual TypeUndoCopy GetType()=0;
	virtual void* GetCurObject()=0;
	virtual UINT GetID()=0;
	BaseUndo():saved(false) {insert =  false;}
	~BaseUndo(){}
};

class EmitterCopy : public BaseUndo
{
	CEmitterData* emitter;
public:
	int ix_group;
	int ix_effect;
	int ix;
	bool active;
	EmitterCopy(CEmitterData* em);
	~EmitterCopy(){ delete emitter; } 
	void Apply(bool undo);
	TypeUndoCopy GetType(){return EMITTER_COPY;}
	void* GetCurObject();
	UINT GetID(){return emitter->GetID();}
};

class EffectCopy : public BaseUndo
{
	CEffectData* effect;
public:
	int ix_group;
	int ix;
	EffectCopy(CEffectData *eff);
	~EffectCopy(){delete effect;} 
	void Apply(bool undo);
	TypeUndoCopy GetType(){return EFFECT_COPY;}
	void* GetCurObject();
	UINT GetID(){return effect->GetID();}
};

class GroupCopy : public BaseUndo
{
	CGroupData* group;
	UINT groupID;
public:
	int ix;
//	bool insert;
	GroupCopy(CGroupData* gr);
	GroupCopy(UINT id) : group(NULL), groupID(id){}
	~GroupCopy(){delete group;} 
	void Apply(bool undo);
	UINT GetID(); 
	bool empty(){return group==NULL;}
	TypeUndoCopy GetType(){return GROUP_COPY;}
	void* GetCurObject();
};

#include <deque>
class CUndoRedo
{
	GroupCopy* top;
	deque<BaseUndo*> history;
	int ix;// current history`s position
	GroupCopy *CreateGroupCopy(CGroupData* group);
	bool is_top;
	void push(BaseUndo* obj);
	void ClosePrevious();
	bool saved;
public:
	CUndoRedo();
	~CUndoRedo();
	void ClearHistory(int beg =0);
	void Undo();
	void Redo();

	void PushEmitter(bool insert =false);
	void PushEmitter(CEmitterData& em, CEffectData* eff =NULL, CGroupData* gr =NULL, bool active =true) ;
	void PushEffect(CEffectData *eff = NULL, CGroupData* grp = NULL, bool insert =false);
	void PushGroup(bool insert =false);
	bool IsSaved();
	void SetSave();
//	void AddingGroup(UINT group_id);
};
