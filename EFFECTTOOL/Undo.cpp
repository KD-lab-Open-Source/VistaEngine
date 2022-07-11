#include "stdafx.h"
#include "EffectTool.h"
#include "EffectToolDoc.h"
#include "Undo.h"
#include "OptTree.h"
#include "EffectTreeView.h"

EmitterCopy::EmitterCopy(CEmitterData* em)
{
	emitter = new CEmitterData(em);
	ix_group = ix_effect = ix = -1;
	active = true;
}
void EmitterCopy::Apply(bool undo)
{
	CEffectData* effect = _pDoc->Group(ix_group)->Effect(ix_effect);
	if (insert)
	{
		if (undo)
		{
			CEmitterData* cur = (CEmitterData*)GetCurObject();
			ASSERT(cur);
			effect->del_emitter(cur);
			_pDoc->SetActiveEmitter(effect->Emitter(0));
		}else
		{
			_pDoc->SetActiveEmitter(effect->add_emitter(emitter));
//			*((UnicalID*)_pDoc->ActiveEmitter()) = *(UnicalID*)emitter;
		}
	}else
	{
		CEmitterData* cur = (CEmitterData*)GetCurObject(); 
		if (cur)
			cur->Reset(emitter);
		else 
			cur = effect->add_emitter(emitter);
		if (active)
			_pDoc->SetActiveEmitter(cur);
	}
	_pDoc->SetActiveGroup(_pDoc->Group(ix_group));
	_pDoc->SetActiveEffect(effect);

//	CEffectData* effect = _pDoc->Group(ix_group)->Effect(ix_effect);
//	for(int i=effect->EmittersSize()-1; i>=0; i--)
//	{
//		if (effect->Emitter(i)->GetID()==emitter->GetID())
//			effect->Emitter(i)->Reset(emitter);
//		return;
//	}
//	_pDoc->SetActiveEmitter(effect->add_emitter());
//	_pDoc->ActiveEmitter()->Reset(emitter);	

//	CEmitterData* cur = effect->Emitter(ix);
//	cur->Reset(emitter);
//	_pDoc->SetActiveEmitter(cur);
}
void* EmitterCopy::GetCurObject()
{
	CEffectData* effect = _pDoc->Group(ix_group)->Effect(ix_effect);
	for(int i=effect->EmittersSize()-1; i>=0;i--)
		if (effect->Emitter(i)->GetID()==emitter->GetID())
			return effect->Emitter(i);
	return NULL;

//	if ((UINT)ix<_pDoc->Group(ix_group)->Effect(ix_effect)->EmittersSize())
//		return _pDoc->Group(ix_group)->Effect(ix_effect)->Emitter(ix);
//	return NULL;
}

EffectCopy::EffectCopy(CEffectData *eff)
{
	effect = new CEffectData(eff);
	ix_group = ix = -1;
}

void EffectCopy::Apply(bool undo)
{
	CGroupData* group = _pDoc->Group(ix_group);
	if (insert)
	{
		if (undo)
		{
			CEffectData* cur = (CEffectData*)GetCurObject();
			ASSERT(cur);
			group->DeleteEffect(cur);
			_pDoc->SetActiveEffect(group->Effect(0));
		}else
		{
			_pDoc->SetActiveEffect(group->AddEffect());
			_pDoc->ActiveEffect()->Reset(effect);
		}
	}else
	{
		CEffectData* cur = (CEffectData*)GetCurObject(); 
		if (!cur)
		{
			cur = group->AddEffect();
			_pDoc->SetActiveEffect(cur);
		}
		cur->Reset(effect);
	}

//	CGroupData* group = _pDoc->Group(ix_group);
//	for(int i=group->EffectsSize()-1; i>=0; i--)
//	{
//		group->Effect(i)->Reset(effect);
//		return;
//	}
//	_pDoc->SetActiveEffect(group->AddEffect());
//	_pDoc->ActiveEffect()->Reset(effect);

//	CEffectData *cur_eff = group->Effect(ix);
//	cur_eff->Reset(effect);
//	_pDoc->SetActiveEffect(cur_eff);
	_pDoc->SetActiveGroup(group);
	_pDoc->SetActiveEmitter(NULL);
}

void* EffectCopy::GetCurObject()
{
	if ((UINT)ix_group<_pDoc->GroupsSize())
		if ((UINT)ix<_pDoc->Group(ix_group)->EffectsSize())
			return _pDoc->Group(ix_group)->Effect(ix);
	return NULL;
}

GroupCopy::GroupCopy(CGroupData* gr)
{
	group = new CGroupData;
	*group = *gr;
	ix = -1;
}

