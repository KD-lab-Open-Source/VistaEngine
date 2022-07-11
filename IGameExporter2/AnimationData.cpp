#include "StdAfx.h"
#include "AnimationData.h"

void AnimationData::Save(Saver& s)
{
	s.push(C3DX_ANIMATION_HEAD);
	SaveGroup(s);
	SaveChain(s);
	SaveVisibleSets(s);
	s.pop();
}

void AnimationData::Load(CLoadDirectory rd)
{
	animation_group.clear();
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_ANIMATION_GROUPS:
		LoadGroup(ld);
		break;
	case C3DX_ANIMATION_CHAIN:
		LoadChain(ld);
		break;
	case C3DX_ANIMATION_CHAIN_GROUP:
		LoadChainGroup(ld);
		break;
	case C3DX_ANIMATION_VISIBLE_SETS:
		LoadVisibleSets(ld);
		break;
	}
	//ConvertPrevFormat();

}
void AnimationData::ConvertPrevFormat()
{
	if (animation_chain_group.size() > 0) 
	{
		AnimationVisibleSet avs;
		avs.name = "PrevFormat";
		avs.animation_visible_groups = animation_chain_group;
		for (int i=0; i<animation_chain_group.size();i++) 
		{
			for(int j=0; j<animation_chain_group[i].invisible_object.size(); j++)
			{
				bool f = false;
				for(int k=0; k<avs.objects.size(); k++)
				{
					if(avs.objects[k] == animation_chain_group[i].invisible_object[j])
					{
						f = true;
					}
				}
				if (!f) 
				{
					avs.objects.push_back(animation_chain_group[i].invisible_object[j]);
				}
			}
		}

		animation_visible_sets.push_back(avs);
	}
}

void AnimationData::SaveGroup(Saver& s)
{
	s.push(C3DX_ANIMATION_GROUPS);

	for(int i=0;i<animation_group.size();i++)
	{
		AnimationGroup& ag=animation_group[i];
		s.push(C3DX_ANIMATION_GROUP);
			s.push(C3DX_AG_HEAD);
			s<<ag.name;
			s.pop();
			s.push(C3DX_AG_LINK);
			s<<(int)ag.groups.size();
			for(int j=0;j<ag.groups.size();j++)
			{
				IGameNode* node=ag.groups[j];
				s<<node->GetName();
//				int inode=pRootExport->FindNodeIndex(node);
//				xassert(inode>=0);
//				s<<inode;
			}
			s.pop();

			s.push(C3DX_AG_LINK_MATERIAL);
			s<<(int)ag.materials.size();
			for(int j=0;j<ag.materials.size();j++)
			{
				IGameMaterial* node=ag.materials[j];
				s<<node->GetMaterialName();
			}
			s.pop();
		s.pop();
	}
	s.pop();
}

void AnimationData::LoadGroup(CLoadDirectory rd)
{
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_ANIMATION_GROUP:
		{
			LoadGroupOne(ld);
		}
		break;
	}
}

void AnimationData::LoadGroupOne(CLoadDirectory rd)
{
	AnimationGroup ag;
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_AG_HEAD:
		{
			CLoadIterator li(ld);
			li>>ag.name;
		}
		break;
	case C3DX_AG_LINK:
		{
			CLoadIterator li(ld);
			int size=0;
			li>>size;
			for(int i=0;i<size;i++)
			{
				IGameNode* node=NULL;
				string node_name;
				li>>node_name;

				node=pRootExport->Find(node_name.c_str());

				if(node)
					ag.groups.push_back(node);
				else
					Msg("Не могу найти ноду %s. Она исчезнет из анимационной группы %s\n",node_name.c_str(),ag.name.c_str());
			}
		}
		break;
	case C3DX_AG_LINK_MATERIAL:
		{
			CLoadIterator li(ld);
			int size=0;
			li>>size;
			for(int i=0;i<size;i++)
			{
				IGameMaterial* node=NULL;
				string node_name;
				li>>node_name;

				node=pRootExport->FindMaterial(node_name.c_str());

				if(node)
					ag.materials.push_back(node);
				else
					Msg("Не могу найти материал %s. Она исчезнет из анимационной группы %s\n",node_name.c_str(),ag.name.c_str());
			}
		}
		break;
	}
	animation_group.push_back(ag);
}


void AnimationData::SaveChain(Saver& s)
{
	if(animation_chain.empty())
	{
		Msg("Error: Нет ни одной анимационной цепочки.");
	}

	s.push(C3DX_ANIMATION_CHAIN);

	for(int ichain=0;ichain<animation_chain.size();ichain++)
	{
		AnimationChain& ac=animation_chain[ichain];
		s.push(C3DX_AC_ONE);
		s<<ac.name;
		s<<ac.begin_frame;
		s<<(ac.end_frame+1);
		s<<ac.time;
		s<<ac.cycled;
		s.pop();
	}
	s.pop();
}

