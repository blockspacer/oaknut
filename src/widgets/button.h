//
// Copyright © 2018 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//


class Button : public Label {
public:
    
    Button();

};

class ToolbarButton : public ImageView {
public:
    
    ToolbarButton();
    
    bool handleInputEvent(INPUTEVENT* event) override;
    
};


