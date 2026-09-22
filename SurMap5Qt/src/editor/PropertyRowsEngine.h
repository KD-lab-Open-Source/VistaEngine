// PropertyRowsEngine.h — engine-side property rows (EditorEngine only).
//
// Typed rows holding real engine values: colors, combo strings, ranged
// wrappers, file selectors. They snapshot the engine object at
// PropertyOArchive time (engine-free doubles/strings for the Qt delegates)
// and write back through assignTo at PropertyIArchive time.
//
// Qt translation units must never include this header (engine types, engine
// compile flags). The Qt side sees these rows only through the engine-free
// editor::PropertyRow interface (RowKind + valueAsString/setValueFromString).

#pragma once

#include "PropertyRows.h"

#include <typeinfo>

#include "XMath/Colors.h"
#include "XMath/ComboListColor.h"
#include "Serialization/SerializationTypes.h"   // ComboListString
#include "Serialization/ComboStrings.h"         // splitComboList
#include "Serialization/RangedWrapper.h"
#include "Serialization/GenericFileSelector.h"
#include "Serialization/ResourceSelector.h"

namespace editor {

// --- Colors (kdw::PropertyRowColor port) -----------------------------------

class PropertyRowEngineColor3c : public PropertyRowColor
{
public:
	PropertyRowEngineColor3c(const char* name, const char* nameAlt, const char* typeName,
	                         const Color3c& color)
		: PropertyRowColor(name, nameAlt, typeName,
		                   color.r, color.g, color.b, 255.0, false)
	{
	}
	bool assignTo(void* object, int size) override
	{
		if(!object || size < (int)sizeof(Color3c))
			return false;
		Color3c& c = *reinterpret_cast<Color3c*>(object);
		c.set((int)r_, (int)g_, (int)b_);
		return true;
	}
};

class PropertyRowEngineColor4c : public PropertyRowColor
{
public:
	PropertyRowEngineColor4c(const char* name, const char* nameAlt, const char* typeName,
	                         const Color4c& color)
		: PropertyRowColor(name, nameAlt, typeName,
		                   color.r, color.g, color.b, color.a, true)
	{
	}
	bool assignTo(void* object, int size) override
	{
		if(!object || size < (int)sizeof(Color4c))
			return false;
		Color4c& c = *reinterpret_cast<Color4c*>(object);
		c.set((int)r_, (int)g_, (int)b_, (int)a_);
		return true;
	}
};

class PropertyRowEngineColor4f : public PropertyRowColor
{
public:
	PropertyRowEngineColor4f(const char* name, const char* nameAlt, const char* typeName,
	                         const Color4f& color)
		: PropertyRowColor(name, nameAlt, typeName,
		                   color.GetR(), color.GetG(), color.GetB(), color.GetA(), true)
	{
	}
	bool assignTo(void* object, int size) override
	{
		if(!object || size < (int)sizeof(Color4f))
			return false;
		Color4f& c = *reinterpret_cast<Color4f*>(object);
		c.set((float)r_ / 255.f, (float)g_ / 255.f, (float)b_ / 255.f, (float)a_ / 255.f);
		return true;
	}
};

// ComboListColor: a color picked from a palette (kdw::
// PropertyRowComboListColor). The palette shows as the Qt color dialog's
// custom colors; write-back maps the color to a palette index, exactly like
// the original's operator=(Color4f) (unknown colors fall back to index 0).
class PropertyRowEngineComboColor : public PropertyRowColor
{
public:
	PropertyRowEngineComboColor(const char* name, const char* nameAlt, const char* typeName,
	                            const ComboListColor& combo)
		: PropertyRowColor(name, nameAlt, typeName, 255, 255, 255, 255, true)
	{
		const Color4f& v = combo.value();
		setColor(v.GetR(), v.GetG(), v.GetB(), v.GetA());
		for(const Color4f& c : combo.comboList())
			palette_.push_back({ (double)c.GetR(), (double)c.GetG(),
			                     (double)c.GetB(), (double)c.GetA() });
	}
	bool assignTo(void* object, int size) override
	{
		if(!object || size < (int)sizeof(ComboListColor))
			return false;
		ComboListColor& combo = *reinterpret_cast<ComboListColor*>(object);
		combo = Color4f((float)r_ / 255.f, (float)g_ / 255.f,
		                (float)b_ / 255.f, (float)a_ / 255.f);
		return true;
	}
	bool colorPalette(std::vector<std::array<double, 4>>& out) const override
	{
		out = palette_;
		return !palette_.empty();
	}

private:
	std::vector<std::array<double, 4>> palette_;
};

// --- Combo strings (kdw::PropertyRowComboListString port) -------------------

class PropertyRowEngineComboList : public PropertyRowCombo
{
public:
	PropertyRowEngineComboList(const char* name, const char* nameAlt, const char* typeName,
	                           const ComboListString& combo)
		: PropertyRowCombo(name, nameAlt, typeName, combo.value())
	{
		ComboStrings strings;
		splitComboList(strings, combo.comboList());
		setEntries(strings);
		setSourceCp1251(!isValidUtf8(combo.value()));
	}
	bool assignTo(void* object, int size) override
	{
		if(!object || size < (int)sizeof(ComboListString))
			return false;
		*reinterpret_cast<ComboListString*>(object) = value_;
		return true;
	}
};

// --- Ranged wrappers (kdw::PropertyRowRanged port) --------------------------

class PropertyRowEngineRangedF : public PropertyRowRanged
{
public:
	PropertyRowEngineRangedF(const char* name, const char* nameAlt, const char* typeName,
	                         const RangedWrapperf& wrapper)
		: PropertyRowRanged(name, nameAlt, typeName, wrapper.value(),
		                    wrapper.range().minimum(), wrapper.range().maximum(),
		                    wrapper.step(), false)
	{
	}
	bool assignTo(void* object, int size) override
	{
		if(!object || size < (int)sizeof(RangedWrapperf))
			return false;
		*reinterpret_cast<RangedWrapperf*>(object) = (float)value_;
		return true;
	}
};

class PropertyRowEngineRangedI : public PropertyRowRanged
{
public:
	PropertyRowEngineRangedI(const char* name, const char* nameAlt, const char* typeName,
	                         const RangedWrapperi& wrapper)
		: PropertyRowRanged(name, nameAlt, typeName, wrapper.value(),
		                    wrapper.range().minimum(), wrapper.range().maximum(),
		                    wrapper.step(), true)
	{
	}
	bool assignTo(void* object, int size) override
	{
		if(!object || size < (int)sizeof(RangedWrapperi))
			return false;
		*reinterpret_cast<RangedWrapperi*>(object) = (int)value_;
		return true;
	}
};

// --- File selectors (kdw::PropertyRowFileSelector port) ----------------------
//
// The row snapshots the path + dialog options from the (usually temporary)
// selector wrapper. assignTo writes the path into the live selector through
// setFileName; the wrapper's destructor propagates it to the bound string,
// exactly like the original.

enum class FileSelectorTag
{
	Generic,
	Resource,
	Model,
};

class PropertyRowEngineFile : public PropertyRowFile
{
public:
	PropertyRowEngineFile(const char* name, const char* nameAlt, const char* typeName,
	                      const std::string& path, const std::string& filter,
	                      const std::string& initialDir, const std::string& title,
	                      bool save, FileSelectorTag tag)
		: PropertyRowFile(name, nameAlt, typeName, path, filter, initialDir, title, save)
		, tag_(tag)
	{
		setSourceCp1251(!isValidUtf8(path));
	}
	bool assignTo(void* object, int size) override
	{
		if(!object)
			return false;
		switch(tag_){
		case FileSelectorTag::Generic:
			if(size < (int)sizeof(GenericFileSelector))
				return false;
			reinterpret_cast<GenericFileSelector*>(object)->setFileName(path_.c_str());
			return true;
		case FileSelectorTag::Resource:
			if(size < (int)sizeof(ResourceSelector))
				return false;
			reinterpret_cast<ResourceSelector*>(object)->setFileName(path_.c_str());
			return true;
		case FileSelectorTag::Model:
			if(size < (int)sizeof(ModelSelector))
				return false;
			reinterpret_cast<ModelSelector*>(object)->setFileName(path_.c_str());
			return true;
		}
		return false;
	}

private:
	FileSelectorTag tag_;
};

// Register the engine rows in the factory under their typeid names (the same
// spelling openStructInternal receives, same compiler). Idempotent.
inline void registerEnginePropertyRows()
{
	PropertyRowFactory& f = PropertyRowFactory::instance();
	if(f.isRegistered(typeid(Color4c).name()))
		return;
	f.registerRow(typeid(Color3c).name(), [](const char* n, const char* na, const char* tn, const void* v) -> PropertyRow* {
		return new PropertyRowEngineColor3c(n, na, tn, v ? *reinterpret_cast<const Color3c*>(v) : Color3c());
	});
	f.registerRow(typeid(Color4c).name(), [](const char* n, const char* na, const char* tn, const void* v) -> PropertyRow* {
		return new PropertyRowEngineColor4c(n, na, tn, v ? *reinterpret_cast<const Color4c*>(v) : Color4c());
	});
	f.registerRow(typeid(Color4f).name(), [](const char* n, const char* na, const char* tn, const void* v) -> PropertyRow* {
		return new PropertyRowEngineColor4f(n, na, tn, v ? *reinterpret_cast<const Color4f*>(v) : Color4f());
	});
	f.registerRow(typeid(ComboListColor).name(), [](const char* n, const char* na, const char* tn, const void* v) -> PropertyRow* {
		return new PropertyRowEngineComboColor(n, na, tn, v ? *reinterpret_cast<const ComboListColor*>(v) : ComboListColor());
	});
	f.registerRow(typeid(RangedWrapperf).name(), [](const char* n, const char* na, const char* tn, const void* v) -> PropertyRow* {
		return new PropertyRowEngineRangedF(n, na, tn, v ? *reinterpret_cast<const RangedWrapperf*>(v) : RangedWrapperf());
	});
	f.registerRow(typeid(RangedWrapperi).name(), [](const char* n, const char* na, const char* tn, const void* v) -> PropertyRow* {
		return new PropertyRowEngineRangedI(n, na, tn, v ? *reinterpret_cast<const RangedWrapperi*>(v) : RangedWrapperi());
	});
	f.registerRow(typeid(GenericFileSelector).name(), [](const char* n, const char* na, const char* tn, const void* v) -> PropertyRow* {
		const GenericFileSelector* s = reinterpret_cast<const GenericFileSelector*>(v);
		return new PropertyRowEngineFile(n, na, tn,
			s ? (const char*)*s : "", s ? s->filter() : "",
			s ? s->initialDir() : "", s ? s->title() : "",
			s ? s->save() : false, FileSelectorTag::Generic);
	});
	f.registerRow(typeid(ResourceSelector).name(), [](const char* n, const char* na, const char* tn, const void* v) -> PropertyRow* {
		const ResourceSelector* s = reinterpret_cast<const ResourceSelector*>(v);
		return new PropertyRowEngineFile(n, na, tn,
			s ? (const char*)*s : "", s ? s->options().filter.c_str() : "",
			s ? s->options().initialDir.c_str() : "", s ? s->options().title.c_str() : "",
			false, FileSelectorTag::Resource);
	});
	f.registerRow(typeid(ModelSelector).name(), [](const char* n, const char* na, const char* tn, const void* v) -> PropertyRow* {
		const ModelSelector* s = reinterpret_cast<const ModelSelector*>(v);
		return new PropertyRowEngineFile(n, na, tn,
			s ? (const char*)*s : "", s ? s->options().filter.c_str() : "",
			s ? s->options().initialDir.c_str() : "", s ? s->options().title.c_str() : "",
			false, FileSelectorTag::Model);
	});
}

} // namespace editor
