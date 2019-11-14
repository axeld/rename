/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */


#include "WindowsRenameAction.h"

#include "rename.h"

#include <LayoutBuilder.h>
#include <TextControl.h>


//	#pragma mark - WindowsRenameAction


WindowsRenameAction::WindowsRenameAction()
	:
	fReplaceString("_")
{
}


WindowsRenameAction::~WindowsRenameAction()
{
}


void
WindowsRenameAction::SetReplaceString(const char* replace)
{
	if (replace == NULL || replace[0] == '\0') {
		fReplaceString = "";
		return;
	}

	// Filter out invalid characters from the replace string
	BString name(replace);
	int32 length = name.CountChars();
	for (int32 index = 0; index < length; index++) {
		int32 charSize;
		const char* pos = name.CharAt(index, &charSize);
		if (charSize == 1 && _IsInvalidCharacter(pos[0])) {
			name.RemoveChars(index--, 1);
			length--;
		}
	}

	if (name.IsEmpty())
		fReplaceString = "_";
	else
		fReplaceString = name;
}


BString
WindowsRenameAction::Rename(BObjectList<Group>& sourceGroups,
	BObjectList<Group>& targetGroups, const char* string) const
{
	BString stringBuffer = string;
	int32 length = stringBuffer.CountChars();
	int groupIndex = 1;
	int begin = -1;
	int index = 0;
	int byteIndex = 0;
	int sourceGroupIndex = 1;
	int sourceBegin = -1;
	int sourceByteIndex = 0;

	for (; index < length; index++) {
		int32 charSize;
		const char* pos = stringBuffer.CharAt(index, &charSize);
		if (charSize == 1 && _IsInvalidCharacter(pos[0])) {
			stringBuffer.Remove(byteIndex, charSize);
			stringBuffer.Insert(fReplaceString, byteIndex);

			if (begin == -1 && !fReplaceString.IsEmpty())
				begin = byteIndex;
			if (sourceBegin == -1)
				sourceBegin = sourceByteIndex;

			byteIndex += fReplaceString.Length() - charSize;
			int32 diff = fReplaceString.CountChars() - 1;
			index += diff;
			length += diff;
		} else {
			if (begin >= 0) {
				targetGroups.AddItem(new Group(groupIndex++, begin, byteIndex));
				begin = -1;
			}
			if (sourceBegin >= 0) {
				sourceGroups.AddItem(new Group(sourceGroupIndex++, sourceBegin,
					sourceByteIndex));
				sourceBegin = -1;
			}
		}
		byteIndex += charSize;
		sourceByteIndex += charSize;
	}

	if (begin >= 0)
		targetGroups.AddItem(new Group(groupIndex++, begin, byteIndex));
	if (sourceBegin >= 0) {
		sourceGroups.AddItem(new Group(sourceGroupIndex++, sourceBegin,
			sourceByteIndex));
	}

	// Mark trailing dots or spaces
	BString text(string);
	int end = sourceByteIndex;
	int sourceIndex = text.CountChars();
	while (--sourceIndex > 0) {
		int32 charSize;
		const char* pos = text.CharAt(sourceIndex, &charSize);
		if (charSize != 1 || (pos[0] != ' ' && pos[0] != '.'))
			break;

		sourceByteIndex -= charSize;
		sourceBegin = sourceByteIndex;
	}

	if (sourceBegin >= 0)
		sourceGroups.AddItem(new Group(sourceGroupIndex++, sourceBegin, end));

	// Cut off trailing dots or spaces
	while (--index > 0) {
		int32 charSize;
		char* pos = const_cast<char*>(stringBuffer.CharAt(index, &charSize));
		if (charSize != 1 || (pos[0] != ' ' && pos[0] != '.'))
			break;

		stringBuffer.TruncateChars(index);
	}

	return stringBuffer;
}


/*static*/ bool
WindowsRenameAction::_IsInvalidCharacter(char c)
{
	if (c <= 0x1f)
		return true;

	switch (c) {
		case '"':
		case '*':
		case ':':
		case '<':
		case '>':
		case '?':
		case '\\':
		case '/':
		case '|':
			return true;
	}

	return false;
}


//	#pragma mark - WindowsRenameView


WindowsRenameView::WindowsRenameView()
	:
	RenameView("method:windows")
{
	fReplaceControl = new BTextControl("Replace character", NULL, NULL);
	fReplaceControl->SetModificationMessage(new BMessage(kMsgUpdatePreview));
	fReplaceControl->SetText("_");

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING, 0, 0, 0)
		.AddGrid(0.f)
			.Add(fReplaceControl->CreateLabelLayoutItem(), 0, 1)
			.Add(fReplaceControl->CreateTextViewLayoutItem(), 1, 1)
		.End()
		.AddGlue();
}


WindowsRenameView::~WindowsRenameView()
{
}


RenameAction*
WindowsRenameView::Action() const
{
	WindowsRenameAction* action = new WindowsRenameAction();
	action->SetReplaceString(fReplaceControl->Text());

	return action;
}


void
WindowsRenameView::RequestFocus() const
{
	fReplaceControl->MakeFocus(true);
}


void
WindowsRenameView::SetSettings(const BMessage& settings)
{
	fReplaceControl->SetText(settings.GetString("replace"));
}


void
WindowsRenameView::GetSettings(BMessage& settings)
{
	settings.SetString("replace", fReplaceControl->Text());
}
