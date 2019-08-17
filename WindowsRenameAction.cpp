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
	fReplaceChar('_')
{
}


WindowsRenameAction::~WindowsRenameAction()
{
}


void
WindowsRenameAction::SetReplaceChar(char replace)
{
	if (replace == '\0' || !_IsInvalidCharacter(replace))
		fReplaceChar = replace;
	else
		fReplaceChar = '_';
}


bool
WindowsRenameAction::AddGroups(BObjectList<Group>& groupList,
	const char* string) const
{
	BString text(string);
	int32 length = text.CountChars();
	int groupIndex = 1;
	int begin = -1;
	int index = 0;
	int byteIndex = 0;

	for (; index < length; index++) {
		int32 charSize;
		const char* pos = text.CharAt(index, &charSize);

		if (charSize == 1 && _IsInvalidCharacter(pos[0])) {
			if (begin < 0)
				begin = byteIndex;
		} else if (begin >= 0) {
			groupList.AddItem(new Group(groupIndex++, begin, byteIndex));
			begin = -1;
		}
		byteIndex += charSize;
	}

	if (begin >= 0) {
		groupList.AddItem(new Group(groupIndex++, begin, byteIndex));
		begin = -1;
	}

	// Mark trailing dots or spaces
	int end = byteIndex;
	while (--index > 0) {
		int32 charSize;
		const char* pos = text.CharAt(index, &charSize);
		if (charSize != 1 || pos[0] != ' ' && pos[0] != '.')
			break;

		byteIndex -= charSize;
		begin = byteIndex;
	}

	if (begin >= 0)
		groupList.AddItem(new Group(groupIndex++, begin, end));

	return groupIndex > 1;
}


BString
WindowsRenameAction::Rename(BObjectList<Group>& groupList,
	const char* string) const
{
	BString stringBuffer = string;
	int32 length = stringBuffer.CountChars();
	char* buffer = stringBuffer.LockBuffer(B_FILE_NAME_LENGTH);
	int groupIndex = 1;
	int begin = -1;
	int index = 0;
	int byteIndex = 0;

	for (; index < length; index++) {
		int32 charSize;
		char* pos = const_cast<char*>(stringBuffer.CharAt(index, &charSize));
		if (charSize == 1 && _IsInvalidCharacter(pos[0])) {
			if (fReplaceChar != '\0') {
				if (begin < 0)
					begin = byteIndex;
				buffer[byteIndex] = fReplaceChar;
			} else {
				memmove(buffer + byteIndex, buffer + byteIndex + 1,
					strlen(buffer + byteIndex));
				byteIndex--;
				index--;
				length--;
			}
		} else if (begin >= 0) {
			groupList.AddItem(new Group(groupIndex++, begin, byteIndex));
			begin = -1;
		}
		byteIndex += charSize;
	}

	if (begin >= 0)
		groupList.AddItem(new Group(groupIndex++, begin, byteIndex));

	// Cut off trailing dots or spaces
	while (--index > 0) {
		int32 charSize;
		char* pos = const_cast<char*>(stringBuffer.CharAt(index, &charSize));
		if (charSize != 1 || pos[0] != ' ' && pos[0] != '.')
			break;

		pos[0] = '\0';
	}

	stringBuffer.UnlockBuffer();
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
	RenameView("windows")
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
	char replaceChar = '\0';
	if (fReplaceControl->TextLength() > 0)
		replaceChar = fReplaceControl->Text()[0];

	action->SetReplaceChar(replaceChar);
	return action;
}


void
WindowsRenameView::RequestFocus() const
{
	fReplaceControl->MakeFocus(true);
}
