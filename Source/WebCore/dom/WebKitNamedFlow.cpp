/*
 * Copyright (C) 2011 Adobe Systems Incorporated. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
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

#include "config.h"
#include "WebKitNamedFlow.h"

#include "EventNames.h"
#include "NamedFlowCollection.h"
#include "RenderNamedFlowThread.h"
#include "RenderRegion.h"
#include "ScriptExecutionContext.h"
#include "StaticNodeList.h"
#include "UIEvent.h"

namespace WebCore {

WebKitNamedFlow::WebKitNamedFlow(PassRefPtr<NamedFlowCollection> manager, const AtomicString& flowThreadName)
    : m_flowThreadName(flowThreadName)
    , m_flowManager(manager)
    , m_parentFlowThread(0)
{
}

WebKitNamedFlow::~WebKitNamedFlow()
{
    // The named flow is not "strong" referenced from anywhere at this time so it shouldn't be reused if the named flow is recreated.
    m_flowManager->discardNamedFlow(this);
}

PassRefPtr<WebKitNamedFlow> WebKitNamedFlow::create(PassRefPtr<NamedFlowCollection> manager, const AtomicString& flowThreadName)
{
    return adoptRef(new WebKitNamedFlow(manager, flowThreadName));
}

const AtomicString& WebKitNamedFlow::name() const
{
    return m_flowThreadName;
}

bool WebKitNamedFlow::overset() const
{
    if (m_flowManager->document())
        m_flowManager->document()->updateLayoutIgnorePendingStylesheets();

    // The renderer may be destroyed or created after the style update.
    // Because this is called from JS, where the wrapper keeps a reference to the NamedFlow, no guard is necessary.
    return m_parentFlowThread ? m_parentFlowThread->overset() : true;
}

static inline bool inFlowThread(RenderObject* renderer, RenderNamedFlowThread* flowThread)
{
    if (!renderer)
        return false;
    RenderFlowThread* currentFlowThread = renderer->flowThreadContainingBlock();
    if (flowThread == currentFlowThread)
        return true;
    if (renderer->flowThreadState() != RenderObject::InsideInFlowThread)
        return false;
    
    // An in-flow flow thread can be nested inside an out-of-flow one, so we have to recur up to check.
    return inFlowThread(currentFlowThread->containingBlock(), flowThread);
}

int WebKitNamedFlow::firstEmptyRegionIndex() const
{
    if (m_flowManager->document())
        m_flowManager->document()->updateLayoutIgnorePendingStylesheets();

    if (!m_parentFlowThread)
        return -1;

    const RenderRegionList& regionList = m_parentFlowThread->renderRegionList();
    if (regionList.isEmpty())
        return -1;

    int countNonPseudoRegions = -1;
    RenderRegionList::const_iterator iter = regionList.begin();
    for (int index = 0; iter != regionList.end(); ++index, ++iter) {
        const RenderRegion* renderRegion = *iter;
        // FIXME: Pseudo-elements are not included in the list.
        // They will be included when we will properly support the Region interface
        // http://dev.w3.org/csswg/css-regions/#the-region-interface
        if (renderRegion->isPseudoElement())
            continue;
        countNonPseudoRegions++;
        if (renderRegion->regionOversetState() == RegionEmpty)
            return countNonPseudoRegions;
    }
    return -1;
}

PassRefPtr<NodeList> WebKitNamedFlow::getRegionsByContent(Node* contentNode)
{
    if (!contentNode)
        return StaticElementList::createEmpty();

    if (m_flowManager->document())
        m_flowManager->document()->updateLayoutIgnorePendingStylesheets();

    // The renderer may be destroyed or created after the style update.
    // Because this is called from JS, where the wrapper keeps a reference to the NamedFlow, no guard is necessary.
    if (!m_parentFlowThread)
        return StaticElementList::createEmpty();

    Vector<Ref<Element>> regionElements;

    if (inFlowThread(contentNode->renderer(), m_parentFlowThread)) {
        const RenderRegionList& regionList = m_parentFlowThread->renderRegionList();
        for (auto iter = regionList.begin(), end = regionList.end(); iter != end; ++iter) {
            const RenderRegion* renderRegion = *iter;
            // FIXME: Pseudo-elements are not included in the list.
            // They will be included when we will properly support the Region interface
            // http://dev.w3.org/csswg/css-regions/#the-region-interface
            if (renderRegion->isPseudoElement())
                continue;
            if (m_parentFlowThread->objectInFlowRegion(contentNode->renderer(), renderRegion)) {
                ASSERT(renderRegion->generatingElement());
                regionElements.append(*renderRegion->generatingElement());
            }
        }
    }

    return StaticElementList::adopt(regionElements);
}

PassRefPtr<NodeList> WebKitNamedFlow::getRegions()
{
    if (m_flowManager->document())
        m_flowManager->document()->updateLayoutIgnorePendingStylesheets();

    // The renderer may be destroyed or created after the style update.
    // Because this is called from JS, where the wrapper keeps a reference to the NamedFlow, no guard is necessary.
    if (!m_parentFlowThread)
        return StaticElementList::createEmpty();

    Vector<Ref<Element>> regionElements;

    const RenderRegionList& regionList = m_parentFlowThread->renderRegionList();
    for (auto iter = regionList.begin(), end = regionList.end(); iter != end; ++iter) {
        const RenderRegion* renderRegion = *iter;
        // FIXME: Pseudo-elements are not included in the list.
        // They will be included when we will properly support the Region interface
        // http://dev.w3.org/csswg/css-regions/#the-region-interface
        if (renderRegion->isPseudoElement())
            continue;
        ASSERT(renderRegion->generatingElement());
        regionElements.append(*renderRegion->generatingElement());
    }

    return StaticElementList::adopt(regionElements);
}

PassRefPtr<NodeList> WebKitNamedFlow::getContent()
{
    if (m_flowManager->document())
        m_flowManager->document()->updateLayoutIgnorePendingStylesheets();

    // The renderer may be destroyed or created after the style update.
    // Because this is called from JS, where the wrapper keeps a reference to the NamedFlow, no guard is necessary.
    if (!m_parentFlowThread)
        return StaticElementList::createEmpty();

    Vector<Ref<Element>> contentElements;

    const NamedFlowContentElements& contentElementsList = m_parentFlowThread->contentElements();
    for (auto it = contentElementsList.begin(), end = contentElementsList.end(); it != end; ++it) {
        Element* element = *it;
        ASSERT(element->computedStyle()->flowThread() == m_parentFlowThread->flowThreadName());
        contentElements.append(*element);
    }

    return StaticElementList::adopt(contentElements);
}

void WebKitNamedFlow::setRenderer(RenderNamedFlowThread* parentFlowThread)
{
    // The named flow can either go from a no_renderer->renderer or renderer->no_renderer state; anything else could indicate a bug.
    ASSERT((!m_parentFlowThread && parentFlowThread) || (m_parentFlowThread && !parentFlowThread));

    // If parentFlowThread is 0, the flow thread will move in the "NULL" state.
    m_parentFlowThread = parentFlowThread;
}

void WebKitNamedFlow::dispatchRegionLayoutUpdateEvent()
{
    ASSERT(!NoEventDispatchAssertion::isEventDispatchForbidden());

    // If the flow is in the "NULL" state the event should not be dispatched any more.
    if (flowState() == FlowStateNull)
        return;

    dispatchEvent(UIEvent::create(eventNames().webkitregionlayoutupdateEvent, false, false, m_flowManager->document()->defaultView(), 0));
}
    
void WebKitNamedFlow::dispatchRegionOversetChangeEvent()
{
    ASSERT(!NoEventDispatchAssertion::isEventDispatchForbidden());
    
    // If the flow is in the "NULL" state the event should not be dispatched any more.
    if (flowState() == FlowStateNull)
        return;

    dispatchEvent(UIEvent::create(eventNames().webkitregionoversetchangeEvent, false, false, m_flowManager->document()->defaultView(), 0));
}

ScriptExecutionContext* WebKitNamedFlow::scriptExecutionContext() const
{
    return m_flowManager->document();
}

Node* WebKitNamedFlow::ownerNode() const
{
    return m_flowManager->document();
}

} // namespace WebCore

