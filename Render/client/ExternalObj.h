#ifndef __EXTERNAL_OBJ_H_INCLUDED__
#define __EXTERNAL_OBJ_H_INCLUDED__

class cExternalObj : public cAnimUnkObj
{
	ExternalObjFunction func;
	bool sort_pass;
public:
	cExternalObj(ExternalObjFunction func);
	~cExternalObj();
	virtual void PreDraw(cCamera *UCamera);
	virtual void Draw(cCamera *UClass);

	void SetSortPass(bool p){sort_pass=p;};
};


#endif
