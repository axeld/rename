/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */
#ifndef PREVIEW_LIST_H
#define PREVIEW_LIST_H


#include <ListView.h>

#include <map>


class PreviewItem;
typedef std::map<entry_ref, PreviewItem*> PreviewItemMap;


class PreviewList : public BListView {
public:
								PreviewList(const char* name);

			void				AddRef(const entry_ref& ref);
			bool				HasRef(const entry_ref& ref) const;
			void				UpdateRef(const entry_ref& oldRef,
									PreviewItem* item);
			PreviewItem*		ItemForRef(const entry_ref& ref);
			void				RemoveUnchanged();

	virtual	void				Draw(BRect updateRect);
	virtual	void				MessageReceived(BMessage* message);
	virtual void				KeyDown(const char* bytes, int32 numBytes);

private:
			PreviewItemMap		fPreviewItemMap;
};


#endif	// PREVIEW_LIST_H
