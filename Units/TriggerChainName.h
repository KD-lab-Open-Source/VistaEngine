#ifndef __TRIGGER_CHAIN_NAME_H__
#define __TRIGGER_CHAIN_NAME_H__

struct TriggerChainName : string
{
	bool restartAlways;

	TriggerChainName(const char* nameIn = "") : string(nameIn), restartAlways(true) {}
	void serialize(Archive& ar);
	string shortName() const;
};

class TriggerChainNames : public vector<TriggerChainName>
{
public:
	void sub(const TriggerChainNames& commonTriggers);
	bool serialize(Archive& ar, const char* name, const char* nameAlt);
};

#endif //__TRIGGER_CHAIN_NAME_H__