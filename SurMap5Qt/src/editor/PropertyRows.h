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

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <type_traits>

namespace editor {

// Strict UTF-8 validation: invalid sequences (overlongs, surrogates,
// truncated tails) reject the text. Engine string data mixes UTF-8 and
// cp1251 fields; the Qt side displays cp1251-decoded text for the latter
// and encodes edits back, so Russian text round-trips.
inline bool isValidUtf8(const std::string& text)
{
	size_t i = 0;
	const size_t n = text.size();
	while(i < n){
		const unsigned char c = (unsigned char)text[i];
		size_t extra = 0;
		unsigned min = 0;
		if(c < 0x80){
			++i;
			continue;
		}
		else if((c & 0xE0) == 0xC0){
			extra = 1;
			min = 0x80;
			if(c < 0xC2)
				return false; // overlong
		}
		else if((c & 0xF0) == 0xE0){
			extra = 2;
			min = 0x800;
		}
		else if((c & 0xF8) == 0xF0){
			extra = 3;
			min = 0x10000;
			if(c > 0xF4)
				return false; // beyond U+10FFFF
		}
		else
			return false;
		if(i + extra >= n)
			return false;
		unsigned code = c & ((1 << (6 - extra)) - 1);
		for(size_t k = 1; k <= extra; ++k){
			const unsigned char d = (unsigned char)text[i + k];
			if((d & 0xC0) != 0x80)
				return false;
			code = (code << 6) | (d & 0x3F);
		}
		if(code < min || (code >= 0xD800 && code <= 0xDFFF))
			return false;
		i += 1 + extra;
	}
	return true;
}

// --- Edit parsing (PropertyTree itemChanged -> setValueFromString) --------
// Strict whole-string number parses: trailing garbage rejects the edit so a
// typo never silently truncates into the row value.

inline bool parseWhole(const std::string& text, double& out)
{
	if(text.empty())
		return false;
	char* end = nullptr;
	out = std::strtod(text.c_str(), &end);
	return end && *end == '\0';
}

inline bool parseWhole(const std::string& text, long long& out)
{
	if(text.empty())
		return false;
	char* end = nullptr;
	out = std::strtoll(text.c_str(), &end, 0);
	return end && *end == '\0';
}

inline bool parseWhole(const std::string& text, unsigned long long& out)
{
	if(text.empty() || text[0] == '-')
		return false;
	char* end = nullptr;
	out = std::strtoull(text.c_str(), &end, 0);
	return end && *end == '\0';
}

// Overloads routing each numeric row type through the right parser.
inline bool parseNumber(const std::string& text, float& out)
{
	double d = 0;
	if(!parseWhole(text, d))
		return false;
	out = (float)d;
	return true;
}

inline bool parseNumber(const std::string& text, double& out)
{
	return parseWhole(text, out);
}

template<class Type>
std::enable_if_t<std::is_integral_v<Type> && std::is_signed_v<Type>, bool>
parseNumber(const std::string& text, Type& out)
{
	long long v = 0;
	if(!parseWhole(text, v))
		return false;
	out = (Type)v;
	return true;
}

template<class Type>
std::enable_if_t<std::is_integral_v<Type> && std::is_unsigned_v<Type>, bool>
parseNumber(const std::string& text, Type& out)
{
	unsigned long long v = 0;
	if(!parseWhole(text, v))
		return false;
	out = (Type)v;
	return true;
}

// --- Concrete leaf rows ---------------------------------------------------

// A string value.
class PropertyRowString : public PropertyRowImpl<std::string>
{
public:
	PropertyRowString(const char* name, const char* nameAlt, const char* typeName, const std::string& value)
		: PropertyRowImpl<std::string>(name, nameAlt, typeName, value) {}
	std::string valueToString(const std::string& v) const override { return v; }
	bool setValueFromString(const std::string& text) override
	{
		value_ = text;
		return true;
	}
	RowKind kind() const override { return RowKind::Text; }
	// True when the source bytes are cp1251 (not valid UTF-8): the Qt side
	// decodes for display and encodes edits back so text round-trips.
	bool sourceCp1251() const { return cp1251_; }
	void setSourceCp1251(bool cp1251) { cp1251_ = cp1251; }

private:
	bool cp1251_ = false;
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
	bool setValueFromString(const std::string& text) override
	{
		Type v{};
		if(!parseNumber(text, v))
			return false;
		this->value_ = v;
		return true;
	}
	RowKind kind() const override { return RowKind::Number; }
	// True for float/double rows (the delegate shows a double spinbox),
	// false for the integral ones (int spinbox).
	bool isFloatingPoint() const { return std::is_floating_point_v<Type>; }
};

// A boolean value.
class PropertyRowBool : public PropertyRowImpl<bool>
{
public:
	PropertyRowBool(const char* name, const char* nameAlt, const char* typeName, const bool& value)
		: PropertyRowImpl<bool>(name, nameAlt, typeName, value) {}
	std::string valueToString(const bool& v) const override { return v ? "true" : "false"; }
	bool setValueFromString(const std::string& text) override
	{
		if(text == "true" || text == "1" || text == "yes" || text == "on"){
			value_ = true;
			return true;
		}
		if(text == "false" || text == "0" || text == "no" || text == "off"){
			value_ = false;
			return true;
		}
		return false;
	}
	RowKind kind() const override { return RowKind::Bool; }
};

// An enum value row (processEnum). Holds the raw int key plus the
// descriptor's entries: parallel display names (nameAlt) and keys, so the
// tree shows "Damage Type" instead of "3" and edits through a combo box
// (kdw::PropertyRowEnum + ComboBox).
class PropertyRowEnum : public PropertyRow
{
public:
	PropertyRowEnum(const char* name, const char* nameAlt, const char* typeName, int value)
		: PropertyRow(name, nameAlt, typeName), value_(value)
	{
	}

