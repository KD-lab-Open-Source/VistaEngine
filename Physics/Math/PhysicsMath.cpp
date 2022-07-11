#include "StdAfx.h"
#include "PhysicsMath.h"
#include "Serialization\Serialization.h"

const Vect6f Vect6f::ZERO(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
const Vect6f Vect6f::ID(1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

const MassMatrixFull MassMatrixFull::ZERO;

const Vect6fN2f::Vect6f2f Vect6fN2f::ZERO(Vect6f::ZERO, Vect2f::ZERO);

const Limits Limits::GREATER_ZERO(0.0f, FLT_INF);
const Limits Limits::LOWER_ZERO(-FLT_INF, 0.0f);
const Limits Limits::ZERO(0.0f, 0.0f);
const Limits Limits::FREE(FLT_INF);

vector<PhysicsPoolBase*> PhysicsPoolBase::children_;