// PropertyArchive.h — Qt port of kdw::PropertyOArchive / PropertyIArchive
// (Util/kdw/PropertyOArchive.h, PropertyIArchive.h).
//
// These bridge a Serializer to the PropertyRow model:
//   - PropertyOArchive (isOutput): walks the Serializer and builds a tree of
//     PropertyRow nodes (the property form).
//   - PropertyIArchive (isInput): walks the Serializer and writes values from
//     the rows back into the object.
// Both inherit the engine's Archive and implement its abstract methods.
//
// Engine-side (compiles in EditorEngine with the engine flags).

#pragma once

#include "Serialization/Serialization.h"
#include "PropertyRow.h"
#include "PropertyRows.h"

namespace editor {

// Builds a PropertyRow tree from a Serializer.
class PropertyOArchive : public Archive
{
public:
	PropertyOArchive()
	{
		root_ = new PropertyRowContainer("", "", "root");
		current_ = root_;
	}

	PropertyRow* root() const { return root_; }

	bool isOutput() const override { return true; }
	bool isInput() const override { return false; }
	bool isEdit() const override { return true; }

	// --- Value fields ---
	bool processValue(bool& v, const char* n, const char* na) override { return addValue("bool", n, na, &v); }
	bool processValue(char& v, const char* n, const char* na) override { return addValue("char", n, na, &v); }
	bool processValue(signed char& v, const char* n, const char* na) override { return addValue("signed char", n, na, &v); }
	bool processValue(signed short& v, const char* n, const char* na) override { return addValue("short", n, na, &v); }
	bool processValue(signed int& v, const char* n, const char* na) override { return addValue("int", n, na, &v); }
	bool processValue(signed long& v, const char* n, const char* na) override { return addValue("long", n, na, &v); }
	bool processValue(unsigned char& v, const char* n, const char* na) override { return addValue("unsigned char", n, na, &v); }
	bool processValue(unsigned short& v, const char* n, const char* na) override { return addValue("unsigned short", n, na, &v); }
	bool processValue(unsigned int& v, const char* n, const char* na) override { return addValue("unsigned int", n, na, &v); }
	bool processValue(unsigned long& v, const char* n, const char* na) override { return addValue("unsigned long", n, na, &v); }
	bool processValue(float& v, const char* n, const char* na) override { return addValue("float", n, na, &v); }
	bool processValue(double& v, const char* n, const char* na) override { return addValue("double", n, na, &v); }
	bool processValue(std::string& v, const char* n, const char* na) override { return addValue("std::string", n, na, &v); }
	bool processValue(std::wstring& v, const char* n, const char* na) override { (void)v; (void)n; (void)na; return true; }
	bool processValue(ComboListString& v, const char* n, const char* na) override { (void)v; (void)n; (void)na; return true; }

	bool processEnum(int& value, const EnumDescriptor& descriptor, const char* name, const char* nameAlt) override
	{
		(void)nameAlt;
		PropertyRow* row = new PropertyRowEnum(name, nameAlt, descriptor.typeName(), value);
		current_->addChild(row);
		return true;
	}

	bool processBitVector(int& flags, const EnumDescriptor& descriptor, const char* name, const char* nameAlt) override
	{
		(void)nameAlt;
		PropertyRow* row = new PropertyRowBitVector(name, nameAlt, descriptor.typeName(), flags);
		current_->addChild(row);
		return true;
	}

	// --- Structs / containers / pointers ---
	bool openStructInternal(void* object, int size, const char* name, const char* nameAlt, const char* typeName, bool polymorphic) override
	{
		(void)object; (void)size; (void)polymorphic;
		// A container row for the struct; descend into it.
		PropertyRowContainer* row = new PropertyRowContainer(name, nameAlt, typeName);
		current_->addChild(row);
		current_ = row;
		return true;
	}

	void closeStruct(const char* name) override
	{
		(void)name;
		if(current_->parent())
			current_ = current_->parent();
	}

	bool openContainer(void* array, int& number, const char* name, const char* nameAlt, const char* typeName, const char* elementTypeName, int elementSize, bool readOnly) override
	{
		(void)array; (void)number; (void)elementTypeName; (void)elementSize; (void)readOnly;
		PropertyRowContainer* row = new PropertyRowContainer(name, nameAlt, typeName);
		current_->addChild(row);
		current_ = row;
		return true;
	}

	void closeContainer(const char* name) override
	{
		closeStruct(name);
	}

	int openPointer(void*& object, const char* name, const char* nameAlt, const char* baseName, const char* derivedName, const char* derivedNameAlt) override
	{
		(void)object; (void)derivedName; (void)derivedNameAlt;
		PropertyRowContainer* row = new PropertyRowContainer(name, nameAlt, baseName);
		current_->addChild(row);
		current_ = row;
		return NULL_POINTER;
	}

	void closePointer(const char* name, const char* typeName, const char* derivedName) override
	{
		(void)typeName; (void)derivedName;
		closeStruct(name);
	}

	bool openBlock(const char* name, const char* nameAlt) override
	{
		(void)name;
		PropertyRowContainer* row = new PropertyRowContainer(nameAlt, nameAlt, "");
		current_->addChild(row);
		current_ = row;
		return true;
	}

