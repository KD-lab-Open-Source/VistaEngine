#ifndef __EDIT_WINDOW_H_INCLUDED__
#define __EDIT_WINDOW_H_INCLUDED__


#include "Serialization/Serializer.h"
#include "kdw/Dialog.h"

namespace kdw{
	class PropertyTree;
};

class Serializer;

class EditWindow : public kdw::Dialog{
public:
	EditWindow(Serializer& serializeable, kdw::Window* parent);
protected:
	kdw::PropertyTree* propertyTree_;
};

#endif