	int value() const { return value_; }
	void setValue(int v) { value_ = v; }

	// Descriptor entries (set by PropertyOArchive::processEnum).
	void setEntries(std::vector<std::string> displayNames, std::vector<int> keys)
	{
		entries_ = std::move(displayNames);
		keys_ = std::move(keys);
	}
	const std::vector<std::string>& entries() const { return entries_; }

	// Position of the current value in entries(), or -1 when stale.
	int comboIndex() const
	{
		for(size_t i = 0; i < keys_.size(); ++i)
			if(keys_[i] == value_)
				return (int)i;
		return -1;
	}
	void setComboIndex(int index)
	{
		if(index >= 0 && index < (int)keys_.size())
			value_ = keys_[index];
	}

	std::string valueAsString() const override
	{
		const int i = comboIndex();
		if(i >= 0 && i < (int)entries_.size())
			return entries_[i];
		return std::to_string(value_);
	}

	bool setValueFromString(const std::string& text) override
	{
		for(size_t i = 0; i < entries_.size(); ++i)
			if(entries_[i] == text){
				setComboIndex((int)i);
				return true;
			}
		int v = 0;
		if(!parseNumber(text, v))
			return false;
		value_ = v;
		return true;
	}

	bool assignTo(void* object, int size) override
	{
		if(!object || size < (int)sizeof(int))
			return false;
		*reinterpret_cast<int*>(object) = value_;
		return true;
	}

	RowKind kind() const override { return RowKind::Enum; }

	int value_;
	std::vector<std::string> entries_;   // display names, same order as keys_
	std::vector<int> keys_;              // enum keys
};

// A bit-vector flags row (processBitVector). Like the enum row, but several
// entries can be set at once; the tree shows "Smoke | Fire" and edits
// through a checklist (kdw::PropertyRowBitVector + CheckComboBox).
class PropertyRowBitVector : public PropertyRow
{
public:
	PropertyRowBitVector(const char* name, const char* nameAlt, const char* typeName, int flags)
		: PropertyRow(name, nameAlt, typeName), flags_(flags)
	{
	}

	int flags() const { return flags_; }
	void setFlags(int f) { flags_ = f; }

	// Flag entries (set by PropertyOArchive::processBitVector).
	void setEntries(std::vector<std::string> displayNames, std::vector<int> keys)
	{
		entries_ = std::move(displayNames);
		keys_ = std::move(keys);
	}
	const std::vector<std::string>& entries() const { return entries_; }
	const std::vector<int>& keys() const { return keys_; }

	std::string valueAsString() const override
	{
		std::string out;
		for(size_t i = 0; i < keys_.size(); ++i)
			if(flags_ & keys_[i]){
				if(!out.empty())
					out += " | ";
				out += entries_[i];
			}
		return out;
	}

