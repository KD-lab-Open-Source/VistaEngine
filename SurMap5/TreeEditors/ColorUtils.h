#ifndef __COLOR_UTILS_H_INCLUDED__
#define __COLOR_UTILS_H_INCLUDED__

inline COLORREF toColorRef (const sColor4f& color) {
        return RGB(round(color.r * 255.0f), round(color.g * 255.0f), round(color.b * 255.0f));
}

#endif
