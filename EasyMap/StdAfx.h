#include <my_STL.h>

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>

#include <stdio.h>

#include <vector>
#include <list>
#include <stack>
#include <map>
#include <hash_map>
#include <string>
using namespace std;
#include <xutil.h>
#include "umath.h"

#include "IRenderDevice.h"
#include "IVisGeneric.h"
//#define UNIVERSALREGION
extern sColor4c WHITE;
extern sColor4c RED;
extern sColor4c GREEN;
extern sColor4c BLUE;
extern sColor4c YELLOW;
extern sColor4c MAGENTA;
extern sColor4c CYAN;
Vect3f To3D(const Vect2f& pos);

void DrawTerraCircle(Vect2f pos,float radius,sColor4c color);

#include "..\Util\DebugUtil.h"
