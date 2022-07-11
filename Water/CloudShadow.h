#ifndef __CLOUD_SHADOW_H_INCLUDED__
#define __CLOUD_SHADOW_H_INCLUDED__

class cCloudShadow : public cBaseGraphObject
{
public:
	cCloudShadow();
	~cCloudShadow();
	void Draw(cCamera *pCamera);
	void Animate(float dt);
	void PreDraw(cCamera *pCamera);
	virtual const MatXf& GetPosition() const {return MatXf::ID;}
//	void serializeColor(Archive& ar);
	void serialize(Archive& ar);
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
//	void SetColor(const sColor4c& c);
	void SetTexture(const string& tex1);
	void SetTexels();

	class VSCloudShadow* vsCloudShadow;
	class PSCloudShadow* psCloudShadow;
};

#endif
