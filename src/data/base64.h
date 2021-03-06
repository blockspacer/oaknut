//
// Copyright © 2018 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//

string base64_encode(const string& str);
string base64_encode(const bytearray& array);
string base64_encode(uint8_t const* buf, unsigned int bufLen);
bytearray base64_decode(string const& encoded_string);

