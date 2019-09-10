/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */


#include "RegularExpressionRenameAction.h"

#include "rename.h"

#include <LayoutBuilder.h>
#include <TextControl.h>


#define MAX_GROUPS 10
#define MAX_MATCHES 20


//	#pragma mark - RegularExpressionRenameAction


RegularExpressionRenameAction::RegularExpressionRenameAction()
	:
	fValidPattern(false)
{
}


RegularExpressionRenameAction::~RegularExpressionRenameAction()
{
	if (fValidPattern)
		regfree(&fCompiledPattern);
}


bool
RegularExpressionRenameAction::SetPattern(const char* pattern)
{
	// TODO: add options for flags
	int result = regcomp(&fCompiledPattern, pattern, REG_EXTENDED);
	// TODO: show/report error!
	fValidPattern = result == 0;

	return fValidPattern;
}

void
RegularExpressionRenameAction::SetReplace(const char* replace)
{
	fReplace = replace;
}


bool
RegularExpressionRenameAction::AddGroups(BObjectList<Group>& groupList,
	const char* string) const
{
	if (!fValidPattern)
		return false;

	regmatch_t groups[MAX_GROUPS];

	if (regexec(&fCompiledPattern, string, MAX_GROUPS, groups, 0))
		return false;

	for (int groupIndex = 1; groupIndex < MAX_GROUPS; groupIndex++) {
		if (groups[groupIndex].rm_so == -1)
			break;

		groupList.AddItem(new Group(groupIndex, groups[groupIndex].rm_so,
			groups[groupIndex].rm_eo));
	}
	if (groupList.IsEmpty())
		groupList.AddItem(new Group(0, groups[0].rm_so, groups[0].rm_eo));

	return true;
}


BString
RegularExpressionRenameAction::Rename(BObjectList<Group>& groupList,
	const char* string) const
{
	BString stringBuffer;
	if (!fValidPattern)
		return string;

	regmatch_t groups[MAX_GROUPS];
	if (regexec(&fCompiledPattern, string, MAX_GROUPS, groups, 0))
		return string;

	stringBuffer = fReplace;

	if (groups[1].rm_so == -1) {
		// There is just a single group -- just replace the match
		stringBuffer.Prepend(string, groups[0].rm_so);
	}

	char* buffer = stringBuffer.LockBuffer(B_PATH_NAME_LENGTH);
	char* target = buffer;

	for (; target[0] != '\0'; target++) {
		if (target[0] == '\\' && target[1] >= '0' && target[1] <= '9') {
			// References a group
			int32 groupIndex = target[1] - '0';
			int startOffset = groups[groupIndex].rm_so;
			int endOffset = groups[groupIndex].rm_eo;
			int length = endOffset - startOffset;
			if (startOffset < 0 || strlen(buffer) + length - 1
					> B_FILE_NAME_LENGTH)
				break;

			memmove(target + length, target + 2, strlen(target) - 1);
			memmove(target, string + startOffset, length);

			startOffset = target - buffer;
			groupList.AddItem(new Group(groupIndex, startOffset,
				startOffset + length));

			target += length - 1;
		}
	}

	stringBuffer.UnlockBuffer();

	if (groups[1].rm_so == -1) {
		groupList.AddItem(new Group(0, groups[0].rm_so,
			stringBuffer.Length()));
		stringBuffer.Append(string + groups[0].rm_eo);
	}

	return stringBuffer;
}


//	#pragma mark - RegularExpressionView


RegularExpressionView::RegularExpressionView()
	:
	RenameView("regular expression")
{
	fPatternControl = new BTextControl("Pattern", NULL, NULL);
	fPatternControl->SetModificationMessage(new BMessage(kMsgUpdatePreview));

	fRenameControl = new BTextControl("Replace with", NULL, NULL);
	fRenameControl->SetModificationMessage(new BMessage(kMsgUpdatePreview));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING, 0, 0, 0)
		.AddGrid(0.f)
			.Add(fPatternControl->CreateLabelLayoutItem(), 0, 0)
			.Add(fPatternControl->CreateTextViewLayoutItem(), 1, 0)
			.Add(fRenameControl->CreateLabelLayoutItem(), 0, 1)
			.Add(fRenameControl->CreateTextViewLayoutItem(), 1, 1)
		.End()
		.AddGlue();

}


RegularExpressionView::~RegularExpressionView()
{
}


RenameAction*
RegularExpressionView::Action() const
{
	RegularExpressionRenameAction* action = new RegularExpressionRenameAction;
	action->SetPattern(fPatternControl->Text());
	action->SetReplace(fRenameControl->Text());
	return action;
}


void
RegularExpressionView::RequestFocus() const
{
	fPatternControl->MakeFocus(true);
}
