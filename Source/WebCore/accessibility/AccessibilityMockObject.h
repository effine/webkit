/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef AccessibilityMockObject_h
#define AccessibilityMockObject_h

#include "AccessibilityObject.h"

namespace WebCore {
    
class AccessibilityMockObject : public AccessibilityObject {
    
protected:
    AccessibilityMockObject();
public:
    virtual ~AccessibilityMockObject();
    
    virtual AccessibilityObject* parentObject() const OVERRIDE { return m_parent; }
    virtual void setParent(AccessibilityObject* parent) { m_parent = parent; };
    virtual bool isEnabled() const OVERRIDE { return true; }

protected:
    AccessibilityObject* m_parent;

    // Must be called when the parent object clears its children.
    virtual void detachFromParent() OVERRIDE { m_parent = 0; }

private:
    virtual bool isMockObject() const OVERRIDE { return true; }

    virtual bool computeAccessibilityIsIgnored() const OVERRIDE;
}; 
    
inline AccessibilityMockObject* toAccessibilityMockObject(AccessibilityObject* object)
{
    ASSERT(!object || object->isMockObject());
    return static_cast<AccessibilityMockObject*>(object);
}
    
} // namespace WebCore 

#endif // AccessibilityMockObject_h
