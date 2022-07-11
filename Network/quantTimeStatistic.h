#ifndef __QUANT_TIME_STATISTIC_H__
#define __QUANT_TIME_STATISTIC_H__

class QuantTimeStatistic{
public:
	QuantTimeStatistic(){ reset(true); }
	void reset(bool isHT); 
	void putLogic(unsigned int time);
	void putGraph(unsigned int time);

	int accessibleQuantPeriod() const;
	int logicTime() const { return round(logicTime_); }
	int graphicsTime() const { return round(graphicsTime_); }

protected:
	bool isHT_;
	float logicTime_;
	float graphicsTime_;
};

#endif //__QUANT_TIME_STATISTIC_H__