UINT GroupCopy::GetID()
{
	if (group)
		return group->GetID();
	return groupID;
}

void GroupCopy::Apply(bool undo)
{
	CGroupData* grp;
	if (insert)
	{
		if (undo)
		{
			grp = (CGroupData*)GetCurObject();
			_pDoc->GetEffTree()->DeleteGroupView(grp);
			grp = _pDoc->Group(0);
//			_pDoc->SetActiveGroup(_pDoc->Group(0));
		}else
		{
			grp = _pDoc->AddGroup();
			*grp = *group;
		}
	}else
	{
		grp = (CGroupData*)GetCurObject();
		xassert(grp);
		*grp = *group;
	}
	_pDoc->SetActiveGroup(grp);
/*
	if (undo&&insert)
	{
		CGroupData* grp = (CGroupData*)GetCurObject();
		_pDoc->GetEffTree()->DeleteGroupView(grp);
		_pDoc->SetActiveGroup(_pDoc->Group(0));
	}else
	if (group)
	{
		CGroupData* cur_gr;
		if (insert)
		{
			cur_gr = _pDoc->AddGroup();
			*cur_gr = *group;
		}else
		{
			cur_gr = (CGroupData*)GetCurObject();
			*cur_gr = *group;
		}
		_pDoc->SetActiveGroup(cur_gr);
	}
/*	else
	{
		CGroupData* grp = (CGroupData*)GetCurObject();
		if (grp)
		{
			_pDoc->GetEffTree()->DeleteGroupView(grp);
			_pDoc->SetActiveGroup(_pDoc->Group(0));
		}else
		{
			_pDoc->SetActiveGroup(_pDoc->AddGroup());
			groupID = _pDoc->ActiveGroup()->GetID();
		}	
	}*/
	_pDoc->SetActiveEffect(_pDoc->ActiveGroup()->Effect(0));
	_pDoc->SetActiveEmitter(NULL);
}
void* GroupCopy::GetCurObject()
{
	if (!group)
	{
		for(int i=_pDoc->GroupsSize()-1;i>=0;i--)
			if (_pDoc->Group(i)->GetID()==groupID)
				return _pDoc->Group(i);
	}
	else 
	if ((UINT)ix<_pDoc->GroupsSize() && group->GetID() == _pDoc->Group(ix)->GetID())
		return _pDoc->Group(ix);
	return NULL;
}
void CUndoRedo::ClosePrevious()
{
	if (ix>=1 && history[ix-1]->GetType()==EMITTER_COPY)
	{
		EmitterCopy *prev = (EmitterCopy*)history[ix-1];
		if (_pDoc->ActiveEmitter() && prev->GetID() != _pDoc->ActiveEmitter()->GetID())
		{
			CEmitterData *em = (CEmitterData*)prev->GetCurObject();
			ASSERT(em);
			EmitterCopy *copy = new EmitterCopy(em);
			copy->ix_group = prev->ix_group;
			copy->ix_effect = prev->ix_effect;
			copy->ix = prev->ix;
			copy->insert = false;
			push(copy);
		}
	}
}
void CUndoRedo::PushEmitter(CEmitterData& em, CEffectData* eff, CGroupData* gr, bool active)
{
	if (!eff) eff = _pDoc->ActiveEffect();
	if (!gr) gr = _pDoc->ActiveGroup();
	CEmitterData *emt = _pDoc->ActiveEmitter();
	CEffectData *efft = _pDoc->ActiveEffect();
	CGroupData  *grt  = _pDoc->ActiveGroup();
	_pDoc->SetActiveEmitter(&em);
	_pDoc->SetActiveEffect(eff);
	_pDoc->SetActiveGroup(gr);
	PushEmitter();
	((EmitterCopy*)history[ix-1])->active = active;
	_pDoc->SetActiveEmitter(emt);
	_pDoc->SetActiveEffect(efft);
	_pDoc->SetActiveGroup(grt);
}

void CUndoRedo::PushEmitter(bool insert)
{
	ClearHistory(ix);
	EmitterCopy *copy = new EmitterCopy(_pDoc->ActiveEmitter());
	copy->ix_group = _pDoc->GroupIndex(_pDoc->ActiveGroup());
	copy->ix_effect = _pDoc->ActiveGroup()->EffectIndex(_pDoc->ActiveEffect());
	copy->ix = _pDoc->ActiveEffect()->EmitterIndex(_pDoc->ActiveEmitter());
	copy->insert = insert;
	push(copy);
}
void CUndoRedo::PushEffect(CEffectData *eff, CGroupData* grp, bool insert)
{
	if (!eff)
		eff = _pDoc->ActiveEffect();
	if (!grp)
		grp = _pDoc->ActiveGroup();
	ClearHistory(ix);
	EffectCopy *copy = new EffectCopy(eff);
	copy->ix_group = _pDoc->GroupIndex(grp);
	copy->ix = grp->EffectIndex(eff);
	copy->insert = insert;
	push(copy);
}
GroupCopy *CUndoRedo::CreateGroupCopy(CGroupData* group)
{
	GroupCopy *copy = new GroupCopy(group);
	copy->ix = _pDoc->GroupIndex(group);
	return copy;
}

