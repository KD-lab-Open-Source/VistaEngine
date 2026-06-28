#pragma once

typedef vector<string> ComboStrings;
typedef vector<wstring> ComboWStrings;

/// Вспомогательная функция для работы с комбо-листами
string cutTokenFromComboList(string& comboList);
void joinComboList(string& outComboList, const ComboStrings& strings, char delimeter = '|');
void splitComboList(ComboStrings& outComboStrings, const char* comboList, char delimeter = '|');
// TODO: переписать
int indexInComboListString(const char* comboList, const char* value);

// Canonicalises a type name so it matches across compilers. typeid().name()
// is spelled differently by MSVC ("struct Foo") and clang/libc++ (mangled,
// e.g. "3Foo"); legacy data files store the MSVC spelling. This demangles
// clang names and strips struct/class/enum/union keywords + whitespace to a
// stable, interned identifier. The returned pointer lives for the program's
// lifetime (safe to store as a StaticString key). See typeid_name_factory_mismatch.
const char* normalizeTypeName(const char* rawName);

void splitComboListW(ComboWStrings& combo_array, const wchar_t* ptr, wchar_t delimeter);
wstring getStringTokenByIndexW(const wchar_t* worker, int number);
int indexInComboListStringW(const wchar_t* comboList, const wchar_t* value);
