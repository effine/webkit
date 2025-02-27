/*
 * Copyright (C) 2010, 2012, 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ThunkGenerators.h"

#include "CodeBlock.h"
#include "JITOperations.h"
#include "JSArray.h"
#include "JSArrayIterator.h"
#include "JSStack.h"
#include "Operations.h"
#include "SpecializedThunkJIT.h"
#include <wtf/InlineASM.h>
#include <wtf/StringPrintStream.h>
#include <wtf/text/StringImpl.h>

#if ENABLE(JIT)

namespace JSC {

inline void emitPointerValidation(CCallHelpers& jit, GPRReg pointerGPR)
{
#if !ASSERT_DISABLED
    CCallHelpers::Jump isNonZero = jit.branchTestPtr(CCallHelpers::NonZero, pointerGPR);
    jit.breakpoint();
    isNonZero.link(&jit);
    jit.push(pointerGPR);
    jit.load8(pointerGPR, pointerGPR);
    jit.pop(pointerGPR);
#else
    UNUSED_PARAM(jit);
    UNUSED_PARAM(pointerGPR);
#endif
}

MacroAssemblerCodeRef throwExceptionFromCallSlowPathGenerator(VM* vm)
{
    CCallHelpers jit(vm);
    
    // We will jump to here if the JIT code thinks it's making a call, but the
    // linking helper (C++ code) decided to throw an exception instead. We will
    // have saved the callReturnIndex in the first arguments of JITStackFrame.
    // Note that the return address will be on the stack at this point, so we
    // need to remove it and drop it on the floor, since we don't care about it.
    // Finally note that the call frame register points at the callee frame, so
    // we need to pop it.
    jit.preserveReturnAddressAfterCall(GPRInfo::nonPreservedNonReturnGPR);
    jit.loadPtr(
        CCallHelpers::Address(
            GPRInfo::callFrameRegister,
            static_cast<ptrdiff_t>(sizeof(Register)) * JSStack::CallerFrame),
        GPRInfo::callFrameRegister);
#if USE(JSVALUE64)
    jit.peek64(GPRInfo::nonPreservedNonReturnGPR, JITSTACKFRAME_ARGS_INDEX);
#else
    jit.peek(GPRInfo::nonPreservedNonReturnGPR, JITSTACKFRAME_ARGS_INDEX);
#endif
    jit.setupArgumentsWithExecState(GPRInfo::nonPreservedNonReturnGPR);
    jit.move(CCallHelpers::TrustedImmPtr(bitwise_cast<void*>(lookupExceptionHandler)), GPRInfo::nonArgGPR0);
    emitPointerValidation(jit, GPRInfo::nonArgGPR0);
    jit.call(GPRInfo::nonArgGPR0);
    emitPointerValidation(jit, GPRInfo::returnValueGPR2);
    jit.jump(GPRInfo::returnValueGPR2);
    
    LinkBuffer patchBuffer(*vm, &jit, GLOBAL_THUNK_ID);
    return FINALIZE_CODE(patchBuffer, ("Throw exception from call slow path thunk"));
}

static void slowPathFor(
    CCallHelpers& jit, VM* vm, P_JITOperation_E slowPathFunction)
{
    jit.preserveReturnAddressAfterCall(GPRInfo::nonArgGPR2);
    emitPointerValidation(jit, GPRInfo::nonArgGPR2);
    jit.storePtr(
        GPRInfo::nonArgGPR2,
        CCallHelpers::Address(
            GPRInfo::callFrameRegister,
            static_cast<ptrdiff_t>(sizeof(Register)) * JSStack::ReturnPC));
    jit.storePtr(GPRInfo::callFrameRegister, &vm->topCallFrame);
#if USE(JSVALUE64)
    jit.poke64(GPRInfo::nonPreservedNonReturnGPR, JITSTACKFRAME_ARGS_INDEX);
#else
    jit.poke(GPRInfo::nonPreservedNonReturnGPR, JITSTACKFRAME_ARGS_INDEX);
#endif
    jit.setupArgumentsExecState();
    jit.move(CCallHelpers::TrustedImmPtr(bitwise_cast<void*>(slowPathFunction)), GPRInfo::nonArgGPR0);
    emitPointerValidation(jit, GPRInfo::nonArgGPR0);
    jit.call(GPRInfo::nonArgGPR0);
    
    // This slow call will return the address of one of the following:
    // 1) Exception throwing thunk.
    // 2) Host call return value returner thingy.
    // 3) The function to call.
    jit.loadPtr(
        CCallHelpers::Address(
            GPRInfo::callFrameRegister,
            static_cast<ptrdiff_t>(sizeof(Register)) * JSStack::ReturnPC),
        GPRInfo::nonPreservedNonReturnGPR);
    jit.storePtr(
        CCallHelpers::TrustedImmPtr(0),
        CCallHelpers::Address(
            GPRInfo::callFrameRegister,
            static_cast<ptrdiff_t>(sizeof(Register)) * JSStack::ReturnPC));
    emitPointerValidation(jit, GPRInfo::nonPreservedNonReturnGPR);
    jit.restoreReturnAddressBeforeReturn(GPRInfo::nonPreservedNonReturnGPR);
    emitPointerValidation(jit, GPRInfo::returnValueGPR);
    jit.jump(GPRInfo::returnValueGPR);
}

static MacroAssemblerCodeRef linkForThunkGenerator(
    VM* vm, CodeSpecializationKind kind)
{
    // The return address is on the stack or in the link register. We will hence
    // save the return address to the call frame while we make a C++ function call
    // to perform linking and lazy compilation if necessary. We expect the callee
    // to be in nonArgGPR0/nonArgGPR1 (payload/tag), the call frame to have already
    // been adjusted, nonPreservedNonReturnGPR holds the exception handler index,
    // and all other registers to be available for use. We use JITStackFrame::args
    // to save important information across calls.
    
    CCallHelpers jit(vm);
    
    slowPathFor(jit, vm, kind == CodeForCall ? operationLinkCall : operationLinkConstruct);
    
    LinkBuffer patchBuffer(*vm, &jit, GLOBAL_THUNK_ID);
    return FINALIZE_CODE(
        patchBuffer,
        ("Link %s slow path thunk", kind == CodeForCall ? "call" : "construct"));
}

MacroAssemblerCodeRef linkCallThunkGenerator(VM* vm)
{
    return linkForThunkGenerator(vm, CodeForCall);
}

MacroAssemblerCodeRef linkConstructThunkGenerator(VM* vm)
{
    return linkForThunkGenerator(vm, CodeForConstruct);
}

// For closure optimizations, we only include calls, since if you're using closures for
// object construction then you're going to lose big time anyway.
MacroAssemblerCodeRef linkClosureCallThunkGenerator(VM* vm)
{
    CCallHelpers jit(vm);
    
    slowPathFor(jit, vm, operationLinkClosureCall);
    
    LinkBuffer patchBuffer(*vm, &jit, GLOBAL_THUNK_ID);
    return FINALIZE_CODE(patchBuffer, ("Link closure call slow path thunk"));
}

static MacroAssemblerCodeRef virtualForThunkGenerator(
    VM* vm, CodeSpecializationKind kind)
{
    // The return address is on the stack, or in the link register. We will hence
    // jump to the callee, or save the return address to the call frame while we
    // make a C++ function call to the appropriate JIT operation.

    CCallHelpers jit(vm);
    
    CCallHelpers::JumpList slowCase;

    // FIXME: we should have a story for eliminating these checks. In many cases,
    // the DFG knows that the value is definitely a cell, or definitely a function.
    
#if USE(JSVALUE64)
    slowCase.append(
        jit.branchTest64(
            CCallHelpers::NonZero, GPRInfo::nonArgGPR0, GPRInfo::tagMaskRegister));
#else
    slowCase.append(
        jit.branch32(
            CCallHelpers::NotEqual, GPRInfo::nonArgGPR1,
            CCallHelpers::TrustedImm32(JSValue::CellTag)));
#endif
    jit.loadPtr(CCallHelpers::Address(GPRInfo::nonArgGPR0, JSCell::structureOffset()), GPRInfo::nonArgGPR2);
    slowCase.append(
        jit.branchPtr(
            CCallHelpers::NotEqual,
            CCallHelpers::Address(GPRInfo::nonArgGPR2, Structure::classInfoOffset()),
            CCallHelpers::TrustedImmPtr(JSFunction::info())));
    
    // Now we know we have a JSFunction.
    
    jit.loadPtr(
        CCallHelpers::Address(GPRInfo::nonArgGPR0, JSFunction::offsetOfExecutable()),
        GPRInfo::nonArgGPR2);
    slowCase.append(
        jit.branch32(
            CCallHelpers::LessThan,
            CCallHelpers::Address(
                GPRInfo::nonArgGPR2, ExecutableBase::offsetOfNumParametersFor(kind)),
            CCallHelpers::TrustedImm32(0)));
    
    // Now we know that we have a CodeBlock, and we're committed to making a fast
    // call.
    
    jit.loadPtr(
        CCallHelpers::Address(GPRInfo::nonArgGPR0, JSFunction::offsetOfScopeChain()),
        GPRInfo::nonArgGPR1);
#if USE(JSVALUE64)
    jit.store64(
        GPRInfo::nonArgGPR1,
        CCallHelpers::Address(
            GPRInfo::callFrameRegister,
            static_cast<ptrdiff_t>(sizeof(Register)) * JSStack::ScopeChain));
#else
    jit.storePtr(
        GPRInfo::nonArgGPR1,
        CCallHelpers::Address(
            GPRInfo::callFrameRegister,
            static_cast<ptrdiff_t>(sizeof(Register)) * JSStack::ScopeChain +
            OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));
    jit.store32(
        CCallHelpers::TrustedImm32(JSValue::CellTag),
        CCallHelpers::Address(
            GPRInfo::callFrameRegister,
            static_cast<ptrdiff_t>(sizeof(Register)) * JSStack::ScopeChain +
            OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
#endif
    
    jit.loadPtr(
        CCallHelpers::Address(GPRInfo::nonArgGPR2, ExecutableBase::offsetOfJITCodeWithArityCheckFor(kind)),
        GPRInfo::regT0);
    
    // Make a tail call. This will return back to JIT code.
    emitPointerValidation(jit, GPRInfo::regT0);
    jit.jump(GPRInfo::regT0);

    slowCase.link(&jit);
    
    // Here we don't know anything, so revert to the full slow path.
    
    slowPathFor(jit, vm, kind == CodeForCall ? operationVirtualCall : operationVirtualConstruct);
    
    LinkBuffer patchBuffer(*vm, &jit, GLOBAL_THUNK_ID);
    return FINALIZE_CODE(
        patchBuffer,
        ("Virtual %s slow path thunk", kind == CodeForCall ? "call" : "construct"));
}

MacroAssemblerCodeRef virtualCallThunkGenerator(VM* vm)
{
    return virtualForThunkGenerator(vm, CodeForCall);
}

MacroAssemblerCodeRef virtualConstructThunkGenerator(VM* vm)
{
    return virtualForThunkGenerator(vm, CodeForConstruct);
}

MacroAssemblerCodeRef stringLengthTrampolineGenerator(VM* vm)
{
    JSInterfaceJIT jit(vm);
    
#if USE(JSVALUE64)
    // Check eax is a string
    JSInterfaceJIT::Jump failureCases1 = jit.emitJumpIfNotJSCell(JSInterfaceJIT::regT0);
    JSInterfaceJIT::Jump failureCases2 = jit.branchPtr(
        JSInterfaceJIT::NotEqual, JSInterfaceJIT::Address(
            JSInterfaceJIT::regT0, JSCell::structureOffset()),
        JSInterfaceJIT::TrustedImmPtr(vm->stringStructure.get()));

    // Checks out okay! - get the length from the Ustring.
    jit.load32(
        JSInterfaceJIT::Address(JSInterfaceJIT::regT0, JSString::offsetOfLength()),
        JSInterfaceJIT::regT0);

    JSInterfaceJIT::Jump failureCases3 = jit.branch32(
        JSInterfaceJIT::LessThan, JSInterfaceJIT::regT0, JSInterfaceJIT::TrustedImm32(0));

    // regT0 contains a 64 bit value (is positive, is zero extended) so we don't need sign extend here.
    jit.emitFastArithIntToImmNoCheck(JSInterfaceJIT::regT0, JSInterfaceJIT::regT0);
    
#else // USE(JSVALUE64)
    // regT0 holds payload, regT1 holds tag

    JSInterfaceJIT::Jump failureCases1 = jit.branch32(
        JSInterfaceJIT::NotEqual, JSInterfaceJIT::regT1,
        JSInterfaceJIT::TrustedImm32(JSValue::CellTag));
    JSInterfaceJIT::Jump failureCases2 = jit.branchPtr(
        JSInterfaceJIT::NotEqual,
        JSInterfaceJIT::Address(JSInterfaceJIT::regT0, JSCell::structureOffset()),
        JSInterfaceJIT::TrustedImmPtr(vm->stringStructure.get()));

    // Checks out okay! - get the length from the Ustring.
    jit.load32(
        JSInterfaceJIT::Address(JSInterfaceJIT::regT0, JSString::offsetOfLength()),
        JSInterfaceJIT::regT2);

    JSInterfaceJIT::Jump failureCases3 = jit.branch32(
        JSInterfaceJIT::Above, JSInterfaceJIT::regT2, JSInterfaceJIT::TrustedImm32(INT_MAX));
    jit.move(JSInterfaceJIT::regT2, JSInterfaceJIT::regT0);
    jit.move(JSInterfaceJIT::TrustedImm32(JSValue::Int32Tag), JSInterfaceJIT::regT1);
#endif // USE(JSVALUE64)

    jit.ret();
    
    JSInterfaceJIT::Call failureCases1Call = jit.makeTailRecursiveCall(failureCases1);
    JSInterfaceJIT::Call failureCases2Call = jit.makeTailRecursiveCall(failureCases2);
    JSInterfaceJIT::Call failureCases3Call = jit.makeTailRecursiveCall(failureCases3);
    
    LinkBuffer patchBuffer(*vm, &jit, GLOBAL_THUNK_ID);
    
    patchBuffer.link(failureCases1Call, FunctionPtr(cti_op_get_by_id_string_fail));
    patchBuffer.link(failureCases2Call, FunctionPtr(cti_op_get_by_id_string_fail));
    patchBuffer.link(failureCases3Call, FunctionPtr(cti_op_get_by_id_string_fail));
    
    return FINALIZE_CODE(patchBuffer, ("string length trampoline"));
}

static MacroAssemblerCodeRef nativeForGenerator(VM* vm, CodeSpecializationKind kind)
{
    int executableOffsetToFunction = NativeExecutable::offsetOfNativeFunctionFor(kind);
    
    JSInterfaceJIT jit(vm);
    
    jit.emitPutImmediateToCallFrameHeader(0, JSStack::CodeBlock);
    jit.storePtr(JSInterfaceJIT::callFrameRegister, &vm->topCallFrame);

#if CPU(X86)
    // Load caller frame's scope chain into this callframe so that whatever we call can
    // get to its global data.
    jit.emitGetFromCallFrameHeaderPtr(JSStack::CallerFrame, JSInterfaceJIT::regT0);
    jit.emitGetFromCallFrameHeaderPtr(JSStack::ScopeChain, JSInterfaceJIT::regT1, JSInterfaceJIT::regT0);
    jit.emitPutCellToCallFrameHeader(JSInterfaceJIT::regT1, JSStack::ScopeChain);

    jit.peek(JSInterfaceJIT::regT1);
    jit.emitPutToCallFrameHeader(JSInterfaceJIT::regT1, JSStack::ReturnPC);

    // Calling convention:      f(ecx, edx, ...);
    // Host function signature: f(ExecState*);
    jit.move(JSInterfaceJIT::callFrameRegister, X86Registers::ecx);

    jit.subPtr(JSInterfaceJIT::TrustedImm32(16 - sizeof(void*)), JSInterfaceJIT::stackPointerRegister); // Align stack after call.

    // call the function
    jit.emitGetFromCallFrameHeaderPtr(JSStack::Callee, JSInterfaceJIT::regT1);
    jit.loadPtr(JSInterfaceJIT::Address(JSInterfaceJIT::regT1, JSFunction::offsetOfExecutable()), JSInterfaceJIT::regT1);
    jit.move(JSInterfaceJIT::regT0, JSInterfaceJIT::callFrameRegister); // Eagerly restore caller frame register to avoid loading from stack.
    jit.call(JSInterfaceJIT::Address(JSInterfaceJIT::regT1, executableOffsetToFunction));

    jit.addPtr(JSInterfaceJIT::TrustedImm32(16 - sizeof(void*)), JSInterfaceJIT::stackPointerRegister);

#elif CPU(X86_64)
    // Load caller frame's scope chain into this callframe so that whatever we call can
    // get to its global data.
    jit.emitGetFromCallFrameHeaderPtr(JSStack::CallerFrame, JSInterfaceJIT::regT0);
    jit.emitGetFromCallFrameHeaderPtr(JSStack::ScopeChain, JSInterfaceJIT::regT1, JSInterfaceJIT::regT0);
    jit.emitPutCellToCallFrameHeader(JSInterfaceJIT::regT1, JSStack::ScopeChain);

    jit.peek(JSInterfaceJIT::regT1);
    jit.emitPutToCallFrameHeader(JSInterfaceJIT::regT1, JSStack::ReturnPC);

#if !OS(WINDOWS)
    // Calling convention:      f(edi, esi, edx, ecx, ...);
    // Host function signature: f(ExecState*);
    jit.move(JSInterfaceJIT::callFrameRegister, X86Registers::edi);

    jit.subPtr(JSInterfaceJIT::TrustedImm32(16 - sizeof(int64_t)), JSInterfaceJIT::stackPointerRegister); // Align stack after call.

    jit.emitGetFromCallFrameHeaderPtr(JSStack::Callee, X86Registers::esi);
    jit.loadPtr(JSInterfaceJIT::Address(X86Registers::esi, JSFunction::offsetOfExecutable()), X86Registers::r9);
    jit.move(JSInterfaceJIT::regT0, JSInterfaceJIT::callFrameRegister); // Eagerly restore caller frame register to avoid loading from stack.
    jit.call(JSInterfaceJIT::Address(X86Registers::r9, executableOffsetToFunction));

    jit.addPtr(JSInterfaceJIT::TrustedImm32(16 - sizeof(int64_t)), JSInterfaceJIT::stackPointerRegister);
#else
    // Calling convention:      f(ecx, edx, r8, r9, ...);
    // Host function signature: f(ExecState*);
    jit.move(JSInterfaceJIT::callFrameRegister, X86Registers::ecx);

    // Leave space for the callee parameter home addresses and align the stack.
    jit.subPtr(JSInterfaceJIT::TrustedImm32(4 * sizeof(int64_t) + 16 - sizeof(int64_t)), JSInterfaceJIT::stackPointerRegister);

    jit.emitGetFromCallFrameHeaderPtr(JSStack::Callee, X86Registers::edx);
    jit.loadPtr(JSInterfaceJIT::Address(X86Registers::edx, JSFunction::offsetOfExecutable()), X86Registers::r9);
    jit.move(JSInterfaceJIT::regT0, JSInterfaceJIT::callFrameRegister); // Eagerly restore caller frame register to avoid loading from stack.
    jit.call(JSInterfaceJIT::Address(X86Registers::r9, executableOffsetToFunction));

    jit.addPtr(JSInterfaceJIT::TrustedImm32(4 * sizeof(int64_t) + 16 - sizeof(int64_t)), JSInterfaceJIT::stackPointerRegister);
#endif

#elif CPU(ARM)
    // Load caller frame's scope chain into this callframe so that whatever we call can
    // get to its global data.
    jit.emitGetFromCallFrameHeaderPtr(JSStack::CallerFrame, JSInterfaceJIT::regT2);
    jit.emitGetFromCallFrameHeaderPtr(JSStack::ScopeChain, JSInterfaceJIT::regT1, JSInterfaceJIT::regT2);
    jit.emitPutCellToCallFrameHeader(JSInterfaceJIT::regT1, JSStack::ScopeChain);

    jit.preserveReturnAddressAfterCall(JSInterfaceJIT::regT3); // Callee preserved
    jit.emitPutToCallFrameHeader(JSInterfaceJIT::regT3, JSStack::ReturnPC);

    // Calling convention:      f(r0 == regT0, r1 == regT1, ...);
    // Host function signature: f(ExecState*);
    jit.move(JSInterfaceJIT::callFrameRegister, ARMRegisters::r0);

    jit.emitGetFromCallFrameHeaderPtr(JSStack::Callee, ARMRegisters::r1);
    jit.move(JSInterfaceJIT::regT2, JSInterfaceJIT::callFrameRegister); // Eagerly restore caller frame register to avoid loading from stack.
    jit.loadPtr(JSInterfaceJIT::Address(ARMRegisters::r1, JSFunction::offsetOfExecutable()), JSInterfaceJIT::regT2);
    jit.call(JSInterfaceJIT::Address(JSInterfaceJIT::regT2, executableOffsetToFunction));

    jit.restoreReturnAddressBeforeReturn(JSInterfaceJIT::regT3);

#elif CPU(SH4)
    // Load caller frame's scope chain into this callframe so that whatever we call can
    // get to its global data.
    jit.emitGetFromCallFrameHeaderPtr(JSStack::CallerFrame, JSInterfaceJIT::regT2);
    jit.emitGetFromCallFrameHeaderPtr(JSStack::ScopeChain, JSInterfaceJIT::regT1, JSInterfaceJIT::regT2);
    jit.emitPutCellToCallFrameHeader(JSInterfaceJIT::regT1, JSStack::ScopeChain);

    jit.preserveReturnAddressAfterCall(JSInterfaceJIT::regT3); // Callee preserved
    jit.emitPutToCallFrameHeader(JSInterfaceJIT::regT3, JSStack::ReturnPC);

    // Calling convention: f(r0 == regT4, r1 == regT5, ...);
    // Host function signature: f(ExecState*);
    jit.move(JSInterfaceJIT::callFrameRegister, JSInterfaceJIT::regT4);

    jit.emitGetFromCallFrameHeaderPtr(JSStack::Callee, JSInterfaceJIT::regT5);
    jit.move(JSInterfaceJIT::regT2, JSInterfaceJIT::callFrameRegister); // Eagerly restore caller frame register to avoid loading from stack.
    jit.loadPtr(JSInterfaceJIT::Address(JSInterfaceJIT::regT5, JSFunction::offsetOfExecutable()), JSInterfaceJIT::regT2);

    jit.call(JSInterfaceJIT::Address(JSInterfaceJIT::regT2, executableOffsetToFunction), JSInterfaceJIT::regT0);
    jit.restoreReturnAddressBeforeReturn(JSInterfaceJIT::regT3);

#elif CPU(MIPS)
    // Load caller frame's scope chain into this callframe so that whatever we call can
    // get to its global data.
    jit.emitGetFromCallFrameHeaderPtr(JSStack::CallerFrame, JSInterfaceJIT::regT0);
    jit.emitGetFromCallFrameHeaderPtr(JSStack::ScopeChain, JSInterfaceJIT::regT1, JSInterfaceJIT::regT0);
    jit.emitPutCellToCallFrameHeader(JSInterfaceJIT::regT1, JSStack::ScopeChain);

    jit.preserveReturnAddressAfterCall(JSInterfaceJIT::regT3); // Callee preserved
    jit.emitPutToCallFrameHeader(JSInterfaceJIT::regT3, JSStack::ReturnPC);

    // Calling convention:      f(a0, a1, a2, a3);
    // Host function signature: f(ExecState*);

    // Allocate stack space for 16 bytes (8-byte aligned)
    // 16 bytes (unused) for 4 arguments
    jit.subPtr(JSInterfaceJIT::TrustedImm32(16), JSInterfaceJIT::stackPointerRegister);

    // Setup arg0
    jit.move(JSInterfaceJIT::callFrameRegister, MIPSRegisters::a0);

    // Call
    jit.emitGetFromCallFrameHeaderPtr(JSStack::Callee, MIPSRegisters::a2);
    jit.loadPtr(JSInterfaceJIT::Address(MIPSRegisters::a2, JSFunction::offsetOfExecutable()), JSInterfaceJIT::regT2);
    jit.move(JSInterfaceJIT::regT0, JSInterfaceJIT::callFrameRegister); // Eagerly restore caller frame register to avoid loading from stack.
    jit.call(JSInterfaceJIT::Address(JSInterfaceJIT::regT2, executableOffsetToFunction));

    // Restore stack space
    jit.addPtr(JSInterfaceJIT::TrustedImm32(16), JSInterfaceJIT::stackPointerRegister);

    jit.restoreReturnAddressBeforeReturn(JSInterfaceJIT::regT3);
#else
#error "JIT not supported on this platform."
    UNUSED_PARAM(executableOffsetToFunction);
    breakpoint();
#endif

    // Check for an exception
#if USE(JSVALUE64)
    jit.load64(vm->addressOfException(), JSInterfaceJIT::regT2);
    JSInterfaceJIT::Jump exceptionHandler = jit.branchTest64(JSInterfaceJIT::NonZero, JSInterfaceJIT::regT2);
#else
    JSInterfaceJIT::Jump exceptionHandler = jit.branch32(
        JSInterfaceJIT::NotEqual,
        JSInterfaceJIT::AbsoluteAddress(reinterpret_cast<char*>(vm->addressOfException()) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)),
        JSInterfaceJIT::TrustedImm32(JSValue::EmptyValueTag));
#endif

    // Return.
    jit.ret();

    // Handle an exception
    exceptionHandler.link(&jit);

    // Grab the return address.
    jit.preserveReturnAddressAfterCall(JSInterfaceJIT::regT1);
    
    jit.move(JSInterfaceJIT::TrustedImmPtr(&vm->exceptionLocation), JSInterfaceJIT::regT2);
    jit.storePtr(JSInterfaceJIT::regT1, JSInterfaceJIT::regT2);

    jit.storePtr(JSInterfaceJIT::callFrameRegister, &vm->topCallFrame);

    jit.move(JSInterfaceJIT::TrustedImmPtr(FunctionPtr(ctiVMHandleException).value()), JSInterfaceJIT::regT1);
    jit.jump(JSInterfaceJIT::regT1);

    LinkBuffer patchBuffer(*vm, &jit, GLOBAL_THUNK_ID);
    return FINALIZE_CODE(patchBuffer, ("native %s trampoline", toCString(kind).data()));
}

MacroAssemblerCodeRef nativeCallGenerator(VM* vm)
{
    return nativeForGenerator(vm, CodeForCall);
}

MacroAssemblerCodeRef nativeConstructGenerator(VM* vm)
{
    return nativeForGenerator(vm, CodeForConstruct);
}

MacroAssemblerCodeRef arityFixup(VM* vm)
{
    JSInterfaceJIT jit(vm);

    // We enter with fixup count in regT0
#if USE(JSVALUE64)
#  if CPU(X86_64)
    jit.pop(JSInterfaceJIT::regT4);
#  endif
    jit.neg64(JSInterfaceJIT::regT0);
    jit.addPtr(JSInterfaceJIT::TrustedImm32(8), JSInterfaceJIT::callFrameRegister, JSInterfaceJIT::regT3);
    jit.load32(JSInterfaceJIT::Address(JSInterfaceJIT::callFrameRegister, JSStack::ArgumentCount * 8), JSInterfaceJIT::regT2);
    jit.add32(JSInterfaceJIT::TrustedImm32(JSStack::CallFrameHeaderSize), JSInterfaceJIT::regT2);

    // Move current frame down regT0 number of slots
    JSInterfaceJIT::Label copyLoop(jit.label());
    jit.load64(JSInterfaceJIT::regT3, JSInterfaceJIT::regT1);
    jit.store64(JSInterfaceJIT::regT1, MacroAssembler::BaseIndex(JSInterfaceJIT::regT3, JSInterfaceJIT::regT0, JSInterfaceJIT::TimesEight));
    jit.addPtr(JSInterfaceJIT::TrustedImm32(8), JSInterfaceJIT::regT3);
    jit.branchSub32(MacroAssembler::NonZero, JSInterfaceJIT::TrustedImm32(1), JSInterfaceJIT::regT2).linkTo(copyLoop, &jit);

    // Fill in regT0 missing arg slots with undefined
    jit.move(JSInterfaceJIT::regT0, JSInterfaceJIT::regT2);
    jit.move(JSInterfaceJIT::TrustedImm64(ValueUndefined), JSInterfaceJIT::regT1);
    JSInterfaceJIT::Label fillUndefinedLoop(jit.label());
    jit.store64(JSInterfaceJIT::regT1, MacroAssembler::BaseIndex(JSInterfaceJIT::regT3, JSInterfaceJIT::regT0, JSInterfaceJIT::TimesEight));
    jit.addPtr(JSInterfaceJIT::TrustedImm32(8), JSInterfaceJIT::regT3);
    jit.branchAdd32(MacroAssembler::NonZero, JSInterfaceJIT::TrustedImm32(1), JSInterfaceJIT::regT2).linkTo(fillUndefinedLoop, &jit);

    // Adjust call frame register to account for missing args
    jit.lshift64(JSInterfaceJIT::TrustedImm32(3), JSInterfaceJIT::regT0);
    jit.addPtr(JSInterfaceJIT::regT0, JSInterfaceJIT::callFrameRegister);

#  if CPU(X86_64)
    jit.push(JSInterfaceJIT::regT4);
#  endif
    jit.ret();
#else
#  if CPU(X86)
    jit.pop(JSInterfaceJIT::regT4);
#  endif
    jit.neg32(JSInterfaceJIT::regT0);
    jit.addPtr(JSInterfaceJIT::TrustedImm32(8), JSInterfaceJIT::callFrameRegister, JSInterfaceJIT::regT3);
    jit.load32(JSInterfaceJIT::Address(JSInterfaceJIT::callFrameRegister, JSStack::ArgumentCount * 8), JSInterfaceJIT::regT2);
    jit.add32(JSInterfaceJIT::TrustedImm32(JSStack::CallFrameHeaderSize), JSInterfaceJIT::regT2);

    // Move current frame down regT0 number of slots
    JSInterfaceJIT::Label copyLoop(jit.label());
    jit.load32(JSInterfaceJIT::regT3, JSInterfaceJIT::regT1);
    jit.store32(JSInterfaceJIT::regT1, MacroAssembler::BaseIndex(JSInterfaceJIT::regT3, JSInterfaceJIT::regT0, JSInterfaceJIT::TimesEight));
    jit.load32(MacroAssembler::Address(JSInterfaceJIT::regT3, 4), JSInterfaceJIT::regT1);
    jit.store32(JSInterfaceJIT::regT1, MacroAssembler::BaseIndex(JSInterfaceJIT::regT3, JSInterfaceJIT::regT0, JSInterfaceJIT::TimesEight, 4));
    jit.addPtr(JSInterfaceJIT::TrustedImm32(8), JSInterfaceJIT::regT3);
    jit.branchSub32(MacroAssembler::NonZero, JSInterfaceJIT::TrustedImm32(1), JSInterfaceJIT::regT2).linkTo(copyLoop, &jit);

    // Fill in regT0 missing arg slots with undefined
    jit.move(JSInterfaceJIT::regT0, JSInterfaceJIT::regT2);
    JSInterfaceJIT::Label fillUndefinedLoop(jit.label());
    jit.move(JSInterfaceJIT::TrustedImm32(0), JSInterfaceJIT::regT1);
    jit.store32(JSInterfaceJIT::regT1, MacroAssembler::BaseIndex(JSInterfaceJIT::regT3, JSInterfaceJIT::regT0, JSInterfaceJIT::TimesEight));
    jit.move(JSInterfaceJIT::TrustedImm32(JSValue::UndefinedTag), JSInterfaceJIT::regT1);
    jit.store32(JSInterfaceJIT::regT1, MacroAssembler::BaseIndex(JSInterfaceJIT::regT3, JSInterfaceJIT::regT0, JSInterfaceJIT::TimesEight, 4));

    jit.addPtr(JSInterfaceJIT::TrustedImm32(8), JSInterfaceJIT::regT3);
    jit.branchAdd32(MacroAssembler::NonZero, JSInterfaceJIT::TrustedImm32(1), JSInterfaceJIT::regT2).linkTo(fillUndefinedLoop, &jit);

    // Adjust call frame register to account for missing args
    jit.lshift32(JSInterfaceJIT::TrustedImm32(3), JSInterfaceJIT::regT0);
    jit.addPtr(JSInterfaceJIT::regT0, JSInterfaceJIT::callFrameRegister);

#  if CPU(X86)
    jit.push(JSInterfaceJIT::regT4);
#  endif
    jit.ret();
#endif

    LinkBuffer patchBuffer(*vm, &jit, GLOBAL_THUNK_ID);
    return FINALIZE_CODE(patchBuffer, ("fixup arity"));
}

static void stringCharLoad(SpecializedThunkJIT& jit, VM* vm)
{
    // load string
    jit.loadJSStringArgument(*vm, SpecializedThunkJIT::ThisArgument, SpecializedThunkJIT::regT0);

    // Load string length to regT2, and start the process of loading the data pointer into regT0
    jit.load32(MacroAssembler::Address(SpecializedThunkJIT::regT0, ThunkHelpers::jsStringLengthOffset()), SpecializedThunkJIT::regT2);
    jit.loadPtr(MacroAssembler::Address(SpecializedThunkJIT::regT0, ThunkHelpers::jsStringValueOffset()), SpecializedThunkJIT::regT0);
    jit.appendFailure(jit.branchTest32(MacroAssembler::Zero, SpecializedThunkJIT::regT0));

    // load index
    jit.loadInt32Argument(0, SpecializedThunkJIT::regT1); // regT1 contains the index

    // Do an unsigned compare to simultaneously filter negative indices as well as indices that are too large
    jit.appendFailure(jit.branch32(MacroAssembler::AboveOrEqual, SpecializedThunkJIT::regT1, SpecializedThunkJIT::regT2));

    // Load the character
    SpecializedThunkJIT::JumpList is16Bit;
    SpecializedThunkJIT::JumpList cont8Bit;
    // Load the string flags
    jit.loadPtr(MacroAssembler::Address(SpecializedThunkJIT::regT0, StringImpl::flagsOffset()), SpecializedThunkJIT::regT2);
    jit.loadPtr(MacroAssembler::Address(SpecializedThunkJIT::regT0, StringImpl::dataOffset()), SpecializedThunkJIT::regT0);
    is16Bit.append(jit.branchTest32(MacroAssembler::Zero, SpecializedThunkJIT::regT2, MacroAssembler::TrustedImm32(StringImpl::flagIs8Bit())));
    jit.load8(MacroAssembler::BaseIndex(SpecializedThunkJIT::regT0, SpecializedThunkJIT::regT1, MacroAssembler::TimesOne, 0), SpecializedThunkJIT::regT0);
    cont8Bit.append(jit.jump());
    is16Bit.link(&jit);
    jit.load16(MacroAssembler::BaseIndex(SpecializedThunkJIT::regT0, SpecializedThunkJIT::regT1, MacroAssembler::TimesTwo, 0), SpecializedThunkJIT::regT0);
    cont8Bit.link(&jit);
}

static void charToString(SpecializedThunkJIT& jit, VM* vm, MacroAssembler::RegisterID src, MacroAssembler::RegisterID dst, MacroAssembler::RegisterID scratch)
{
    jit.appendFailure(jit.branch32(MacroAssembler::AboveOrEqual, src, MacroAssembler::TrustedImm32(0x100)));
    jit.move(MacroAssembler::TrustedImmPtr(vm->smallStrings.singleCharacterStrings()), scratch);
    jit.loadPtr(MacroAssembler::BaseIndex(scratch, src, MacroAssembler::ScalePtr, 0), dst);
    jit.appendFailure(jit.branchTestPtr(MacroAssembler::Zero, dst));
}

MacroAssemblerCodeRef charCodeAtThunkGenerator(VM* vm)
{
    SpecializedThunkJIT jit(vm, 1);
    stringCharLoad(jit, vm);
    jit.returnInt32(SpecializedThunkJIT::regT0);
    return jit.finalize(vm->jitStubs->ctiNativeCall(vm), "charCodeAt");
}

MacroAssemblerCodeRef charAtThunkGenerator(VM* vm)
{
    SpecializedThunkJIT jit(vm, 1);
    stringCharLoad(jit, vm);
    charToString(jit, vm, SpecializedThunkJIT::regT0, SpecializedThunkJIT::regT0, SpecializedThunkJIT::regT1);
    jit.returnJSCell(SpecializedThunkJIT::regT0);
    return jit.finalize(vm->jitStubs->ctiNativeCall(vm), "charAt");
}

MacroAssemblerCodeRef fromCharCodeThunkGenerator(VM* vm)
{
    SpecializedThunkJIT jit(vm, 1);
    // load char code
    jit.loadInt32Argument(0, SpecializedThunkJIT::regT0);
    charToString(jit, vm, SpecializedThunkJIT::regT0, SpecializedThunkJIT::regT0, SpecializedThunkJIT::regT1);
    jit.returnJSCell(SpecializedThunkJIT::regT0);
    return jit.finalize(vm->jitStubs->ctiNativeCall(vm), "fromCharCode");
}

MacroAssemblerCodeRef sqrtThunkGenerator(VM* vm)
{
    SpecializedThunkJIT jit(vm, 1);
    if (!jit.supportsFloatingPointSqrt())
        return MacroAssemblerCodeRef::createSelfManagedCodeRef(vm->jitStubs->ctiNativeCall(vm));

    jit.loadDoubleArgument(0, SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::regT0);
    jit.sqrtDouble(SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::fpRegT0);
    jit.returnDouble(SpecializedThunkJIT::fpRegT0);
    return jit.finalize(vm->jitStubs->ctiNativeCall(vm), "sqrt");
}


#define UnaryDoubleOpWrapper(function) function##Wrapper
enum MathThunkCallingConvention { };
typedef MathThunkCallingConvention(*MathThunk)(MathThunkCallingConvention);
extern "C" {

double jsRound(double) REFERENCED_FROM_ASM;
double jsRound(double d)
{
    double integer = ceil(d);
    return integer - (integer - d > 0.5);
}

}

#if CPU(X86_64) && COMPILER(GCC) && (PLATFORM(MAC) || OS(LINUX))

#define defineUnaryDoubleOpWrapper(function) \
    asm( \
        ".text\n" \
        ".globl " SYMBOL_STRING(function##Thunk) "\n" \
        HIDE_SYMBOL(function##Thunk) "\n" \
        SYMBOL_STRING(function##Thunk) ":" "\n" \
        "call " GLOBAL_REFERENCE(function) "\n" \
        "ret\n" \
    );\
    extern "C" { \
        MathThunkCallingConvention function##Thunk(MathThunkCallingConvention); \
    } \
    static MathThunk UnaryDoubleOpWrapper(function) = &function##Thunk;

#elif CPU(X86) && COMPILER(GCC) && (PLATFORM(MAC) || OS(LINUX))
#define defineUnaryDoubleOpWrapper(function) \
    asm( \
        ".text\n" \
        ".globl " SYMBOL_STRING(function##Thunk) "\n" \
        HIDE_SYMBOL(function##Thunk) "\n" \
        SYMBOL_STRING(function##Thunk) ":" "\n" \
        "subl $8, %esp\n" \
        "movsd %xmm0, (%esp) \n" \
        "call " GLOBAL_REFERENCE(function) "\n" \
        "fstpl (%esp) \n" \
        "movsd (%esp), %xmm0 \n" \
        "addl $8, %esp\n" \
        "ret\n" \
    );\
    extern "C" { \
        MathThunkCallingConvention function##Thunk(MathThunkCallingConvention); \
    } \
    static MathThunk UnaryDoubleOpWrapper(function) = &function##Thunk;

#elif CPU(ARM_THUMB2) && COMPILER(GCC) && PLATFORM(IOS)

#define defineUnaryDoubleOpWrapper(function) \
    asm( \
        ".text\n" \
        ".align 2\n" \
        ".globl " SYMBOL_STRING(function##Thunk) "\n" \
        HIDE_SYMBOL(function##Thunk) "\n" \
        ".thumb\n" \
        ".thumb_func " THUMB_FUNC_PARAM(function##Thunk) "\n" \
        SYMBOL_STRING(function##Thunk) ":" "\n" \
        "push {lr}\n" \
        "vmov r0, r1, d0\n" \
        "blx " GLOBAL_REFERENCE(function) "\n" \
        "vmov d0, r0, r1\n" \
        "pop {lr}\n" \
        "bx lr\n" \
    ); \
    extern "C" { \
        MathThunkCallingConvention function##Thunk(MathThunkCallingConvention); \
    } \
    static MathThunk UnaryDoubleOpWrapper(function) = &function##Thunk;
#else

#define defineUnaryDoubleOpWrapper(function) \
    static MathThunk UnaryDoubleOpWrapper(function) = 0
#endif

defineUnaryDoubleOpWrapper(jsRound);
defineUnaryDoubleOpWrapper(exp);
defineUnaryDoubleOpWrapper(log);
defineUnaryDoubleOpWrapper(floor);
defineUnaryDoubleOpWrapper(ceil);

static const double oneConstant = 1.0;
static const double negativeHalfConstant = -0.5;
static const double zeroConstant = 0.0;
static const double halfConstant = 0.5;
    
MacroAssemblerCodeRef floorThunkGenerator(VM* vm)
{
    SpecializedThunkJIT jit(vm, 1);
    MacroAssembler::Jump nonIntJump;
    if (!UnaryDoubleOpWrapper(floor) || !jit.supportsFloatingPoint())
        return MacroAssemblerCodeRef::createSelfManagedCodeRef(vm->jitStubs->ctiNativeCall(vm));
    jit.loadInt32Argument(0, SpecializedThunkJIT::regT0, nonIntJump);
    jit.returnInt32(SpecializedThunkJIT::regT0);
    nonIntJump.link(&jit);
    jit.loadDoubleArgument(0, SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::regT0);
    SpecializedThunkJIT::Jump intResult;
    SpecializedThunkJIT::JumpList doubleResult;
    if (jit.supportsFloatingPointTruncate()) {
        jit.loadDouble(&zeroConstant, SpecializedThunkJIT::fpRegT1);
        doubleResult.append(jit.branchDouble(MacroAssembler::DoubleEqual, SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::fpRegT1));
        SpecializedThunkJIT::JumpList slowPath;
        // Handle the negative doubles in the slow path for now.
        slowPath.append(jit.branchDouble(MacroAssembler::DoubleLessThanOrUnordered, SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::fpRegT1));
        slowPath.append(jit.branchTruncateDoubleToInt32(SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::regT0));
        intResult = jit.jump();
        slowPath.link(&jit);
    }
    jit.callDoubleToDoublePreservingReturn(UnaryDoubleOpWrapper(floor));
    jit.branchConvertDoubleToInt32(SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::regT0, doubleResult, SpecializedThunkJIT::fpRegT1);
    if (jit.supportsFloatingPointTruncate())
        intResult.link(&jit);
    jit.returnInt32(SpecializedThunkJIT::regT0);
    doubleResult.link(&jit);
    jit.returnDouble(SpecializedThunkJIT::fpRegT0);
    return jit.finalize(vm->jitStubs->ctiNativeCall(vm), "floor");
}

MacroAssemblerCodeRef ceilThunkGenerator(VM* vm)
{
    SpecializedThunkJIT jit(vm, 1);
    if (!UnaryDoubleOpWrapper(ceil) || !jit.supportsFloatingPoint())
        return MacroAssemblerCodeRef::createSelfManagedCodeRef(vm->jitStubs->ctiNativeCall(vm));
    MacroAssembler::Jump nonIntJump;
    jit.loadInt32Argument(0, SpecializedThunkJIT::regT0, nonIntJump);
    jit.returnInt32(SpecializedThunkJIT::regT0);
    nonIntJump.link(&jit);
    jit.loadDoubleArgument(0, SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::regT0);
    jit.callDoubleToDoublePreservingReturn(UnaryDoubleOpWrapper(ceil));
    SpecializedThunkJIT::JumpList doubleResult;
    jit.branchConvertDoubleToInt32(SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::regT0, doubleResult, SpecializedThunkJIT::fpRegT1);
    jit.returnInt32(SpecializedThunkJIT::regT0);
    doubleResult.link(&jit);
    jit.returnDouble(SpecializedThunkJIT::fpRegT0);
    return jit.finalize(vm->jitStubs->ctiNativeCall(vm), "ceil");
}

MacroAssemblerCodeRef roundThunkGenerator(VM* vm)
{
    SpecializedThunkJIT jit(vm, 1);
    if (!UnaryDoubleOpWrapper(jsRound) || !jit.supportsFloatingPoint())
        return MacroAssemblerCodeRef::createSelfManagedCodeRef(vm->jitStubs->ctiNativeCall(vm));
    MacroAssembler::Jump nonIntJump;
    jit.loadInt32Argument(0, SpecializedThunkJIT::regT0, nonIntJump);
    jit.returnInt32(SpecializedThunkJIT::regT0);
    nonIntJump.link(&jit);
    jit.loadDoubleArgument(0, SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::regT0);
    SpecializedThunkJIT::Jump intResult;
    SpecializedThunkJIT::JumpList doubleResult;
    if (jit.supportsFloatingPointTruncate()) {
        jit.loadDouble(&zeroConstant, SpecializedThunkJIT::fpRegT1);
        doubleResult.append(jit.branchDouble(MacroAssembler::DoubleEqual, SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::fpRegT1));
        SpecializedThunkJIT::JumpList slowPath;
        // Handle the negative doubles in the slow path for now.
        slowPath.append(jit.branchDouble(MacroAssembler::DoubleLessThanOrUnordered, SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::fpRegT1));
        jit.loadDouble(&halfConstant, SpecializedThunkJIT::fpRegT1);
        jit.addDouble(SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::fpRegT1);
        slowPath.append(jit.branchTruncateDoubleToInt32(SpecializedThunkJIT::fpRegT1, SpecializedThunkJIT::regT0));
        intResult = jit.jump();
        slowPath.link(&jit);
    }
    jit.callDoubleToDoublePreservingReturn(UnaryDoubleOpWrapper(jsRound));
    jit.branchConvertDoubleToInt32(SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::regT0, doubleResult, SpecializedThunkJIT::fpRegT1);
    if (jit.supportsFloatingPointTruncate())
        intResult.link(&jit);
    jit.returnInt32(SpecializedThunkJIT::regT0);
    doubleResult.link(&jit);
    jit.returnDouble(SpecializedThunkJIT::fpRegT0);
    return jit.finalize(vm->jitStubs->ctiNativeCall(vm), "round");
}

MacroAssemblerCodeRef expThunkGenerator(VM* vm)
{
    if (!UnaryDoubleOpWrapper(exp))
        return MacroAssemblerCodeRef::createSelfManagedCodeRef(vm->jitStubs->ctiNativeCall(vm));
    SpecializedThunkJIT jit(vm, 1);
    if (!jit.supportsFloatingPoint())
        return MacroAssemblerCodeRef::createSelfManagedCodeRef(vm->jitStubs->ctiNativeCall(vm));
    jit.loadDoubleArgument(0, SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::regT0);
    jit.callDoubleToDoublePreservingReturn(UnaryDoubleOpWrapper(exp));
    jit.returnDouble(SpecializedThunkJIT::fpRegT0);
    return jit.finalize(vm->jitStubs->ctiNativeCall(vm), "exp");
}

MacroAssemblerCodeRef logThunkGenerator(VM* vm)
{
    if (!UnaryDoubleOpWrapper(log))
        return MacroAssemblerCodeRef::createSelfManagedCodeRef(vm->jitStubs->ctiNativeCall(vm));
    SpecializedThunkJIT jit(vm, 1);
    if (!jit.supportsFloatingPoint())
        return MacroAssemblerCodeRef::createSelfManagedCodeRef(vm->jitStubs->ctiNativeCall(vm));
    jit.loadDoubleArgument(0, SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::regT0);
    jit.callDoubleToDoublePreservingReturn(UnaryDoubleOpWrapper(log));
    jit.returnDouble(SpecializedThunkJIT::fpRegT0);
    return jit.finalize(vm->jitStubs->ctiNativeCall(vm), "log");
}

MacroAssemblerCodeRef absThunkGenerator(VM* vm)
{
    SpecializedThunkJIT jit(vm, 1);
    if (!jit.supportsFloatingPointAbs())
        return MacroAssemblerCodeRef::createSelfManagedCodeRef(vm->jitStubs->ctiNativeCall(vm));
    MacroAssembler::Jump nonIntJump;
    jit.loadInt32Argument(0, SpecializedThunkJIT::regT0, nonIntJump);
    jit.rshift32(SpecializedThunkJIT::regT0, MacroAssembler::TrustedImm32(31), SpecializedThunkJIT::regT1);
    jit.add32(SpecializedThunkJIT::regT1, SpecializedThunkJIT::regT0);
    jit.xor32(SpecializedThunkJIT::regT1, SpecializedThunkJIT::regT0);
    jit.appendFailure(jit.branch32(MacroAssembler::Equal, SpecializedThunkJIT::regT0, MacroAssembler::TrustedImm32(1 << 31)));
    jit.returnInt32(SpecializedThunkJIT::regT0);
    nonIntJump.link(&jit);
    // Shame about the double int conversion here.
    jit.loadDoubleArgument(0, SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::regT0);
    jit.absDouble(SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::fpRegT1);
    jit.returnDouble(SpecializedThunkJIT::fpRegT1);
    return jit.finalize(vm->jitStubs->ctiNativeCall(vm), "abs");
}

MacroAssemblerCodeRef powThunkGenerator(VM* vm)
{
    SpecializedThunkJIT jit(vm, 2);
    if (!jit.supportsFloatingPoint())
        return MacroAssemblerCodeRef::createSelfManagedCodeRef(vm->jitStubs->ctiNativeCall(vm));

    jit.loadDouble(&oneConstant, SpecializedThunkJIT::fpRegT1);
    jit.loadDoubleArgument(0, SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::regT0);
    MacroAssembler::Jump nonIntExponent;
    jit.loadInt32Argument(1, SpecializedThunkJIT::regT0, nonIntExponent);
    jit.appendFailure(jit.branch32(MacroAssembler::LessThan, SpecializedThunkJIT::regT0, MacroAssembler::TrustedImm32(0)));
    
    MacroAssembler::Jump exponentIsZero = jit.branchTest32(MacroAssembler::Zero, SpecializedThunkJIT::regT0);
    MacroAssembler::Label startLoop(jit.label());

    MacroAssembler::Jump exponentIsEven = jit.branchTest32(MacroAssembler::Zero, SpecializedThunkJIT::regT0, MacroAssembler::TrustedImm32(1));
    jit.mulDouble(SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::fpRegT1);
    exponentIsEven.link(&jit);
    jit.mulDouble(SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::fpRegT0);
    jit.rshift32(MacroAssembler::TrustedImm32(1), SpecializedThunkJIT::regT0);
    jit.branchTest32(MacroAssembler::NonZero, SpecializedThunkJIT::regT0).linkTo(startLoop, &jit);

    exponentIsZero.link(&jit);

    {
        SpecializedThunkJIT::JumpList doubleResult;
        jit.branchConvertDoubleToInt32(SpecializedThunkJIT::fpRegT1, SpecializedThunkJIT::regT0, doubleResult, SpecializedThunkJIT::fpRegT0);
        jit.returnInt32(SpecializedThunkJIT::regT0);
        doubleResult.link(&jit);
        jit.returnDouble(SpecializedThunkJIT::fpRegT1);
    }

    if (jit.supportsFloatingPointSqrt()) {
        nonIntExponent.link(&jit);
        jit.loadDouble(&negativeHalfConstant, SpecializedThunkJIT::fpRegT3);
        jit.loadDoubleArgument(1, SpecializedThunkJIT::fpRegT2, SpecializedThunkJIT::regT0);
        jit.appendFailure(jit.branchDouble(MacroAssembler::DoubleLessThanOrEqual, SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::fpRegT1));
        jit.appendFailure(jit.branchDouble(MacroAssembler::DoubleNotEqualOrUnordered, SpecializedThunkJIT::fpRegT2, SpecializedThunkJIT::fpRegT3));
        jit.sqrtDouble(SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::fpRegT0);
        jit.divDouble(SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::fpRegT1);

        SpecializedThunkJIT::JumpList doubleResult;
        jit.branchConvertDoubleToInt32(SpecializedThunkJIT::fpRegT1, SpecializedThunkJIT::regT0, doubleResult, SpecializedThunkJIT::fpRegT0);
        jit.returnInt32(SpecializedThunkJIT::regT0);
        doubleResult.link(&jit);
        jit.returnDouble(SpecializedThunkJIT::fpRegT1);
    } else
        jit.appendFailure(nonIntExponent);

    return jit.finalize(vm->jitStubs->ctiNativeCall(vm), "pow");
}

MacroAssemblerCodeRef imulThunkGenerator(VM* vm)
{
    SpecializedThunkJIT jit(vm, 2);
    MacroAssembler::Jump nonIntArg0Jump;
    jit.loadInt32Argument(0, SpecializedThunkJIT::regT0, nonIntArg0Jump);
    SpecializedThunkJIT::Label doneLoadingArg0(&jit);
    MacroAssembler::Jump nonIntArg1Jump;
    jit.loadInt32Argument(1, SpecializedThunkJIT::regT1, nonIntArg1Jump);
    SpecializedThunkJIT::Label doneLoadingArg1(&jit);
    jit.mul32(SpecializedThunkJIT::regT1, SpecializedThunkJIT::regT0);
    jit.returnInt32(SpecializedThunkJIT::regT0);

    if (jit.supportsFloatingPointTruncate()) {
        nonIntArg0Jump.link(&jit);
        jit.loadDoubleArgument(0, SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::regT0);
        jit.branchTruncateDoubleToInt32(SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::regT0, SpecializedThunkJIT::BranchIfTruncateSuccessful).linkTo(doneLoadingArg0, &jit);
        jit.xor32(SpecializedThunkJIT::regT0, SpecializedThunkJIT::regT0);
        jit.jump(doneLoadingArg0);
    } else
        jit.appendFailure(nonIntArg0Jump);

    if (jit.supportsFloatingPointTruncate()) {
        nonIntArg1Jump.link(&jit);
        jit.loadDoubleArgument(1, SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::regT1);
        jit.branchTruncateDoubleToInt32(SpecializedThunkJIT::fpRegT0, SpecializedThunkJIT::regT1, SpecializedThunkJIT::BranchIfTruncateSuccessful).linkTo(doneLoadingArg1, &jit);
        jit.xor32(SpecializedThunkJIT::regT1, SpecializedThunkJIT::regT1);
        jit.jump(doneLoadingArg1);
    } else
        jit.appendFailure(nonIntArg1Jump);

    return jit.finalize(vm->jitStubs->ctiNativeCall(vm), "imul");
}

MacroAssemblerCodeRef arrayIteratorNextThunkGenerator(VM* vm)
{
    typedef SpecializedThunkJIT::TrustedImm32 TrustedImm32;
    typedef SpecializedThunkJIT::TrustedImmPtr TrustedImmPtr;
    typedef SpecializedThunkJIT::Address Address;
    typedef SpecializedThunkJIT::BaseIndex BaseIndex;
    typedef SpecializedThunkJIT::Jump Jump;
    
    SpecializedThunkJIT jit(vm);
    // Make sure we're being called on an array iterator, and load m_iteratedObject, and m_nextIndex into regT0 and regT1 respectively
    jit.loadArgumentWithSpecificClass(JSArrayIterator::info(), SpecializedThunkJIT::ThisArgument, SpecializedThunkJIT::regT4, SpecializedThunkJIT::regT1);

    // Early exit if we don't have a thunk for this form of iteration
    jit.appendFailure(jit.branch32(SpecializedThunkJIT::AboveOrEqual, Address(SpecializedThunkJIT::regT4, JSArrayIterator::offsetOfIterationKind()), TrustedImm32(ArrayIterateKeyValue)));
    
    jit.loadPtr(Address(SpecializedThunkJIT::regT4, JSArrayIterator::offsetOfIteratedObject()), SpecializedThunkJIT::regT0);
    
    jit.load32(Address(SpecializedThunkJIT::regT4, JSArrayIterator::offsetOfNextIndex()), SpecializedThunkJIT::regT1);
    
    // Pull out the butterfly from iteratedObject
    jit.loadPtr(Address(SpecializedThunkJIT::regT0, JSCell::structureOffset()), SpecializedThunkJIT::regT2);
    
    jit.load8(Address(SpecializedThunkJIT::regT2, Structure::indexingTypeOffset()), SpecializedThunkJIT::regT3);
    jit.loadPtr(Address(SpecializedThunkJIT::regT0, JSObject::butterflyOffset()), SpecializedThunkJIT::regT2);
    
    jit.and32(TrustedImm32(IndexingShapeMask), SpecializedThunkJIT::regT3);
    
    Jump notDone = jit.branch32(SpecializedThunkJIT::Below, SpecializedThunkJIT::regT1, Address(SpecializedThunkJIT::regT2, Butterfly::offsetOfPublicLength()));
    // Return the termination signal to indicate that we've finished
    jit.move(TrustedImmPtr(vm->iterationTerminator.get()), SpecializedThunkJIT::regT0);
    jit.returnJSCell(SpecializedThunkJIT::regT0);
    
    notDone.link(&jit);
    
    
    Jump notKey = jit.branch32(SpecializedThunkJIT::NotEqual, Address(SpecializedThunkJIT::regT4, JSArrayIterator::offsetOfIterationKind()), TrustedImm32(ArrayIterateKey));
    // If we're doing key iteration we just need to increment m_nextIndex and return the current value
    jit.add32(TrustedImm32(1), Address(SpecializedThunkJIT::regT4, JSArrayIterator::offsetOfNextIndex()));
    jit.returnInt32(SpecializedThunkJIT::regT1);
    
    notKey.link(&jit);
    
    
    // Okay, now we're returning a value so make sure we're inside the vector size
    jit.appendFailure(jit.branch32(SpecializedThunkJIT::AboveOrEqual, SpecializedThunkJIT::regT1, Address(SpecializedThunkJIT::regT2, Butterfly::offsetOfVectorLength())));
    
    // So now we perform inline loads for int32, value/undecided, and double storage
    Jump undecidedStorage = jit.branch32(SpecializedThunkJIT::Equal, SpecializedThunkJIT::regT3, TrustedImm32(UndecidedShape));
    Jump notContiguousStorage = jit.branch32(SpecializedThunkJIT::NotEqual, SpecializedThunkJIT::regT3, TrustedImm32(ContiguousShape));
    
    undecidedStorage.link(&jit);
    
    jit.loadPtr(Address(SpecializedThunkJIT::regT0, JSObject::butterflyOffset()), SpecializedThunkJIT::regT2);
    
#if USE(JSVALUE64)
    jit.load64(BaseIndex(SpecializedThunkJIT::regT2, SpecializedThunkJIT::regT1, SpecializedThunkJIT::TimesEight), SpecializedThunkJIT::regT0);
    Jump notHole = jit.branchTest64(SpecializedThunkJIT::NonZero, SpecializedThunkJIT::regT0);
    jit.move(JSInterfaceJIT::TrustedImm64(ValueUndefined), JSInterfaceJIT::regT0);
    notHole.link(&jit);
    jit.addPtr(TrustedImm32(1), Address(SpecializedThunkJIT::regT4, JSArrayIterator::offsetOfNextIndex()));
    jit.returnJSValue(SpecializedThunkJIT::regT0);
#else
    jit.load32(BaseIndex(SpecializedThunkJIT::regT2, SpecializedThunkJIT::regT1, SpecializedThunkJIT::TimesEight, JSValue::offsetOfTag()), SpecializedThunkJIT::regT3);
    Jump notHole = jit.branch32(SpecializedThunkJIT::NotEqual, SpecializedThunkJIT::regT3, TrustedImm32(JSValue::EmptyValueTag));
    jit.move(JSInterfaceJIT::TrustedImm32(JSValue::UndefinedTag), JSInterfaceJIT::regT1);
    jit.move(JSInterfaceJIT::TrustedImm32(0), JSInterfaceJIT::regT0);
    jit.add32(TrustedImm32(1), Address(SpecializedThunkJIT::regT4, JSArrayIterator::offsetOfNextIndex()));
    jit.returnJSValue(SpecializedThunkJIT::regT0, JSInterfaceJIT::regT1);
    notHole.link(&jit);
    jit.load32(BaseIndex(SpecializedThunkJIT::regT2, SpecializedThunkJIT::regT1, SpecializedThunkJIT::TimesEight, JSValue::offsetOfPayload()), SpecializedThunkJIT::regT0);
    jit.add32(TrustedImm32(1), Address(SpecializedThunkJIT::regT4, JSArrayIterator::offsetOfNextIndex()));
    jit.move(SpecializedThunkJIT::regT3, SpecializedThunkJIT::regT1);
    jit.returnJSValue(SpecializedThunkJIT::regT0, SpecializedThunkJIT::regT1);
#endif
    notContiguousStorage.link(&jit);
    
    Jump notInt32Storage = jit.branch32(SpecializedThunkJIT::NotEqual, SpecializedThunkJIT::regT3, TrustedImm32(Int32Shape));
    jit.loadPtr(Address(SpecializedThunkJIT::regT0, JSObject::butterflyOffset()), SpecializedThunkJIT::regT2);
    jit.load32(BaseIndex(SpecializedThunkJIT::regT2, SpecializedThunkJIT::regT1, SpecializedThunkJIT::TimesEight, JSValue::offsetOfPayload()), SpecializedThunkJIT::regT0);
    jit.add32(TrustedImm32(1), Address(SpecializedThunkJIT::regT4, JSArrayIterator::offsetOfNextIndex()));
    jit.returnInt32(SpecializedThunkJIT::regT0);
    notInt32Storage.link(&jit);
    
    jit.appendFailure(jit.branch32(SpecializedThunkJIT::NotEqual, SpecializedThunkJIT::regT3, TrustedImm32(DoubleShape)));
    jit.loadPtr(Address(SpecializedThunkJIT::regT0, JSObject::butterflyOffset()), SpecializedThunkJIT::regT2);
    jit.loadDouble(BaseIndex(SpecializedThunkJIT::regT2, SpecializedThunkJIT::regT1, SpecializedThunkJIT::TimesEight), SpecializedThunkJIT::fpRegT0);
    jit.add32(TrustedImm32(1), Address(SpecializedThunkJIT::regT4, JSArrayIterator::offsetOfNextIndex()));
    jit.returnDouble(SpecializedThunkJIT::fpRegT0);
    
    return jit.finalize(vm->jitStubs->ctiNativeCall(vm), "array-iterator-next");
}
    
}

#endif // ENABLE(JIT)
