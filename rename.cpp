/* rename - Batch rename Tracker add-on
 *
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */


#include "rename.h"

#include "CaseRenameAction.h"
#include "RefModel.h"
#include "RegularExpressionRenameAction.h"
#include "RenameSettings.h"
#include "SearchReplaceRenameAction.h"
#include "WindowsRenameAction.h"

#include <Application.h>
#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <CardView.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <Directory.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <ObjectList.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <String.h>
#include <TextControl.h>

#include <Path.h>
#include <File.h>
#include <NodeInfo.h>
#include <Node.h>
#include <Directory.h>
#include <FindDirectory.h>

#include <fs_attr.h>

#include <map>
#include <new>
#include <set>

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


typedef std::set<BString> StringSet;


static const uint32 kMsgSetAction = 'stAc';
static const uint32 kMsgRename = 'okRe';
static const uint32 kMsgProcessAndCheckRename = 'pchR';
static const uint32 kMsgProcessed = 'prcd';
static const uint32 kMsgRemoveUnchanged = 'rmUn';
static const uint32 kMsgRefsRemoved = 'rfrm';
static const uint32 kMsgRecursive = 'recu';
static const uint32 kMsgFilterChanged = 'fich';
static const uint32 kMsgResetRemoved = 'rsrm';


static rgb_color kGroupColor[] = {
	make_color(100, 200, 100),
	make_color(200, 100, 200),
	make_color(100, 200, 200),
	make_color(150, 200, 150),
	make_color(100, 250, 100),
};
static const uint32 kGroupColorCount = 5;

#define B_TRANSLATION_CONTEXT "Rename"


enum Error {
	NO_ERROR,
	INVALID_NAME,
	DUPLICATE,
	EXISTS
};


#define FILES_AND_FOLDERS	0
#define	FILES_ONLY			1
#define FOLDERS_ONLY		2


class PreviewItem : public BStringItem {
public:
								PreviewItem(const entry_ref& ref);

			void				SetRenameAction(const RenameAction& action);
			status_t			Rename();

			const entry_ref&	Ref() const
									{ return fRef; }
			const BString&		Target() const
									{ return fTarget; }
			void				SetTarget(const BString& target);
			bool				HasTarget() const
									{ return !fTarget.IsEmpty(); }
			void				UpdateProcessed(int32 from, int32 to,
									const BString& replace);

			bool				IsValid() const
									{ return fError == NO_ERROR; }
			::Error				Error() const
									{ return fError; }
			void				SetError(::Error error)
									{ fError = error; }
			const char*			ErrorMessage() const;

	virtual	void				DrawItem(BView* owner, BRect frame,
									bool complete);

	static	int					Compare(const void* _a, const void* _b);

private:
			void				_DrawGroupedText(BView* owner, BRect frame,
									float x, const BString& text,
									const BObjectList<Group>& groups,
									int32 first);
			void				_DrawGroup(BView* owner, uint32 groupIndex,
									BRect frame, float start, float end);
			bool				_IsValidName(const char* name) const;

private:
			entry_ref			fRef;
			BString				fTarget;
			BObjectList<Group>	fGroups;
			BObjectList<Group>	fRenameGroups;
			::Error				fError;
};

typedef std::map<entry_ref, PreviewItem*> PreviewItemMap;


class PreviewList : public BListView {
public:
								PreviewList(const char* name);

			void				AddRef(const entry_ref& ref);
			bool				HasRef(const entry_ref& ref) const;
			void				UpdateRef(const entry_ref& oldRef,
									PreviewItem* item);
			PreviewItem*		ItemForRef(const entry_ref& ref);
			void				RemoveUnchanged();

	virtual	void				Draw(BRect updateRect);
	virtual	void				MessageReceived(BMessage* message);
	virtual void				KeyDown(const char* bytes, int32 numBytes);

private:
			PreviewItemMap		fPreviewItemMap;
};


class RenameWindow : public BWindow {
public:
								RenameWindow(RenameSettings& settings);
	virtual						~RenameWindow();

