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

// The editor kind of a row: which in-place control the Qt PropertyTree
// shows for it (Qt port of kdw's per-row widgets — PropertyRowWidget,
// ColorChooser, CheckComboBox, sliders...). Containers never edit.
enum class RowKind
{
	Container,   // struct/object with children
	Text,        // free string (Entry)
	Number,      // numeric text / spinbox
	Bool,        // checkbox toggle
	Enum,        // combo box over descriptor entries
	Flags,       // checklist over bitvector entries
	Color,       // swatch + color dialog
	Combo,       // editable combo over a string list
	Ranged,      // slider + spinbox over [min, max]
	File,        // path + file-dialog button
	Other,       // leaf of an unregistered type — display only
};

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

	// Which in-place editor the tree shows for this row.
	virtual RowKind kind() const { return isContainer() ? RowKind::Container : RowKind::Other; }

	// The row's value as a string (for display).
	virtual std::string valueAsString() const { return ""; }

	// Parse an edited display string back into the row's value (the Qt
	// PropertyTree's itemChanged path). Returns false when the text does
	// not parse — the tree then reverts the edit. Containers ignore it.
	virtual bool setValueFromString(const std::string& /*text*/) { return false; }

	// Write the value into a typed slot (PropertyIArchive).
	virtual bool assignTo(void* object, int size) { (void)object; (void)size; return false; }

	// Children (containers only).
	std::vector<PropertyRow*>& children() { return children_; }
	const std::vector<PropertyRow*>& children() const { return children_; }
	void addChild(PropertyRow* child) { children_.push_back(child); child->parent_ = this; }

	// Parent (set by addChild).
	PropertyRow* parent() const { return parent_; }

	// The concrete derived type name for pointer rows (openPointer's
	// derivedName). Lets PropertyIArchive map the row back to the factory
	// index so a polymorphic write-back updates the live object in place
	// instead of deleting it (serializePolymorphic deletes on NULL_POINTER).
	const std::string& derivedName() const { return derivedName_; }
	void setDerivedName(const char* derived) { derivedName_ = derived ? derived : ""; }

protected:
	std::string name_;
	std::string nameAlt_;
	std::string typeName_;
	std::string derivedName_;
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
	RowKind kind() const override { return RowKind::Container; }
};

} // namespace editor