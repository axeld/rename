/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */


#include "SearchReplaceRenameAction.h"

#include "rename.h"

#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <TextControl.h>
#include <UnicodeChar.h>


//	#pragma mark - SearchReplaceRenameAction


SearchReplaceRenameAction::SearchReplaceRenameAction()
	:
	fCaseInsensitive(false),
	fIgnoreExtension(false)
{
}


SearchReplaceRenameAction::~SearchReplaceRenameAction()
{
}


void
SearchReplaceRenameAction::SetPattern(const char* pattern)
{
	fPattern = pattern;
}

void
SearchReplaceRenameAction::SetReplace(const char* replace)
{
	fReplace = replace;
}


BString
SearchReplaceRenameAction::Rename(BObjectList<Group>& sourceGroups,
	BObjectList<Group>& targetGroups, const char* string) const
{
	if (fPattern.IsEmpty())
		return string;

	BString output;

	int32 suffixIndex = -1;
	if (fIgnoreExtension)
		suffixIndex = SuffixIndex(string);

	int stringLength = suffixIndex >= 0 ? suffixIndex : strlen(string);
	bool wasAny = false;
	int seenAnyIndex = -1;
	int firstMatchIndex = -1;
	int copyIndex = 0;
	int patternIndex = _NextPatternIndex(-1, wasAny);
	int groupIndex = 0;
	if (wasAny)
		seenAnyIndex = patternIndex;

	for (int valueIndex = 0; valueIndex < stringLength; valueIndex++) {
		if (patternIndex >= fPattern.Length()) {
			// Pattern matched, copy pending
			output.Append(string + copyIndex, firstMatchIndex - copyIndex);
			copyIndex = valueIndex;

			bool newGroup = _AddGroup(sourceGroups, groupIndex, firstMatchIndex,
				wasAny ? stringLength : valueIndex);

			if (!fReplace.IsEmpty()) {
				_AddGroup(targetGroups, groupIndex, output.Length(),
					output.Length() + fReplace.Length());
				output.Append(fReplace);
			}
			if (newGroup)
				groupIndex++;

			if (wasAny) {
				copyIndex = stringLength;
				break;
			}

			// Start again
			patternIndex = _NextPatternIndex(-1, wasAny);
			firstMatchIndex = -1;
		}

		char patternChar = fPattern.ByteAt(patternIndex);
		char valueChar = string[valueIndex];
		if (patternChar!='?' && ((!fCaseInsensitive && valueChar!=patternChar)
			|| (fCaseInsensitive && BUnicodeChar::ToLower(valueChar)
					!=BUnicodeChar::ToLower(patternChar)))) {
			if (wasAny)
				continue;
			if (seenAnyIndex != -1) {
				patternIndex = seenAnyIndex;
				continue;
			}

			// Try again...
			patternIndex = _NextPatternIndex(-1, wasAny);
			firstMatchIndex = -1;
			continue;
		}

		if (firstMatchIndex == -1)
			firstMatchIndex = valueIndex;

		patternIndex = _NextPatternIndex(patternIndex, wasAny);
		if (wasAny)
			seenAnyIndex = patternIndex;
	}

	if (patternIndex >= fPattern.Length()) {
		// Pattern matched, copy pending
		output.Append(string + copyIndex, firstMatchIndex - copyIndex);

		_AddGroup(sourceGroups, groupIndex, firstMatchIndex, stringLength);
		if (!fReplace.IsEmpty()) {
			_AddGroup(targetGroups, groupIndex, output.Length(),
				output.Length() + fReplace.Length());

			output.Append(fReplace);
		}
	} else if (copyIndex < stringLength)
		output.Append(string + copyIndex, stringLength - copyIndex);

	if (suffixIndex >= 0)
		output.Append(string + suffixIndex);

	return output;
}


int
SearchReplaceRenameAction::_NextPatternIndex(int index, bool& wasAny) const
{
	wasAny = false;

	if (++index >= fPattern.Length())
		return index;

	while (index < fPattern.Length() && fPattern.ByteAt(index) == '*') {
		index++;
		wasAny = true;
	}

	return index;
}


bool
SearchReplaceRenameAction::_AddGroup(BObjectList<Group>& groups,
	int32 groupIndex, int32 start, int32 end) const
{
	bool newGroup = groups.IsEmpty();
	Group* last = newGroup ? NULL : groups.ItemAt(groups.CountItems() - 1);
	if (last != NULL)
		newGroup = last->end != start;

	if (newGroup) {
		groups.AddItem(new Group(groupIndex, start, end));
		return true;
	}

	// Bump last
	last->end = end;
	return false;
}


//	#pragma mark - SearchReplaceView


SearchReplaceView::SearchReplaceView()
	:
	RenameView("search & replace")
{
	Init();
}


SearchReplaceView::~SearchReplaceView()
{
}


RenameAction*
SearchReplaceView::Action() const
{
	SearchReplaceRenameAction* action = new SearchReplaceRenameAction;
	action->SetPattern(fPatternControl->Text());
	action->SetReplace(fReplaceControl->Text());
	action->SetCaseInsensitive(
		fCaseInsensitiveCheckBox->Value() == B_CONTROL_ON);
	action->SetIgnoreExtension(
		fIgnoreExtensionCheckBox->Value() == B_CONTROL_ON);
	return action;
}


void
SearchReplaceView::RequestFocus() const
{
	fPatternControl->MakeFocus(true);
}


SearchReplaceView::SearchReplaceView(const char* name)
	:
	RenameView(name)
{
	Init();
}


void
SearchReplaceView::Init()
{
	fPatternControl = new BTextControl("Pattern", NULL, NULL);
	fPatternControl->SetModificationMessage(new BMessage(kMsgUpdatePreview));

	fReplaceControl = new BTextControl("Replace with", NULL, NULL);
	fReplaceControl->SetModificationMessage(new BMessage(kMsgUpdatePreview));

	fIgnoreExtensionCheckBox = new BCheckBox("extension", "Ignore extension",
		new BMessage(kMsgUpdatePreview));
	fCaseInsensitiveCheckBox = new BCheckBox("case", "Case insensitive",
		new BMessage(kMsgUpdatePreview));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING, 0, 0, 0)
		.AddGrid(0.f)
			.Add(fPatternControl->CreateLabelLayoutItem(), 0, 0)
			.Add(fPatternControl->CreateTextViewLayoutItem(), 1, 0)
			.Add(fReplaceControl->CreateLabelLayoutItem(), 0, 1)
			.Add(fReplaceControl->CreateTextViewLayoutItem(), 1, 1)
		.End()
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fIgnoreExtensionCheckBox)
			.Add(fCaseInsensitiveCheckBox)
		.End()
		.AddGlue();
}
