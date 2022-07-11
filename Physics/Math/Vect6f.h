#ifndef __VECT6F_H__
#define __VECT6F_H__

///////////////////////////////////////////////////////////////
//
//    class Vect6f
//
///////////////////////////////////////////////////////////////

#include "Serialization\Serialization.h"

class Vect6f
{
public:	
	Vect6f(){}
	Vect6f(const Vect6f& b) : x_(b.x_), y_(b.y_), z_(b.z_), a_(b.a_), b_(b.b_), c_(b.c_) {}
	Vect6f(const Vect3f& l, const Vect3f& a) : x_(l.x), y_(l.y), z_(l.z), a_(a.x), b_(a.y), c_(a.z) {}
	Vect6f(float x, float y, float z, float a, float b, float c) : x_(x), y_(y), z_(z), a_(a), b_(b), c_(c) {}
	float dot(const Vect6f& b) const { return x_ * b.x_ + y_ * b.y_ + z_ * b.z_ + a_ * b.a_ + b_ * b.b_ + c_ * b.c_; }
	const Vect6f& operator += (const Vect6f& b)
	{
		x_ += b.x_;
		y_ += b.y_;
		z_ += b.z_;
		a_ += b.a_;
		b_ += b.b_;
		c_ += b.c_;
		return *this;
	}
	const Vect6f& operator /= (float b)
	{
		x_ /= b;
		y_ /= b;
		z_ /= b;
		a_ /= b;
		b_ /= b;
		c_ /= b;
		return *this;
	}
	const Vect6f& scaleAdd(const Vect6f& b, float c)
	{
		x_ += b.x_ * c;
		y_ += b.y_ * c;
		z_ += b.z_ * c;
		a_ += b.a_ * c;
		b_ += b.b_ * c;
		c_ += b.c_ * c;
		return *this;
	}
	const Vect6f& operator *= (const Vect2f& b)
	{
		x_ *= b.x;
		y_ *= b.x;
		z_ *= b.x;
		a_ *= b.y;
		b_ *= b.y;
		c_ *= b.y;
		return *this;
	}
	const Vect3f& linear() const {	return *(Vect3f*)&x_;	}
	const Vect3f& angular() const { return *(Vect3f*)&a_; }
	Vect3f& linear() { return *(Vect3f*)&x_; }
	Vect3f& angular() { return *(Vect3f*)&a_; }
	void serialize(Archive& ar)
	{
		ar.serialize(linear(), "linear", "linear");
		ar.serialize(angular(), "angular", "angular");
	}


private:
	float x_, y_, z_, a_, b_, c_;

public:
	static const Vect6f ZERO;
	static const Vect6f ID;
};

///////////////////////////////////////////////////////////////
//
//    class VectN2f
//
///////////////////////////////////////////////////////////////

class VectN2f : public vector<Vect2f>
{
public:
	VectN2f(int n) : vector<Vect2f>(n) {}
	VectN2f(int n, const Vect2f& a) : vector<Vect2f>(n, a) {}
	VectN2f& operator = (const Vect2f& b)
	{
		iterator i;
		FOR_EACH(*this, i)
			*i = b;
		return *this;
	}
	float dot(const VectN2f& b) const
	{
		dassert(size() == b.size());
		float a = 0.0f;
		int n = size();
		for(int i = 0; i < n; ++i)
			a += operator[](i).dot(b[i]);
		return a;
	}
	VectN2f& operator += (const VectN2f& b)
	{
		dassert(size() == b.size());
		int n = size();
		for(int i = 0; i < n; ++i)
			operator[](i) += b[i];
		return *this;
	}
	VectN2f& operator /= (float b)
	{
		iterator i;
		FOR_EACH(*this, i)
			*i /= b;
		return *this;
	}
	VectN2f& operator *= (const Vect2f& b)
	{
		iterator i;
		FOR_EACH(*this, i)
			*i *= b;
		return *this;
	}
	VectN2f& scaleAdd(const VectN2f& b, float c)
	{
		dassert(size() == b.size());
		int n = size();
		for(int i = 0; i < n; ++i)
			operator[](i).scaleAdd(b[i], c);
		return *this;
	}
	bool serialize(Archive& ar, const char* name, const char* nameAlt)
	{
		return ar.serialize(static_cast<vector<Vect2f>&>(*this), name, nameAlt);
	}
};

///////////////////////////////////////////////////////////////
//
//    class Vect6fN2f
//
///////////////////////////////////////////////////////////////

class Vect6fN2f
{
public:
	class Vect6f2f;

