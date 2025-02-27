/*
 * Copyright (C) 2008-2013 Apple Inc. All rights reserved.
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

#ifndef WebScriptDebugger_h
#define WebScriptDebugger_h

#include <heap/Strong.h>
#include <debugger/Debugger.h>

#include <wtf/RetainPtr.h>

namespace WTF {
class String;
}

namespace JSC {
    class DebuggerCallFrame;
    class ExecState;
    class JSGlobalObject;
    class JSObject;
    class ArgList;
}

@class WebScriptCallFrame;

class WebScriptDebugger FINAL : public JSC::Debugger {
public:
    WebScriptDebugger(JSC::JSGlobalObject*);

    JSC::JSGlobalObject* globalObject() const { return m_globalObject.get(); }

private:
    virtual void sourceParsed(JSC::ExecState*, JSC::SourceProvider*, int errorLine, const WTF::String& errorMsg) OVERRIDE;
    virtual void callEvent(JSC::CallFrame*) OVERRIDE { }
    virtual void atStatement(JSC::CallFrame*) OVERRIDE { }
    virtual void returnEvent(JSC::CallFrame*) OVERRIDE { }
    virtual void exception(JSC::CallFrame*, JSC::JSValue exceptionValue, bool hasHandler) OVERRIDE;
    virtual void willExecuteProgram(JSC::CallFrame*) OVERRIDE { }
    virtual void didExecuteProgram(JSC::CallFrame*) OVERRIDE { }
    virtual void didReachBreakpoint(JSC::CallFrame*) OVERRIDE { }

    bool m_callingDelegate;

    JSC::Strong<JSC::JSGlobalObject> m_globalObject;
};

#endif // WebScriptDebugger_h
