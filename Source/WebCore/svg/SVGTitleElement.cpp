/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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

#include "config.h"
#if ENABLE(SVG)
#include "SVGTitleElement.h"

#include "Document.h"
#include "SVGNames.h"

namespace WebCore {

inline SVGTitleElement::SVGTitleElement(const QualifiedName& tagName, Document& document)
    : SVGElement(tagName, document)
{
    ASSERT(hasTagName(SVGNames::titleTag));
}

PassRefPtr<SVGTitleElement> SVGTitleElement::create(const QualifiedName& tagName, Document& document)
{
    return adoptRef(new SVGTitleElement(tagName, document));
}

Node::InsertionNotificationRequest SVGTitleElement::insertedInto(ContainerNode& rootParent)
{
    SVGElement::insertedInto(rootParent);
    if (!rootParent.inDocument())
        return InsertionDone;
    if (firstChild())
        // FIXME: does SVG have a title text direction?
        document().setTitleElement(StringWithDirection(textContent(), LTR), this);
    return InsertionDone;
}

void SVGTitleElement::removedFrom(ContainerNode& rootParent)
{
    SVGElement::removedFrom(rootParent);
    if (rootParent.inDocument())
        document().removeTitle(this);
}

void SVGTitleElement::childrenChanged(const ChildChange& change)
{
    SVGElement::childrenChanged(change);
    if (inDocument())
        // FIXME: does SVG have title text direction?
        document().setTitleElement(StringWithDirection(textContent(), LTR), this);
}

}

#endif // ENABLE(SVG)