			void				AddRef(const entry_ref& ref);
			int32				AddRefs(const BMessage& message);

	virtual	void				MessageReceived(BMessage* message);

private:
			void				_HandleProcessed(BMessage* message);
			void				_UpdatePreviewItems();
			void				_UpdateFilter();
			void				_RenameFiles();

private:
			RenameSettings&		fSettings;
			BMenuField*			fActionMenuField;
			BPopUpMenu*			fActionMenu;
			BCardView*			fCardView;
			RenameView*			fView;
			BButton*			fOkButton;
			BButton*			fResetRemovedButton;
			BButton*			fRemoveUnchangedButton;
			BCheckBox*			fRecursiveCheckBox;
			BTextControl*		fFilterControl;
			BCheckBox*			fRegExpFilterCheckBox;
			BCheckBox*			fReverseFilterCheckBox;
			BMenuField*			fTypeMenuField;
			BPopUpMenu*			fTypeMenu;
			PreviewList*		fPreviewList;
			RefModel*			fRefModel;
			BMessenger			fRenameProcessor;
};


class RenameProcessor : public BLooper {
public:
								RenameProcessor();

	virtual	void				MessageReceived(BMessage* message);

private:
			void				_ProcessRef(BMessage& update,
									const entry_ref& ref,
									const BString& target);
			bool				_CheckRef(const entry_ref& ref,
									const BString& target);
			BString				_ReadAttribute(const entry_ref& ref,
									const char* name);
			int					_Extract(const char* buffer, int length,
									char open, BString& expression);
};


extern const char* __progname;
static const char* kProgramName = __progname;


static struct option const kOptions[] = {
	{"ui", no_argument, 0, 'u'},
	{"verbose", no_argument, 0, 'v'},
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0}
};


bool gRecursive = true;
bool gVerbose = false;


status_t
handleFile(BEntry& entry)
{
	char name[B_FILE_NAME_LENGTH];
	entry.GetName(name);

	// Open File (on error, return showing error)

	BFile file(&entry, B_READ_ONLY);
	if (file.InitCheck() < B_OK) {
		fprintf(stderr, "could not open '%s'.\n", name);
		return B_IO_ERROR;
	}
	return B_OK;
}


bool
handleDirectory(BEntry &entry, int32 level)
{
	BPath path(&entry);

	if (!entry.IsDirectory()) {
		fprintf(stderr, "\"%s\" is not a directory\n", path.Path());
		return false;
	}

	BDirectory directory(&entry);
	BEntry entryIterator;

	directory.Rewind();
	while (directory.GetNextEntry(&entryIterator, false) == B_OK) {
		if (entryIterator.IsDirectory()) {
			if (gRecursive)
				handleDirectory(entryIterator, level + 1);

			continue;
		}

		if (handleFile(entryIterator) < B_OK)
			continue;
	}

	return true;
}


//	#pragma mark - RenameAction


RenameAction::~RenameAction()
{
}


int32
RenameAction::SuffixIndex(const char* string) const
{
	int32 index = strlen(string);
	while (index-- > 0) {
		if (string[index] == '.')
			return index;
	}
	return -1;
}


//	#pragma mark - PreviewList


PreviewList::PreviewList(const char* name)
	:
	BListView(name, B_MULTIPLE_SELECTION_LIST,
		B_FULL_UPDATE_ON_RESIZE | B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE)
{
}


void
PreviewList::AddRef(const entry_ref& ref)
{
	PreviewItem* item = new PreviewItem(ref);
	fPreviewItemMap.insert(std::make_pair(ref, item));

	AddItem(item);
	SortItems(&PreviewItem::Compare);
}


bool
PreviewList::HasRef(const entry_ref& ref) const
{
	PreviewItemMap::const_iterator found = fPreviewItemMap.find(ref);
	return found != fPreviewItemMap.end();
}


