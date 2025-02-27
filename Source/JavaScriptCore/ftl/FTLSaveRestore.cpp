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
#include "FTLSaveRestore.h"

#if ENABLE(FTL_JIT)

#include "FPRInfo.h"
#include "GPRInfo.h"
#include "MacroAssembler.h"

namespace JSC { namespace FTL {

static size_t bytesForGPRs()
{
    return (MacroAssembler::lastRegister() - MacroAssembler::firstRegister() + 1) * sizeof(int64_t);
}

static size_t bytesForFPRs()
{
    // FIXME: It might be worthwhile saving the full state of the FP registers, at some point.
    // Right now we don't need this since we only do the save/restore just prior to OSR exit, and
    // OSR exit will be guaranteed to only need the double portion of the FP registers.
    return (MacroAssembler::lastFPRegister() - MacroAssembler::firstFPRegister() + 1) * sizeof(double);
}

size_t requiredScratchMemorySizeInBytes()
{
    return bytesForGPRs() + bytesForFPRs();
}

size_t offsetOfGPR(GPRReg reg)
{
    return (reg - MacroAssembler::firstRegister()) * sizeof(int64_t);
}

size_t offsetOfFPR(FPRReg reg)
{
    return bytesForGPRs() + (reg - MacroAssembler::firstFPRegister()) * sizeof(double);
}

void saveAllRegisters(MacroAssembler& jit, char* scratchMemory)
{
    // Get the first register out of the way, so that we can use it as a pointer.
    jit.poke64(MacroAssembler::firstRealRegister(), 0);
    jit.move(MacroAssembler::TrustedImmPtr(scratchMemory), MacroAssembler::firstRealRegister());
    
    // Get all of the other GPRs out of the way.
    for (MacroAssembler::RegisterID reg = MacroAssembler::secondRealRegister(); reg <= MacroAssembler::lastRegister(); reg = MacroAssembler::nextRegister(reg))
        jit.store64(reg, MacroAssembler::Address(MacroAssembler::firstRealRegister(), offsetOfGPR(reg)));
    
    // Restore the first register into the second one and save it.
    jit.peek64(MacroAssembler::secondRealRegister(), 0);
    jit.store64(MacroAssembler::secondRealRegister(), MacroAssembler::Address(MacroAssembler::firstRealRegister(), offsetOfGPR(MacroAssembler::firstRealRegister())));
    
    // Finally save all FPR's.
    for (MacroAssembler::FPRegisterID reg = MacroAssembler::firstFPRegister(); reg <= MacroAssembler::lastFPRegister(); reg = MacroAssembler::nextFPRegister(reg))
        jit.storeDouble(reg, MacroAssembler::Address(MacroAssembler::firstRealRegister(), offsetOfFPR(reg)));
}

void restoreAllRegisters(MacroAssembler& jit, char* scratchMemory)
{
    // Give ourselves a pointer to the scratch memory.
    jit.move(MacroAssembler::TrustedImmPtr(scratchMemory), MacroAssembler::firstRealRegister());
    
    // Restore all FPR's.
    for (MacroAssembler::FPRegisterID reg = MacroAssembler::firstFPRegister(); reg <= MacroAssembler::lastFPRegister(); reg = MacroAssembler::nextFPRegister(reg))
        jit.loadDouble(MacroAssembler::Address(MacroAssembler::firstRealRegister(), offsetOfFPR(reg)), reg);
    
    for (MacroAssembler::RegisterID reg = MacroAssembler::secondRealRegister(); reg <= MacroAssembler::lastRegister(); reg = MacroAssembler::nextRegister(reg))
        jit.load64(MacroAssembler::Address(MacroAssembler::firstRealRegister(), offsetOfGPR(reg)), reg);
    
    jit.load64(MacroAssembler::Address(MacroAssembler::firstRealRegister(), offsetOfGPR(MacroAssembler::firstRealRegister())), MacroAssembler::firstRealRegister());
}

} } // namespace JSC::FTL

#endif // ENABLE(FTL_JIT)

