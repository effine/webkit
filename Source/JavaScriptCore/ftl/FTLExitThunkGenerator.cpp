/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#include "config.h"
#include "FTLExitThunkGenerator.h"

#if ENABLE(FTL_JIT)

#include "FTLOSRExitCompilationInfo.h"
#include "FTLState.h"

namespace JSC { namespace FTL {

using namespace JSC::DFG;

ExitThunkGenerator::ExitThunkGenerator(State& state)
    : CCallHelpers(&state.graph.m_vm, state.graph.m_codeBlock)
    , m_state(state)
    , m_didThings(false)
{
}

ExitThunkGenerator::~ExitThunkGenerator()
{
}

void ExitThunkGenerator::emitThunk(unsigned index)
{
    OSRExitCompilationInfo& info = m_state.finalizer->osrExit[index];
    
    info.m_thunkLabel = label();
    if (Options::ftlOSRExitUsesStackmap())
        push(TrustedImm32(index));
    else
        move(TrustedImm32(index), GPRInfo::nonArgGPR0);
    info.m_thunkJump = patchableJump();
    
    m_didThings = true;
}

void ExitThunkGenerator::emitThunks()
{
    for (unsigned i = 0; i < m_state.finalizer->osrExit.size(); ++i)
        emitThunk(i);
}

} } // namespace JSC::FTL

#endif // ENABLE(FTL_JIT)

