/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */
#ifndef PREVIEW_ITEM_H
#define PREVIEW_ITEM_H


#include <Entry.h>
#include <ObjectList.h>
#include <StringItem.h>
#include <String.h>


class Group;
class RenameAction;


enum Error {
	NO_ERROR,
	INVALID_NAME,
	DUPLICATE,
	EXISTS,
	MISSING_REPLACEMENT
};


class PreviewItem : public BStringItem {
public:
								PreviewItem(const entry_ref& ref);

			void				SetRenameAction(const RenameAction& action);
			status_t			Rename();

			const entry_ref&	Ref() const
									{ return fRef; }
			const BString&		Target() const
									{ return fTarget; }
			void				SetTarget(const BString& target);
			bool				HasTarget() const
									{ return !fTarget.IsEmpty(); }
			void				UpdateProcessed(int32 from, int32 to,
									const BString& replace);

			bool				IsValid() const
									{ return fError == NO_ERROR; }
			::Error				Error() const
									{ return fError; }
			void				SetError(::Error error)
									{ fError = error; }
			const char*			ErrorMessage() const;

	virtual	void				DrawItem(BView* owner, BRect frame,
									bool complete);

	static	int					Compare(const void* _a, const void* _b);

private:
			void				_DrawGroupedText(BView* owner, BRect frame,
									float x, const BString& text,
									const BObjectList<Group>& groups,
									int32 first);
			void				_DrawGroup(BView* owner, uint32 groupIndex,
									BRect frame, float start, float end);
			bool				_IsValidName(const char* name) const;

private:
			entry_ref			fRef;
			BString				fTarget;
			BObjectList<Group>	fGroups;
			BObjectList<Group>	fRenameGroups;
			::Error				fError;
};


#endif	// PREVIEW_ITEM_H
