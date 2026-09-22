// PropertyText.h — engine-string display/edit helpers for the Qt side.
//
// Engine string data mixes UTF-8 and cp1251 fields (library element names,
// attribute values). Valid UTF-8 shows as-is; anything else decodes as
// cp1251 — the same fallback the old editor effectively used by showing
// system-locale text. Edits encode back to the row's source encoding so
// Russian text round-trips instead of turning into mojibake.
//
// Qt-clean and engine-free (the cp1251 table is inline, no codec needed).

#pragma once

#include <QString>

#include <string>

#include "editor/PropertyRows.h"   // RowKind + row classes (engine-free)

namespace propertytext {

// cp1251 byte (0x80..0xFF) -> Unicode. 0x98 is unassigned in cp1251.
inline char32_t cp1251ToUnicode(unsigned char c)
{
	static const char32_t table[128] = {
		0x20AC, 0x0402, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021, // 80-87
		0x02C6, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F, // 88-8F
		0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, // 90-97
		0xFFFD, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F, // 98-9F
		0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7, // A0-A7
		0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407, // A8-AF
		0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7, // B0-B7
		0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457, // B8-BF
		0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, // C0-C7
		0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F, // C8-CF
		0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, // D0-D7
		0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F, // D8-DF
		0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, // E0-E7
		0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F, // E8-EF
		0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, // F0-F7
		0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F, // F8-FF
	};
	if(c < 0x80)
		return c;
	return table[c - 0x80];
}

// Unicode -> cp1251 byte. Returns false when unrepresentable (caller emits '?').
inline bool unicodeToCp1251(char32_t u, unsigned char& out)
{
	if(u < 0x80){
		out = (unsigned char)u;
		return true;
	}
	if(u >= 0x0410 && u <= 0x044F){
		out = (unsigned char)(u - 0x0410 + 0xC0);
		return true;
	}
	for(int c = 0x80; c < 0x100; ++c){
		if(cp1251ToUnicode((unsigned char)c) == u){
			out = (unsigned char)c;
			return true;
		}
	}
	return false;
}

// Display raw engine bytes: valid UTF-8 as-is, otherwise cp1251-decoded.
inline QString displayBytes(const std::string& raw)
{
	if(raw.empty() || editor::isValidUtf8(raw))
		return QString::fromUtf8(raw.data(), (qsizetype)raw.size());
	QString out;
	out.reserve((qsizetype)raw.size());
	for(unsigned char c : raw){
		if(c < 0x80)
			out.append(QChar(c));
		else
			out.append(QChar(cp1251ToUnicode(c)));
	}
	return out;
}

// Display a row's value cell: string-like rows decode through displayBytes,
// everything else (numbers, enums, colors) is plain ASCII.
inline QString displayRowText(const editor::PropertyRow* row)
{
	if(!row)
		return QString();
	switch(row->kind()){
	case editor::RowKind::Text:
	case editor::RowKind::Combo:
	case editor::RowKind::File:
		return displayBytes(row->valueAsString());
	default:
		return QString::fromStdString(row->valueAsString());
	}
}

// Encode edited text back to the row's source encoding (cp1251 rows take
// the table path so Russian text round-trips; the rest is UTF-8).
inline std::string encodeRowBytes(const QString& text, bool cp1251)
{
	if(!cp1251)
		return text.toStdString();
	std::string out;
	out.reserve(text.size());
	for(QChar ch : text){
		unsigned char c = '?';
		unicodeToCp1251(ch.unicode(), c);
		out.push_back((char)c);
	}
	return out;
}

// Commit edited display text into a string-like row with encoding round-trip.
// Returns false when the text does not parse (row keeps the old value).
inline bool commitRowText(editor::PropertyRow* row, const QString& text)
{
	if(!row)
		return false;
	switch(row->kind()){
	case editor::RowKind::Text: {
		auto* stringRow = static_cast<editor::PropertyRowString*>(row);
		return stringRow->setValueFromString(
			encodeRowBytes(text, stringRow->sourceCp1251()));
	}
	case editor::RowKind::Combo: {
		auto* comboRow = static_cast<editor::PropertyRowCombo*>(row);
		return comboRow->setValueFromString(
			encodeRowBytes(text, comboRow->sourceCp1251()));
	}
	case editor::RowKind::File: {
		auto* fileRow = static_cast<editor::PropertyRowFile*>(row);
		fileRow->setPath(encodeRowBytes(text, fileRow->sourceCp1251()));
		return true;
	}
	default:
		return row->setValueFromString(text.toStdString());
	}
}

} // namespace propertytext
