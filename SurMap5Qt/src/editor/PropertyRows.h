// PropertyRows.h — Qt port of kdw::PropertyRowFactory + the builtin row types
// (Util/kdw/PropertyRow.cpp, _PropertyRowBuiltin.h).
//
// The factory picks a PropertyRow subclass by the C++ type name
// (typeid(T).name()), exactly as the original's REGISTER_PROPERTY_ROW macro.
// PropertyOArchive calls factory.create(typeName) to build a row for a field;
// PropertyIArchive uses the row's assignTo to write a value back.
//
// Engine-free data model (no Qt, no kdw) so it compiles in EditorEngine.

#pragma once

#include "PropertyRow.h"

#include <map>
#include <string>

namespace editor {

// --- Concrete leaf rows ---------------------------------------------------

// A string value.
class PropertyRowString : public PropertyRowImpl<std::string>
{
public:
	PropertyRowString(const char* name, const char* nameAlt, const char* typeName, const std::string& value)
		: PropertyRowImpl<std::string>(name, nameAlt, typeName, value) {}
	std::string valueToString(const std::string& v) const override { return v; }
};

// A numeric value (int/float/double/...).
template<class Type>
class PropertyRowNumeric : public PropertyRowImpl<Type>
{
public:
	PropertyRowNumeric(const char* name, const char* nameAlt, const char* typeName, const Type& value)
		: PropertyRowImpl<Type>(name, nameAlt, typeName, value) {}
	std::string valueToString(const Type& v) const override
	{
		// Format floats with a few decimals, ints plainly.
		if(sizeof(Type) <= sizeof(int))
			return std::to_string((long long)v);
		return std::to_string((double)v);
	}
};

// A boolean value.
class PropertyRowBool : public PropertyRowImpl<bool>
{
public:
	PropertyRowBool(const char* name, const char* nameAlt, const char* typeName, const bool& value)
		: PropertyRowImpl<bool>(name, nameAlt, typeName, value) {}
	std::string valueToString(const bool& v) const override { return v ? "true" : "false"; }
};

// An enum value row (processEnum). Holds the raw int + the EnumDescriptor.
class PropertyRowEnum : public PropertyRow
{
public:
	PropertyRowEnum(const char* name, const char* nameAlt, const char* typeName, int value)
		: PropertyRow(name, nameAlt, typeName), value_(value)
	{
	}

	int value() const { return value_; }
	void setValue(int v) { value_ = v; }

	std::string valueAsString() const override { return std::to_string(value_); }

	bool assignTo(void* object, int size) override
	{
		if(!object || size < (int)sizeof(int))
			return false;
		*reinterpret_cast<int*>(object) = value_;
		return true;
	}

	int value_;
};

// A bit-vector flags row (processBitVector).
class PropertyRowBitVector : public PropertyRow
{
public:
	PropertyRowBitVector(const char* name, const char* nameAlt, const char* typeName, int flags)
		: PropertyRow(name, nameAlt, typeName), flags_(flags)
	{
	}

	int flags() const { return flags_; }
	void setFlags(int f) { flags_ = f; }

	std::string valueAsString() const override { return std::to_string(flags_); }

	bool assignTo(void* object, int size) override
	{
		if(!object || size < (int)sizeof(int))
			return false;
		*reinterpret_cast<int*>(object) = flags_;
		return true;
	}

	int flags_;
};

// --- Factory --------------------------------------------------------------

// Creates a PropertyRow for a C++ type name. Registered types map
// typeid(T).name() -> a row constructor.
class PropertyRowFactory
{
public:
	static PropertyRowFactory& instance()
	{
		static PropertyRowFactory f;
		return f;
	}

	// A row constructor: (name, nameAlt, typeName, value) -> row.
	typedef PropertyRow* (*RowCtor)(const char*, const char*, const char*, const void*);

	// Register a type name -> row constructor. (Named registerRow: `register`
	// is a C++ storage-class keyword and cannot be a method name.)
	void registerRow(const std::string& typeName, RowCtor ctor)
	{
		ctors_[typeName] = ctor;
	}

	// Create a row for the given C++ type name. Returns null if unregistered.
	// `value` is a pointer to the field's value (may be null for containers).
	PropertyRow* create(const std::string& typeName, const char* name, const char* nameAlt, const void* value)
	{
		auto it = ctors_.find(typeName);
		if(it == ctors_.end())
			return nullptr;
		return it->second(name, nameAlt, typeName.c_str(), value);
	}

	bool isRegistered(const std::string& typeName) const
	{
		return ctors_.find(typeName) != ctors_.end();
	}

private:
	std::map<std::string, RowCtor> ctors_;
};

// --- Registration helpers -------------------------------------------------

// Register a numeric row type for a C++ type.
template<class Type>
PropertyRow* makeNumericRow(const char* name, const char* nameAlt, const char* typeName, const void* value)
{
	return new PropertyRowNumeric<Type>(name, nameAlt, typeName, value ? *reinterpret_cast<const Type*>(value) : Type());
}

// Register the builtin row types (called once at startup).
void registerBuiltinPropertyRows();

} // namespace editor