#include "StdAfx.h"
#include "PhysicsMath.h"
#include "..\RigidBodyPrm.h"
#include "ConstraintHandler.h"

const Vect6f Vect6f::ZERO(0, 0, 0, 0, 0, 0);
const Vect6f Vect6f::ID(1, 1, 1, 1, 1, 1);

void Vect6f::serialize(Archive& ar)
{
	ar.serialize(linear(), "linear", "linear");
	ar.serialize(angular(), "angular", "angular");
}

void SparsityMatrix::computeBMatrix(ConstraintHandler& constraintHandler, float dt)
{
	ConstraintHandler::iterator it;
	FOR_EACH(constraintHandler, it){
		SparsityRowSimple& rowSimple(it->j);
		SparsityRow& row(operator[](rowSimple.map0()));
		row.multDiag(it->b0, rowSimple.matrix0(), dt);
		if(rowSimple.map1() >= 0){
			if(!row.empty())
				row.multNonDiag(it->b1, rowSimple.map1(), rowSimple.matrix1(), dt);
			row = operator[](rowSimple.map1());
			row.multDiag(it->b1, rowSimple.matrix1(), dt);
			if(!row.empty())
				row.multNonDiag(it->b0, rowSimple.map0(), rowSimple.matrix0(), dt);
		}
		else
			it->b1 = Vect6f::ZERO;
	}
}