	bool setValueFromString(const std::string& text) override
	{
		// Either a plain number or a "Name | Name" combination.
		int v = 0;
		if(parseNumber(text, v)){
			flags_ = v;
			return true;
		}
		int flags = 0;
		size_t pos = 0;
		while(pos <= text.size()){
			size_t bar = text.find('|', pos);
			std::string token = text.substr(pos, bar == std::string::npos ? bar : bar - pos);
			// Trim spaces.
			while(!token.empty() && token.front() == ' ')
				token.erase(token.begin());
			while(!token.empty() && token.back() == ' ')
				token.pop_back();
			if(token.empty())
				return false;
			bool found = false;
			for(size_t i = 0; i < entries_.size(); ++i)
				if(entries_[i] == token){
					flags |= keys_[i];
					found = true;
					break;
				}
			if(!found)
				return false;
			if(bar == std::string::npos)
				break;
			pos = bar + 1;
		}
		flags_ = flags;
		return true;
	}

	bool assignTo(void* object, int size) override
	{
		if(!object || size < (int)sizeof(int))
			return false;
		*reinterpret_cast<int*>(object) = flags_;
		return true;
	}

	RowKind kind() const override { return RowKind::Flags; }

	int flags_;
	std::vector<std::string> entries_;   // display names, same order as keys_
	std::vector<int> keys_;              // flag bits
};

// A color value row (Color3c/Color4c/Color4f port). Components are 0..255
// doubles; hasAlpha is false for Color3c. The tree shows a swatch plus
// "#RRGGBB[AA]" and edits through a color dialog. The engine-side subclass
// (PropertyRowsEngine.h) copies the engine color in and writes it back.
class PropertyRowColor : public PropertyRow
{
public:
	PropertyRowColor(const char* name, const char* nameAlt, const char* typeName,
	                 double r, double g, double b, double a, bool hasAlpha)
		: PropertyRow(name, nameAlt, typeName)
		, r_(r), g_(g), b_(b), a_(a), hasAlpha_(hasAlpha)
	{
	}

	void color(double& r, double& g, double& b, double& a) const
	{
		r = r_; g = g_; b = b_; a = a_;
	}
	bool hasAlpha() const { return hasAlpha_; }
	void setColor(double r, double g, double b, double a)
	{
		r_ = clampByte(r); g_ = clampByte(g); b_ = clampByte(b); a_ = clampByte(a);
	}
	// Optional palette (ComboListColor port): candidate colors shown as the
	// color dialog's custom colors. Empty = free color.
	virtual bool colorPalette(std::vector<std::array<double, 4>>&) const { return false; }

	std::string valueAsString() const override
	{
		char buf[16];
		if(hasAlpha_)
			snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X",
			         (int)r_, (int)g_, (int)b_, (int)a_);
		else
			snprintf(buf, sizeof(buf), "#%02X%02X%02X", (int)r_, (int)g_, (int)b_);
		return buf;
	}

