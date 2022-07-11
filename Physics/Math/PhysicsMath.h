#ifndef __PHYSICS_MATH_H__
#define __PHYSICS_MATH_H__

class Archive;

///////////////////////////////////////////////////////////////
//
//    class MassMatrix
//
///////////////////////////////////////////////////////////////

class MassMatrix
{
public:
	MassMatrix() {}
	MassMatrix(float mass, const Mat3f& toi) { set(mass, toi); }
	void set(float mass, const Mat3f& toi)
	{
		mass_ = mass;
		toi_[0] = toi[0][0];
		toi_[1] = toi[0][1];
		toi_[2] = toi[0][2];
		toi_[3] = toi[1][1];
		toi_[4] = toi[1][2];
		toi_[5] = toi[2][2];
	}
	void multiply(Vect3f& a,const Vect3f& b) const
	{
		a.x = b.x * toi_[0] + b.y * toi_[1] + b.z * toi_[2];
		a.y = b.x * toi_[1] + b.y * toi_[3] + b.z * toi_[4];
		a.z = b.x * toi_[2] + b.y * toi_[4] + b.z * toi_[5];
	}
	float getMass() const { return mass_;	}
	float operator [] (int i) const
	{
		xassert(i >= 0 && i < 6);
		return toi_[i];
	}
	void square(MassMatrix m)
	{
		mass_ = m.mass_;
		mass_ *= mass_;
		toi_[1] = (m.toi_[0] + m.toi_[3]) * m.toi_[1] + m.toi_[2] * m.toi_[4];
		toi_[2] = (m.toi_[0] + m.toi_[5]) * m.toi_[2] + m.toi_[1] * m.toi_[4];
		toi_[4] = (m.toi_[3] + m.toi_[5]) * m.toi_[4] + m.toi_[1] * m.toi_[2];
		float t1, t2, t4;
		t1 = sqr(m.toi_[1]);
		t2 = sqr(m.toi_[2]);
		t4 = sqr(m.toi_[4]);
		toi_[0] = sqr(m.toi_[0]) + t1 + t2;
		toi_[3] = sqr(m.toi_[3]) + t1 + t4;
		toi_[5] = sqr(m.toi_[5]) + t2 + t4;
	}
private:
	float mass_;
	float toi_[6];
};

///////////////////////////////////////////////////////////////
//
//    class Vect6f
//
///////////////////////////////////////////////////////////////

