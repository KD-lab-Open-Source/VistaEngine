#include "StdAfx.h"
#include "EditWindow.h"

#include "kdw/PropertyTree.h"

EditWindow::EditWindow(Serializer& serializeable, kdw::Window* parent)
: kdw::Dialog(parent)
{
	setDefaultSize(Vect2i(500, 500));
	addButton("OK", kdw::RESPONSE_OK);
	addButton("Cancel", kdw::RESPONSE_CANCEL);

	propertyTree_ = new kdw::PropertyTree();
	propertyTree_->attach(serializeable);
	add(propertyTree_);

	showAll();
}
