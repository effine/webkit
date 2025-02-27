/*
 * Copyright (C) 1999 Lars Knoll <knoll@kde.org>
 * Copyright (C) 1999 Antti Koivisto <koivisto@kde.org>
 * Copyright (C) 2006 Allan Sandfeld Jensen <kde@carewolf.com>
 * Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
 * Copyright (C) 2004, 2005, 2006, 2007, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
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
 *
 */

#ifndef RenderImageResource_h
#define RenderImageResource_h

#include "CachedResourceHandle.h"
#include "Image.h"
#include "LayoutSize.h"
#include "StyleImage.h"

namespace WebCore {

class RenderElement;

class RenderImageResource {
    WTF_MAKE_NONCOPYABLE(RenderImageResource); WTF_MAKE_FAST_ALLOCATED;
public:
    virtual ~RenderImageResource();

    static PassOwnPtr<RenderImageResource> create()
    {
        return adoptPtr(new RenderImageResource);
    }

    virtual void initialize(RenderElement*);
    virtual void shutdown();

    void setCachedImage(CachedImage*);
    CachedImage* cachedImage() const { return m_cachedImage.get(); }
    virtual bool hasImage() const { return m_cachedImage; }

    void resetAnimation();

    virtual PassRefPtr<Image> image(int width = 0, int height = 0) const;
    virtual bool errorOccurred() const;

    virtual void setContainerSizeForRenderer(const IntSize&);
    virtual bool usesImageContainerSize() const;
    virtual bool imageHasRelativeWidth() const;
    virtual bool imageHasRelativeHeight() const;

    virtual LayoutSize imageSize(float multiplier) const;
    virtual LayoutSize intrinsicSize(float multiplier) const;

    virtual WrappedImagePtr imagePtr() const { return m_cachedImage.get(); }

protected:
    RenderImageResource();
    RenderElement* m_renderer;
    CachedResourceHandle<CachedImage> m_cachedImage;

private:
    static Image* nullImage();
};

} // namespace WebCore

#endif // RenderImage_h
