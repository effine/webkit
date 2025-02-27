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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(MEDIA_STREAM)

#include "MediaSourceStates.h"

#include <wtf/NeverDestroyed.h>

namespace WebCore {

RefPtr<MediaSourceStates> MediaSourceStates::create(const MediaStreamSourceStates& states)
{
    return adoptRef(new MediaSourceStates(states));
}

MediaSourceStates::MediaSourceStates(const MediaStreamSourceStates& states)
    : m_SourceStates(states)
{
}

const AtomicString& MediaSourceStates::facingMode(MediaStreamSourceStates::VideoFacingMode mode)
{
    static NeverDestroyed<AtomicString> userFacing("user", AtomicString::ConstructFromLiteral);
    static NeverDestroyed<AtomicString> environmentFacing("environment", AtomicString::ConstructFromLiteral);
    static NeverDestroyed<AtomicString> leftFacing("left", AtomicString::ConstructFromLiteral);
    static NeverDestroyed<AtomicString> rightFacing("right", AtomicString::ConstructFromLiteral);
    
    switch (mode) {
    case MediaStreamSourceStates::User:
        return userFacing;
    case MediaStreamSourceStates::Environment:
        return environmentFacing;
    case MediaStreamSourceStates::Left:
        return leftFacing;
    case MediaStreamSourceStates::Right:
        return rightFacing;
    }

    ASSERT_NOT_REACHED();
    return emptyAtom;
}

const AtomicString& MediaSourceStates::sourceType(MediaStreamSourceStates::SourceType sourceType)
{
    static NeverDestroyed<AtomicString> none("none", AtomicString::ConstructFromLiteral);
    static NeverDestroyed<AtomicString> camera("camera", AtomicString::ConstructFromLiteral);
    static NeverDestroyed<AtomicString> microphone("microphone", AtomicString::ConstructFromLiteral);

    switch (sourceType) {
    case MediaStreamSourceStates::None:
        return none;
    case MediaStreamSourceStates::Camera:
        return camera;
    case MediaStreamSourceStates::Microphone:
        return microphone;
    }

    ASSERT_NOT_REACHED();
    return emptyAtom;
}

const AtomicString& MediaSourceStates::sourceType() const
{
    return MediaSourceStates::sourceType(m_SourceStates.sourceType);
}

const AtomicString& MediaSourceStates::facingMode() const
{
    return MediaSourceStates::facingMode(m_SourceStates.facingMode);
}


} // namespace WebCore

#endif
