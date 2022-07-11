#include "stdafx.h"

#include "UnitAttribute.h"
#include "SourceDeleteGrass.h"
#include "Environment.h"
#include "Render\src\Grass.h"

void SourceDeleteGrass::quant()
{
	__super::quant();
	
	if(!active())
		return;
	
	if (environment&&environment->grass())
	{
		environment->grass()->DeleteGrass(pose().trans(),radius());
	}
	if(!isUnderEditor())
		kill();
}
