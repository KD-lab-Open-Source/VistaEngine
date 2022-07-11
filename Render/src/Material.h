#ifndef _MATERIAL_H_
#define _MATERIAL_H_

struct cObjMaterial : public sAttribute
{
	sColor4f		Diffuse,Specular;
	sColor4f		Ambient;
	float			Power;
	cObjMaterial()
	{
		Diffuse.set(1,1,1,1);
		Specular.set(0,0,0,1);
		Ambient.set(0,0,0,0);
		Power=0;
	}
};

#endif // _MATERIAL_H_
