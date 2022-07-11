#include "StdAfx.h"
#include "..\Network\quantTimeStatistic.h"
#include "..\Network\P2P_interface.h"


void QuantTimeStatistic::reset(bool isHT) //использовать до 2-х потоковости
{
	isHT_ = isHT;
	if(isHT_){
		logicTime_ = NORMAL_QUANT_INTERVAL;
		graphicsTime_ = NORMAL_QUANT_INTERVAL;
	}
	else{
		logicTime_ = NORMAL_QUANT_INTERVAL/2;
		graphicsTime_ = NORMAL_QUANT_INTERVAL/2;
	}
}

void QuantTimeStatistic::putLogic(unsigned int time)
{
	average(logicTime_, time, 0.05f);
}

void QuantTimeStatistic::putGraph(unsigned int time)
{
	average(graphicsTime_, time, 0.05f);
}

int QuantTimeStatistic::accessibleQuantPeriod() const
{
	int logic = round(logicTime_);
	int graphics = round(graphicsTime_);
	int result = isHT_ ? max(logic, graphics) : logic + graphics;
	statistics_add(calcAccessibleLogicQuantPeriod, result);
	return result;
}
