/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */
#ifndef REGULAR_EXPRESSION_RENAME_ACTION_H
#define REGULAR_EXPRESSION_RENAME_ACTION_H


#include "RenameAction.h"
#include "RenameView.h"

#include <regex.h>


class BCheckBox;
class BTextControl;


class RegularExpressionRenameAction : public RenameAction {
public:
								RegularExpressionRenameAction();
	virtual						~RegularExpressionRenameAction();

			bool				SetPattern(const char* pattern,
									bool caseInsensitive);
			void				SetReplace(const char* replace);

			void				SetIgnoreExtension(bool ignore)
									{ fIgnoreExtension = ignore; }

	virtual BString				Rename(BObjectList<Group>& sourceGroups,
									BObjectList<Group>& targetGroups,
									const char* string) const;

private:
			regex_t				fCompiledPattern;
			bool				fValidPattern;
			bool				fIgnoreExtension;
			BString				fReplace;
};


class RegularExpressionView : public RenameView {
public:
								RegularExpressionView();
	virtual						~RegularExpressionView();

	virtual	RenameAction*		Action() const;
	virtual void				RequestFocus() const;

private:
			BTextControl*		fPatternControl;
			BTextControl*		fReplaceControl;
			BCheckBox*			fIgnoreExtensionCheckBox;
			BCheckBox*			fCaseInsensitiveCheckBox;
};


#endif	// REGULAR_EXPRESSION_RENAME_ACTION_H
