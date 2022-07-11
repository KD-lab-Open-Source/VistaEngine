#ifndef __GUID_SERIALIZATION_H_INCLUDED__
#define __GUID_SERIALIZATION_H_INCLUDED__

class Archive;

class GUIDSerialization
{
public:

	GUIDSerialization(GUID& _value)
		: g(_value)
	{}

	bool serialize(Archive& ar, const char* name, const char* nameAlt);

private:
	GUID& g;
};


class XGUID : public GUID
{
public:
	XGUID() {}
	XGUID(const GUID& gd) { 
		static_cast<GUID&>(*this) = gd;
	}

	bool serialize(Archive& ar, const char* name, const char* nameAlt);
};

typedef vector<XGUID> GUIDcontainer;

#endif //__GUID_SERIALIZATION_H_INCLUDED__
