/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */


#include "CaseRenameAction.h"

#include "rename.h"

#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <UnicodeChar.h>

#include <stdio.h>
#include <unicode/utf8.h>


static const uint32 kMsgSetMode = 'stmd';


//	#pragma mark - CaseRenameAction


CaseRenameAction::CaseRenameAction()
	:
	fMode(TITLE_CASE)
{
}


CaseRenameAction::~CaseRenameAction()
{
}


BString
CaseRenameAction::Rename(BObjectList<Group>& sourceGroups,
	BObjectList<Group>& targetGroups, const char* string) const
{
	BString output;
	char* buffer = output.LockBuffer(B_PATH_NAME_LENGTH + 8);

	int32 extensionStart = strlen(string);
	while (extensionStart-- > 0) {
		if (string[extensionStart] == '.')
			break;
	}
	if (extensionStart >= 0)
		extensionStart++;

	case_mode mode = fMode;

	int32 outLength = 0;
	int32 fromIndex = 0;
	bool inWord = false;
	int32 groupIndex = 0;
	int32 groupStart = -1;

	// Rename
	while (string[0] != '\0') {
		if (fromIndex == extensionStart) {
			switch (fExtensionMode) {
				case LOWER_CASE_EXTENSION:
					mode = LOWER_CASE;
					break;
				case UPPER_CASE_EXTENSION:
					mode = UPPER_CASE;
					break;
				case LEAVE_EXTENSION_UNCHANGED:
					mode = (case_mode)-1;
					break;
				default:
					break;
			}
		}

		const char* next = string;
		uint32 c = BUnicodeChar::FromUTF8(&next);
		uint32 origChar = c;

		if (mode == TITLE_CASE) {
			if (BUnicodeChar::IsAlpha(c)) {
				if (!inWord) {
					c = BUnicodeChar::ToTitle(c);
					inWord = true;
				} else if (fForce) {
					c = BUnicodeChar::ToLower(c);
				}
			} else
				inWord = false;
		} else if (mode == UPPER_CASE && BUnicodeChar::IsAlpha(c)) {
			c = BUnicodeChar::ToUpper(c);
		} else if (mode == LOWER_CASE && BUnicodeChar::IsAlpha(c)) {
			c = BUnicodeChar::ToLower(c);
		}

		if (outLength >= B_PATH_NAME_LENGTH)
			break;

		if (c != origChar && groupStart < 0)
			groupStart = fromIndex;
		else if (c == origChar && groupStart >= 0) {
			sourceGroups.AddItem(new Group(groupIndex, groupStart, fromIndex));
			targetGroups.AddItem(new Group(groupIndex++, groupStart,
				fromIndex));
			groupStart = -1;
		}

		int charLength = 0;
		U8_APPEND_UNSAFE(buffer, charLength, c);
		outLength += charLength;
		buffer += charLength;

		fromIndex += next - string;
		string = next;
	}
	buffer[0] = '\0';

	if (groupStart >= 0) {
		sourceGroups.AddItem(new Group(groupIndex, groupStart, fromIndex));
		targetGroups.AddItem(new Group(groupIndex, groupStart, fromIndex));
	}

	output.UnlockBuffer(B_PATH_NAME_LENGTH);
	return output;
}


//	#pragma mark - CaseRenameView


CaseRenameView::CaseRenameView()
	:
	RenameView("case")
{
	BMenu* menu = new BPopUpMenu("Mode");
	BMenuItem* item = new BMenuItem("Title case", new BMessage(kMsgSetMode));
	item->SetMarked(true);
	menu->AddItem(item);
	menu->AddItem(new BMenuItem("Upper case", new BMessage(kMsgSetMode)));
	menu->AddItem(new BMenuItem("Lower case", new BMessage(kMsgSetMode)));

	fModeField = new BMenuField("mode", "Mode", menu);

	menu = new BPopUpMenu("Extension mode");
	item = new BMenuItem("Lower case", new BMessage(kMsgUpdatePreview));
	item->SetMarked(true);
	menu->AddItem(item);
	menu->AddItem(new BMenuItem("Upper case", new BMessage(kMsgUpdatePreview)));
	menu->AddItem(new BMenuItem("Leave unchanged",
		new BMessage(kMsgUpdatePreview)));
	menu->AddItem(new BMenuItem("Use case mode",
		new BMessage(kMsgUpdatePreview)));

	fExtensionModeField = new BMenuField("extension mode", "Extension mode",
		menu);

	fForceCheckBox = new BCheckBox("force", "Force on all characters",
		new BMessage(kMsgUpdatePreview));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING, 0, 0, 0)
		.AddGrid(0.f)
			.Add(fModeField->CreateLabelLayoutItem(), 0, 0)
			.Add(fModeField->CreateMenuBarLayoutItem(), 1, 0)
			.Add(fExtensionModeField->CreateLabelLayoutItem(), 0, 1)
			.Add(fExtensionModeField->CreateMenuBarLayoutItem(), 1, 1)
		.End()
		.Add(fForceCheckBox);
}


CaseRenameView::~CaseRenameView()
{
}


RenameAction*
CaseRenameView::Action() const
{
	CaseRenameAction* action = new CaseRenameAction();
	action->SetMode((case_mode)fModeField->Menu()->FindMarkedIndex());
	action->SetExtensionMode(
		(extension_mode)fExtensionModeField->Menu()->FindMarkedIndex());
	action->SetForce(fForceCheckBox->Value() == B_CONTROL_ON);

	return action;
}


void
CaseRenameView::AttachedToWindow()
{
	fModeField->Menu()->SetTargetForItems(this);
}


void
CaseRenameView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSetMode:
			fForceCheckBox->SetEnabled(
				fModeField->Menu()->FindMarkedIndex() == 0);
			Window()->PostMessage(kMsgUpdatePreview);
			break;

		default:
			RenameView::MessageReceived(message);
			break;
	}
}
