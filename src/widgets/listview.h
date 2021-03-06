//
// Copyright © 2018 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//

typedef int64_t LISTINDEX;

#define LISTINDEX_NONE ((int64_t)-1)
#define LISTINDEX_SECTION(i) ((i)>>32)
#define LISTINDEX_ITEM(i) ((int32_t)((i)&0xFFFFFFFF))
#define LISTINDEX_MAKE(s,i) ((((int64_t)s)<<32)|(i))

/**
 * @ingroup widgets
 * @brief Raw interface to be implemented by anything that provides items to a ListView. 99% of the time
 * you will be better off using the far easier `SimpleListAdapter` instead of this.
 */

class IListAdapter {
public:
    virtual void setListView(class ListView* listView) =0;
    virtual int getSectionCount() = 0;
    virtual float getHeaderHeight(int section) = 0;
	virtual int getItemCount(int section) = 0;
	virtual float getItemHeight(LISTINDEX index) = 0;
	virtual View* createItemView(LISTINDEX index) = 0;
    virtual View* createHeaderView(int section) = 0;
    virtual void bindItemView(View* itemview, LISTINDEX index) = 0;
    virtual bool canDeleteItem(LISTINDEX index) = 0;
    virtual void deleteItem(LISTINDEX index) = 0;
};

/**
 * @ingroup widgets
 * @brief A vertical list of items, conceptually similar to the Android widget of the same name.
 */

class ListView : public View {
public:
    
    class ItemView : public View {
    public:
        ListView* _listView;
        LISTINDEX _listIndex;
        View* _contentView;
        ImageView* _deleteButton;
        Label* _deleteConfirmButton;
        View* _reorderHandle;
        
        ItemView(ListView* listView, LISTINDEX listIndex, View* contentView);
        void updateDeleteButton(bool animate);
        void showDeleteConfirmButton(bool show);
        
        void attachToWindow(Window* window) override;
        void updateIntrinsicSize(SIZE constrainingSize) override;
    };
    
    bool _editMode;

    vector<pair<LISTINDEX,ItemView*>> _itemViews;
    vector<pair<int,View*>> _headerViews;
	float _dividerHeight;
    COLOR _dividerColor;
    LISTINDEX _selectedIndex;
    std::function<void(View*, LISTINDEX index)> _onItemTapDelegate;
    std::function<void(View*, LISTINDEX index)> _onItemLongPressDelegate;
    typedef struct {
        int top;
        int headerHeight;
        int totalHeight;
    } SECTION_METRICS;
    vector<SECTION_METRICS> _sectionMetrics;
    View* _headerView;
    ViewController* _editingViewController;
    LISTINDEX _deleteConfirmIndex;
    
	// Public API
    ListView();
	virtual void removeAllItemViews();
	virtual void onItemTap(View* itemView, LISTINDEX index);
    virtual void reload();
    virtual void setHeaderView(View* headerView);
    virtual void startEditing(ViewController* editingViewController);
    virtual void deleteRow(LISTINDEX index);

    // Properties
    virtual IListAdapter* getAdapter() const;
    virtual void setAdapter(IListAdapter* adapter);

	// Overrides
    bool applySingleStyle(const string &name, const style& value) override;
	void layout(RECT constraint) override;
	void updateIntrinsicSize(SIZE constrainingSize) override;
    void setContentOffset(POINT contentOffset, bool animated=false) override;
	bool handleInputEvent(INPUTEVENT* event) override;
    void removeSubview(View* subview) override;
	
	
protected:
	virtual ItemView* indexToView(LISTINDEX index);
	virtual void setSelectedIndex(LISTINDEX index);
    virtual void updateVisibleItems();
    virtual pair<LISTINDEX,View*> createItemView(LISTINDEX index, bool atFront, float itemHeight, float top);
    LISTINDEX offsetIndex(LISTINDEX index, int offset);
    virtual void showDeleteConfirmButton(ItemView* itemView);
    
    IListAdapter* _adapter;

};