void
PreviewList::UpdateRef(const entry_ref& oldRef, PreviewItem* item)
{
	fPreviewItemMap.erase(oldRef);
	fPreviewItemMap.insert(std::make_pair(item->Ref(), item));
}


PreviewItem*
PreviewList::ItemForRef(const entry_ref& ref)
{
	PreviewItemMap::iterator found = fPreviewItemMap.find(ref);
	if (found != fPreviewItemMap.end())
		return found->second;

	return NULL;
}


void
PreviewList::RemoveUnchanged()
{
	BMessage update(kMsgRefsRemoved);

	for (int32 index = 0; index < CountItems(); index++) {
		PreviewItem* item = static_cast<PreviewItem*>(ItemAt(index));
		if (!item->HasTarget()) {
			update.AddRef("refs", &item->Ref());

			fPreviewItemMap.erase(item->Ref());
			delete RemoveItem(index);
			index--;
		}
	}

	if (!update.IsEmpty())
		Looper()->PostMessage(&update);
}


void
PreviewList::Draw(BRect updateRect)
{
	BListView::Draw(updateRect);

	BRect rect = Bounds();
	rect.right = rect.left + rect.Width() / 2.f;
	if (rect.right <= updateRect.right) {
		StrokeLine(BPoint(rect.right, rect.top),
			BPoint(rect.right, rect.bottom));
	}
}


void
PreviewList::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgUpdateRefs:
		{
			BList items;
			entry_ref ref;
			for (int32 index = 0; message->FindRef("add", index, &ref) == B_OK;
					index++) {
				if (!HasRef(ref)) {
					PreviewItem* item = new PreviewItem(ref);
					fPreviewItemMap.insert(std::make_pair(ref, item));
					items.AddItem(item);
				}
			}
			AddList(&items);

			// TODO: Removing is super slow!
			for (int32 index = 0; message->FindRef("remove", index, &ref)
					== B_OK; index++) {
				PreviewItem* item = ItemForRef(ref);
				if (item != NULL) {
					fPreviewItemMap.erase(item->Ref());
					RemoveItem(item);
					delete item;
				}
			}
			SortItems(&PreviewItem::Compare);
			if (!items.IsEmpty())
				Looper()->PostMessage(kMsgUpdatePreview);
			break;
		}
		default:
			BListView::MessageReceived(message);
	}
}


void
PreviewList::KeyDown(const char* bytes, int32 numBytes)
{
	if (bytes[0] == B_DELETE) {
		// Remove selected entries
		while (true) {
			int32 selectedIndex = CurrentSelection(0);
			if (selectedIndex < 0)
				break;

			PreviewItem* item = static_cast<PreviewItem*>(
				ItemAt(selectedIndex));

			BMessage update(kMsgRefsRemoved);
			update.AddRef("refs", &item->Ref());

			fPreviewItemMap.erase(item->Ref());
			delete RemoveItem(selectedIndex);

			Looper()->PostMessage(&update);
		}
	} else
		BListView::KeyDown(bytes, numBytes);
}


//	#pragma mark - PreviewItem


PreviewItem::PreviewItem(const entry_ref& ref)
	:
	BStringItem(ref.name),
	fRef(ref),
	fGroups(10, true),
	fError(NO_ERROR)
{
}


void
PreviewItem::SetTarget(const BString& target)
{
	fTarget = target;
}


void
PreviewItem::UpdateProcessed(int32 from, int32 to, const BString& replace)
{
	// Update text
	fTarget.Remove(from, to - from);
	fTarget.Insert(replace, from);

	// Update groups
	int32 diff = replace.Length() - to + from;

	for (int32 index = 0; index < fRenameGroups.CountItems(); index++) {
		Group& group = *fRenameGroups.ItemAt(index);

		if (group.start == from)
			group.end = from + replace.Length();
		else if (group.end > to)
			group.end += diff;

		if (group.start > from)
			group.start += diff;
	}
}


