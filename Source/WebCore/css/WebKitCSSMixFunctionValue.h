/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef WebKitCSSMixFunctionValue_h
#define WebKitCSSMixFunctionValue_h

#if ENABLE(CSS_SHADERS)

#include "CSSValueList.h"
#include <wtf/PassRefPtr.h>

namespace WebCore {

class WebKitCSSMixFunctionValue : public CSSValueList {
public:
    static PassRefPtr<WebKitCSSMixFunctionValue> create()
    {
        return adoptRef(new WebKitCSSMixFunctionValue());
    }

    String customCSSText() const;

    PassRefPtr<WebKitCSSMixFunctionValue> cloneForCSSOM() const;

    bool equals(const WebKitCSSMixFunctionValue&) const;

private:
    WebKitCSSMixFunctionValue();
    WebKitCSSMixFunctionValue(const WebKitCSSMixFunctionValue& cloneFrom);
};

inline WebKitCSSMixFunctionValue* toWebKitCSSMixFunctionValue(CSSValue* value)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!value || value->isWebKitCSSMixFunctionValue());
    return static_cast<WebKitCSSMixFunctionValue*>(value);
}

inline const WebKitCSSMixFunctionValue* toWebKitCSSMixFunctionValue(const CSSValue* value)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!value || value->isWebKitCSSMixFunctionValue());
    return static_cast<const WebKitCSSMixFunctionValue*>(value);
}

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS)

#endif
