#ifndef __ADD_ELEMENT_H_INCLUDED__
#define __ADD_ELEMENT_H_INCLUDED__
/********************************************************************
	created:	2003/07/29
	created:	29:7:2003   19:35
	filename: 	d:\Projects\Quest\QuestEditor\AddElement.h
	file path:	d:\Projects\Quest\QuestEditor
	file base:	AddElement
	file ext:	h
	powerd by:	Илюха
	
	purpose:	Добавляет элемент в триггер
*********************************************************************/

class TEBaseWorkMode;

#include "TEEngine/TEGrid.h"
#include "TriggerExport.h"
#include <memory>

class AddElement
{
public:
	AddElement(TEGrid const& grid, 
				CPoint const& cell,
				TriggerChain & chain, 
				Trigger const& trigger,
				int position);
	~AddElement(void);
	bool operator()();
	static bool run(TEGrid const& grid, CPoint const& cell,
		TriggerChain & chain, 
		Trigger const& trigger, int position);
private:
	TEGrid const&					grid_;
	TriggerChain&	chain_;
	Trigger							trigger_;
	CPoint							cell_;
	int								position_;
};

#endif
