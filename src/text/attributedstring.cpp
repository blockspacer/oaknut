//
// Copyright © 2018 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//

#include <oaknut.h>


AttributedString::AttributedString() {
    
}
AttributedString::AttributedString(const string& str) {
    assign(str.data(), str.lengthInBytes());
}
AttributedString::AttributedString(const AttributedString& str) {
    _attributes = str._attributes;
}

void AttributedString::setAttribute(const Attribute& attribute, int32_t start, int32_t end) {
    _attributes.emplace(attribute, start, end);
}