	Vect6fN2f(const Vect3f& a, const Vect3f& b, int n) : vect6f_(a, b), vectN2f_(n, Vect2f::ZERO) {}
	Vect6fN2f(const Vect6f& a, int n) : vect6f_(a), vectN2f_(n, Vect2f::ZERO) {}
	Vect6fN2f(const Vect6f2f& b, int n) : vect6f_(b.vect6f_), vectN2f_(n, b.vect2f_) {}
	Vect6f& vect6f() { return vect6f_; }
	const Vect6f& vect6f() const { return vect6f_; }
	VectN2f& vectN2f() { return vectN2f_; }
	const VectN2f& vectN2f() const { return vectN2f_; }
	float dot(const Vect6fN2f& c) const
	{
		return vect6f_.dot(c.vect6f_) + vectN2f_.dot(c.vectN2f_);
	}
	Vect6fN2f& operator = (const Vect6f2f& b)
	{
		vect6f_ = b.vect6f_;
		vectN2f_ = b.vect2f_;
		return *this;
	}
	Vect6fN2f& operator += (const Vect6fN2f& b)
	{
		vect6f_ += b.vect6f_;
		vectN2f_ += b.vectN2f_;
		return *this;
	}
	Vect6fN2f& operator /= (float b)
	{
		vect6f_ /= b;
		vectN2f_ /= b;
		return *this;
	}
	Vect6fN2f& scaleAdd(const Vect6fN2f& b, float c)
	{
		vect6f_.scaleAdd(b.vect6f_, c);
		vectN2f_.scaleAdd(b.vectN2f_, c);
		return *this;
	}
	const Vect6fN2f& operator *= (const Vect2f& b)
	{
		vect6f_ *= b;
		vectN2f_ *= b;
		return *this;
	}
	void serialize(Archive& ar)
	{
		ar.serialize(vect6f_, "Vect6f", "Vect6f");
		ar.serialize(vectN2f_, "VectN2f", "VectN2f");
	}

	class Vect6f2f
	{
	public:
		Vect6f2f(const Vect6f& a, const Vect2f& b) : vect6f_(a), vect2f_(b) {}

		friend Vect6fN2f::Vect6fN2f(const Vect6f2f& b, int n);
		friend Vect6fN2f& Vect6fN2f::operator = (const Vect6f2f& b);

	private:
		Vect6f vect6f_;
		Vect2f vect2f_;
	};

private:
	Vect6f vect6f_;
	VectN2f vectN2f_;

public:
	static const Vect6f2f ZERO;
};

///////////////////////////////////////////////////////////////
//
//    class MassMatrixBase
//
///////////////////////////////////////////////////////////////

class MassMatrixBase
{
public:
	MassMatrixBase() :
		mass_(1.0f)
	{
	}
	void setMass(float mass) { mass_ = mass; }
	float mass() const { return mass_; }
	void square(const MassMatrixBase& m)
	{
		mass_ = m.mass_;
		mass_ *= mass_;
	}
	MassMatrixBase& invert(const MassMatrixBase& m)
	{
		mass_ = 1.0f / m.mass_;
		return *this;
	}
	const Vect3f& multiply(Vect3f& a, const Vect3f& b) const
	{
		a.scale(b, mass());
		return a;
	}
	void computePermanentForce(Vect3f& force, const Vect3f& gravity) const
	{
		force.scale(gravity, mass_);
	}

private:
	float mass_;
};

///////////////////////////////////////////////////////////////
//
//    class TOIFull
//
///////////////////////////////////////////////////////////////

