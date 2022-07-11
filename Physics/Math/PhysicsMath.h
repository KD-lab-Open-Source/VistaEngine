#ifndef __PHYSICS_MATH_H__
#define __PHYSICS_MATH_H__

#include "Physics\RigidBodyPhysics.h"

///////////////////////////////////////////////////////////////
//
//    class Limits
//
///////////////////////////////////////////////////////////////

class Limits
{
public:
	Limits() {}

	Limits(float lowerLimit, float upperLimit) :
		lowerLimit_(lowerLimit), 
		upperLimit_(upperLimit)
	{
		xassert(lowerLimit_ <= upperLimit_);
	}

	Limits(float limit) :
		lowerLimit_(-limit), 
		upperLimit_(limit)
	{
	}

	void apply(float& a) const
	{
		a = max(lowerLimit_, min(a, upperLimit_));
	}

	Limits& operator = (float limit)
	{
		lowerLimit_ = -limit;
		upperLimit_ = limit;
		return *this;
	}

private:
	float lowerLimit_;
	float upperLimit_;

public:
	static const Limits GREATER_ZERO;
	static const Limits LOWER_ZERO;
	static const Limits ZERO;
	static const Limits FREE;
};

///////////////////////////////////////////////////////////////
//
//    class SparsityRowBase
//
///////////////////////////////////////////////////////////////

template<class VectXf, class MassMatrix>
class SparsityRowBase
{
public:
	SparsityRowBase(RigidBodyPhysicsCore<VectXf, MassMatrix>* body, const VectXf& j) :
		body_(body),
		j_(j),
		b_(j)
	{
	}
	float computeEta(float dt) const
	{
		return body_->computeEta(j_, dt);
	}
	float computeDet()
	{
		return body_->computeDet(b_, j_);
	}
	void preciseDV(float lambda) const
	{
		body_->preciseDV(b_, lambda);
	}
	void preciseAccelerationExternal(float lambda) const
	{
		body_->preciseAccelerationExternal(b_, lambda);
	}
	float preciseEta() const
	{
		return body_->preciseEta(j_);
	}
	
protected:
	VectXf j_;
	VectXf b_;
	RigidBodyPhysicsCore<VectXf, MassMatrix>* body_;
};

///////////////////////////////////////////////////////////////
//
//    class SparsityRow
//
///////////////////////////////////////////////////////////////

template<class VectXf, class MassMatrix>
class SparsityRow : public SparsityRowBase<VectXf, MassMatrix>
{
public:
	SparsityRow(RigidBodyPhysicsCore<VectXf, MassMatrix>* body0, RigidBodyPhysicsCore<VectXf, MassMatrix>* body1,
		const VectXf& j0, const VectXf& j1) :
		SparsityRowBase<VectXf, MassMatrix>(body0, j0),
		matrix_(body1, j1)
	{
	}
	float computeEta(float dt) const
	{
		return __super::computeEta(dt) + matrix_.computeEta(dt);
	}
	float computeDet()
	{
		return __super::computeDet() + matrix_.computeDet();
	}
	void preciseDV(float lambda) const
	{
		__super::preciseDV(lambda);
		matrix_.preciseDV(lambda);
	}
	void preciseAccelerationExternal(float lambda) const
	{
		__super::preciseAccelerationExternal(lambda);
		matrix_.preciseAccelerationExternal(lambda);
	}
	float preciseEta() const
	{
		return __super::preciseEta() + matrix_.preciseEta();
	}
		
private:
	SparsityRowBase<VectXf, MassMatrix> matrix_;
};

///////////////////////////////////////////////////////////////
//
//    class IterateQuantitiesBase
//
///////////////////////////////////////////////////////////////

template<class SparsityRow>
class IterateQuantitiesFriction;

class IterateQuantitiesBase
{
public:
	IterateQuantitiesBase(const Limits& lambdaLimits, float eta) :
		lambdaLimits_(lambdaLimits),
		eta_(eta)
	{
	}
	IterateQuantitiesBase(float phi, float lowerLimits, float upperLimits, float relaxationTime)
	{
		xassert(upperLimits >= lowerLimits);
		if(phi <= lowerLimits){
			lambdaLimits_ = Limits::LOWER_ZERO;
			eta_ = (phi - lowerLimits) / relaxationTime;
		}else if(phi >= upperLimits){
			lambdaLimits_ = Limits::GREATER_ZERO;
			eta_ = (phi - upperLimits) / relaxationTime;
		}else{
			lambdaLimits_ = Limits::ZERO;
			eta_ = 0.0f;
		}
	}
	virtual void quant(vector<IterateQuantitiesBase*>::iterator it) = 0;
	virtual void preciseDV() const = 0;
	virtual void update(float dt) = 0;
	
protected:
	Limits lambdaLimits_;
	float eta_;

