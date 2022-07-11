#pragma once
#ifndef __VISTARPC_SHOW_LOG_H_INCLUDED__
#define __VISTARPC_SHOW_LOG_H_INCLUDED__

#include "kdw\VBox.h"

namespace kdw {
	class ObjectsTree;
	class Slider;
};

class ClientLog : public kdw::VBox
{
public:
	ClientLog();
	virtual ~ClientLog();

	void addRecord(const char* txt);

private:
	kdw::ObjectsTree* tree_;
};

#endif //__VISTARPC_SHOW_LOG_H_INCLUDED__