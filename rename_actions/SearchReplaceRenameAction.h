/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */
#ifndef SEARCH_REPLACE_RENAME_ACTION_H
#define SEARCH_REPLACE_RENAME_ACTION_H


#include "RenameAction.h"
#include "RenameView.h"


class BCheckBox;
class BTextControl;


class SearchReplaceRenameAction : public RenameAction {
public:
								SearchReplaceRenameAction();
	virtual						~SearchReplaceRenameAction();

			void				SetPattern(const char* pattern);
			void				SetReplace(const char* replace);

			void				SetCaseInsensitive(bool insensitive)
									{ fCaseInsensitive = insensitive; }
			void				SetIgnoreExtension(bool ignore)
									{ fIgnoreExtension = ignore; }

	virtual BString				Rename(BObjectList<Group>& sourceGroups,
									BObjectList<Group>& targetGroups,
									const char* string) const;

private:
			int					_NextPatternIndex(int index,
									bool& wasAny) const;
			bool				_AddGroup(BObjectList<Group>& groups,
									int32 groupIndex, int32 start,
									int32 end) const;

private:
			BString				fPattern;
			BString				fReplace;
			bool				fCaseInsensitive;
			bool				fIgnoreExtension;
};


class SearchReplaceView : public RenameView {
public:
								SearchReplaceView();
	virtual						~SearchReplaceView();

	virtual	RenameAction*		Action() const;
	virtual void				RequestFocus() const;

protected:
								SearchReplaceView(const char* name);

			void				Init();

protected:
			BTextControl*		fPatternControl;
			BTextControl*		fReplaceControl;
			BCheckBox*			fIgnoreExtensionCheckBox;
			BCheckBox*			fCaseInsensitiveCheckBox;
};


#endif	// SEARCH_REPLACE_RENAME_ACTION_H
