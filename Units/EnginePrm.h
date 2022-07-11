#pragma  once

#include "Serialization\LibraryWrapper.h"

struct EnginePrm : public LibraryWrapper<EnginePrm>
{
	struct ActionAttackLabel {
		float helixDeltaAngle;
		float helixDeltaRadius;
		float sourceRadius;
		float digRadius;
		float throwRadius;
		float pickRadius;

		ActionAttackLabel();
		void serialize(Archive& ar);
	};

	ActionAttackLabel actionAttackLabel;

	EnginePrm();
	void serialize(Archive& ar);
};