void
PreviewItem::SetRenameAction(const RenameAction& action)
{
	fGroups.MakeEmpty();
	fRenameGroups.MakeEmpty();
	fError = NO_ERROR;

	BString newName = action.Rename(fGroups, fRenameGroups, fRef.name);
	if (newName != fRef.name)
		fTarget = newName;
	else
		fTarget = "";

	if (HasTarget() && !_IsValidName(fTarget))
		fError = INVALID_NAME;
}


status_t
PreviewItem::Rename()
{
	if (!HasTarget())
		return B_ENTRY_NOT_FOUND;

	BEntry entry(&fRef);
	status_t status = entry.InitCheck();
	if (status != B_OK)
		return status;

	// Make sure all sub directories exist
	int32 index = fTarget.FindLast('/');
	if (index >= 0) {
		BPath path;
		status = path.SetTo(&fRef);
		if (status == B_OK)
			status = path.GetParent(&path);
		if (status == B_OK)
			status = path.Append(BString().SetTo(fTarget, index));
		if (status == B_OK)
			status = create_directory(path.Path(), 0755);
		if (status != B_OK)
			return status;
	}

	status = entry.Rename(fTarget.String());
	if (status != B_OK)
		return status;

	entry.GetRef(&fRef);
	fTarget = "";

	return B_OK;
}


const char*
PreviewItem::ErrorMessage() const
{
	switch (fError) {
		default:
		case NO_ERROR:
			return NULL;
		case INVALID_NAME:
			return "Invalid name!";
		case DUPLICATE:
			return "Duplicated name!";
		case EXISTS:
			return "Already exists!";
	}
}


void
PreviewItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	rgb_color lowColor = owner->LowColor();
	rgb_color highColor = owner->HighColor();
	BRect bounds = owner->Bounds();
	float half = bounds.Width() / 2.f;
	float width = half + be_control_look->DefaultLabelSpacing();

	if (IsSelected() || complete) {
		rgb_color color;
		if (IsSelected())
			color = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
		else
			color = owner->ViewColor();

		owner->SetLowColor(color);
		owner->FillRect(frame, B_SOLID_LOW);
	} else
		owner->SetLowColor(owner->ViewColor());

	float x = frame.left + be_control_look->DefaultLabelSpacing();
	BRect rect = bounds;
	rect.right = half - 1;

	owner->PushState();
	owner->ClipToRect(rect);

	if (fGroups.IsEmpty()) {
		// Text does not match
		owner->SetLowColor(200, 100, 100);
		owner->FillRect(BRect(x, frame.top, x + owner->StringWidth(fRef.name),
			frame.bottom), B_SOLID_LOW);

		owner->MovePenTo(x, frame.top + BaselineOffset());
		owner->DrawString(fRef.name);
	} else {
		// Text does match, fill groups in different colors
		int startIndex = 0;//fGroups.CountItems() == 1 ? 0 : 1;
		_DrawGroupedText(owner, frame, x, fRef.name, fGroups, startIndex);
	}

	owner->PopState();

	if (fError != NO_ERROR && HasTarget())
		owner->SetHighColor(200, 50, 50);

	if (fRenameGroups.IsEmpty()) {
		owner->MovePenTo(x + width, frame.top + BaselineOffset());
		owner->DrawString(fTarget);
	} else {
		_DrawGroupedText(owner, frame, x + width, fTarget.String(),
			fRenameGroups, 0);
	}

	if (fError != NO_ERROR && HasTarget()) {
		const char* errorText = ErrorMessage();
		float width = owner->StringWidth(errorText);

		owner->MovePenBy(10, 0);
		BPoint location = owner->PenLocation();
		owner->SetLowColor(255, 0, 0);
		owner->FillRect(BRect(location.x - 2, frame.top,
			location.x + width + 2, frame.bottom), B_SOLID_LOW);
		owner->SetHighColor(255, 255, 255);
		owner->DrawString(errorText);
	}

	owner->SetLowColor(lowColor);
	owner->SetHighColor(highColor);
}