	friend class IterateQuantitiesFriction;
};

///////////////////////////////////////////////////////////
//
//    class PhysicsPoolBase
//
///////////////////////////////////////////////////////////

class PhysicsPoolBase
{
public:
	static void freeAll()
	{
		vector<PhysicsPoolBase*>::iterator i;
		FOR_EACH(children_, i)
			(*i)->destruct();
	}

protected:
	PhysicsPoolBase() : free_(0)
	{
		children_.push_back(this);
	}
	PhysicsPoolBase(const PhysicsPoolBase& t);
	virtual void destruct() = 0;

	int free_;

private:
	static vector<PhysicsPoolBase*> children_;
};

///////////////////////////////////////////////////////////
//
//    class PhysicsPool
//
///////////////////////////////////////////////////////////

template<class T>
class PhysicsPool : PhysicsPoolBase
{
public:
	~PhysicsPool()
	{
		destruct();
		vector<void*>::iterator it;
		FOR_EACH(pool_, it)
			free(*it);
	}
	static void* get()
	{
		return Singleton<PhysicsPool<T> >::instance().getElement();
	}

private:
	void destruct()
	{
		vector<void*>::iterator end = pool_.begin() + free_;
		for(vector<void*>::iterator it = pool_.begin(); it < end; ++it)
			static_cast<T*>(*it)->~T();
		free_ = 0;
	}
	void* getElement()
	{
		if(free_ == pool_.size()){
			int size  = 10;
			pool_.resize(pool_.size() + size);
			for(vector<void*>::iterator it = pool_.end() - size; it < pool_.end(); ++it)
				*it = malloc(sizeof(T));
		}
		return pool_[free_++];
	}

	friend class Singleton<PhysicsPool<T> >;

	vector<void*> pool_;
};

///////////////////////////////////////////////////////////////
//
//    class IterateQuantities
//
///////////////////////////////////////////////////////////////

template<class SparsityRow>
class IterateQuantities : public IterateQuantitiesBase
{
public:
	IterateQuantities(const SparsityRow& j, float restitution, const Limits& lambdaLimits, float eta) :
		IterateQuantitiesBase(lambdaLimits, eta),
		j_(j),
		lambda_(0.0f),
		restitutionInv_(1.0f / (restitution + 1.0f))
	{
	}
	IterateQuantities(const SparsityRow& j, float phi, float lowerLimits, 
		float upperLimits, float relaxationTime) :
		IterateQuantitiesBase(phi, lowerLimits, upperLimits, relaxationTime),
		j_(j),
		lambda_(0.0f),
		restitutionInv_(1.0f)
	{
	}
	void quant(vector<IterateQuantitiesBase*>::iterator it)
	{
		lambda0_ = lambda_;
		lambda_ -= (eta_ + j_.preciseEta()) * d_;
		lambdaLimits_.apply(lambda_);
		j_.preciseAccelerationExternal((lambda_ - lambda0_) * restitutionInv_);
	}
	void preciseDV() const
	{
		j_.preciseDV(lambda_);
	}
	void update(float dt)
	{
		eta_ *= (1.0f / restitutionInv_ - 2.0f) / dt;
		eta_ += j_.computeEta(dt);
		d_ = 1.0f / j_.computeDet();
		j_.preciseAccelerationExternal(lambda_ * restitutionInv_);
	}
	void* operator new(size_t)
	{
		return PhysicsPool<IterateQuantities<SparsityRow> >::get();
	}
	void operator delete(void*)
	{
		xxassert(false, "попытка удаления объекта из PhysicsPool");
	}

protected:
	SparsityRow j_;
	float lambda_;
	float lambda0_;
	float restitutionInv_;
	float d_;
};

///////////////////////////////////////////////////////////////
//
//    class IterateQuantitiesFriction
//
///////////////////////////////////////////////////////////////

template<class SparsityRow>
class IterateQuantitiesFriction : public IterateQuantities<SparsityRow>
{
public:
	IterateQuantitiesFriction(const SparsityRow& j, float restitution, float friction, 
		const Limits& lambdaLimits, float eta) :
		IterateQuantities<SparsityRow>(j, restitution, lambdaLimits, eta),
		friction_(friction)
	{
	}
	void quant(vector<IterateQuantitiesBase*>::iterator it)
	{
		__super::quant(it);
		 float limit = friction_ * lambda_;
		(*(++it))->lambdaLimits_ = limit;
		(*(++it))->lambdaLimits_ = limit;
	}
	void* operator new(size_t)
	{
		return PhysicsPool<IterateQuantitiesFriction<SparsityRow> >::get();
	}

protected:
	float friction_;
};

