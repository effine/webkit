/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(TEMPLATE_ELEMENT)

#include "HTMLTemplateElement.h"

#include "DOMImplementation.h"
#include "DocumentFragment.h"
#include "HTMLDocument.h"
#include "TemplateContentDocumentFragment.h"
#include "markup.h"

namespace WebCore {

using namespace HTMLNames;

inline HTMLTemplateElement::HTMLTemplateElement(const QualifiedName& tagName, Document& document)
    : HTMLElement(tagName, document)
{
}

HTMLTemplateElement::~HTMLTemplateElement()
{
}

PassRefPtr<HTMLTemplateElement> HTMLTemplateElement::create(const QualifiedName& tagName, Document& document)
{
    return adoptRef(new HTMLTemplateElement(tagName, document));
}

DocumentFragment* HTMLTemplateElement::content() const
{
    if (!m_content)
        m_content = TemplateContentDocumentFragment::create(*document().ensureTemplateDocument(), this);

    return m_content.get();
}

PassRefPtr<Node> HTMLTemplateElement::cloneNode(bool deep)
{
    if (!deep)
        return cloneElementWithoutChildren();

    RefPtr<Node> clone = cloneElementWithChildren();
    if (m_content)
        content()->cloneChildNodes(toHTMLTemplateElement(clone.get())->content());
    return clone.release();
}

void HTMLTemplateElement::didMoveToNewDocument(Document* oldDocument)
{
    HTMLElement::didMoveToNewDocument(oldDocument);
    if (!m_content)
        return;
    document().ensureTemplateDocument()->adoptIfNeeded(m_content.get());
}

} // namespace WebCore

#endif // ENABLE(TEMPLATE_ELEMENT)
