/*
 * Copyright (C) 2011 Google Inc. All Rights Reserved.
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
 *  THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 *  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "config.h"
#include "StyleGridData.h"

#include "RenderStyle.h"

namespace WebCore {

StyleGridData::StyleGridData()
    : m_gridColumns(RenderStyle::initialGridColumns())
    , m_gridRows(RenderStyle::initialGridRows())
    , m_gridAutoFlow(RenderStyle::initialGridAutoFlow())
    , m_gridAutoRows(RenderStyle::initialGridAutoRows())
    , m_gridAutoColumns(RenderStyle::initialGridAutoColumns())
    , m_namedGridArea(RenderStyle::initialNamedGridArea())
    , m_namedGridAreaRowCount(RenderStyle::initialNamedGridAreaCount())
    , m_namedGridAreaColumnCount(RenderStyle::initialNamedGridAreaCount())
{
}

StyleGridData::StyleGridData(const StyleGridData& o)
    : RefCounted<StyleGridData>()
    , m_gridColumns(o.m_gridColumns)
    , m_gridRows(o.m_gridRows)
    , m_namedGridColumnLines(o.m_namedGridColumnLines)
    , m_namedGridRowLines(o.m_namedGridRowLines)
    , m_gridAutoFlow(o.m_gridAutoFlow)
    , m_gridAutoRows(o.m_gridAutoRows)
    , m_gridAutoColumns(o.m_gridAutoColumns)
    , m_namedGridArea(o.m_namedGridArea)
    , m_namedGridAreaRowCount(o.m_namedGridAreaRowCount)
    , m_namedGridAreaColumnCount(o.m_namedGridAreaColumnCount)
{
}

} // namespace WebCore