///////////////////////////////////////////////////////////////
//
//    class IterateQuantitiesSimple
//
///////////////////////////////////////////////////////////////

template<class VectXf, class MassMatrix>
class IterateQuantitiesSimple : public IterateQuantities<SparsityRowBase<VectXf, MassMatrix> >
{
public:
	IterateQuantitiesSimple(RigidBodyPhysicsCore<VectXf, MassMatrix>* body, const VectXf& j, 
		float eta = 0.0f, const Limits& limits = Limits::ZERO, float restitution = 0.0f) :
		IterateQuantities<SparsityRowBase<VectXf, MassMatrix> >(SparsityRowBase<VectXf, MassMatrix>(body, j), 
			restitution, limits, eta)
	{
	}
	IterateQuantitiesSimple(RigidBodyPhysicsCore<VectXf, MassMatrix>* body, 
		const VectXf& j, float phi, float lowerLimits, float upperLimits, float relaxationTime) :
		IterateQuantities<SparsityRowBase<VectXf, MassMatrix> >(SparsityRowBase<VectXf, MassMatrix>(body, j), 
			phi, lowerLimits, upperLimits, relaxationTime)
	{
	}
};

typedef IterateQuantitiesSimple<Vect6f, MassMatrixFull> IterateQuantitiesSimple6f;

///////////////////////////////////////////////////////////////
//
//    class IterateQuantitiesSimpleFriction
//
///////////////////////////////////////////////////////////////

template<class VectXf, class MassMatrix>
class IterateQuantitiesSimpleFriction : public IterateQuantitiesFriction<SparsityRowBase<VectXf, MassMatrix> >
{
public:
	IterateQuantitiesSimpleFriction(RigidBodyPhysicsCore<VectXf, MassMatrix>* body, const VectXf& j, 
		float eta, const Limits& limits, float restitution, float friction) :
		IterateQuantitiesFriction<SparsityRowBase<VectXf, MassMatrix> >(
			SparsityRowBase<VectXf, MassMatrix>(body, j), restitution, friction, limits, eta)
	{
	}
};

typedef IterateQuantitiesSimpleFriction<Vect6f, MassMatrixFull> IterateQuantitiesSimpleFriction6f;

///////////////////////////////////////////////////////////////
//
//    class IterateQuantities6f
//
///////////////////////////////////////////////////////////////

template<class VectXf, class MassMatrix>
class IterateQuantitiesInteraction : public IterateQuantities<SparsityRow<VectXf, MassMatrix> >
{
public:
	IterateQuantitiesInteraction(RigidBodyPhysicsCore<VectXf, MassMatrix>* body0, 
		RigidBodyPhysicsCore<VectXf, MassMatrix>* body1, const VectXf& j0, const VectXf& j1, 
		float eta = 0.0f, const Limits& limits = Limits::ZERO, float restitution = 0.0f) :
		IterateQuantities<SparsityRow<VectXf, MassMatrix> >(
			SparsityRow<VectXf, MassMatrix>(body0, body1, j0, j1), restitution, limits, eta)
	{
	}
	IterateQuantitiesInteraction(RigidBodyPhysicsCore<VectXf, MassMatrix>* body0, 
		RigidBodyPhysicsCore<VectXf, MassMatrix>* body1, const VectXf& j0, const VectXf& j1, float phi,
		float lowerLimits, float upperLimits, float relaxationTime) :
		IterateQuantities<SparsityRow<VectXf, MassMatrix> >(
			SparsityRow<VectXf, MassMatrix>(body0, body1, j0, j1), phi, lowerLimits, upperLimits, relaxationTime)
	{
	}
};

typedef IterateQuantitiesInteraction<Vect6f, MassMatrixFull> IterateQuantities6f;

///////////////////////////////////////////////////////////////
//
//    class IterateQuantitiesXfFriction
//
///////////////////////////////////////////////////////////////

template<class VectXf, class MassMatrix>
class IterateQuantitiesInteractionFriction : public IterateQuantitiesFriction<SparsityRow<VectXf, MassMatrix> >
{
public:
	IterateQuantitiesInteractionFriction(RigidBodyPhysicsCore<VectXf, MassMatrix>* body0, 
		RigidBodyPhysicsCore<VectXf, MassMatrix>* body1, const VectXf& j0, const VectXf& j1, 
		float eta, const Limits& limits, float restitution, float friction) :
		IterateQuantitiesFriction<SparsityRow<VectXf, MassMatrix> >(
			SparsityRow<VectXf, MassMatrix>(body0, body1, j0, j1), restitution, friction, limits, eta)
	{
	}
};

typedef IterateQuantitiesInteractionFriction<Vect6f, MassMatrixFull> IterateQuantities6fFriction;

#endif // __PHYSICS_MATH_H__