class Vect6f
{
public:	
	Vect6f(){}
	Vect6f(const Vect6f& b)
	{
		for(int i = 0; i < 6; ++i)
			vect_[i] = b[i];
	}
	Vect6f(float x, float y, float z, float a, float b, float c)
	{
		vect_[0] = x;
		vect_[1] = y;
		vect_[2] = z;
		vect_[3] = a;
		vect_[4] = b;
		vect_[5] = c;
	}
	~Vect6f(){}
	float& operator [] (int i)
	{
		xassert(i >= 0 && i < 6);
		return vect_[i];
	}
	float operator [] (int i) const
	{
		xassert(i >= 0 && i < 6);
		return vect_[i];
	}
	float operator * (const Vect6f& c) const
	{
		float a = 0;
		for(int i = 0; i < 6; ++i)
			a += vect_[i] * c.vect_[i];
		return a;
	}
	Vect6f& operator += (const Vect6f& c)
	{
		for(int i = 0; i < 6; ++i)
			vect_[i] += c.vect_[i];
		return *this;
	}
	Vect6f& operator -= (const Vect6f& b)
	{
		for(int i = 0; i < 6; ++i)
			vect_[i] -= b.vect_[i];
		return *this;
	}
	Vect6f& operator *= (float b)
	{
		for(int i = 0; i < 6 ; ++i)
			vect_[i] *= b;
		return *this;
	}
	Vect6f& operator /= (float b)
	{
		xassert(b != 0);
		for(int i = 0; i < 6; ++i)
			vect_[i] /= b;
		return *this;
	}
	Vect6f& addTimes(const Vect6f& b, float c)
	{
		for(int i = 0; i < 6; ++i)
			vect_[i] += b.vect_[i] * c;
		return *this;
	}
	Vect6f& setTimes(const Vect6f& b, float c)
	{
		for(int i = 0; i < 6; ++i)
			vect_[i] = b.vect_[i] * c;
		return *this;
	}
	Vect6f& set(const Vect3f& b,const Vect3f& c)
	{
		*(Vect3f*)vect_ = b;
		*(Vect3f*)&vect_[3] = c;
		return *this;
	}
	void setLinear(const Vect3f& c) {	*(Vect3f*)vect_ = c; }
	void setAngular(const Vect3f& c) { *(Vect3f*)&vect_[3] = c; }
	const Vect3f& getLinear() const {	return *(Vect3f*)vect_;	}
	const Vect3f& getAngular() const { return *(Vect3f*)&vect_[3]; }
	Vect3f& linear() { return *(Vect3f*)vect_; }
	Vect3f& angular() { return *(Vect3f*)&vect_[3]; }
	float norm2() const
	{
		return vect_[0] * vect_[0] + vect_[1] * vect_[1] + vect_[2] * vect_[2]+
			   vect_[3] * vect_[3] + vect_[4] * vect_[4] + vect_[5] * vect_[5];
	}
	void multiply(const MassMatrix& b, const Vect6f& c)
	{
		for(int i = 0; i < 3; ++i)
			vect_[i] = b.getMass() * c.vect_[i];
		vect_[3] = c.vect_[3] * b[0] + c.vect_[4] * b[1] + c.vect_[5] * b[2];
		vect_[4] = c.vect_[3] * b[1] + c.vect_[4] * b[3] + c.vect_[5] * b[4];
		vect_[5] = c.vect_[3] * b[2] + c.vect_[4] * b[4] + c.vect_[5] * b[5];
	}
	float computeEta(const Vect6f& v, const Vect6f& a_ex, float dt) const
	{
		float eta = 0;
		for(int i = 0; i < 6; ++i)
			eta += vect_[i] * (v.vect_[i] / dt + a_ex.vect_[i]);
		return eta;
	}

	void serialize(Archive& ar);

private:
	float vect_[6];

public:
	static const Vect6f ZERO;
	static const Vect6f ID;
};

///////////////////////////////////////////////////////////////
//
//    class VectNf
//
///////////////////////////////////////////////////////////////

class VectNf : public vector<float>
{
public:
	VectNf() {}
	VectNf(int n) {	resize(n); }
};

///////////////////////////////////////////////////////////////
//
//    class VectN3f
//
///////////////////////////////////////////////////////////////

class VectN3f : public vector<Vect3f>
{
public:
	VectN3f() {}
	VectN3f(int n) { resize(n); }
};

///////////////////////////////////////////////////////////////
//
//    class VectN6f
//
///////////////////////////////////////////////////////////////

class VectN6f : public vector<Vect6f>
{
public:
	VectN6f() {}
	VectN6f(int n) { resize(n);	}
	VectN6f& operator *= (float b)
	{
		iterator i;
		FOR_EACH(*this, i)
			*i *= b;
		return *this;
	}
	VectN6f& operator = (const Vect6f& b)
	{
		iterator i;
		FOR_EACH(*this, i)
			*i = b;
		return *this;
	}
};

///////////////////////////////////////////////////////////////
//
//    class SparsityRowSimple
//
///////////////////////////////////////////////////////////////

class SparsityRowSimple
{
public:
	void setMatrix0(const Vect6f& matrix0) { matrix0_ = matrix0; }
	void setMatrix1(const Vect6f& matrix1) { matrix1_ = matrix1; }
	void setMap0(int map0) { map0_ = map0; }
	void setMap1(int map1) { map1_ = map1; }
	Vect6f& matrix0() { return matrix0_; }
	Vect6f& matrix1() { return matrix1_; }
	const Vect6f& matrix0() const { return matrix0_; }
	const Vect6f& matrix1() const { return matrix1_; }
	int map0() const { return map0_; }
	int map1() const { return map1_; }

private:
	Vect6f matrix0_;
	Vect6f matrix1_;
	int map0_;
	int map1_;
};

