/*
 * Copyright (C) 2008, 2013 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JavaScriptCallFrame_h
#define JavaScriptCallFrame_h

#if ENABLE(JAVASCRIPT_DEBUGGER)

#include <debugger/DebuggerCallFrame.h>
#include <interpreter/CallFrame.h>
#include <wtf/Forward.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/TextPosition.h>

namespace WebCore {

class JavaScriptCallFrame : public RefCounted<JavaScriptCallFrame> {
public:
    static PassRefPtr<JavaScriptCallFrame> create(PassRefPtr<JSC::DebuggerCallFrame> debuggerCallFrame)
    {
        return adoptRef(new JavaScriptCallFrame(debuggerCallFrame));
    }

    JavaScriptCallFrame* caller();
    intptr_t sourceID() const { return m_debuggerCallFrame->sourceId(); }
    const TextPosition position() const { return m_debuggerCallFrame->position(); }
    int line() const { return m_debuggerCallFrame->line(); }
    int column() const { return m_debuggerCallFrame->column(); }

    String functionName() const { return m_debuggerCallFrame->functionName(); }
    JSC::DebuggerCallFrame::Type type() const { return m_debuggerCallFrame->type(); }
    JSC::JSScope* scopeChain() const { return m_debuggerCallFrame->scope(); }
    JSC::JSGlobalObject* dynamicGlobalObject() const { return m_debuggerCallFrame->dynamicGlobalObject(); }

    JSC::JSValue thisValue() const { return m_debuggerCallFrame->thisValue(); }
    JSC::JSValue evaluate(const String& script, JSC::JSValue& exception) const  { return m_debuggerCallFrame->evaluate(script, exception); }
    
private:
    JavaScriptCallFrame(PassRefPtr<JSC::DebuggerCallFrame>);

    RefPtr<JSC::DebuggerCallFrame> m_debuggerCallFrame;
    RefPtr<JavaScriptCallFrame> m_caller;
};

} // namespace WebCore

#endif // ENABLE(JAVASCRIPT_DEBUGGER)

#endif // JavaScriptCallFrame_h
