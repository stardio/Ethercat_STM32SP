#ifndef GUI_COMMON_NUMBERFORMAT_HPP
#define GUI_COMMON_NUMBERFORMAT_HPP

#include <stdint.h>
#include <touchgfx/Color.hpp>
#include <touchgfx/Font.hpp>
#include <touchgfx/Unicode.hpp>
#include <touchgfx/containers/progress_indicators/TextProgress.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>
#include <texts/TextKeysAndLanguages.hpp>

namespace gui
{
static const uint16_t kNumericBufferSize = 24U;

inline uint32_t magnitudeOf(int32_t value)
{
    return (value < 0) ? (static_cast<uint32_t>(-(value + 1)) + 1U) : static_cast<uint32_t>(value);
}

inline void formatUnsignedWithCommas(uint32_t value, touchgfx::Unicode::UnicodeChar* out, uint16_t outSize)
{
    if ((out == 0) || (outSize == 0U))
    {
        return;
    }

    char reversed[20];
    uint16_t reversedIndex = 0U;
    uint16_t groupCount = 0U;

    do
    {
        if ((groupCount == 3U) && (reversedIndex < (sizeof(reversed) - 1U)))
        {
            reversed[reversedIndex++] = ',';
            groupCount = 0U;
        }

        if (reversedIndex < (sizeof(reversed) - 1U))
        {
            reversed[reversedIndex++] = static_cast<char>('0' + (value % 10U));
        }

        value /= 10U;
        groupCount++;
    } while ((value > 0U) && (reversedIndex < (sizeof(reversed) - 1U)));

    uint16_t outIndex = 0U;
    while ((reversedIndex > 0U) && (outIndex < (outSize - 1U)))
    {
        out[outIndex++] = static_cast<touchgfx::Unicode::UnicodeChar>(reversed[--reversedIndex]);
    }
    out[outIndex] = 0;
}

inline void formatSignedWithCommas(int32_t value, touchgfx::Unicode::UnicodeChar* out, uint16_t outSize)
{
    if ((out == 0) || (outSize == 0U))
    {
        return;
    }

    if (value < 0)
    {
        if (outSize < 2U)
        {
            out[0] = 0;
            return;
        }

        out[0] = '-';
        formatUnsignedWithCommas(magnitudeOf(value), out + 1, outSize - 1U);
        return;
    }

    formatUnsignedWithCommas(static_cast<uint32_t>(value), out, outSize);
}

inline void formatAbsoluteWithCommas(int32_t value, touchgfx::Unicode::UnicodeChar* out, uint16_t outSize)
{
    formatUnsignedWithCommas(magnitudeOf(value), out, outSize);
}

inline void formatTorquePercent(int16_t torqueValue, touchgfx::Unicode::UnicodeChar* out, uint16_t outSize)
{
    formatUnsignedWithCommas(static_cast<uint32_t>(magnitudeOf(static_cast<int32_t>(torqueValue)) / 10U), out, outSize);
}

inline bool hasWildcardPlaceholder(const touchgfx::TypedText& typedText)
{
    const touchgfx::Unicode::UnicodeChar* text = typedText.getText();
    if (text == 0)
    {
        return false;
    }

    for (const touchgfx::Unicode::UnicodeChar* current = text; *current != 0; current++)
    {
        if ((current[0] == '<') && (current[1] == '>'))
        {
            return true;
        }
    }

    return false;
}

inline touchgfx::TypedText wildcardTypedTextForAlignment(touchgfx::Alignment alignment)
{
    if (alignment == touchgfx::RIGHT)
    {
        return touchgfx::TypedText(T_NUMERIC_WILDCARD_RIGHT);
    }
    if (alignment == touchgfx::CENTER)
    {
        return touchgfx::TypedText(T_NUMERIC_WILDCARD_CENTER);
    }
    return touchgfx::TypedText(T_NUMERIC_WILDCARD_LEFT);
}

template <typename TSource>
inline void configureNumericOverlay(touchgfx::TextAreaWithOneWildcard& overlay, TSource& source, touchgfx::Unicode::UnicodeChar* buffer)
{
    int16_t overlayX = source.getX();
    int16_t overlayY = source.getY();
    int16_t overlayWidth = source.getWidth();
    int16_t overlayHeight = source.getHeight();

    const uint16_t sourceTextWidth = source.getTextWidth();
    const int16_t sourceTextHeight = source.getTextHeight();
    const touchgfx::TypedText sourceTypedText = source.getTypedText();
    const touchgfx::Font* font = source.getTypedText().getFont();

    if (overlayWidth <= 0)
    {
        int16_t anchorWidth = 0;
        if (sourceTextWidth > 0U)
        {
            anchorWidth = static_cast<int16_t>(sourceTextWidth);
        }
        else if ((font != 0) && (sourceTypedText.getText() != 0))
        {
            anchorWidth = static_cast<int16_t>(font->getStringWidth(sourceTypedText.getText()));
        }

        overlayWidth = 220;

        if ((anchorWidth > 0) && (sourceTypedText.getAlignment() == touchgfx::RIGHT))
        {
            overlayX -= (overlayWidth - anchorWidth);
        }
        else if ((anchorWidth > 0) && (sourceTypedText.getAlignment() == touchgfx::CENTER))
        {
            overlayX -= ((overlayWidth - anchorWidth) / 2);
        }
    }

    if (overlayHeight <= 0)
    {
        if (sourceTextHeight > 0)
        {
            overlayHeight = sourceTextHeight;
        }
        else if (font != 0)
        {
            overlayHeight = static_cast<int16_t>(font->getHeight() + source.getLinespacing());
        }
        else
        {
            overlayHeight = 30;
        }
    }

    touchgfx::colortype overlayColor = source.getColor();
    if (overlayColor == touchgfx::Color::getColorFromRGB(0, 0, 0))
    {
        overlayColor = touchgfx::Color::getColorFromRGB(255, 255, 255);
    }

    overlay.setPosition(overlayX, overlayY, overlayWidth, overlayHeight);
    overlay.setLinespacing(source.getLinespacing());
    overlay.setColor(overlayColor);
    overlay.setTypedText(hasWildcardPlaceholder(sourceTypedText) ? sourceTypedText : wildcardTypedTextForAlignment(sourceTypedText.getAlignment()));
    overlay.setWildcard(buffer);
    source.setVisible(false);
}

inline void configureNumericOverlay(touchgfx::TextAreaWithOneWildcard& overlay, touchgfx::TextProgress& source, touchgfx::Unicode::UnicodeChar* buffer)
{
    int16_t overlayX = source.getX();
    int16_t overlayY = source.getY();
    int16_t overlayWidth = source.getWidth();
    int16_t overlayHeight = source.getHeight();

    const touchgfx::Font* font = source.getTypedText().getFont();

    if (overlayWidth <= 0)
    {
        overlayWidth = 220;
    }

    if (overlayHeight <= 0)
    {
        if (font != 0)
        {
            overlayHeight = static_cast<int16_t>(font->getHeight());
        }
        else
        {
            overlayHeight = 30;
        }
    }

    touchgfx::colortype overlayColor = source.getColor();
    if (overlayColor == touchgfx::Color::getColorFromRGB(0, 0, 0))
    {
        overlayColor = touchgfx::Color::getColorFromRGB(255, 255, 255);
    }

    const touchgfx::TypedText sourceTypedText = source.getTypedText();

    overlay.setPosition(overlayX, overlayY, overlayWidth, overlayHeight);
    overlay.setColor(overlayColor);
    overlay.setTypedText(hasWildcardPlaceholder(sourceTypedText) ? sourceTypedText : wildcardTypedTextForAlignment(sourceTypedText.getAlignment()));
    overlay.setWildcard(buffer);
    source.setVisible(false);
}
}

#endif // GUI_COMMON_NUMBERFORMAT_HPP