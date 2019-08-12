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
	int groupIndex = 1;
	int begin = -1;
	int index = 0;

	for (; string[index] != '\0'; index++) {
		if (_IsInvalidCharacter(string[index])) {
			if (begin < 0)
				begin = index;
		} else if (begin >= 0) {
			groupList.AddItem(new Group(groupIndex++, begin, index));
			begin = -1;
		}
	}

	if (begin >= 0) {
		groupList.AddItem(new Group(groupIndex++, begin, index));
		begin = -1;
	}

	// Mark trailing dots or spaces
	int end = index;
	while (index > 0) {
		char c = string[index - 1];
		if (c != ' ' && c != '.')
			break;

		begin = --index;
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
	char* buffer = stringBuffer.LockBuffer(B_FILE_NAME_LENGTH);
	int groupIndex = 1;
	int begin = -1;
	int index = 0;

	for (; buffer[index] != '\0'; index++) {
		if (_IsInvalidCharacter(buffer[index])) {
			if (fReplaceChar != '\0') {
				if (begin < 0)
					begin = index;
				buffer[index] = fReplaceChar;
			} else {
				memmove(buffer + index, buffer + index + 1,
					strlen(buffer + index));
				index--;
			}
		} else if (begin >= 0) {
			groupList.AddItem(new Group(groupIndex++, begin, index));
			begin = -1;
		}
	}

	// Cut off trailing dots or spaces
	while (index > 0) {
		char c = buffer[index - 1];
		if (c != ' ' && c != '.')
			break;

		buffer[--index] = '\0';
	}

	if (begin >= 0)
		groupList.AddItem(new Group(groupIndex++, begin, index));

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
