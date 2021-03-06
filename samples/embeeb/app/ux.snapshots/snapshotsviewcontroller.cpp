//
//  Copyright © 2018 Sandcastle Software Ltd. All rights reserved.
//

#include "snapshotsviewcontroller.h"

class SnapshotView : public View {
public:
    SnapshotView() {
        app->layoutInflateExistingView(this, "layout/snapshot_listitem.res");
        imageView = (ImageView*)findViewById("image");
        titleLabel = (Label*)findViewById("title");
        subtitleLabel = (Label*)findViewById("subtitle");
    }
    void bind(Snapshot* snapshot) {
        imageView->setBitmap(snapshot->_thumbnail);
        titleLabel->setText(snapshot->_diskInfo?snapshot->_diskInfo->_title:"No disk");
        subtitleLabel->setText(app->friendlyTimeString(snapshot->_timestamp));
    }
    ImageView* imageView;
    Label* titleLabel;
    Label* subtitleLabel;
};

class SnapshotsAdapter : public SimpleListAdapter<Snapshot*> {
public:
    SnapshotsAdapter(LocalStore* snapshotStore) : SimpleListAdapter() {
        _store = snapshotStore;
    }

    View* createItemView(LISTINDEX index) override {
        return new SnapshotView();
    }

    void bindItemView(View* itemview, LISTINDEX index) override {
        Snapshot* snapshot = _items[LISTINDEX_ITEM(index)];
        ((SnapshotView*)itemview)->bind(snapshot);
    }
    
    void reload() {
        _items.clear();
        _store->getAll([=] (variant* value) {
            if (value) {
                Snapshot* snapshot = new Snapshot();
                snapshot->fromVariant(*value);
                _items.push_back(snapshot);
            } else {
                if (_adapterView) {
                    _adapterView->reload();
                }
            }
        });
    }

    void deleteItem(LISTINDEX index) override {
        int realIndex = listIndexToRealIndex(index);
        Snapshot* snapshot = _items[realIndex];
        _store->remove(variant(snapshot->_timestamp), [=]() {
            SimpleListAdapter::deleteItem(index);
        });
    }

    LocalStore* _store;
};


SnapshotsViewController::SnapshotsViewController(Beeb* beeb, BeebView* beebView, DiskInfo* diskInfo, SnapshotSelectedDelegate delegate) {
    _delegate = delegate;
    _showCreateButton = true;
    _beeb = beeb;
    _beebView = beebView;
    _diskInfo = diskInfo;

    // Root view
    View* view = new View();
    view->setBackgroundColor(0xfff8f8f8);
    setView(view);

    // Navbar
    setTitle("Snapshots");
    addNavButton(true, "images/add.png", [=] () {
        Snapshot* snapshot = new Snapshot();
        snapshot->_diskInfo = _diskInfo;
        _showCreateButton = false;
        uint32_t dataSize = saveSnapshot(snapshot);
        //s_currentSnapshot = snapshot;
        
        //[self.view makeToast:[NSString stringWithFormat:@"Snapshot created. (%ld KB)", (long)dataSize/1024]
        //            duration:2.0
        //            position:CSToastPositionBottom];

    });

    // Listview
    _listView = new ListView();
    _listView->setLayoutSize(MEASURESPEC::Fill(), MEASURESPEC::Fill());
    //float statusBarHeight = style::get("statusbar.height")->floatVal();
    //_minTopScrollInset = style::get("navbar.height")->floatVal() + statusBarHeight;
    //_listView->setScrollInsets(EDGEINSETS(0, _minTopScrollInset, 0, 0));
    
    _snapshotStore = LocalStore::create("snapshots", "timestamp");
    SnapshotsAdapter* adapter = new SnapshotsAdapter(_snapshotStore);
    _listView->setAdapter(adapter);

    _listView->_onItemTapDelegate = [&] (View* view, LISTINDEX index) {
        Snapshot* snapshot = adapter->getItem(index);
        if (_delegate) _delegate(snapshot);
        _navigationController->popViewController();
    };
    _listView->_onItemLongPressDelegate = [&] (View* view, LISTINDEX index) {
        _listView->startEditing(this);
    };
    view->addSubview(_listView);

    /*Button* createButton = new Button();
    createButton->setLayoutSize(MEASURESPEC_FillParent, MEASURESPEC_Abs(app.dp(72)));
    createButton->setBackgroundColour(0xFF3083FB);
    createButton->setTextColour(0xFFFFFFFF);
    createButton->setText("Create Snapshot");
    createButton->_onClickDelegate = [&] (View*) {
    };
    _listView->setHeaderView(createButton);*/
    /*
    self.emptyLabel = [[UILabel alloc] initWithFrame:self.tableView.bounds];
    self.emptyLabel.textColor = [UIColor grayColor];
    self.emptyLabel.numberOfLines = 0;
    self.emptyLabel.textAlignment = NSTextAlignmentCenter;
    self.emptyLabel.text = @"No snapshots. Press \"Create Snapshot\" to create one from your current Beeb state.";
     */

    _snapshotStore->open([=]() {
        adapter->reload();
    });


}

uint32_t SnapshotsViewController::saveSnapshot(Snapshot* snapshot) {

    // Create a scaled-down thumbnail of the beeb's display
    sp<Canvas> canvas = Canvas::create();
    RECT rectSrc = _beebView->_visibleArea;
    rectSrc.scale(_beebView->_bitmap->_width, _beebView->_bitmap->_height);
    RECT rectDst = {0,0, app->dp(72), app->dp(52)}; // matches layout
    canvas->resize(rectDst.size.width, rectDst.size.height);
    canvas->drawBitmap(_beebView->_bitmap, rectSrc, rectDst);

    bytearray data(256*1024);
    uint32_t cb = (uint32_t)(_beeb->serialize(true, data.data()) - data.data());
    snapshot->_diskInfo = _diskInfo;
    snapshot->updateWithData(data, canvas->getBitmap(), _controllerId);

    _snapshotStore->put(snapshot, [=]() {
        //_snapshotStore->flush();
        SnapshotsAdapter* adapter = (SnapshotsAdapter*)(_listView->getAdapter());
        adapter->reload();
    });
    //updateEmptyUx();
    return (uint32_t)cb;
}