	void closeBlock() override
	{
		closeStruct("block");
	}

private:
	// Add a leaf row for a typed value.
	bool addValue(const std::string& typeName, const char* name, const char* nameAlt, const void* value)
	{
		PropertyRow* row = PropertyRowFactory::instance().create(typeName, name, nameAlt, value);
		if(!row)
			row = new PropertyRow(name, nameAlt, typeName.c_str());
		current_->addChild(row);
		return true;
	}

	PropertyRowContainer* root_;
	PropertyRow* current_;
};

// Writes values from a PropertyRow tree back into a Serializer's object.
class PropertyIArchive : public Archive
{
public:
	PropertyIArchive(PropertyRow* root)
		: root_(root), current_(root)
	{
	}

	bool isOutput() const override { return false; }
	bool isInput() const override { return true; }
	bool isEdit() const override { return true; }

	// --- Value fields: read from the matching row into the value ---
	bool processValue(bool& v, const char* n, const char* na) override { return readValue("bool", n, na, &v); }
	bool processValue(char& v, const char* n, const char* na) override { return readValue("char", n, na, &v); }
	bool processValue(signed char& v, const char* n, const char* na) override { return readValue("signed char", n, na, &v); }
	bool processValue(signed short& v, const char* n, const char* na) override { return readValue("short", n, na, &v); }
	bool processValue(signed int& v, const char* n, const char* na) override { return readValue("int", n, na, &v); }
	bool processValue(signed long& v, const char* n, const char* na) override { return readValue("long", n, na, &v); }
	bool processValue(unsigned char& v, const char* n, const char* na) override { return readValue("unsigned char", n, na, &v); }
	bool processValue(unsigned short& v, const char* n, const char* na) override { return readValue("unsigned short", n, na, &v); }
	bool processValue(unsigned int& v, const char* n, const char* na) override { return readValue("unsigned int", n, na, &v); }
	bool processValue(unsigned long& v, const char* n, const char* na) override { return readValue("unsigned long", n, na, &v); }
	bool processValue(float& v, const char* n, const char* na) override { return readValue("float", n, na, &v); }
	bool processValue(double& v, const char* n, const char* na) override { return readValue("double", n, na, &v); }
	bool processValue(std::string& v, const char* n, const char* na) override { return readValue("std::string", n, na, &v); }
	bool processValue(std::wstring& v, const char* n, const char* na) override { (void)v; (void)n; (void)na; return true; }
	bool processValue(ComboListString& v, const char* n, const char* na) override { (void)v; (void)n; (void)na; return true; }

	bool processEnum(int& value, const EnumDescriptor& descriptor, const char* name, const char* nameAlt) override
	{
		(void)descriptor; (void)nameAlt;
		return readValue(descriptor.typeName(), name, nameAlt, &value);
	}

	bool processBitVector(int& flags, const EnumDescriptor& descriptor, const char* name, const char* nameAlt) override
	{
		(void)descriptor; (void)nameAlt;
		return readValue(descriptor.typeName(), name, nameAlt, &flags);
	}

	bool openStructInternal(void* object, int size, const char* name, const char* nameAlt, const char* typeName, bool polymorphic) override
	{
		(void)object; (void)size; (void)nameAlt; (void)typeName; (void)polymorphic;
		// Descend into the matching child container.
		PropertyRow* child = findChild(name);
		if(child)
			current_ = child;
		return true;
	}

	void closeStruct(const char* name) override
	{
		(void)name;
		if(current_->parent())
			current_ = current_->parent();
	}

	bool openContainer(void* array, int& number, const char* name, const char* nameAlt, const char* typeName, const char* elementTypeName, int elementSize, bool readOnly) override
	{
		(void)array; (void)nameAlt; (void)typeName; (void)elementTypeName; (void)elementSize; (void)readOnly;
		PropertyRow* child = findChild(name);
		if(child)
			current_ = child;
		return true;
	}

	void closeContainer(const char* name) override
	{
		closeStruct(name);
	}

	int openPointer(void*& object, const char* name, const char* nameAlt, const char* baseName, const char* derivedName, const char* derivedNameAlt) override
	{
		(void)object; (void)nameAlt; (void)derivedName; (void)derivedNameAlt;
		PropertyRow* child = findChild(name);
		if(child)
			current_ = child;
		return NULL_POINTER;
	}

	void closePointer(const char* name, const char* typeName, const char* derivedName) override
	{
		(void)typeName; (void)derivedName;
		closeStruct(name);
	}

	bool openBlock(const char* name, const char* nameAlt) override
	{
		(void)name;
		PropertyRow* child = findChild(nameAlt);
		if(child)
			current_ = child;
		return true;
	}

	void closeBlock() override
	{
		closeStruct("block");
	}

private:
	// Find a child row by name.
	PropertyRow* findChild(const char* name)
	{
		for(PropertyRow* c : current_->children())
			if(c->name() == name)
				return c;
		return nullptr;
	}

	// Read a typed value from the matching child row.
	bool readValue(const std::string& typeName, const char* name, const char* nameAlt, void* value)
	{
		(void)typeName; (void)nameAlt;
		PropertyRow* child = findChild(name);
		if(!child)
			return false;
		return child->assignTo(value, (int)sizeof(void*));
	}

	PropertyRow* root_;
	PropertyRow* current_;
};

} // namespace editor