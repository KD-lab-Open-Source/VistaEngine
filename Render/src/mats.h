#ifndef __MATS_H_INCLUDED__
#define __MATS_H_INCLUDED__
class Mats
{
public:
	QuatF q;
	Vect3f d;
	float s;
public:
	void Identify();
	const QuatF&  rot()   const {return q;}
	const Vect3f& trans() const {return d;}
	const float& scale()  const {return s;}
	QuatF&  rot()	    {return q;}
	Vect3f& trans()	    {return d;}
	float& scale()		{return s;}
	Se3f& se(){return *(Se3f*)&q;}
	const Se3f& se()const {return *(const Se3f*)&q;}

	void mult(const Mats& t,const Mats& u);
	void copy_right(MatXf& mat) const;
	void set(MatXf mat);//Предполагается что в матрице изотропный скэйлинг.
};

void slerp_fast(QuatF& out,const QuatF& a,const QuatF& b,float t);
void lerp(QuatF& out,const QuatF& a,const QuatF& b,float t);

#endif