	bool setValueFromString(const std::string& text) override
	{
		// "#RRGGBB[AA]" or "r g b [a]".
		if(!text.empty() && text[0] == '#'){
			unsigned v = 0;
			if(sscanf(text.c_str() + 1, "%x", &v) != 1)
				return false;
			if(text.size() == 7){
				setColor((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF, a_);
				return true;
			}
			if(text.size() == 9){
				setColor((v >> 24) & 0xFF, (v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF);
				return true;
			}
			return false;
		}
		double c[4] = { r_, g_, b_, a_ };
		size_t pos = 0;
		for(int i = 0; i < 4; ++i){
			while(pos < text.size() && text[pos] == ' ')
				++pos;
			if(pos >= text.size())
				break;
			char* end = nullptr;
			c[i] = std::strtod(text.c_str() + pos, &end);
			if(end == text.c_str() + pos)
				return false;
			pos = end - text.c_str();
		}
		while(pos < text.size() && text[pos] == ' ')
			++pos;
		if(pos != text.size())
			return false;
		setColor(c[0], c[1], c[2], c[3]);
		return true;
	}

	RowKind kind() const override { return RowKind::Color; }

protected:
	static double clampByte(double v) { return v < 0 ? 0 : v > 255 ? 255 : v; }

	double r_, g_, b_, a_;
	bool hasAlpha_;
};

// A combo-box string row (ComboListString port): the value must be picked
// from entries, but free text is accepted too (the combo is editable).
class PropertyRowCombo : public PropertyRow
{
public:
	PropertyRowCombo(const char* name, const char* nameAlt, const char* typeName,
	                 const std::string& value)
		: PropertyRow(name, nameAlt, typeName), value_(value)
	{
	}

	const std::string& value() const { return value_; }
	void setEntries(std::vector<std::string> entries) { entries_ = std::move(entries); }
	const std::vector<std::string>& entries() const { return entries_; }

	int comboIndex() const
	{
		for(size_t i = 0; i < entries_.size(); ++i)
			if(entries_[i] == value_)
				return (int)i;
		return -1;
	}
	void setComboIndex(int index)
	{
		if(index >= 0 && index < (int)entries_.size())
			value_ = entries_[index];
	}

	std::string valueAsString() const override { return value_; }
	bool setValueFromString(const std::string& text) override
	{
		value_ = text;
		return true;
	}

	RowKind kind() const override { return RowKind::Combo; }
	bool sourceCp1251() const { return cp1251_; }
	void setSourceCp1251(bool cp1251) { cp1251_ = cp1251; }

	std::string value_;
	std::vector<std::string> entries_;

private:
	bool cp1251_ = false;
};

// A ranged scalar row (RangedWrapperf/i port): value in [min, max], snapped
// to step (0 = free). Edits through a slider + spinbox pair; text edits
// clip and snap the same way (kdw::PropertyRowRanged::handleMouse).
class PropertyRowRanged : public PropertyRow
{
public:
	PropertyRowRanged(const char* name, const char* nameAlt, const char* typeName,
	                  double value, double minimum, double maximum, double step, bool integer)
		: PropertyRow(name, nameAlt, typeName)
		, value_(value), minimum_(minimum), maximum_(maximum), step_(step), integer_(integer)
	{
		clip();
	}

	double value() const { return value_; }
	double minimum() const { return minimum_; }
	double maximum() const { return maximum_; }
	double step() const { return step_; }
	bool isInteger() const { return integer_; }
	void setValue(double v)
	{
		value_ = v;
		clip();
	}

	std::string valueAsString() const override
	{
		if(integer_)
			return std::to_string((long long)value_);
		return std::to_string(value_);
	}

	bool setValueFromString(const std::string& text) override
	{
		double v = 0;
		if(!parseWhole(text, v))
			return false;
		setValue(v);
		return true;
	}

	RowKind kind() const override { return RowKind::Ranged; }

protected:
	void clip()
	{
		if(step_ != 0)
			value_ -= std::fmod(value_, step_);
		if(value_ < minimum_)
			value_ = minimum_;
		if(value_ > maximum_)
			value_ = maximum_;
		if(integer_)
			value_ = std::round(value_);
	}

	double value_, minimum_, maximum_, step_;
	bool integer_;
};

// A file-path row (GenericFileSelector/ResourceSelector/ModelSelector
// port): the path plus the dialog's filter/initial directory. Edits through
// a line edit + "..." file-dialog button. The engine-side subclass snapshots
// the selector and writes the path back through setFileName.
class PropertyRowFile : public PropertyRow
{
public:
	PropertyRowFile(const char* name, const char* nameAlt, const char* typeName,
	                const std::string& path, const std::string& filter,
	                const std::string& initialDir, const std::string& title, bool save)
		: PropertyRow(name, nameAlt, typeName)
		, path_(path), filter_(filter), initialDir_(initialDir), title_(title), save_(save)
	{
	}

	const std::string& path() const { return path_; }
	const std::string& filter() const { return filter_; }
	const std::string& initialDir() const { return initialDir_; }
	const std::string& title() const { return title_; }
	bool save() const { return save_; }
	void setPath(const std::string& path) { path_ = path; }

	std::string valueAsString() const override { return path_; }
	bool setValueFromString(const std::string& text) override
	{
		path_ = text;
		return true;
	}

	RowKind kind() const override { return RowKind::File; }
	bool sourceCp1251() const { return cp1251_; }
	void setSourceCp1251(bool cp1251) { cp1251_ = cp1251; }

	std::string path_, filter_, initialDir_, title_;
	bool save_;

private:
	bool cp1251_ = false;
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