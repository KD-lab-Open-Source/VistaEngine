#pragma once

struct AnimationChain
{
	string name;
	int begin_frame;
	int end_frame;
	float time;//Время, за которое должна проигрываться анимационная цепочка.
	bool cycled; //Нужно для корректной интерполяции
};

struct AnimationVisibleGroup
{
	string name;
	vector<IVisNode*> invisible_object;
};

struct AnimationGroup
{
	string name;
	vector<IVisNode*> groups;
	vector<IVisMaterial*> materials;
};

struct AnimationVisibleSet
{
	string name;
	vector<AnimationVisibleGroup> animation_visible_groups;
	vector<IVisNode*> objects;
	void LoadVisibleSet(CLoadDirectory rd);
};


class AnimationData
{
public:
	vector<AnimationChain> animation_chain;
	vector<AnimationVisibleGroup> animation_chain_group;
	vector<AnimationVisibleSet> animation_visible_sets;

	vector<AnimationGroup> animation_group;
	void Save(Saver& s);
	void Load(CLoadDirectory rd);

	int FindAnimationGroupIndex(IVisNode* node);//индекс в animation_group. -1 - не найдено
protected:
	void SaveGroup(Saver& s);
	void SaveChain(Saver& s);
	void SaveChainGroup(Saver& s);
	void SaveVisibleSets(Saver& s);
	void LoadGroup(CLoadDirectory rd);
	void LoadGroupOne(CLoadDirectory rd);

	void LoadChain(CLoadDirectory rd);
	void LoadChainGroup(CLoadDirectory rd);
	void LoadVisibleSets(CLoadDirectory rd);

	void ConvertPrevFormat();
};
