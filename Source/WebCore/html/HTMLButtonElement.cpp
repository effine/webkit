/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2010 Apple Inc. All rights reserved.
 *           (C) 2006 Alexey Proskuryakov (ap@nypop.com)
 * Copyright (C) 2007 Samuel Weinig (sam@webkit.org)
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

#include "config.h"
#include "HTMLButtonElement.h"

#include "Attribute.h"
#include "EventNames.h"
#include "FormDataList.h"
#include "HTMLFormElement.h"
#include "HTMLNames.h"
#include "KeyboardEvent.h"
#include "RenderButton.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

using namespace HTMLNames;

inline HTMLButtonElement::HTMLButtonElement(const QualifiedName& tagName, Document& document, HTMLFormElement* form)
    : HTMLFormControlElement(tagName, document, form)
    , m_type(SUBMIT)
    , m_isActivatedSubmit(false)
{
    ASSERT(hasTagName(buttonTag));
}

PassRefPtr<HTMLButtonElement> HTMLButtonElement::create(const QualifiedName& tagName, Document& document, HTMLFormElement* form)
{
    return adoptRef(new HTMLButtonElement(tagName, document, form));
}

void HTMLButtonElement::setType(const AtomicString& type)
{
    setAttribute(typeAttr, type);
}

RenderElement* HTMLButtonElement::createRenderer(RenderArena& arena, RenderStyle&)
{
    return new (arena) RenderButton(*this);
}

const AtomicString& HTMLButtonElement::formControlType() const
{
    switch (m_type) {
        case SUBMIT: {
            DEFINE_STATIC_LOCAL(const AtomicString, submit, ("submit", AtomicString::ConstructFromLiteral));
            return submit;
        }
        case BUTTON: {
            DEFINE_STATIC_LOCAL(const AtomicString, button, ("button", AtomicString::ConstructFromLiteral));
            return button;
        }
        case RESET: {
            DEFINE_STATIC_LOCAL(const AtomicString, reset, ("reset", AtomicString::ConstructFromLiteral));
            return reset;
        }
    }

    ASSERT_NOT_REACHED();
    return emptyAtom;
}

bool HTMLButtonElement::isPresentationAttribute(const QualifiedName& name) const
{
    if (name == alignAttr) {
        // Don't map 'align' attribute.  This matches what Firefox and IE do, but not Opera.
        // See http://bugs.webkit.org/show_bug.cgi?id=12071
        return false;
    }

    return HTMLFormControlElement::isPresentationAttribute(name);
}

void HTMLButtonElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == typeAttr) {
        if (equalIgnoringCase(value, "reset"))
            m_type = RESET;
        else if (equalIgnoringCase(value, "button"))
            m_type = BUTTON;
        else
            m_type = SUBMIT;
        setNeedsWillValidateCheck();
    } else
        HTMLFormControlElement::parseAttribute(name, value);
}

void HTMLButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().DOMActivateEvent && !isDisabledFormControl()) {
        if (form() && m_type == SUBMIT) {
            m_isActivatedSubmit = true;
            form()->prepareForSubmission(event);
            event->setDefaultHandled();
            m_isActivatedSubmit = false; // Do this in case submission was canceled.
        }
        if (form() && m_type == RESET) {
            form()->reset();
            event->setDefaultHandled();
        }
    }

    if (event->isKeyboardEvent()) {
        if (event->type() == eventNames().keydownEvent && static_cast<KeyboardEvent*>(event)->keyIdentifier() == "U+0020") {
            setActive(true, true);
            // No setDefaultHandled() - IE dispatches a keypress in this case.
            return;
        }
        if (event->type() == eventNames().keypressEvent) {
            switch (static_cast<KeyboardEvent*>(event)->charCode()) {
                case '\r':
                    dispatchSimulatedClick(event);
                    event->setDefaultHandled();
                    return;
                case ' ':
                    // Prevent scrolling down the page.
                    event->setDefaultHandled();
                    return;
            }
        }
        if (event->type() == eventNames().keyupEvent && static_cast<KeyboardEvent*>(event)->keyIdentifier() == "U+0020") {
            if (active())
                dispatchSimulatedClick(event);
            event->setDefaultHandled();
            return;
        }
    }

    HTMLFormControlElement::defaultEventHandler(event);
}

bool HTMLButtonElement::willRespondToMouseClickEvents()
{
    if (!isDisabledFormControl() && form() && (m_type == SUBMIT || m_type == RESET))
        return true;
    return HTMLFormControlElement::willRespondToMouseClickEvents();
}

bool HTMLButtonElement::isSuccessfulSubmitButton() const
{
    // HTML spec says that buttons must have names to be considered successful.
    // However, other browsers do not impose this constraint.
    return m_type == SUBMIT && !isDisabledFormControl();
}

bool HTMLButtonElement::isActivatedSubmit() const
{
    return m_isActivatedSubmit;
}

void HTMLButtonElement::setActivatedSubmit(bool flag)
{
    m_isActivatedSubmit = flag;
}

bool HTMLButtonElement::appendFormData(FormDataList& formData, bool)
{
    if (m_type != SUBMIT || name().isEmpty() || !m_isActivatedSubmit)
        return false;
    formData.appendData(name(), value());
    return true;
}

void HTMLButtonElement::accessKeyAction(bool sendMouseEvents)
{
    focus();

    dispatchSimulatedClick(0, sendMouseEvents ? SendMouseUpDownEvents : SendNoEvents);
}

bool HTMLButtonElement::isURLAttribute(const Attribute& attribute) const
{
    return attribute.name() == formactionAttr || HTMLFormControlElement::isURLAttribute(attribute);
}

String HTMLButtonElement::value() const
{
    return getAttribute(valueAttr);
}

bool HTMLButtonElement::recalcWillValidate() const
{
    return m_type == SUBMIT && HTMLFormControlElement::recalcWillValidate();
}

} // namespace