void CUndoRedo::PushGroup(bool insert)
{
	ClearHistory(ix);
	GroupCopy *copy = CreateGroupCopy(_pDoc->ActiveGroup());
	copy->insert = insert;
	push(copy);
}
/*
void CUndoRedo::AddingGroup(UINT group_id)
{
	ClearHistory(ix);
	GroupCopy * copy = new GroupCopy(group_id);
	push(copy);
}
*/
void CUndoRedo::push(BaseUndo* copy)
{
//	if (!history.empty())
	if (history.size()>30)
	{
		delete history.front();
		history.pop_front();
	}
	else ix++;
	copy->saved = saved;
	saved = false;
	history.push_back(copy);
	is_top = false;
}

void CUndoRedo::ClearHistory(int j)
{
	if ((UINT)j>history.size())
		return;
	for(int i=j;i<history.size(); i++)
		delete history[i];
	history.erase(history.begin()+j, history.end());
	ix = j;
//	ClosePrevious();
	//	if ((UINT)j>=history.size())
//		return;
//	for(int i=j+1;i<history.size(); i++)
//		delete history[i];
//	history.erase(history.begin()+j+1, history.end());
//	ix = j;
}
void CUndoRedo::Undo()
{
	if ((UINT)(ix) == history.size()&& !history.empty() && !is_top)
	{
		BaseUndo* base = history[ix-1];
		if (!base->insert)
		switch(base->GetType())
		{
			case EMITTER_COPY:
			{
				CEmitterData* em = (CEmitterData*)base->GetCurObject();
				if (em)
				{
					_pDoc->SetActiveEmitter(em);
					PushEmitter();
					ix--;
//					ix = max(0,ix-1);
				}
				break;
			}
			case EFFECT_COPY:
			{
				CEffectData* eff = (CEffectData*)base->GetCurObject();
				if (eff)
				{
					PushEffect(eff, _pDoc->Group(((EffectCopy*)base)->ix_group));
					ix--;
//					ix = max(0,ix-1);
				}
				break;
			}
			case GROUP_COPY:
			{
				CGroupData* group = (CGroupData*)base->GetCurObject();
				_pDoc->SetActiveGroup(group);
				PushGroup();
				ix--;
				break;
			}
		}
//		else ix++;
		is_top = true;
	}
/*	ix--;
	if (ix<-1) ix=-1;
	if(ix>=0)
	{
		history[ix]->Apply(true);
		_pDoc->Update();
	}
*/
/*
	if ((UINT)ix < history.size() && history[ix]->insert)
	{
		history[ix]->Apply(true);
		ix--;
		if (ix<0)ix=0;
	}else if ((UINT)(ix-1) < history.size())
	{
		ix--;
		history[ix]->Apply(true);
	}else return;
	_pDoc->Update();

*/

	if ((UINT)(ix-1) > history.size())
		return;
	ix--;
	history[ix]->Apply(true);
	_pDoc->Update();
}

void CUndoRedo::Redo()
{
/*
	ix++;
	if (ix>history.size())
		ix = history.size();
	if (ix<history.size())
	{
		history[ix]->Apply(false);
		_pDoc->Update();
	}
*/
	if ((UINT)ix < history.size() && history[ix]->insert)
	{
		history[ix]->Apply(false);
		ix++;
	}else if ((UINT)ix+1 < history.size())
	{
		ix++;
		history[ix]->Apply(false);
	}else return;
	_pDoc->Update();
}

CUndoRedo::CUndoRedo()
{
	ix = 0;
	is_top = false;
	saved = false;
}
CUndoRedo::~CUndoRedo()
{
	ClearHistory();
}

bool CUndoRedo::IsSaved()
{
	if (history.empty())
		return true;
	if (ix==history.size())
		return saved;
	return history[ix]->saved;
}
void CUndoRedo::SetSave()
{
	if ((UINT)ix<history.size())
	{
		deque<BaseUndo*>::iterator it;
		FOR_EACH(history, it)
			(*it)->saved = false;
		xassert((UINT)ix<history.size());
		history[ix]->saved = true;
	}
	saved = true;
}



