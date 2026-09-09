// PropertyRow.h — Qt port of kdw::PropertyRow (Util/kdw/PropertyRow.h).
//
// A row in the property tree: one field of a serialized object. The original
// was a Win32-GDI tree row; here it is a plain model node the Qt PropertyTree
// renders. PropertyOArchive builds these from a Serializer; PropertyIArchive
// writes values back. Engine-free data model (no Qt, no kdw) so it compiles
// in the EditorEngine library.
//
// The row holds a typed value (PropertyRowImpl<Type>) or is a container of
// children (PropertyRowContainer). The type is chosen by the C++ type name
// (typeid(T).name()) via PropertyRowFactory, exactly as the original.

#pragma once

#include <string>
#include <vector>

namespace editor {

// A property row: a named, typed value (leaf) or a container of children.
class PropertyRow
{
public:
	PropertyRow(const char* name, const char* nameAlt, const char* typeName)
		: name_(name ? name : ""), nameAlt_(nameAlt ? nameAlt : ""), typeName_(typeName ? typeName : "")
	{
	}

	virtual ~PropertyRow() = default;

	const std::string& name() const { return name_; }
	const std::string& nameAlt() const { return nameAlt_; }
	const std::string& typeName() const { return typeName_; }

	// Leaf vs container.
	virtual bool isLeaf() const { return true; }
	virtual bool isContainer() const { return false; }

	// The row's value as a string (for display).
	virtual std::string valueAsString() const { return ""; }

	// Write the value into a typed slot (PropertyIArchive).
	virtual bool assignTo(void* object, int size) { (void)object; (void)size; return false; }

	// Children (containers only).
	std::vector<PropertyRow*>& children() { return children_; }
	const std::vector<PropertyRow*>& children() const { return children_; }
	void addChild(PropertyRow* child) { children_.push_back(child); child->parent_ = this; }

	// Parent (set by addChild).
	PropertyRow* parent() const { return parent_; }

protected:
	std::string name_;
	std::string nameAlt_;
	std::string typeName_;
	std::vector<PropertyRow*> children_;
	PropertyRow* parent_ = nullptr;
};

// A leaf row holding a typed value.
template<class Type>
class PropertyRowImpl : public PropertyRow
{
public:
	PropertyRowImpl(const char* name, const char* nameAlt, const char* typeName, const Type& value)
		: PropertyRow(name, nameAlt, typeName), value_(value)
	{
	}

	Type value() const { return value_; }
	void setValue(const Type& v) { value_ = v; }

	std::string valueAsString() const override { return valueToString(value_); }

	bool assignTo(void* object, int size) override
	{
		if(!object || size < (int)sizeof(Type))
			return false;
		*reinterpret_cast<Type*>(object) = value_;
		return true;
	}

	// Convert a value to a display string. This is a NEW virtual method (it
	// does not exist in the base PropertyRow — the base only has
	// valueAsString()), so it must NOT be marked override here. Derived rows
	// (PropertyRowString/Numeric/Bool) override it.
	virtual std::string valueToString(const Type& v) const { (void)v; return ""; }

	Type value_;
};

// A container row (a struct/object with children).
class PropertyRowContainer : public PropertyRow
{
public:
	PropertyRowContainer(const char* name, const char* nameAlt, const char* typeName)
		: PropertyRow(name, nameAlt, typeName)
	{
	}

	bool isLeaf() const override { return false; }
	bool isContainer() const override { return true; }
};

} // namespace editor