#pragma once
#ifndef __VISTARPC_SHOW_LOG_H_INCLUDED__
#define __VISTARPC_SHOW_LOG_H_INCLUDED__

#include "kdw\VBox.h"

namespace kdw {
	class ObjectsTree;
	class Slider;
	class Label;
};

class ShowLog : public kdw::VBox
{
public:
	ShowLog();
	virtual ~ShowLog();

	void addRecord(const char* txt);
	void setStatus(const char* txt);

	void clearLog();

private:
	kdw::ObjectsTree* tree_;
	kdw::Label* status_;
};

#endif //__VISTARPC_SHOW_LOG_H_INCLUDED__