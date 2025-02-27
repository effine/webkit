/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CSSAspectRatioValue_h
#define CSSAspectRatioValue_h

#include "CSSPrimitiveValue.h"
#include "CSSValue.h"

namespace WebCore {

class CSSAspectRatioValue : public CSSValue {
public:
    static PassRefPtr<CSSAspectRatioValue> create(float numeratorValue, float denominatorValue)
    {
        return adoptRef(new CSSAspectRatioValue(numeratorValue, denominatorValue));
    }

    String customCSSText() const;

    float numeratorValue() const { return m_numeratorValue; }
    float denominatorValue() const { return m_denominatorValue; }

    bool equals(const CSSAspectRatioValue&) const;

private:
    CSSAspectRatioValue(float numeratorValue, float denominatorValue)
        : CSSValue(AspectRatioClass)
        , m_numeratorValue(numeratorValue)
        , m_denominatorValue(denominatorValue)
    {
    }

    float m_numeratorValue;
    float m_denominatorValue;
};

CSS_VALUE_TYPE_CASTS(AspectRatioValue)

}

#endif
