/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */


#include "PreviewList.h"

#include "PreviewItem.h"
#include "RefModel.h"
#include "RenameView.h"
#include "RenameWindow.h"


PreviewList::PreviewList(const char* name)
	:
	BListView(name, B_MULTIPLE_SELECTION_LIST,
		B_FULL_UPDATE_ON_RESIZE | B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE)
{
}


void
PreviewList::AddRef(const entry_ref& ref)
{
	PreviewItem* item = new PreviewItem(ref);
	fPreviewItemMap.insert(std::make_pair(ref, item));

	AddItem(item);
	SortItems(&PreviewItem::Compare);
}


bool
PreviewList::HasRef(const entry_ref& ref) const
{
	PreviewItemMap::const_iterator found = fPreviewItemMap.find(ref);
	return found != fPreviewItemMap.end();
}


void
PreviewList::UpdateRef(const entry_ref& oldRef, PreviewItem* item)
{
	fPreviewItemMap.erase(oldRef);
	fPreviewItemMap.insert(std::make_pair(item->Ref(), item));
}


PreviewItem*
PreviewList::ItemForRef(const entry_ref& ref)
{
	PreviewItemMap::iterator found = fPreviewItemMap.find(ref);
	if (found != fPreviewItemMap.end())
		return found->second;

	return NULL;
}


void
PreviewList::RemoveUnchanged()
{
	BMessage update(kMsgRefsRemoved);

	for (int32 index = 0; index < CountItems(); index++) {
		PreviewItem* item = static_cast<PreviewItem*>(ItemAt(index));
		if (!item->HasTarget()) {
			update.AddRef("refs", &item->Ref());

			fPreviewItemMap.erase(item->Ref());
			delete RemoveItem(index);
			index--;
		}
	}

	if (!update.IsEmpty())
		Looper()->PostMessage(&update);
}


void
PreviewList::Draw(BRect updateRect)
{
	BListView::Draw(updateRect);

	BRect rect = Bounds();
	rect.right = rect.left + rect.Width() / 2.f;
	if (rect.right <= updateRect.right) {
		StrokeLine(BPoint(rect.right, rect.top),
			BPoint(rect.right, rect.bottom));
	}
}


void
PreviewList::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgUpdateRefs:
		{
			BList items;
			entry_ref ref;
			for (int32 index = 0; message->FindRef("add", index, &ref) == B_OK;
					index++) {
				if (!HasRef(ref)) {
					PreviewItem* item = new PreviewItem(ref);
					fPreviewItemMap.insert(std::make_pair(ref, item));
					items.AddItem(item);
				}
			}
			AddList(&items);

			// TODO: Removing is super slow!
			for (int32 index = 0; message->FindRef("remove", index, &ref)
					== B_OK; index++) {
				PreviewItem* item = ItemForRef(ref);
				if (item != NULL) {
					fPreviewItemMap.erase(item->Ref());
					RemoveItem(item);
					delete item;
				}
			}
			SortItems(&PreviewItem::Compare);
			if (!items.IsEmpty())
				Looper()->PostMessage(kMsgUpdatePreview);
			break;
		}
		default:
			BListView::MessageReceived(message);
	}
}


void
PreviewList::KeyDown(const char* bytes, int32 numBytes)
{
	if (bytes[0] == B_DELETE) {
		// Remove selected entries
		while (true) {
			int32 selectedIndex = CurrentSelection(0);
			if (selectedIndex < 0)
				break;

			PreviewItem* item = static_cast<PreviewItem*>(
				ItemAt(selectedIndex));

			BMessage update(kMsgRefsRemoved);
			update.AddRef("refs", &item->Ref());

			fPreviewItemMap.erase(item->Ref());
			delete RemoveItem(selectedIndex);

			Looper()->PostMessage(&update);
		}
	} else
		BListView::KeyDown(bytes, numBytes);
}
