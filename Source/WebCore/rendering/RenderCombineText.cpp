/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "RenderCombineText.h"

#include "RenderBlock.h"
#include "StyleInheritedData.h"

namespace WebCore {

const float textCombineMargin = 1.15f; // Allow em + 15% margin

RenderCombineText::RenderCombineText(Text& textNode, PassRefPtr<StringImpl> string)
    : RenderText(&textNode, string)
    , m_combinedTextWidth(0)
    , m_isCombined(false)
    , m_needsFontUpdate(false)
{
}

void RenderCombineText::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    // FIXME: This is pretty hackish.
    m_combineFontStyle = RenderStyle::clone(style());

    RenderText::styleDidChange(diff, oldStyle);

    if (m_isCombined) {
        RenderText::setTextInternal(originalText()); // This RenderCombineText has been combined once. Restore the original text for the next combineText().
        m_isCombined = false;
    }

    m_needsFontUpdate = true;
}

void RenderCombineText::setTextInternal(const String& text)
{
    RenderText::setTextInternal(text);

    m_needsFontUpdate = true;
}

float RenderCombineText::width(unsigned from, unsigned length, const Font& font, float xPosition, HashSet<const SimpleFontData*>* fallbackFonts, GlyphOverflow* glyphOverflow) const
{
    if (!characters())
        return 0;

    if (m_isCombined)
        return font.size();

    return RenderText::width(from, length, font, xPosition, fallbackFonts, glyphOverflow);
}

void RenderCombineText::adjustTextOrigin(FloatPoint& textOrigin, const FloatRect& boxRect) const
{
    if (m_isCombined)
        textOrigin.move(boxRect.height() / 2 - ceilf(m_combinedTextWidth) / 2, style()->font().pixelSize());
}

void RenderCombineText::getStringToRender(int start, String& string, int& length) const
{
    ASSERT(start >= 0);
    if (m_isCombined) {
        string = originalText();
        length = string.length();
        return;
    }
 
    string = text();
    string = string.substringSharingImpl(static_cast<unsigned>(start), length);
}

void RenderCombineText::combineText()
{
    if (!m_needsFontUpdate)
        return;

    m_isCombined = false;
    m_needsFontUpdate = false;

    // CSS3 spec says text-combine works only in vertical writing mode.
    if (style()->isHorizontalWritingMode())
        return;

    TextRun run = RenderBlock::constructTextRun(this, originalFont(), this, *style());
    FontDescription description = originalFont().fontDescription();
    float emWidth = description.computedSize() * textCombineMargin;
    bool shouldUpdateFont = false;

    description.setOrientation(Horizontal); // We are going to draw combined text horizontally.
    m_combinedTextWidth = originalFont().width(run);
    m_isCombined = m_combinedTextWidth <= emWidth;

    FontSelector* fontSelector = style()->font().fontSelector();

    if (m_isCombined)
        shouldUpdateFont = m_combineFontStyle->setFontDescription(description); // Need to change font orientation to horizontal.
    else {
        // Need to try compressed glyphs.
        static const FontWidthVariant widthVariants[] = { HalfWidth, ThirdWidth, QuarterWidth };
        for (size_t i = 0 ; i < WTF_ARRAY_LENGTH(widthVariants) ; ++i) {
            description.setWidthVariant(widthVariants[i]);
            Font compressedFont = Font(description, style()->font().letterSpacing(), style()->font().wordSpacing());
            compressedFont.update(fontSelector);
            float runWidth = compressedFont.width(run);
            if (runWidth <= emWidth) {
                m_combinedTextWidth = runWidth;
                m_isCombined = true;

                // Replace my font with the new one.
                shouldUpdateFont = m_combineFontStyle->setFontDescription(description);
                break;
            }
        }
    }

    if (!m_isCombined)
        shouldUpdateFont = m_combineFontStyle->setFontDescription(originalFont().fontDescription());

    if (shouldUpdateFont)
        m_combineFontStyle->font().update(fontSelector);

    if (m_isCombined) {
        DEFINE_STATIC_LOCAL(String, objectReplacementCharacterString, (&objectReplacementCharacter, 1));
        RenderText::setTextInternal(objectReplacementCharacterString.impl());
    }
}

} // namespace WebCore