/*static*/ int
PreviewItem::Compare(const void* _a, const void* _b)
{
	const PreviewItem* a = *static_cast<const PreviewItem* const*>(_a);
	const PreviewItem* b = *static_cast<const PreviewItem* const*>(_b);
	const entry_ref& refA = a->Ref();
	const entry_ref& refB = b->Ref();

	return strcasecmp(refA.name, refB.name);
}


void
PreviewItem::_DrawGroupedText(BView* owner, BRect frame, float x,
	const BString& text, const BObjectList<Group>& groups, int32 first)
{
	uint32 count = text.CountChars();
	BPoint sizes[count];
	be_plain_font->GetEscapements(text.String(), count, NULL, sizes);

	float fontSize = be_plain_font->Size();
	int32 byteIndex = 0;
	bool inGroup = false;
	uint32 groupIndex = first;
	const Group* group = groups.ItemAt(groupIndex);
	float startOffset = 0;
	float endOffset = 0;

	for (uint32 i = 0; i < count; i++) {
		int32 characterBytes = text.CountBytes(i, 1);
		float offset = endOffset;
		endOffset += sizes[i].x * fontSize;

		if (byteIndex == group->start) {
			startOffset = offset;
			inGroup = true;
		}

		byteIndex += characterBytes;

		if (byteIndex == group->end) {
			// End group
			if (inGroup) {
				_DrawGroup(owner, group->index, frame, x + startOffset,
					x + endOffset);
				inGroup = false;
			}

			// Select next group, if any
			if (++groupIndex == (uint32)groups.CountItems())
				break;
			group = groups.ItemAt(groupIndex);
		}
	}
	if (inGroup) {
		// End last group
		_DrawGroup(owner, group->index, frame, x + startOffset,
			x + endOffset);
	}

	// TODO: Draw parts with the correct low color
	owner->MovePenTo(x, frame.top + BaselineOffset());
	owner->DrawString(text.String());
}


void
PreviewItem::_DrawGroup(BView* owner, uint32 groupIndex, BRect frame,
	float start, float end)
{
	owner->SetLowColor(kGroupColor[groupIndex % kGroupColorCount]);
	owner->FillRect(BRect(start, frame.top, end, frame.bottom), B_SOLID_LOW);
}


bool
PreviewItem::_IsValidName(const char* name) const
{
	// TODO: move could be optional
//	return strchr(name, '/') == NULL;
	return true;
}


//	#pragma mark - RenameView


RenameView::RenameView(const char* name)
	:
	BView(name, 0)
{
}


RenameView::~RenameView()
{
}


void
RenameView::RequestFocus() const
{
}


