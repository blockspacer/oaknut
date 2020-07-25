//
//  Copyright © 2018 Sandcastle Software Ltd. All rights reserved.
//

#include "../app.h"
#include "../model/diskinfo.h"


class Snapshot : public Object, public ISerializeToVariant {
public:
    sp<DiskInfo> _diskInfo;
    string _label;
    TIMESTAMP _timestamp;
    sp<Bitmap> _thumbnail;
    bytearray _data;
    string _controllerId;

    Snapshot();
    void updateWithData(const bytearray& data, Bitmap* thumbnail, string controllerId);

    // ISerializeToVariant
    void fromVariant(const variant& v) override;
    void toVariant(variant& v) override;
};

