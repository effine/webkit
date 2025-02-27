/*
 * (C) 1999 Lars Knoll (knoll@kde.org)
 * (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef RenderText_h
#define RenderText_h

#include "RenderElement.h"
#include <wtf/Forward.h>

namespace WebCore {

class InlineTextBox;
class Text;

class RenderText : public RenderObject {
public:
    RenderText(Text*, const String&);
#ifndef NDEBUG
    virtual ~RenderText();
#endif

    static RenderText* createAnonymous(Document&, const String&);

    virtual const char* renderName() const OVERRIDE;

    Text* textNode() const;

    virtual bool isTextFragment() const;

    RenderStyle* style() const;
    RenderStyle* firstLineStyle() const;

    virtual String originalText() const;

    void extractTextBox(InlineTextBox*);
    void attachTextBox(InlineTextBox*);
    void removeTextBox(InlineTextBox*);

    StringImpl* text() const { return m_text.impl(); }
    String textWithoutConvertingBackslashToYenSymbol() const;

    InlineTextBox* createInlineTextBox();
    void dirtyLineBoxes(bool fullLayout);

    virtual void absoluteRects(Vector<IntRect>&, const LayoutPoint& accumulatedOffset) const OVERRIDE FINAL;
    void absoluteRectsForRange(Vector<IntRect>&, unsigned startOffset = 0, unsigned endOffset = UINT_MAX, bool useSelectionHeight = false, bool* wasFixed = 0);

    virtual void absoluteQuads(Vector<FloatQuad>&, bool* wasFixed) const OVERRIDE FINAL;
    void absoluteQuadsForRange(Vector<FloatQuad>&, unsigned startOffset = 0, unsigned endOffset = UINT_MAX, bool useSelectionHeight = false, bool* wasFixed = 0);

    enum ClippingOption { NoClipping, ClipToEllipsis };
    void absoluteQuads(Vector<FloatQuad>&, bool* wasFixed = 0, ClippingOption = NoClipping) const;

    virtual VisiblePosition positionForPoint(const LayoutPoint&) OVERRIDE;

    bool is8Bit() const { return m_text.is8Bit(); }
    const LChar* characters8() const { return m_text.impl()->characters8(); }
    const UChar* characters16() const { return m_text.impl()->characters16(); }
    const UChar* characters() const { return m_text.characters(); }
    UChar characterAt(unsigned i) const { return is8Bit() ? characters8()[i] : characters16()[i]; }
    UChar operator[](unsigned i) const { return characterAt(i); }
    unsigned textLength() const { return m_text.length(); } // non virtual implementation of length()
    void positionLineBox(InlineBox*);

    virtual float width(unsigned from, unsigned len, const Font&, float xPos, HashSet<const SimpleFontData*>* fallbackFonts = 0, GlyphOverflow* = 0) const;
    virtual float width(unsigned from, unsigned len, float xPos, bool firstLine = false, HashSet<const SimpleFontData*>* fallbackFonts = 0, GlyphOverflow* = 0) const;

    float minLogicalWidth() const;
    float maxLogicalWidth() const;

    void trimmedPrefWidths(float leadWidth,
                           float& beginMinW, bool& beginWS,
                           float& endMinW, bool& endWS,
                           bool& hasBreakableChar, bool& hasBreak,
                           float& beginMaxW, float& endMaxW,
                           float& minW, float& maxW, bool& stripFrontSpaces);

    virtual IntRect linesBoundingBox() const;
    LayoutRect linesVisualOverflowBoundingBox() const;

    FloatPoint firstRunOrigin() const;
    float firstRunX() const;
    float firstRunY() const;

    virtual void setText(const String&, bool force = false);
    void setTextWithOffset(const String&, unsigned offset, unsigned len, bool force = false);

    virtual void transformText();

    virtual bool canBeSelectionLeaf() const OVERRIDE { return true; }
    virtual void setSelectionState(SelectionState s) OVERRIDE FINAL;
    virtual LayoutRect selectionRectForRepaint(const RenderLayerModelObject* repaintContainer, bool clipToVisibleContent = true) OVERRIDE;
    virtual LayoutRect localCaretRect(InlineBox*, int caretOffset, LayoutUnit* extraWidthToEndOfLine = 0) OVERRIDE;

    LayoutUnit marginLeft() const { return minimumValueForLength(style()->marginLeft(), 0); }
    LayoutUnit marginRight() const { return minimumValueForLength(style()->marginRight(), 0); }

    virtual LayoutRect clippedOverflowRectForRepaint(const RenderLayerModelObject* repaintContainer) const OVERRIDE FINAL;

    InlineTextBox* firstTextBox() const { return m_firstTextBox; }
    InlineTextBox* lastTextBox() const { return m_lastTextBox; }

    virtual int caretMinOffset() const OVERRIDE;
    virtual int caretMaxOffset() const OVERRIDE;
    unsigned renderedTextLength() const;

    virtual int previousOffset(int current) const OVERRIDE FINAL;
    virtual int previousOffsetForBackwardDeletion(int current) const OVERRIDE FINAL;
    virtual int nextOffset(int current) const OVERRIDE FINAL;

    bool containsReversedText() const { return m_containsReversedText; }

    bool isSecure() const { return style()->textSecurity() != TSNONE; }
    void momentarilyRevealLastTypedCharacter(unsigned lastTypedCharacterOffset);

    InlineTextBox* findNextInlineTextBox(int offset, int& pos) const;

    void checkConsistency() const;

    bool isAllCollapsibleWhitespace() const;

    bool canUseSimpleFontCodePath() const { return m_canUseSimpleFontCodePath; }
    bool knownToHaveNoOverflowAndNoFallbackFonts() const { return m_knownToHaveNoOverflowAndNoFallbackFonts; }

    void removeAndDestroyTextBoxes();

    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);

#if ENABLE(IOS_TEXT_AUTOSIZING)
    float candidateComputedTextSize() const { return m_candidateComputedTextSize; }
    void setCandidateComputedTextSize(float s) { m_candidateComputedTextSize = s; }
#endif

protected:
    virtual void computePreferredLogicalWidths(float leadWidth);
    virtual void willBeDestroyed() OVERRIDE;

    virtual void setTextInternal(const String&);
    virtual UChar previousCharacter() const;
    
    virtual InlineTextBox* createTextBox(); // Subclassed by SVG.

private:
    virtual bool canHaveChildren() const OVERRIDE FINAL { return false; }

    void computePreferredLogicalWidths(float leadWidth, HashSet<const SimpleFontData*>& fallbackFonts, GlyphOverflow&);

    bool computeCanUseSimpleFontCodePath() const;
    
    // Make length() private so that callers that have a RenderText*
    // will use the more efficient textLength() instead, while
    // callers with a RenderObject* can continue to use length().
    virtual unsigned length() const OVERRIDE FINAL { return textLength(); }

    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, const HitTestLocation&, const LayoutPoint&, HitTestAction) OVERRIDE FINAL { ASSERT_NOT_REACHED(); return false; }

    void deleteTextBoxes();
    bool containsOnlyWhitespace(unsigned from, unsigned len) const;
    float widthFromCache(const Font&, int start, int len, float xPos, HashSet<const SimpleFontData*>* fallbackFonts, GlyphOverflow*) const;
    bool isAllASCII() const { return m_isAllASCII; }
    bool computeUseBackslashAsYenSymbol() const;

    void secureText(UChar mask);

    void node() const WTF_DELETED_FUNCTION;

    // We put the bitfield first to minimize padding on 64-bit.
    bool m_hasBreakableChar : 1; // Whether or not we can be broken into multiple lines.
    bool m_hasBreak : 1; // Whether or not we have a hard break (e.g., <pre> with '\n').
    bool m_hasTab : 1; // Whether or not we have a variable width tab character (e.g., <pre> with '\t').
    bool m_hasBeginWS : 1; // Whether or not we begin with WS (only true if we aren't pre)
    bool m_hasEndWS : 1; // Whether or not we end with WS (only true if we aren't pre)
    bool m_linesDirty : 1; // This bit indicates that the text run has already dirtied specific
                           // line boxes, and this hint will enable layoutInlineChildren to avoid
                           // just dirtying everything when character data is modified (e.g., appended/inserted
                           // or removed).
    bool m_containsReversedText : 1;
    bool m_isAllASCII : 1;
    bool m_canUseSimpleFontCodePath : 1;
    mutable bool m_knownToHaveNoOverflowAndNoFallbackFonts : 1;
    bool m_useBackslashAsYenSymbol : 1;
    
#if ENABLE(IOS_TEXT_AUTOSIZING)
    // FIXME: This should probably be part of the text sizing structures in Document instead. That would save some memory.
    float m_candidateComputedTextSize;
#endif
    float m_minWidth;
    float m_maxWidth;
    float m_beginMinWidth;
    float m_endMinWidth;

    String m_text;

    InlineTextBox* m_firstTextBox;
    InlineTextBox* m_lastTextBox;
};

inline RenderText& toRenderText(RenderObject& object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(object.isText());
    return static_cast<RenderText&>(object);
}

inline const RenderText& toRenderText(const RenderObject& object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(object.isText());
    return static_cast<const RenderText&>(object);
}

inline RenderText* toRenderText(RenderObject* object)
{ 
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isText());
    return static_cast<RenderText*>(object);
}

inline const RenderText* toRenderText(const RenderObject* object)
{ 
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isText());
    return static_cast<const RenderText*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderText(const RenderText*);
void toRenderText(const RenderText&);

inline RenderStyle* RenderText::style() const
{
    return parent()->style();
}

inline RenderStyle* RenderText::firstLineStyle() const
{
    return parent()->firstLineStyle();
}

#ifdef NDEBUG
inline void RenderText::checkConsistency() const
{
}
#endif

void applyTextTransform(const RenderStyle*, String&, UChar);

} // namespace WebCore

#endif // RenderText_h