//	#pragma mark - RenameWindow


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

	fTypeMenu = new BPopUpMenu("Types");

	item = new BMenuItem("Files & folders", new BMessage(kMsgFilterChanged));
	item->SetMarked(true);
	fTypeMenu->AddItem(item);

	item = new BMenuItem("Files only", new BMessage(kMsgFilterChanged));
	fTypeMenu->AddItem(item);

	item = new BMenuItem("Folders only", new BMessage(kMsgFilterChanged));
	fTypeMenu->AddItem(item);

	fTypeMenuField = new BMenuField("type", "", fTypeMenu);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING, 0.f)
			.SetInsets(B_USE_WINDOW_SPACING)
			.AddGrid(0.f)
				.AddMenuField(fActionMenuField, 0, 0)
			.End()
			.Add(fCardView)
		.End()
		.AddGroup(B_HORIZONTAL)
			.SetInsets(B_USE_DEFAULT_SPACING, 0,
				B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.Add(fRecursiveCheckBox)
			.AddGlue()
			.AddGroup(B_VERTICAL)
				.AddGlue()
				.AddGrid(5, 1)
					.AddMenuField(fTypeMenuField, 0, 0)
					.AddGlue(2, 0)
					.AddTextControl(fFilterControl, 3, 0)
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
	int32 processedCount = 0;
	int32 errorCount = 0;
	entry_ref ref;
	for (int32 index = 0; message->FindRef("ref", index, &ref)
			== B_OK; index++) {
		BString replace;
		int32 from;
		int32 to;
		if (message->FindString("replace", index, &replace) != B_OK
			|| message->FindInt32("from", index, &from) != B_OK
			|| message->FindInt32("to", index, &to) != B_OK)
			continue;

		PreviewItem* item = fPreviewList->ItemForRef(ref);
		if (item != NULL) {
			item->UpdateProcessed(from, to, replace);
			processedCount++;
		}
	}

	for (int32 index = 0; message->FindRef("exists", index, &ref)
			== B_OK; index++) {
		PreviewItem* item = fPreviewList->ItemForRef(ref);
		if (item != NULL) {
			item->SetError(EXISTS);
			errorCount++;
		}
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


//	#pragma mark - RenameProcessor


RenameProcessor::RenameProcessor()
	:
	BLooper("Rename processor")
{
}


void
RenameProcessor::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgProcessAndCheckRename:
		{
			BMessage reply(kMsgProcessed);

			entry_ref ref;
			int32 index;
			for (index = 0; message->FindRef("source", index, &ref) == B_OK;
					index++) {
				BString target;
				if (message->FindString("target", index, &target) == B_OK) {
					_ProcessRef(reply, ref, target);
					if (!_CheckRef(ref, target))
						reply.AddRef("exists", &ref);
				}
			}

			message->SendReply(&reply);
			break;
		}

		default:
			BLooper::MessageReceived(message);
			break;
	}
}


//!	Evaluate expressions in the target name.
void
RenameProcessor::_ProcessRef(BMessage& update, const entry_ref& ref,
	const BString& target)
{
	int length = target.Length();
	const char* buffer = target.String();
	int diff = 0;

	for (int index = 0; index < length - 3; index++) {
		if (buffer[index] == '$') {
			char open = buffer[index + 1];
			BString expression;
			int expressionLength = -1;
			if (open == '(' || open == '[') {
				expressionLength = _Extract(buffer + index + 2,
					length - index - 2, open, expression);
			}

			BString result;
			bool changed = false;

			if (expressionLength > 0 && open == '(') {
				// bash script
				// TODO!
				result = "bash";
				changed = true;
			} else if (expressionLength > 0 && open == '[') {
				// Attribute replacement
				result = _ReadAttribute(ref, expression.String());
				changed = true;
			}

			if (changed) {
				update.AddRef("ref", &ref);
				update.AddInt32("from", index + diff);
				update.AddInt32("to", index + expressionLength + 3 + diff);
				update.AddString("replace", result);

				diff += result.Length() - expressionLength - 3;
				index += expressionLength + 2;
			}
		}
	}
}


bool
RenameProcessor::_CheckRef(const entry_ref& ref, const BString& target)
{
	entry_ref targetRef = ref;
	int32 index = target.FindFirst('/');
	if (index >= 0) {
		BPath path(&ref);
		if (path.InitCheck() != B_OK
			|| path.GetParent(&path) != B_OK
			|| path.Append(target) != B_OK
			|| get_ref_for_path(path.Path(), &targetRef) != B_OK)
			return true;
	} else
		targetRef.set_name(target.String());

	BEntry entry(&targetRef);
	return !entry.Exists();
}


