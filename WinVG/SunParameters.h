class SunParameters
{
public:

	CKeyColor ambient_,diffuse_,specular_;
	CKeyPos direction_;

SunParameters()
{
	#include "sun.inl"
	ambient_.resize(size+1);
	diffuse_.resize(size+1);
	specular_.resize(size+1);
	direction_.resize(size+1);
	for(int i=0;i<=size;i++)
	{
		float time=i/float(size);
		int in=i;
		if(i==size)
			in=0;
		ambient_[i].time=time;
		diffuse_[i].time=time;
		specular_[i].time=time;
		direction_[i].time=time;

		ambient_[i].Val()=ambient[in];
		diffuse_[i].Val()=diffuse[in];
		specular_[i].Val()=specular[in];
		direction_[i].Val()=direction[in];
	}
}

void Set(float time)
{
	if (!gb_Scene)
		return;
	xassert(time>=0 && time<=24);
	float f=time/24;

	Vect3d light_vector=direction_.Get(f);
	sColor4f ambient_color=ambient_.Get(f);
	sColor4f diffuse_color=diffuse_.Get(f);
	sColor4f specular_color=specular_.Get(f);

	if(light_vector.z>-0.01f)
		light_vector.z=-0.01f;

	light_vector.Normalize();
	gb_Scene->SetSun(light_vector,ambient_color,diffuse_color,specular_color);
}

};
