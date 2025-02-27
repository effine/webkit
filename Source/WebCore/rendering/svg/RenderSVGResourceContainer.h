/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef RenderSVGResourceContainer_h
#define RenderSVGResourceContainer_h

#if ENABLE(SVG)
#include "RenderSVGHiddenContainer.h"
#include "RenderSVGResource.h"

namespace WebCore {

class RenderLayer;

class RenderSVGResourceContainer : public RenderSVGHiddenContainer,
                                   public RenderSVGResource {
public:
    virtual ~RenderSVGResourceContainer();

    virtual void layout() OVERRIDE;
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle) OVERRIDE FINAL;

    virtual bool isSVGResourceContainer() const OVERRIDE FINAL { return true; }
    virtual RenderSVGResourceContainer* toRenderSVGResourceContainer() OVERRIDE FINAL { return this; }

    static bool shouldTransformOnTextPainting(RenderObject*, AffineTransform&);
    static AffineTransform transformOnNonScalingStroke(RenderObject*, const AffineTransform& resourceTransform);

    void idChanged();
    void addClientRenderLayer(RenderLayer*);
    void removeClientRenderLayer(RenderLayer*);

protected:
    explicit RenderSVGResourceContainer(SVGElement&);

    enum InvalidationMode {
        LayoutAndBoundariesInvalidation,
        BoundariesInvalidation,
        RepaintInvalidation,
        ParentOnlyInvalidation
    };

    // Used from the invalidateClient/invalidateClients methods from classes, inheriting from us.
    void markAllClientsForInvalidation(InvalidationMode);
    void markAllClientLayersForInvalidation();
    void markClientForInvalidation(RenderObject*, InvalidationMode);

private:
    friend class SVGResourcesCache;
    void addClient(RenderObject*);
    void removeClient(RenderObject*);

private:
    virtual void willBeDestroyed() OVERRIDE FINAL;
    void registerResource();

    AtomicString m_id;
    bool m_registered : 1;
    bool m_isInvalidating : 1;
    HashSet<RenderObject*> m_clients;
    HashSet<RenderLayer*> m_clientLayers;
};

inline RenderSVGResourceContainer* getRenderSVGResourceContainerById(Document& document, const AtomicString& id)
{
    if (id.isEmpty())
        return 0;

    if (RenderSVGResourceContainer* renderResource = document.accessSVGExtensions()->resourceById(id))
        return renderResource;

    return 0;
}

template<typename Renderer>
Renderer* getRenderSVGResourceById(Document& document, const AtomicString& id)
{
    if (RenderSVGResourceContainer* container = getRenderSVGResourceContainerById(document, id))
        return container->cast<Renderer>();

    return 0;
}

}

#endif
#endif
