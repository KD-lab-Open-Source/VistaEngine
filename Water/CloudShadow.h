#ifndef __CLOUD_SHADOW_H_INCLUDED__
#define __CLOUD_SHADOW_H_INCLUDED__

#include "Render\inc\IVisGenericInternal.h"
#include "Render\inc\IRenderDevice.h"

class cCloudShadow : public BaseGraphObject
{
public:
	cCloudShadow();
	~cCloudShadow();
	void Draw(Camera* camera);
	void Animate(float dt);
	void PreDraw(Camera* camera);
//	void serializeColor(Archive& ar);
	void serialize(Archive& ar);
	int sortIndex() const { return -1; }

protected:
	cTexture* texture1;
	Vect2f uv1;
	Vect2f uv2;
	float du,dv;
	typedef sVertexXYZDT2 VType;
	sPtrVertexBuffer earth_vb;
	sPtrIndexBuffer earth_ib;
	string tex1_name;
	BYTE color;

	float rotate_angle;
//	void SetColor(const Color4c& c);
	void SetTexture(const string& tex1);
	void SetTexels();

	class VSCloudShadow* vsCloudShadow;
	class PSCloudShadow* psCloudShadow;
};

#endif
