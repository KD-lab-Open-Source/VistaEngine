#include "stdafx.h"
#ifdef _WIN32
#  include <objbase.h>
#endif
#include <format>
#include "FileUtils/XGUID.h"
#include "Serialization/Serialization.h"

static const GUID gz = {0, 0, 0, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
const XGUID XGUID::ZERO = gz;

XGUID::XGUID(const GUID& gd) 
{ 
	static_cast<GUID&>(*this) = gd;
}

void XGUID::generate()
{
	HRESULT result = CoCreateGuid(this);
	xassert(result == S_OK);
}

bool XGUID::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	string data;

	// std::format takes each field's width from its type instead of from a conversion
	// the caller has to keep in sync by hand. The printf original did not survive the
	// move to 64 bits: "%08lX" for a 32-bit Data1 took 64 bits off the varargs, which
	// printed a 16-digit number, overran the 80-byte static buffer it wrote into, and
	// shifted every argument after it by one. (Its "%02wX" was not a conversion at all,
	// only an MSVC-ism.) The text is unchanged — the canonical 78-character form the
	// 32-bit build wrote, which is what the profiles and mission headers hold.
	if(ar.isOutput())
		data = std::format(
			"{{0x{:08X}, 0x{:04X}, 0x{:04X}, {{0x{:02X}, 0x{:02X}, 0x{:02X}, 0x{:02X}, 0x{:02X}, 0x{:02X}, 0x{:02X}, 0x{:02X}}}}}",
			Data1, Data2, Data3, Data4[0],
			Data4[1], Data4[2], Data4[3], Data4[4], Data4[5], Data4[6], Data4[7]);

	bool res = ar.serialize(data, name, nameAlt);

	if(res && ar.isInput()){
		// sscanf offers no such type checking, so read every field into the same type
		// and narrow afterwards: "%lx" straight into Data1 wrote eight bytes into four
		// of them, over Data2 and Data3. A string that does not parse now leaves the
		// GUID alone rather than half-assigning it.
		unsigned rd[11] = {0};
		if(sscanf(data.c_str(),
				"{%x, %x, %x, {%x, %x, %x, %x, %x, %x, %x, %x}}",
				&rd[0], &rd[1], &rd[2], &rd[3], &rd[4], &rd[5],
				&rd[6], &rd[7], &rd[8], &rd[9], &rd[10]) == 11){
			Data1 = rd[0];
			Data2 = rd[1] & 0xFFFF;
			Data3 = rd[2] & 0xFFFF;
			for(int idx = 0; idx < 8; ++idx)
				Data4[idx] = rd[idx + 3] & 0xFF;
		}
	}

	return res;
}


