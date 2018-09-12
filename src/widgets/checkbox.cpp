//
// Copyright © 2018 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//

#include <oaknut.h>

DECLARE_DYNCREATE(Checkbox);

Checkbox::Checkbox() {
}


bool Checkbox::onInputEvent(INPUTEVENT* event) {
    if (event->type == INPUT_EVENT_TAP) {
        setChecked(!isChecked());
        return true;
    }
    return ImageView::onInputEvent(event);
}

void Checkbox::onStateChanged(STATESET changes) {
    ImageView::onStateChanged(changes);
    if (changes.mask & STATE_CHECKED) {
        if (onIsCheckedChanged) {
            onIsCheckedChanged(this, isChecked());
        }
    }
}
