#ifndef __GRADIENTS_H_INCLUDED__
#define __GRADIENTS_H_INCLUDED__

#include "NParticleKey.h"

struct TimeNoAlphaGradient: CKeyColor{
};

struct BytePositionGradient: CKeyColor{
};

struct BytePositionNoColorGradient: CKeyColor{
};

struct NoAlphaGradient: CKeyColor{
};

struct WaterGradient: CKeyColor{
};

struct SkyGradient: CKeyColor{//no alpha
};

struct SkyAlphaGradient: CKeyColor{
};

#endif