///////////////////////////////////////////////////////////////
//
//    class SparsityRowEntry
//
///////////////////////////////////////////////////////////////

class SparsityRowEntry
{
public:
	SparsityRowEntry(int map, const Mat3f& mLinear, const Mat3f& mAngular) : 
		map_(map),
		mLinear_(mLinear),
		mAngular_(mAngular)
	{		
	}
	void add(const Mat3f& mLinear, const Mat3f& mAngular)
	{
		mLinear_.add(mLinear);
		mAngular_.add(mAngular);
	}
	Vect6f& mult(Vect6f& vect) const
	{
		mLinear_.xform(vect.linear());
		mAngular_.xform(vect.angular());
		return vect;
	}
	bool operator==(int map) const
	{
		return map_ == map;
	}
	int map() const { return map_; }

private:
	Mat3f mLinear_;
	Mat3f mAngular_;
	int map_;
};

///////////////////////////////////////////////////////////////
//
//    class SparsityRow
//
///////////////////////////////////////////////////////////////

class SparsityRow : public vector<SparsityRowEntry>
{
public:
	void addDiagonalElement(const Mat3f& mLinear, const Mat3f& mAngular = Mat3f::ZERO)
	{
		if(empty()){
			inverseTOIWorldSqr_.square(inverseTOIWorld_);
			push_back(SparsityRowEntry(-1, mLinear, mAngular));
		}else
			begin()->add(mLinear, mAngular);
	}
	void addElement(int map, const Mat3f& mLinear, const Mat3f& mAngular = Mat3f::ZERO)
	{
		xassert(!empty());
		iterator i(find(begin(), end(), map));
		if(i != end())
			i->add(mLinear, mAngular);
		else
			push_back(SparsityRowEntry(map, mLinear, mAngular));
	}
	void computeAccelerationExternal(Vect6f& accelerationExternal) const
	{
		accelerationExternal.multiply(inverseTOIWorld_, force_);
	}
	void mult(Vect6f& b, const_iterator entry, const Vect6f& matrix, float dt) const
	{
		Vect6f t1(matrix), t2;
		entry->mult(t1);
		t2.multiply(inverseTOIWorldSqr(), t1);
		b.addTimes(t2, dt);
	}
	void multDiag(Vect6f& b, const Vect6f& matrix, float dt) const
	{
		b.multiply(inverseTOIWorld(), matrix);
		if(!empty())
			mult(b, begin(), matrix, dt);
	}
	void multNonDiag(Vect6f& b, int map, const Vect6f& matrix, float dt)
	{
		iterator i(find(begin(), end(), map));
		if(i != end())
			mult(b, i, matrix, dt);
	}
	Vect6f& force() { return force_; }
	const Vect6f& force() const { return force_; }
	MassMatrix& inverseTOIWorld() { return inverseTOIWorld_; }
	const MassMatrix& inverseTOIWorld() const { return inverseTOIWorld_; }
	const MassMatrix& inverseTOIWorldSqr() const { return inverseTOIWorldSqr_; }

private:
	Vect6f force_;
	MassMatrix inverseTOIWorld_;
	MassMatrix inverseTOIWorldSqr_;
};

///////////////////////////////////////////////////////////////
//
//    class SparsityMatrix
//
///////////////////////////////////////////////////////////////

class ConstraintHandler;

class SparsityMatrix : public vector<SparsityRow>
{
public:
	SparsityMatrix() {}
	SparsityMatrix(int s) { resize(s); }
	void computeAccelerationExternal(VectN6f& accelerationExternal) const
	{
		const_iterator i;
		VectN6f::iterator idv = accelerationExternal.begin();
		FOR_EACH(*this, i){
			i->computeAccelerationExternal(*idv);
			if(!i->empty()){
				SparsityRow::const_iterator iRow = i->begin();
				Vect6f f(i->force());
				*idv += iRow->mult(f);
				for(++iRow; iRow != i->end(); ++iRow){
					f = operator[](iRow->map()).force();
					*idv += iRow->mult(f);
				}
			}
			++idv;
		}
	}
	void computeBMatrix(ConstraintHandler& constraintHandler, float dt);
};

#endif // __PHYSICS_MATH_H__
