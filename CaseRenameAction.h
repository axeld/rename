/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */
#ifndef CASE_RENAME_ACTION_H
#define CASE_RENAME_ACTION_H


#include "RenameAction.h"
#include "RenameView.h"

class BCheckBox;
class BMenuField;


enum case_mode {
	TITLE_CASE,
	UPPER_CASE,
	LOWER_CASE
};

enum extension_mode {
	LOWER_CASE_EXTENSION,
	UPPER_CASE_EXTENSION,
	LEAVE_EXTENSION_UNCHANGED,
	USE_CASE_MODE
};


class CaseRenameAction : public RenameAction {
public:
								CaseRenameAction();
	virtual						~CaseRenameAction();

			void				SetMode(case_mode mode)
									{ fMode = mode; }
			void				SetExtensionMode(extension_mode mode)
									{ fExtensionMode = mode; }
			void				SetForce(bool force)
									{ fForce = force; }

	virtual BString				Rename(BObjectList<Group>& sourceGroups,
									BObjectList<Group>& targetGroups,
									const char* string) const;

private:
			case_mode			fMode;
			extension_mode		fExtensionMode;
			bool				fForce;
};


class CaseRenameView : public RenameView {
public:
								CaseRenameView();
	virtual						~CaseRenameView();

	virtual	RenameAction*		Action() const;

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

private:
	static	BMessage*			_CreateMessage(case_mode mode);
	static	BMessage*			_CreateExtensionMessage(extension_mode mode);

private:
			BMenuField*			fModeField;
			BMenuField*			fExtensionModeField;
			BCheckBox*			fForceCheckBox;
};


#endif	// CASE_RENAME_ACTION_H