class TOIFull
{
public:
	TOIFull() :
		toiDiagonal_(Vect3f::ID)
	{
	}
	void setTOI(const Vect3f& toi) 
	{ 
		toiDiagonal_ = toi;
		checkTOI();
	}
	const Vect3f& toiDiagonal() const { return toiDiagonal_; }
	void invert(const TOIFull& m)
	{
		toiDiagonal_.x = 1.0f / m.toiDiagonal_.x;
		toiDiagonal_.y = 1.0f / m.toiDiagonal_.y;
		toiDiagonal_.z = 1.0f / m.toiDiagonal_.z;
		checkTOI();
	}
	void computeTOIWorld(const Mat3f& r)
	{
		toi_[0] = toiDiagonal_.x * r.xx * r.xx + toiDiagonal_.y * r.xy * r.xy + toiDiagonal_.z * r.xz * r.xz;
		toi_[1] = toiDiagonal_.x * r.xx * r.yx + toiDiagonal_.y * r.xy * r.yy + toiDiagonal_.z * r.xz * r.yz;
		toi_[2] = toiDiagonal_.x * r.xx * r.zx + toiDiagonal_.y * r.xy * r.zy + toiDiagonal_.z * r.xz * r.zz;
		toi_[3] = toiDiagonal_.x * r.yx * r.yx + toiDiagonal_.y * r.yy * r.yy + toiDiagonal_.z * r.yz * r.yz;
		toi_[4] = toiDiagonal_.x * r.zx * r.yx + toiDiagonal_.y * r.zy * r.yy + toiDiagonal_.z * r.zz * r.yz;
		toi_[5] = toiDiagonal_.x * r.zx * r.zx + toiDiagonal_.y * r.zy * r.zy + toiDiagonal_.z * r.zz * r.zz;
	}
	void multiply(Vect3f& a, const Vect3f& b) const
	{
		a.x = b.x * toi_[0] + b.y * toi_[1] + b.z * toi_[2];
		a.y = b.x * toi_[1] + b.y * toi_[3] + b.z * toi_[4];
		a.z = b.x * toi_[2] + b.y * toi_[4] + b.z * toi_[5];
	}
	void square(const TOIFull& m)
	{
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

protected:
	void checkTOI()
	{
		if(fabsf(toiDiagonal_.x) < 1.e-5f)
			toiDiagonal_.x = 1.e-5f;
		if(fabsf(toiDiagonal_.y) < 1.e-5f)
			toiDiagonal_.y = 1.e-5f;
		if(fabsf(toiDiagonal_.z) < 1.e-5f)
			toiDiagonal_.z = 1.e-5f;
	}

	Vect3f toiDiagonal_;
	float toi_[6];
};

///////////////////////////////////////////////////////////////
//
//    class MassMatrix6f
//
///////////////////////////////////////////////////////////////

class MassMatrix6f
{
public:
	void setMass(float mass) { mass_.setMass(mass); }
	void setTOI(const Vect3f& toi) { toi_.setTOI(toi); }
	float mass() const { return mass_.mass(); }
	const Vect6f& multiply(Vect6f& a, const Vect6f& b) const
	{
		mass_.multiply(a.linear(), b.linear());
		toi_.multiply(a.angular(), b.angular());
		return a;
	}
	void square(const MassMatrix6f& m)
	{
		mass_.square(m.mass_);
		toi_.square(m.toi_);
	}
	MassMatrix6f& invert(const MassMatrix6f& m)
	{
		mass_.invert(m.mass_);
		toi_.invert(m.toi_);
		return *this;
	}
	void computePermanentForce(Vect6f& force, const Mat3f& r, const Vect3f& gravity, const Vect6f& velocity, bool coriolisForce)
	{
		mass_.computePermanentForce(force.linear(), gravity);
		if(coriolisForce){
			toi_.computeTOIWorld(r);
			toi_.multiply(force.angular(), velocity.angular());
			force.angular().postcross(velocity.angular());
		}else
			force.angular() = Vect3f::ZERO;
	}
	void computeTOIWorld(const Mat3f& r)
	{
		toi_.computeTOIWorld(r);
	}
	void evolve(Se3f& pose, Vect6f& velocity, float dt, bool angularEvolve) const
	{
		pose.trans().scaleAdd(velocity.linear(), dt);
		if(angularEvolve){
			float absq = velocity.angular().norm2();
			if(absq > 1.e-3f){
				absq = sqrtf(absq);
				QuatF q_local;
				float theta = absq * dt / 2.0f;
				float normsin = sinf(theta) / absq;
				q_local.s() = cosf(theta);
				q_local.x() = velocity.angular().x * normsin;
				q_local.y() = velocity.angular().y * normsin;
				q_local.z() = velocity.angular().z * normsin;
				pose.rot().premult(q_local);
				pose.rot().normalize();
			}
		}
	}
	static Vect6f computeJ(const Vect3f& axis, const Vect3f& local_point)
	{
		return Vect6f(axis,	Vect3f().cross(local_point, axis));
	}

protected:
	MassMatrixBase mass_;
	TOIFull toi_;
};

///////////////////////////////////////////////////////////////
//
//    class MassMatrix
//
///////////////////////////////////////////////////////////////

template<class MassMatrixXf, class VectXf>
class MassMatrixNXf : public MassMatrixXf
{
public:
	MassMatrixNXf(const MassMatrixXf& massMatrixZero) :
		MassMatrixXf(massMatrixZero),
		inverseTOI_(massMatrixZero),
		gravity_(Vect3f::ZERO),
		damping_(Vect2f::ID)
	{
	}
	void computePermanentForce(VectXf& force, const Mat3f& r, const VectXf& velocity, bool coriolisForce)
	{
		__super::computePermanentForce(force, r, gravity_, velocity, coriolisForce);
		inverseTOI_.computeTOIWorld(r);
	}
	void setGravityZ(float gravityZ) { gravity_.z = gravityZ; }
	void setDamping(float linearDamping, float angularDamping)
	{
		damping_.x = 1.0f - linearDamping;
		damping_.y = 1.0f - angularDamping;
	}
	const VectXf& multiply(VectXf& a, const VectXf& b) const
	{
		return inverseTOI_.multiply(a, b);
	}
	void evolve(Se3f& pose, VectXf& velocity, float dt, bool angularEvolve) const
	{
		velocity *= damping_;
		__super::evolve(pose, velocity, dt, angularEvolve);
	}
	
protected:
	MassMatrixXf inverseTOI_;
	Vect3f gravity_;
	Vect2f damping_;
};

///////////////////////////////////////////////////////////////
//
//    class MassMatrixFull
//
///////////////////////////////////////////////////////////////

class MassMatrixFull : public MassMatrixNXf<MassMatrix6f, Vect6f>
{
public:
	MassMatrixFull() :
		MassMatrixNXf<MassMatrix6f, Vect6f>(MassMatrix6f())
	{
	}

	void setTOI(const Vect3f& toi)
	{
		__super::setTOI(toi);
		inverseTOI_.invert(*this);
	}

	static const MassMatrixFull ZERO;
};

#endif // __VECT6F_H__