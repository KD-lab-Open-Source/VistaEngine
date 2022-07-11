#include "stdafx.h"
#include "EnginePrm.h"
#include "Serialization\Serialization.h"

WRAP_LIBRARY(EnginePrm, "EnginePrm", "EnginePrm", "Scripts\\Engine\\EnginePrm", 0, 0);

EnginePrm::EnginePrm()
{
}

void EnginePrm::serialize(Archive& ar)
{
	ar.serialize(actionAttackLabel, "actionAttackLabel", "actionAttackLabel");
}

EnginePrm::ActionAttackLabel::ActionAttackLabel()
{
	helixDeltaAngle = M_PI;
	helixDeltaRadius = 1;
	sourceRadius = 50;
	digRadius = 15;
	throwRadius = 75;
	pickRadius = 5;
}

void EnginePrm::ActionAttackLabel::serialize(Archive& ar)
{
	ar.serialize(helixDeltaAngle, "helixDeltaAngle", "helixDeltaAngle");
	ar.serialize(helixDeltaRadius, "helixDeltaRadius", "helixDeltaRadius");
	ar.serialize(sourceRadius, "sourceRadius", "sourceRadius");
	ar.serialize(digRadius, "digRadius", "digRadius");
	ar.serialize(throwRadius, "throwRadius", "throwRadius");
	ar.serialize(pickRadius, "pickRadius", "pickRadius");
}