void AnimationData::SaveChainGroup(Saver& s)
{
	s.push(C3DX_ANIMATION_CHAIN_GROUP);
	for(int igroup=0;igroup<animation_chain_group.size();igroup++)
	{
		AnimationVisibleGroup& ag=animation_chain_group[igroup];
		s.push(C3DX_ACG_ONE);
		s<<ag.name;
		s<<(int)ag.invisible_object.size();
		for(int i=0;i<ag.invisible_object.size();i++)
		{
			const char* name=ag.invisible_object[i]->GetName();
			s<<name;
		}
		s.pop();
	}
	s.pop();
}

void AnimationData::LoadChain(CLoadDirectory rd)
{
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_AC_ONE:
		{
			CLoadIterator li(ld);
			AnimationChain ac;
			ac.cycled=false;
			li>>ac.name;
			li>>ac.begin_frame;
			li>>ac.end_frame;
			ac.end_frame--;
			ac.end_frame=max(ac.begin_frame,ac.end_frame);
			li>>ac.time;
			li>>ac.cycled;
			animation_chain.push_back(ac);
		}
		break;
	}
}

void AnimationData::LoadChainGroup(CLoadDirectory rd)
{
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_ACG_ONE:
		{
			CLoadIterator li(ld);
			AnimationVisibleGroup ag;
			li>>ag.name;
			int size=0;
			li>>size;
			for(int i=0;i<size;i++)
			{
				string node_name;
				li>>node_name;
				IGameNode* node=pRootExport->Find(node_name.c_str());
				if(node)
					ag.invisible_object.push_back(node);
				else
				{
					Msg("Не могу найти ноду %s. Она станет видимой в группе видимости %s.\n",node_name.c_str(),ag.name.c_str());
				}
			}
			animation_chain_group.push_back(ag);
		}
		break;
	}
}
void AnimationData::SaveVisibleSets(Saver& s)
{
	s.push(C3DX_ANIMATION_VISIBLE_SETS);
	for(int iset=0; iset<animation_visible_sets.size(); iset++)
	{
		AnimationVisibleSet& avs = animation_visible_sets[iset];
		s.push(C3DX_AVS_ONE);
		  s.push(C3DX_AVS_ONE_NAME);
		  s << avs.name;
		  s.pop();

		  s.push(C3DX_AVS_ONE_OBJECTS);
		  s << (int)avs.objects.size();
		  for(int i=0;i<avs.objects.size();i++)
		  {
			const char* name=avs.objects[i]->GetName();
			s<<name;
		  }
		  s.pop();

		  s.push(C3DX_AVS_ONE_VISIBLE_GROUPS);
			for (int igroup=0; igroup<avs.animation_visible_groups.size(); igroup++)
			{
				AnimationVisibleGroup& ag = avs.animation_visible_groups[igroup];
				s.push(C3DX_AVS_ONE_VISIBLE_GROUP);
				s << ag.name;
				s<<(int)ag.invisible_object.size();//
				for(int i=0;i<ag.invisible_object.size();i++)
				{
					const char* name=ag.invisible_object[i]->GetName();
					s<<name;
				}
				s.pop();
			}
		  s.pop();
		s.pop();
	}
	s.pop();
}

void AnimationData::LoadVisibleSets(CLoadDirectory rd)
{
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_AVS_ONE:
		{
			AnimationVisibleSet avs;
			avs.LoadVisibleSet(ld);
			animation_visible_sets.push_back(avs);
			break;
		}
	}
}

void AnimationVisibleSet::LoadVisibleSet(CLoadDirectory rd)
{

	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_AVS_ONE_NAME:
		{
			CLoadIterator li(ld);
			li >> name;
		}
		break;
	case C3DX_AVS_ONE_OBJECTS:
		{
			CLoadIterator li(ld);
			int size = 0;
			li >> size;
			for(int i=0; i<size; i++)
			{
				string node_name;
				li>>node_name;
				IGameNode* node=pRootExport->Find(node_name.c_str());
				if(node)
					objects.push_back(node);
				else
				{
					Msg("Не могу найти ноду %s. Она станет видимой в группе видимости %s.\n",node_name.c_str(),name.c_str());
				}
			}
		}
		break;
	case C3DX_AVS_ONE_VISIBLE_GROUPS:
		{
			CLoadDirectory rdd(ld);
			while(CLoadData* ldd=rdd.next())
			switch(ldd->id)
			{
			case C3DX_AVS_ONE_VISIBLE_GROUP:
				{
					CLoadIterator li(ldd);
					AnimationVisibleGroup ag;
					li >> ag.name;
					int gsize = 0;
					li >> gsize;
					for (int j=0; j<gsize; j++)
					{
						string node_name;
						li>>node_name;
						IGameNode* node=pRootExport->Find(node_name.c_str());
						if(node)
							ag.invisible_object.push_back(node);
						else
						{
							Msg("Не могу найти ноду %s. Она станет видимой в группе видимости %s.\n",node_name.c_str(),name.c_str());
						}
					}

					animation_visible_groups.push_back(ag);
				}
				break;
			}
		}
		break;
	}

}

int AnimationData::FindAnimationGroupIndex(IGameNode* node)
{
	for(int i=0;i<animation_group.size();i++)
	{
		AnimationGroup& g=animation_group[i];
		vector<IGameNode*>::iterator it=find(g.groups.begin(),g.groups.end(),node);
		if(it!=g.groups.end())
			return i;
	}
	return -1;
}
