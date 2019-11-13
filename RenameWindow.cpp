/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */


#include "RenameWindow.h"

#include "CaseRenameAction.h"
#include "PreviewItem.h"
#include "PreviewList.h"
#include "RefModel.h"
#include "rename.h"
#include "RegularExpressionRenameAction.h"
#include "RenameProcessor.h"
#include "RenameSettings.h"
#include "SearchReplaceRenameAction.h"
#include "WindowsRenameAction.h"

#include <Button.h>
#include <Catalog.h>
#include <CardView.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <TextControl.h>

#include <set>

#include <stdio.h>
#include <string.h>


typedef std::set<BString> StringSet;


static const uint32 kMsgSetAction = 'stAc';
static const uint32 kMsgRename = 'okRe';
static const uint32 kMsgRemoveUnchanged = 'rmUn';
static const uint32 kMsgRecursive = 'recu';
static const uint32 kMsgFilterChanged = 'fich';
static const uint32 kMsgResetRemoved = 'rsrm';


#define B_TRANSLATION_CONTEXT "Rename"


RenameWindow::RenameWindow(RenameSettings& settings)
	:
	BWindow(BRect(0, 0, 99, 99), B_TRANSLATE("Rename files"), B_DOCUMENT_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS),
	fSettings(settings)
{
	status_t status = fSettings.Load();
	if (status != B_OK) {
		fprintf(stderr, "%s: could not load settings: %s\n", kProgramName,
			strerror(status));
	}
	BRect frame = settings.WindowFrame();
	MoveTo(frame.LeftTop());
	ResizeTo(frame.Width(), frame.Height());

	fOkButton = new BButton("ok", "Rename", new BMessage(kMsgRename));
	fOkButton->SetEnabled(false);

	fResetRemovedButton = new BButton("reset", "Reset removed",
		new BMessage(kMsgResetRemoved));
	fResetRemovedButton->SetEnabled(false);
	fRemoveUnchangedButton = new BButton("remove", "Remove unchanged",
		new BMessage(kMsgRemoveUnchanged));
	fRemoveUnchangedButton->SetEnabled(false);

	fRecursiveCheckBox = new BCheckBox("recursive",
		"Enter directories recursively", new BMessage(kMsgRecursive));
	fRecursiveCheckBox->SetValue(
		fSettings.Recursive() ? B_CONTROL_ON : B_CONTROL_OFF);

	// File type menu field

	fTypeMenu = new BPopUpMenu("Types");

	fTypeMenu->AddItem(new BMenuItem("files & folders",
		new BMessage(kMsgFilterChanged)));
	fTypeMenu->AddItem(new BMenuItem("files only",
		new BMessage(kMsgFilterChanged)));
	fTypeMenu->AddItem(new BMenuItem("folders only",
		new BMessage(kMsgFilterChanged)));

	fTypeMenuField = new BMenuField("type", "Rename", fTypeMenu);

	// Replacement menu field

	fReplacementMenu = new BPopUpMenu("Mode");

	fReplacementMenu->AddItem(new BMenuItem("may all be missing",
		new BMessage(kMsgUpdatePreview)));
	fReplacementMenu->AddItem(new BMenuItem("at least one must be set",
		new BMessage(kMsgUpdatePreview)));
	fReplacementMenu->AddItem(new BMenuItem("must all be set",
		new BMessage(kMsgUpdatePreview)));

	fReplacementMenuField = new BMenuField("replacement", "Replacements",
		fReplacementMenu);

	// Rename methods

	RenameView* searchReplaceView = fView = new SearchReplaceView();
	RenameView* regularExpressionView = new RegularExpressionView();
	RenameView* windowsRenameView = new WindowsRenameView();
	RenameView* caseRenameView = new CaseRenameView();

	fActionMenu = new BPopUpMenu("Actions");

	BMenuItem* item = new BMenuItem("Search & Replace",
		new BMessage(kMsgSetAction));
	item->SetMarked(true);
	fActionMenu->AddItem(item);

	item = new BMenuItem("Regular expression", new BMessage(kMsgSetAction));
	fActionMenu->AddItem(item);

	item = new BMenuItem("Windows compliant", new BMessage(kMsgSetAction));
	fActionMenu->AddItem(item);

	item = new BMenuItem("Change case", new BMessage(kMsgSetAction));
	fActionMenu->AddItem(item);

	fActionMenuField = new BMenuField("action", "Rename method", fActionMenu);

	fCardView = new BCardView("action");
	fCardView->AddChild(searchReplaceView);
	fCardView->AddChild(regularExpressionView);
	fCardView->AddChild(windowsRenameView);
	fCardView->AddChild(caseRenameView);
	fCardView->CardLayout()->SetVisibleItem(0L);

	fPreviewList = new PreviewList("preview");

	// Filter options

	fFilterControl = new BTextControl("Filter", NULL, NULL);
	fFilterControl->SetModificationMessage(new BMessage(kMsgFilterChanged));

	fRegExpFilterCheckBox = new BCheckBox("regexp",
		"Regular expression", new BMessage(kMsgFilterChanged));
	fReverseFilterCheckBox = new BCheckBox("reverse",
		"Remove matching", new BMessage(kMsgFilterChanged));

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 0.f)
			.SetInsets(B_USE_WINDOW_SPACING)
			.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING, 1.f)
				.SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET))
				.AddGrid(0.f)
					.AddMenuField(fActionMenuField, 0, 0)
				.End()
				.Add(fCardView)
			.End()
			.Add(new BSeparatorView(B_VERTICAL), 0.f)
			.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING, 0.f)
				.Add(fRecursiveCheckBox)
				.AddGrid(0.f)
					.AddMenuField(fTypeMenuField, 0, 0)
					.AddMenuField(fReplacementMenuField, 0, 1)
				.End()
				.AddGlue()
			.End()
		.End()
		.AddGroup(B_HORIZONTAL)
			.SetInsets(B_USE_DEFAULT_SPACING, 0,
				B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING, 10.f)
				.AddGlue()
				.AddGrid(0.f)
					.AddTextControl(fFilterControl, 0, 0)
				.End()
				.AddGlue()
			.End()
			.Add(fRegExpFilterCheckBox)
			.Add(fReverseFilterCheckBox)
			.AddGlue()
			.Add(fResetRemovedButton)
			.Add(fRemoveUnchangedButton)
			.AddGlue()
			.Add(fOkButton)
		.End()
		.AddGroup(B_VERTICAL)
			.SetInsets(-2)
			.Add(new BScrollView("scroller", fPreviewList, 0, true, true))
		.End();

	fView->RequestFocus();

	fRefModel = new RefModel(fPreviewList);
	if (fRefModel->InitCheck() != B_OK)
		debugger("No model!");

	fRefModel->SetRecursive(fSettings.Recursive());

	RenameProcessor* processor = new RenameProcessor();
	processor->Run();

	fRenameProcessor = processor;
}


