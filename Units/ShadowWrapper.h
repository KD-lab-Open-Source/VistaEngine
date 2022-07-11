#ifndef __SHADOW_WRAPPER_H_INCLUDED__
#define __SHADOW_WRAPPER_H_INCLUDED__

#include "UnitAttribute.h"

class ShadowWrapper
{
public:
	ShadowWrapper() { started = false; }
	void addRef(c3dx* model);
	void decRef(c3dx* model);
	void setForModel(c3dx* model, ShadowType shadowType, float shadowRadiusRelative);
	void serializeForModel(Archive& ar, c3dx* model, float factor);
	void serialize(Archive& ar);
	
private:
	class ModelShadowWithRef : public ModelShadow
	{
	public:
		ModelShadowWithRef()
		{
			model_ = 0;
			refCounter_ = 0;
		}
		void setModel(c3dx* model) { model_ = model; }
		bool active() { return refCounter_; }
		bool addRef()
		{
			++refCounter_;
			return refCounter_ == 1;
		}
		void decRef() {	--refCounter_; }
		void apply()
		{
			xassert(refCounter_ && model_);
			if(model_)
				setShadowType(model_);
			model_ = 0;
		}

	private:
		c3dx* model_;
		int refCounter_;
	};

	typedef StaticMap<string, ModelShadowWithRef> ModelShadows;
	ModelShadows modelShadows_;
	bool started;
};

#endif // __SHADOW_WRAPPER_H_INCLUDED__