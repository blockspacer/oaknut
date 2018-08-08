//
// Copyright © 2017 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//

#include <oaknut.h>

App app;

App::App() {
    StringProcessor parser(
#include "styles.res"
    );
    _styles.parse(parser);
}

#ifndef ANDROID
void App::log(char const* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    printf("\n");
}
void App::warn(char const* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("Warning: ");
    vprintf(fmt, args);
    printf("\n");
}
#endif


float App::dp(float dp) {
    return dp*_window->_scale;
}
float App::idp(float pix) {
    return pix/_window->_scale;
}


void App::loadStyleAsset(const string& assetPath) {
    ObjPtr<ByteBuffer> data = app.loadAsset(assetPath.data());
    if (!data) {
        return;
    }
    StringProcessor it(data->toString(false));
    _styles.parse(it);
}

Font* App::getStyleFont(const string &key) {
    string fontName = getStyleString(key + ".font-name", "");
    float fontSize = getStyleFloat(key + ".font-size");
    Font* font = Font::get(fontName, fontSize);
    return font;
}

StyleValue* App::getStyleValue(const string& keypath) {
    StyleValue* value = _styles.getValue(keypath);
    return value;
}


string App::getStyleString(const string& keypath, const char* defaultString) {
    StyleValue* value = getStyleValue(keypath);
    if (!value) {
        if (defaultString) {
            return string(defaultString);
        }
        app.warn("Missing string style info '%s'", keypath.data());
        return "";
    }
    return value->stringVal();
}

float App::getStyleFloat(const string& keypath) {
    StyleValue* value = getStyleValue(keypath);
    if (!value) {
        app.warn("Missing float style info '%s'", keypath.data());
        return 0;
    }
    return value->floatVal();
}
COLOR App::getStyleColor(const string& key) {
    StyleValue* value = getStyleValue(key);
    if (!value) {
        app.warn("Missing color style info '%s'", key.data());
        return 0;
    }
    return value->colorVal();
}

static void importAttributes(map<string, StyleValue*>& attribs, map<string, StyleValue*> attribsToImport) {
    for (auto i : attribsToImport) {
        if (i.first == "style") {
            importAttributes(attribs, i.second->compoundVal()->_values);
        }
    }
    for (auto i : attribsToImport) {
        if (i.first != "style") {
            attribs[i.first] = i.second;
        }
    }
}

static View* inflateFromResource(pair<string, StyleValue*> r, View* parent) {
    string viewClassName = r.first;
    View* view = (View*)Object::createByName(viewClassName);
    if (parent) {
        parent->addSubview(view);
    }
    
    map<string, StyleValue*> attribs;

    // Bring in default style attributes for this view type
    StyleValue* styleAttrib = NULL;
    styleAttrib = app.getStyleValue(viewClassName);
    if (styleAttrib) {
        importAttributes(attribs, styleAttrib->compoundVal()->_values);
    }
    
    // Process the custom attributes and subviews
    StyleValue* props = r.second;
    assert(props->type == StyleValue::Type::Compound);
    vector<pair<string, StyleValue*>> subviews;
    map<string, StyleValue*> customAttribs;
    for (auto i : props->compoundVal()->_valuesList)  {
        StyleValue* val = i.second;
        if (val) {
            auto a = make_pair(i.first, val);
            auto firstNameChar = i.first.charAt(0);
            if (val->type == StyleValue::Type::Compound && (firstNameChar >='A' && firstNameChar<='Z')) {
                subviews.push_back(a);
            } else {
                customAttribs[i.first] = val;
            }
        }
    }
    importAttributes(attribs, customAttribs);

    
    // Apply the attributes to the inflated view.
    view->applyStyleValues(attribs);
    
    // Inflate the subviews
    for (auto j : subviews) {
        inflateFromResource(j, view);
    }
    
    return view;
}


View* App::layoutInflate(const string& assetPath) {
    ObjPtr<ByteBuffer> data = loadAsset(assetPath.data());
    if (!data) {
        return NULL;
    }
    StringProcessor it(data->toString(false));
    StyleMap layoutRoot;
    bool parsed = layoutRoot.parse(it);
    assert(parsed);
    assert(layoutRoot._values.size()==1);
    auto i = layoutRoot._values.begin();
    return inflateFromResource(make_pair(i->first, i->second), NULL);
}






string App::friendlyTimeString(TIMESTAMP timestamp) {
    TIMESTAMP now = currentMillis();
    int age = (int)(now - timestamp);
    int seconds = age / 1000;
    int minutes = seconds / 60;
    seconds = seconds % 60;
    int hours = minutes / 60;
    minutes = minutes % 60;
    
    if (minutes<1) {
        return "Just now";
    }
    if (hours <1) {
        return string::format("%d min", minutes);
    }
    char buff[256];
    time_t time = timestamp / 1000;
    struct tm* tm = localtime(&time);
    strftime(buff, sizeof(buff), "%d %b %Y", tm);
    return buff;
}