RenameWindow::~RenameWindow()
{
	fRenameProcessor.SendMessage(B_QUIT_REQUESTED);

	fSettings.SetWindowFrame(Frame());
	fSettings.SetRecursive(fRecursiveCheckBox->Value() == B_CONTROL_ON);
	fSettings.Save();

	delete fRefModel;
}


void
RenameWindow::AddRef(const entry_ref& ref)
{
	if (fPreviewList->HasRef(ref)) {
		// Ignore duplicate entry
		return;
	}

	fPreviewList->AddRef(ref);
	fRefModel->AddRef(ref);
}


int32
RenameWindow::AddRefs(const BMessage& message)
{
	entry_ref ref;
	int32 index;
	for (index = 0; message.FindRef("refs", index, &ref) == B_OK; index++) {
		AddRef(ref);
	}
	return index;
}


void
RenameWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSetAction:
		{
			fCardView->CardLayout()->SetVisibleItem(
				fActionMenuField->Menu()->FindMarkedIndex());
			fView = dynamic_cast<RenameView*>(
				fCardView->CardLayout()->VisibleItem()->View());

			// supposed to fall through
		}

		case kMsgUpdatePreview:
			_UpdatePreviewItems();
			break;

		case kMsgRecursive:
			fRefModel->SetRecursive(
				fRecursiveCheckBox->Value() == B_CONTROL_ON);
			break;

		case kMsgFilterChanged:
			_UpdateFilter();
			break;

		case kMsgResetRemoved:
			fRefModel->ResetRemoved();
			fResetRemovedButton->SetEnabled(false);
			break;

		case kMsgRefsRemoved:
		{
			entry_ref ref;
			for (int32 index = 0; message->FindRef("refs", index, &ref) == B_OK;
					index++) {
				fRefModel->RemoveRef(ref);
			}
			fResetRemovedButton->SetEnabled(true);
			break;
		}

		case kMsgProcessed:
		{
			_HandleProcessed(message);
			break;
		}

		case kMsgRemoveUnchanged:
		{
			fPreviewList->RemoveUnchanged();
			fRemoveUnchangedButton->SetEnabled(false);
			break;
		}

		case kMsgRename:
			_RenameFiles();
			break;

		case B_SIMPLE_DATA:
			AddRefs(*message);
			_UpdatePreviewItems();
			break;
	}
}


