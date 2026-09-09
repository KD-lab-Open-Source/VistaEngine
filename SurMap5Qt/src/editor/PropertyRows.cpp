// PropertyRows.cpp — see PropertyRows.h.

#include "PropertyRows.h"

namespace editor {

void registerBuiltinPropertyRows()
{
	PropertyRowFactory& f = PropertyRowFactory::instance();

	// String.
	f.registerRow("std::string", [](const char* n, const char* na, const char* tn, const void* v) -> PropertyRow* {
		return new PropertyRowString(n, na, tn, v ? *reinterpret_cast<const std::string*>(v) : std::string());
	});

	// Bool.
	f.registerRow("bool", [](const char* n, const char* na, const char* tn, const void* v) -> PropertyRow* {
		return new PropertyRowBool(n, na, tn, v ? *reinterpret_cast<const bool*>(v) : false);
	});

	// Numerics.
	f.registerRow("int", [](const char* n, const char* na, const char* tn, const void* v) -> PropertyRow* {
		return makeNumericRow<int>(n, na, tn, v);
	});
	f.registerRow("unsigned int", [](const char* n, const char* na, const char* tn, const void* v) -> PropertyRow* {
		return makeNumericRow<unsigned int>(n, na, tn, v);
	});
	f.registerRow("long", [](const char* n, const char* na, const char* tn, const void* v) -> PropertyRow* {
		return makeNumericRow<long>(n, na, tn, v);
	});
	f.registerRow("unsigned long", [](const char* n, const char* na, const char* tn, const void* v) -> PropertyRow* {
		return makeNumericRow<unsigned long>(n, na, tn, v);
	});
	f.registerRow("short", [](const char* n, const char* na, const char* tn, const void* v) -> PropertyRow* {
		return makeNumericRow<short>(n, na, tn, v);
	});
	f.registerRow("unsigned short", [](const char* n, const char* na, const char* tn, const void* v) -> PropertyRow* {
		return makeNumericRow<unsigned short>(n, na, tn, v);
	});
	f.registerRow("char", [](const char* n, const char* na, const char* tn, const void* v) -> PropertyRow* {
		return makeNumericRow<char>(n, na, tn, v);
	});
	f.registerRow("unsigned char", [](const char* n, const char* na, const char* tn, const void* v) -> PropertyRow* {
		return makeNumericRow<unsigned char>(n, na, tn, v);
	});
	f.registerRow("float", [](const char* n, const char* na, const char* tn, const void* v) -> PropertyRow* {
		return makeNumericRow<float>(n, na, tn, v);
	});
	f.registerRow("double", [](const char* n, const char* na, const char* tn, const void* v) -> PropertyRow* {
		return makeNumericRow<double>(n, na, tn, v);
	});
}

} // namespace editor