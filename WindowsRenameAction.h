/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */
#ifndef WINDOWS_RENAME_ACTION_H
#define WINDOWS_RENAME_ACTION_H


#include "RenameAction.h"
#include "RenameView.h"

class BTextControl;


class WindowsRenameAction : public RenameAction {
public:
								WindowsRenameAction();
	virtual						~WindowsRenameAction();

			void				SetReplaceChar(char replace);

	virtual	bool				AddGroups(BObjectList<Group>& groups,
									const char* string) const;
	virtual BString				Rename(BObjectList<Group>& groups,
									const char* string) const;

private:
	static	bool				_IsInvalidCharacter(char c);

private:
			char				fReplaceChar;
};


class WindowsRenameView : public RenameView {
public:
								WindowsRenameView();
	virtual						~WindowsRenameView();

	virtual	RenameAction*		Action() const;
	virtual void				RequestFocus() const;

private:
			BTextControl*		fReplaceControl;
};


#endif	// WINDOWS_RENAME_ACTION_H
