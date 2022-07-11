#pragma once
#ifndef __VISTARPC_MAIN_VIEW_H_INCLUDED__
#define __VISTARPC_MAIN_VIEW_H_INCLUDED__

#include "kdw\Viewport2D.h"

class MainView : public kdw::Viewport2D
{
public:
	MainView();
	virtual ~MainView();

	void serialize(Archive& ar);
};

#endif //__VISTARPC_MAIN_VIEW_H_INCLUDED__