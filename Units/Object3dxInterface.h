#ifndef __OBJECT3DX_INTERFACE_H__
#define __OBJECT3DX_INTERFACE_H__

#include "SerializationTypes.h"

class StringIndexBase
{
public:
	StringIndexBase();
	void set(const char* name);

	bool serialize(Archive& ar, const char* name, const char* nameAlt);

	operator int() const { return index_; }
	const char* c_str() const { return name_; }

protected:
	ComboListString name_;
	int index_;

	virtual void update() = 0;
};

struct ChainName : public StringIndexBase
{
public:
	void update();
};

class AnimationGroupName : public StringIndexBase
{
public:
	void update();
};

class VisibilityGroupName : public StringIndexBase
{
public:
	void update();
};

struct VisibilitySetName : StringIndexBase
{
	void update();
};

class VisibilityGroupOfSet : public StringIndexBase
{
public:
	static void setVisibilitySet(int visibilitySet) { visibilitySet_ = visibilitySet; }
private:
	static int visibilitySet_;
	void update();
};

class Logic3dxNode : public StringIndexBase
{
public:
	void update() { updateInternal(true); }

protected:
	void updateInternal(bool logic);
};

class Object3dxNode : public Logic3dxNode
{
public:
	void update() { updateInternal(false); }
};

class Logic3dxNodeBound : public StringIndexBase
{
public:
	void update() { updateInternal(true); }

protected:
	void updateInternal(bool logic);
};

#endif //__OBJECT3DX_INTERFACE_H__
