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
#include "Serialization/ComboStrings.h"   // indexInComboListString (pointer write-back)
#include "Serialization/EnumDescriptor.h" // descriptor entries for enum/flag rows
#include "PropertyRow.h"
#include "PropertyRows.h"
#include "PropertyRowsEngine.h"           // engine rows (EditorEngine only)

#include <climits>

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
	bool processValue(ComboListString& v, const char* n, const char* na) override
	{
		// A combo-box string row (kdw::PropertyRowComboListString): the
		// value plus its pipe-separated string list.
		PropertyRowEngineComboList* row = new PropertyRowEngineComboList(n, na, "ComboListString", v);
		current_->addChild(row);
		return true;
	}

	bool processEnum(int& value, const EnumDescriptor& descriptor, const char* name, const char* nameAlt) override
	{
		// An enum row with the descriptor's entries (kdw::PropertyRowEnum):
		// display names for the combo, keys for the value mapping.
		PropertyRowEnum* row = new PropertyRowEnum(name, nameAlt, descriptor.typeName(), value);
		fillEntries(*row, descriptor);
		current_->addChild(row);
		return true;
	}

	bool processBitVector(int& flags, const EnumDescriptor& descriptor, const char* name, const char* nameAlt) override
	{
		// A flags row with the descriptor's entries (kdw::
		// PropertyRowBitVector + CheckComboBox).
		PropertyRowBitVector* row = new PropertyRowBitVector(name, nameAlt, descriptor.typeName(), flags);
		fillEntries(*row, descriptor);
		current_->addChild(row);
		return true;
	}

	// --- Structs / containers / pointers ---
	bool openStructInternal(void* object, int size, const char* name, const char* nameAlt, const char* typeName, bool polymorphic) override
	{
		(void)polymorphic;
		// Struct types with a registered row (colors, ranged wrappers,
		// file selectors, combo colors) become LEAF rows that snapshot the
		// object — kdw::PropertyOArchive::openStructInternal consulted the
		// row factory the same way. The inner serialize body is skipped
		// (return false), exactly like the original.
		if(object && typeName && PropertyRowFactory::instance().isRegistered(typeName)){
			PropertyRow* row = PropertyRowFactory::instance().create(typeName, name, nameAlt, object);
			if(row && row->isLeaf()){
				current_->addChild(row);
				return false;
			}
			delete row;
		}
		(void)object; (void)size;
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
		(void)object; (void)derivedNameAlt;
		// Keep the concrete type name on the row (kdw::PropertyRowPointer::
		// derivedName_): PropertyIArchive maps it back to the factory index
		// so a polymorphic write-back recreates the object instead of
		// deleting it (serializePolymorphic deletes on NULL_POINTER).
		PropertyRowContainer* row = new PropertyRowContainer(name, nameAlt, baseName);
		row->setDerivedName(derivedName);
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
	// Fill an enum/flags row's combo entries from the descriptor: display
	// (alt) names with their keys, so the tree shows names and edits
	// through a combo/checklist.
	void fillEntries(PropertyRowEnum& row, const EnumDescriptor& descriptor)
	{
		const ComboStrings& names = descriptor.comboStrings();
		const ComboStrings& namesAlt = descriptor.comboStringsAlt();
		std::vector<std::string> display;
		std::vector<int> keys;
		for(size_t i = 0; i < names.size(); ++i){
			keys.push_back(descriptor.keyByName(names[i].c_str()));
			display.push_back(i < namesAlt.size() ? namesAlt[i] : names[i]);
		}
		row.setEntries(std::move(display), std::move(keys));
	}

	void fillEntries(PropertyRowBitVector& row, const EnumDescriptor& descriptor)
	{
		const ComboStrings& names = descriptor.comboStrings();
		const ComboStrings& namesAlt = descriptor.comboStringsAlt();
		std::vector<std::string> display;
		std::vector<int> keys;
		for(size_t i = 0; i < names.size(); ++i){
			keys.push_back(descriptor.keyByName(names[i].c_str()));
			display.push_back(i < namesAlt.size() ? namesAlt[i] : names[i]);
		}
		row.setEntries(std::move(display), std::move(keys));
	}

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
		// A leaf row (color, ranged wrapper, file selector, ...) snapshots
		// the whole struct: write it back and skip descending, mirroring
		// the OArchive side (which returned false there, so closeStruct is
		// not called for it either).
		PropertyRow* child = findChild(name);
		if(child && !child->isContainer()){
			child->assignTo(object, size);
			return false;
		}
		// Descend into the matching child container.
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
		// assignTo checks `size < sizeof(Type)`; the row already holds a
		// correctly typed value, so pass an unbounded size. (Passing
		// sizeof(void*) here used to reject every type larger than a
		// pointer — e.g. std::string write-back always failed.)
		return child->assignTo(value, INT_MAX);
	}

	PropertyRow* root_;
	PropertyRow* current_;
};

} // namespace editor