void
RenameWindow::_HandleProcessed(BMessage* message)
{
	ReplacementMode replacementMode
		= (ReplacementMode)fReplacementMenu->FindMarkedIndex();
	int32 processedCount = 0;
	int32 errorCount = 0;
	BMessage update;
	for (int32 index = 0; message->FindMessage("update", index, &update)
			== B_OK; index++) {
		entry_ref ref;
		if (update.FindRef("ref", &ref) != B_OK)
			continue;

		PreviewItem* item = fPreviewList->ItemForRef(ref);
		if (item == NULL)
			continue;

		bool hasEmpty = update.GetBool("has empty");
		bool allEmpty = update.GetBool("all empty");
		if (replacementMode == NEED_ANY_REPLACEMENTS && allEmpty
			|| replacementMode == NEED_ALL_REPLACEMENTS && hasEmpty) {
			item->SetError(MISSING_REPLACEMENT);
			errorCount++;
		} else if (update.GetBool("exists")) {
			item->SetError(EXISTS);
			errorCount++;
		}

		BString replace;
		int32 from;
		int32 to;
		for (int32 replaceIndex = 0; update.FindString("replace",
				replaceIndex, &replace) == B_OK; replaceIndex++) {
			if (update.FindInt32("from", replaceIndex, &from) != B_OK
				|| update.FindInt32("to", replaceIndex, &to) != B_OK)
				break;

			item->UpdateProcessed(from, to, replace);
		}

		processedCount++;

	}

	if (errorCount != 0 || processedCount != 0)
		fPreviewList->Invalidate();
	if (errorCount == 0)
		fOkButton->SetEnabled(true);

	// Check if there are any unchanged entries
	bool unchanged = false;
	for (int32 index = 0; index < fPreviewList->CountItems(); index++) {
		PreviewItem* item = static_cast<PreviewItem*>(
			fPreviewList->ItemAt(index));
		if (!item->HasTarget()) {
			unchanged = true;
			break;
		}
	}
	fRemoveUnchangedButton->SetEnabled(unchanged);
}


void
RenameWindow::_UpdatePreviewItems()
{
	RenameAction* action = fView->Action();

	BMessage check(kMsgProcessAndCheckRename);
	int errorCount = 0;
	int validCount = 0;
	PreviewItemMap map;
	for (int32 i = 0; i < fPreviewList->CountItems(); i++) {
		PreviewItem* item = static_cast<PreviewItem*>(
			fPreviewList->ItemAt(i));
		item->SetRenameAction(*action);

		if (item->IsValid()) {
			if (item->HasTarget()) {
				entry_ref targetRef = item->Ref();
				targetRef.set_name(item->Target());
				PreviewItemMap::iterator found = map.find(targetRef);
				if (found != map.end()) {
					found->second->SetError(DUPLICATE);
					item->SetError(DUPLICATE);
					errorCount++;
				} else {
					map.insert(std::make_pair(targetRef, item));
					validCount++;
				}
			}
		} else if (item->HasTarget()) {
			errorCount++;
		}

		if (errorCount == 0 && item->HasTarget()) {
			check.AddRef("source", &item->Ref());
			check.AddString("target", item->Target());
		}
	}

	fPreviewList->Invalidate();

	fOkButton->SetEnabled(false);

	if (errorCount == 0 && validCount > 0) {
		// Check paths on disk
		fRenameProcessor.SendMessage(&check, this);
	} else if (fPreviewList->CountItems() > 0)
		fRemoveUnchangedButton->SetEnabled(true);

	delete action;
}


void
RenameWindow::_UpdateFilter()
{
	RefFilter* textFilter = NULL;
	const char* text = fFilterControl->Text();
	if (text[0] != '\0') {
		if (fRegExpFilterCheckBox->Value() == B_CONTROL_ON)
			textFilter = new RegularExpressionFilter(text);
		else
			textFilter = new TextFilter(text);

		if (fReverseFilterCheckBox->Value() == B_CONTROL_ON)
			textFilter = new ReverseFilter(textFilter);
	}

	RefFilter* typeFilter = NULL;
	int32 type = fTypeMenu->FindMarkedIndex();
	if (type == FILES_ONLY)
		typeFilter = new FilesOnlyFilter();
	else if (type == FOLDERS_ONLY)
		typeFilter = new FoldersOnlyFilter();

	AndFilter* filter = new AndFilter();
	filter->AddFilter(typeFilter);
	filter->AddFilter(textFilter);

	if (filter->IsEmpty()) {
		delete filter;
		filter = NULL;
	}

	fRefModel->SetFilter(filter);
}


void
RenameWindow::_RenameFiles()
{
	for (int32 index = 0; index < fPreviewList->CountItems(); index++) {
		PreviewItem* item = static_cast<PreviewItem*>(
			fPreviewList->ItemAt(index));
		if (!item->HasTarget())
			continue;

		entry_ref originalRef = item->Ref();

		status_t status = item->Rename();
		if (status != B_OK) {
			// TODO: Proper error reporting!
			fprintf(stderr, "Renaming failed: %s\n", strerror(status));
		}

		fRefModel->UpdateRef(originalRef, item->Ref());
		fPreviewList->UpdateRef(originalRef, item);
	}

	fPreviewList->SortItems(&PreviewItem::Compare);
	_UpdatePreviewItems();
}
