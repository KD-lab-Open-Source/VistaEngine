// STLPort my_stl.h compat stub
#pragma once
#ifndef FOR_EACH
#define FOR_EACH(list, iterator) \
    for((iterator) = (list).begin(); (iterator) != (list).end(); ++(iterator))
#endif
