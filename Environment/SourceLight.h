#ifndef __SOURCE_LIGHT_H_INCLUDED__
#define __SOURCE_LIGHT_H_INCLUDED__

#include "SourceBase.h"
#include "Render\src\UnkLight.h"

class SourceLight : public SourceBase
{
public:

	SourceLight();
	SourceLight(const SourceLight& l);
	SourceBase* clone() const {
		return new SourceLight(*this);
	}
	~SourceLight();
	
	void serialize(Archive& ar);
	void quant();

	void setRadius(float _radius);
	void setPose (const Se3f& pos, bool init);
	
	SourceType type() const { return SOURCE_LIGHT; }

	//cUnkLight* getLight() { return pSpherLight; }

protected:
	
	void start();
	void stop();

private:
	void createLight(cScene* scene);

	cUnkLight* light_;
	float frequency_;
	sLightKey AnimKeys_[2];
	bool toObjects_;

	InterpolatorPose poseInterpolator_;
};

#endif //#ifndef __SOURCE_LIGHT_H_INCLUDED__