BString
RenameProcessor::_ReadAttribute(const entry_ref& ref, const char* name)
{
	BNode node(&ref);
	status_t status = node.InitCheck();
	if (status != B_OK)
		return "";

	attr_info info;
	status = node.GetAttrInfo(name, &info);
	if (status != B_OK)
		return "";

	char buffer[1024];
	off_t size = info.size;
	if (size > sizeof(buffer))
		size = sizeof(buffer);

	ssize_t bytesRead = node.ReadAttr(name, info.type, 0, buffer, size);
	if (bytesRead != size)
		return "";

	// Taken over from Haiku's listattr.cpp
	BString result;
	switch (info.type) {
		case B_INT8_TYPE:
			result.SetToFormat("%" B_PRId8, *((int8 *)buffer));
			break;
		case B_UINT8_TYPE:
			result.SetToFormat("%" B_PRIu8, *((uint8 *)buffer));
			break;
		case B_INT16_TYPE:
			result.SetToFormat("%" B_PRId16, *((int16 *)buffer));
			break;
		case B_UINT16_TYPE:
			result.SetToFormat("%" B_PRIu16, *((uint16 *)buffer));
			break;
		case B_INT32_TYPE:
			result.SetToFormat("%" B_PRId32, *((int32 *)buffer));
			break;
		case B_UINT32_TYPE:
			result.SetToFormat("%" B_PRIu32, *((uint32 *)buffer));
			break;
		case B_INT64_TYPE:
			result.SetToFormat("%" B_PRId64, *((int64 *)buffer));
			break;
		case B_UINT64_TYPE:
			result.SetToFormat("%" B_PRIu64, *((uint64 *)buffer));
			break;
		case B_FLOAT_TYPE:
			result.SetToFormat("%f", *((float *)buffer));
			break;
		case B_DOUBLE_TYPE:
			result.SetToFormat("%f", *((double *)buffer));
			break;
		case B_BOOL_TYPE:
			result.SetToFormat("%d", *((unsigned char *)buffer));
			break;
		case B_TIME_TYPE:
		{
			char stringBuffer[256];
			struct tm timeInfo;
			localtime_r((time_t *)buffer, &timeInfo);
			strftime(stringBuffer, sizeof(stringBuffer), "%c", &timeInfo);
			result.SetToFormat("%s", stringBuffer);
			break;
		}
		case B_STRING_TYPE:
		case B_MIME_STRING_TYPE:
		case 'MSIG':
		case 'MSDC':
		case 'MPTH':
			result.SetToFormat("%s", buffer);
			break;
	}
	return result;
}


int
RenameProcessor::_Extract(const char* buffer, int length, char open,
	BString& expression)
{
	// Find end
	char close = open == '(' ? ')' : ']';
	int level = 0;
	for (int index = 0; index < length; index++) {
		if (buffer[index] == open)
			level++;
		else if (buffer[index] == close) {
			level--;
			if (level < 0) {
				// Extract expression
				expression.SetTo(buffer, index);
				return index;
			}
		}
	}

	return -1;
}


//	#pragma mark -


extern "C" void
process_refs(entry_ref directoryRef, BMessage* msg, void*)
{
	RenameSettings settings;
	RenameWindow* window = new RenameWindow(settings);

	int32 count = window->AddRefs(*msg);
	if (count == 0)
		window->AddRef(directoryRef);

	window->Show();

	wait_for_thread(window->Thread(), NULL);
}


void
printUsage()
{
	printf("Copyright (c) 2019 pinc software.\n"
		"Usage: %s [-vrmfictds] <list of directories>\n"
		"  -v\tverbose mode\n"
		"  -u\tshow UI\n",
		kProgramName);
}


int
main(int argc, char** argv)
{
	BApplication app("application/x-vnd.pinc.rename");

	if (argc == 1) {
		printUsage();
		return 1;
	}

	bool useUI = false;

	int c;
	while ((c = getopt_long(argc, argv, "uhv", kOptions, NULL)) != -1) {
		switch (c) {
			case 0:
				break;
			case 'u':
				useUI = true;
				break;
			case 'h':
				printUsage();
				return 0;
			case 'v':
				gVerbose = true;
				break;
			default:
				printUsage();
				return 1;
		}
	}

	RenameWindow* window = NULL;
	RenameSettings settings;
	if (useUI) {
		window = new RenameWindow(settings);
	}

	for (int index = optind; index < argc; index++) {
		if (useUI) {
			entry_ref ref;
			status_t status = get_ref_for_path(argv[index], &ref);
			if (status == B_OK)
				window->AddRef(ref);
		}
	}

	if (useUI) {
		window->Show();

		wait_for_thread(window->Thread(), NULL);
	}

	return 0;
}
