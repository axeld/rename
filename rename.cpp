/* rename - Batch rename Tracker add-on
 *
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */


#include <Application.h>
#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <CardView.h>
#include <ControlLook.h>
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

#include <map>
#include <set>

#include <getopt.h>
#include <regex.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


typedef std::set<entry_ref> EntrySet;
typedef std::set<BString> StringSet;


static const uint32 kMsgRenameSettings = 'pReS';

static const uint32 kMsgSetAction = 'stAc';
static const uint32 kMsgUpdatePreview = 'upPv';
static const uint32 kMsgRename = 'okRe';
static const uint32 kMsgCheckRename = 'chRe';
static const uint32 kMsgChecked = 'chkd';


static rgb_color kGroupColor[] = {
	make_color(100, 200, 100),
	make_color(200, 100, 200),
	make_color(100, 200, 200),
	make_color(150, 200, 150),
	make_color(100, 250, 100),
};
static const uint32 kGroupColorCount = 5;

#define B_TRANSLATION_CONTEXT "Rename"

#define MAX_GROUPS 10
#define MAX_MATCHES 20


struct Group {
	int32	index;
	int32	start;
	int32	end;

	Group(int32 index, int32 start, int32 end)
		:
		index(index),
		start(start),
		end(end)
	{
	}
};


class RenameAction {
public:
	virtual						~RenameAction();

	virtual	bool				AddGroups(BObjectList<Group>& groups,
									const char* string) const = 0;
	virtual BString				Rename(BObjectList<Group>& groups,
									const char* string) const = 0;
};


class RegularExpressionRenameAction : public RenameAction {
public:
								RegularExpressionRenameAction();
	virtual						~RegularExpressionRenameAction();

			bool				SetPattern(const char* pattern);
			void				SetReplace(const char* replace);

	virtual	bool				AddGroups(BObjectList<Group>& groups,
									const char* string) const;
	virtual BString				Rename(BObjectList<Group>& groups,
									const char* string) const;

private:
			regex_t				fCompiledPattern;
			bool				fValidPattern;
			BString				fReplace;
};


class WindowsRenameAction : public RenameAction {
public:
								WindowsRenameAction();
	virtual						~WindowsRenameAction();

			void				SetReplaceChar(char replace);

	virtual	bool				AddGroups(BObjectList<Group>& groups,
									const char* string) const;
	virtual BString				Rename(BObjectList<Group>& groups,
									const char* string) const;

private:
	static	bool				_IsInvalidCharacter(char c);

private:
			char				fReplaceChar;
};


class PreviewList : public BListView {
public:
								PreviewList(const char* name);

	virtual	void				Draw(BRect updateRect);
};


enum Error {
	NO_ERROR,
	INVALID_NAME,
	DUPLICATE,
	EXISTS
};


class PreviewItem : public BStringItem {
public:
								PreviewItem(const entry_ref& ref);

			void				SetRenameAction(const RenameAction& action);
			status_t			Rename();

			const entry_ref&	Ref() const
									{ return fRef; }
			const entry_ref&	TargetRef() const
									{ return fTargetRef; }
			bool				HasTarget() const
									{ return fTargetRef.name != NULL
										&& fTargetRef.name[0] != '\0'; }
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
			entry_ref			fTargetRef;
			BObjectList<Group>	fGroups;
			BObjectList<Group>	fRenameGroups;
			::Error				fError;
};

typedef std::map<entry_ref, PreviewItem*> PreviewItemMap;
typedef std::map<BString, PreviewItem*> NameMap;


class RenameView : public BView {
public:
								RenameView(const char* name);
	virtual						~RenameView();

	virtual	RenameAction*		Action() const = 0;
	virtual void				RequestFocus() const = 0;
};


class RegularExpressionView : public RenameView {
public:
								RegularExpressionView();
	virtual						~RegularExpressionView();

	virtual	RenameAction*		Action() const;
	virtual void				RequestFocus() const;

private:
			BTextControl*		fPatternControl;
			BTextControl*		fRenameControl;
};


class WindowsRenameView : public RenameView {
public:
								WindowsRenameView();
	virtual						~WindowsRenameView();

	virtual	RenameAction*		Action() const;
	virtual void				RequestFocus() const;

private:
			BTextControl*		fReplaceControl;
};


class RenameWindow : public BWindow {
public:
								RenameWindow(BRect rect);
	virtual						~RenameWindow();

			void				AddRef(const entry_ref& ref);
			int32				AddRefs(const BMessage& message);

	virtual	void				MessageReceived(BMessage* message);

private:
			void				_UpdatePreviewItems();
			void				_RenameFiles();

private:
			BMenuField*			fActionMenuField;
			BPopUpMenu*			fActionMenu;
			BCardView*			fCardView;
			RenameView*			fView;
			BButton*			fOkButton;
			BListView*			fPreviewList;
			PreviewItemMap		fPreviewItemMap;
			BMessenger			fRenameChecker;
};


class RenameChecker : public BLooper {
public:
								RenameChecker();

	virtual	void				MessageReceived(BMessage* message);

private:
			bool				_CheckRef(const entry_ref& ref,
									const entry_ref& targetRef);
};


extern const char* __progname;
static const char* kProgramName = __progname;


static struct option const kOptions[] = {
	{"ui", no_argument, 0, 'u'},
	{"verbose", no_argument, 0, 'v'},
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0}
};


BRect gWindowFrame(150, 150, 600, 500);
bool gRecursive = true;
bool gVerbose = false;


status_t
getSettingsPath(BPath &path)
{
	status_t status;
	if ((status = find_directory(B_USER_SETTINGS_DIRECTORY, &path)) != B_OK)
		return status;

	path.Append("pinc.rename settings");
	return B_OK;
}


status_t
saveSettings()
{
	status_t status;

	BPath path;
	if ((status = getSettingsPath(path)) != B_OK)
		return status;

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if ((status = file.InitCheck()) != B_OK)
		return status;

	file.SetSize(0);

	BMessage save(kMsgRenameSettings);
	save.AddRect("window frame", gWindowFrame);

	return save.Flatten(&file);
}


status_t
readSettings()
{
	status_t status;

	BPath path;
	if ((status = getSettingsPath(path)) != B_OK)
		return status;

	BFile file(path.Path(), B_READ_ONLY);
	if ((status = file.InitCheck()) != B_OK) {
		fprintf(stderr, "%s: could not load settings: %s\n", kProgramName,
			strerror(status));
		return status;
	}

	BMessage load;
	if ((status = load.Unflatten(&file)) != B_OK)
		return status;

	if (load.what != kMsgRenameSettings)
		return B_ENTRY_NOT_FOUND;

	BRect rect;
	if (load.FindRect("window frame", &rect) == B_OK)
		gWindowFrame = rect;

	return B_OK;
}


//	#pragma mark -


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
		return stringBuffer;

	regmatch_t groups[MAX_GROUPS];
	if (regexec(&fCompiledPattern, string, MAX_GROUPS, groups, 0))
		return stringBuffer;

	stringBuffer = fReplace;
	if (groups[1].rm_so == -1) {
		// There is just a single group -- just replace the match
		stringBuffer.Prepend(string, groups[0].rm_so);
		stringBuffer.Append(string + groups[0].rm_eo);
		return stringBuffer;
	}

	char* buffer = stringBuffer.LockBuffer(B_FILE_NAME_LENGTH);
	char* target = buffer;

	for (; target[0] != '\0'; target++) {
		if (target[0] == '\\' && target[1] > '0' && target[1] <= '9') {
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
	return stringBuffer;
}


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

//	#pragma mark - PreviewList


PreviewList::PreviewList(const char* name)
	:
	BListView(name, B_SINGLE_SELECTION_LIST,
		B_FULL_UPDATE_ON_RESIZE | B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE)
{
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


//	#pragma mark - PreviewItem


PreviewItem::PreviewItem(const entry_ref& ref)
	:
	BStringItem(ref.name),
	fRef(ref),
	fGroups(10, true),
	fError(NO_ERROR)
{
	fTargetRef.device = fRef.device;
	fTargetRef.directory = fRef.directory;
}


void
PreviewItem::SetRenameAction(const RenameAction& action)
{
	fGroups.MakeEmpty();
	fRenameGroups.MakeEmpty();
	fError = NO_ERROR;

	action.AddGroups(fGroups, fRef.name);

	BString newName = action.Rename(fRenameGroups, fRef.name);
	if (newName != fRef.name)
		fTargetRef.set_name(newName.String());

	if (HasTarget() && !_IsValidName(fTargetRef.name))
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

	status = entry.Rename(fTargetRef.name);
	if (status != B_OK)
		return status;

	fRef.set_name(fTargetRef.name);
	fTargetRef.set_name(NULL);

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
		owner->DrawString(fTargetRef.name);
	} else {
		_DrawGroupedText(owner, frame, x + width, fTargetRef.name,
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
	return strchr(name, '/') == NULL;
}


//	#pragma mark -


RenameView::RenameView(const char* name)
	:
	BView(name, 0)
{
}


RenameView::~RenameView()
{
}


//	#pragma mark -


RegularExpressionView::RegularExpressionView()
	:
	RenameView("regular expression")
{
	fPatternControl = new BTextControl("Pattern", NULL, NULL);
	fPatternControl->SetModificationMessage(new BMessage(kMsgUpdatePreview));

	fRenameControl = new BTextControl("Replace with", NULL, NULL);
	fRenameControl->SetModificationMessage(new BMessage(kMsgUpdatePreview));

	BLayoutBuilder::Grid<>(this, 0.f)
		.SetInsets(B_USE_DEFAULT_SPACING, 0, 0, 0)
		.Add(fPatternControl->CreateLabelLayoutItem(), 0, 0)
		.Add(fPatternControl->CreateTextViewLayoutItem(), 1, 0)
		.Add(fRenameControl->CreateLabelLayoutItem(), 0, 1)
		.Add(fRenameControl->CreateTextViewLayoutItem(), 1, 1);
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


//	#pragma mark -


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


//	#pragma mark -


RenameWindow::RenameWindow(BRect rect)
	:
	BWindow(rect, B_TRANSLATE("Rename files"), B_DOCUMENT_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS)
{
	fOkButton = new BButton("ok", "Rename", new BMessage(kMsgRename));
	fOkButton->SetEnabled(false);

	RenameView* regularExpressionView = fView = new RegularExpressionView();
	RenameView* windowsRenameView = new WindowsRenameView();

	fActionMenu = new BPopUpMenu("Actions");

	BMessage* message = new BMessage(kMsgSetAction);
	message->AddInt32("index", 0);
	BMenuItem* item = new BMenuItem("Regular expression", message);
	item->SetMarked(true);
	fActionMenu->AddItem(item);

	message = new BMessage(kMsgSetAction);
	message->AddInt32("index", 1);
	item = new BMenuItem("Windows compliant", message);
	fActionMenu->AddItem(item);

	fActionMenuField = new BMenuField("action", "Rename method", fActionMenu);

	fCardView = new BCardView("action");
	fCardView->AddChild(regularExpressionView);
	fCardView->AddChild(windowsRenameView);
	fCardView->CardLayout()->SetVisibleItem(0L);

	fPreviewList = new PreviewList("preview");

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.AddGroup(B_VERTICAL)
			.SetInsets(B_USE_WINDOW_SPACING)
			.AddGrid(0.f)
				.Add(fActionMenuField->CreateLabelLayoutItem(), 0, 0)
				.Add(fActionMenuField->CreateMenuBarLayoutItem(), 1, 0)
			.End()
			.Add(fCardView)
		.End()
		.AddGroup(B_HORIZONTAL)
			.SetInsets(B_USE_DEFAULT_SPACING, 0,
				B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.AddGlue()
			.Add(fOkButton)
		.End()
		.AddGroup(B_VERTICAL)
			.SetInsets(-2)
			.Add(new BScrollView("scroller", fPreviewList, 0, true, true))
		.End();

	fView->RequestFocus();

	RenameChecker* checker = new RenameChecker();
	checker->Run();

	fRenameChecker = checker;
}


RenameWindow::~RenameWindow()
{
	gWindowFrame = Frame();
	fRenameChecker.SendMessage(B_QUIT_REQUESTED);

	saveSettings();
}


void
RenameWindow::AddRef(const entry_ref& ref)
{
	PreviewItemMap::iterator found = fPreviewItemMap.find(ref);
	if (found != fPreviewItemMap.end()) {
		// Ignore duplicate entry
		return;
	}

	PreviewItem* item = new PreviewItem(ref);
	fPreviewItemMap.insert(std::make_pair(ref, item));

	fPreviewList->AddItem(item);
	fPreviewList->SortItems(&PreviewItem::Compare);
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
			int32 index = message->GetInt32("index", 0);
			fCardView->CardLayout()->SetVisibleItem(index);
			fView = dynamic_cast<RenameView*>(
				fCardView->CardLayout()->VisibleItem()->View());

			// supposed to fall through
		}

		case kMsgUpdatePreview:
			_UpdatePreviewItems();
			break;

		case kMsgChecked:
		{
			int32 errorCount = 0;
			entry_ref ref;
			int32 index = 0;
			for (; message->FindRef("refs", index, &ref) == B_OK; index++) {
				PreviewItemMap::iterator found = fPreviewItemMap.find(ref);
				if (found != fPreviewItemMap.end()) {
					found->second->SetError(EXISTS);
					errorCount++;
				}
			}
			if (errorCount != 0)
				fPreviewList->Invalidate();
			else
				fOkButton->SetEnabled(true);
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
RenameWindow::_UpdatePreviewItems()
{
	RenameAction* action = fView->Action();

	BMessage check(kMsgCheckRename);
	int errorCount = 0;
	int validCount = 0;
	NameMap map;
	for (int32 i = 0; i < fPreviewList->CountItems(); i++) {
		PreviewItem* item = static_cast<PreviewItem*>(
			fPreviewList->ItemAt(i));
		item->SetRenameAction(*action);

		if (item->IsValid()) {
			if (item->HasTarget()) {
				BString name = item->TargetRef().name;
				NameMap::iterator found = map.find(name);
				if (found != map.end()) {
					found->second->SetError(DUPLICATE);
					item->SetError(DUPLICATE);
					errorCount++;
				} else {
					map.insert(std::make_pair(name, item));
					validCount++;
				}
			}
		} else if (item->HasTarget()) {
			errorCount++;
		}

		if (errorCount == 0 && item->HasTarget()) {
			check.AddRef("source", &item->Ref());
			check.AddRef("target", &item->TargetRef());
		}
	}

	fPreviewList->Invalidate();

	fOkButton->SetEnabled(false);

	if (errorCount == 0 && validCount > 0) {
		// Check paths on disk
		fRenameChecker.SendMessage(&check, this);
	}

	delete action;
}


void
RenameWindow::_RenameFiles()
{
	fPreviewItemMap.clear();
	for (int32 index = 0; index < fPreviewList->CountItems(); index++) {
		PreviewItem* item = static_cast<PreviewItem*>(
			fPreviewList->ItemAt(index));

		status_t status = item->Rename();
		if (status != B_OK) {
			// TODO: Proper error reporting!
			fprintf(stderr, "Renaming failed: %s\n", strerror(status));
		}

		fPreviewItemMap.insert(std::make_pair(item->Ref(), item));
	}

	fPreviewList->SortItems(&PreviewItem::Compare);
	_UpdatePreviewItems();
}


//	#pragma mark -


RenameChecker::RenameChecker()
	:
	BLooper("Rename checker")
{
}


void
RenameChecker::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgCheckRename:
		{
			BMessage reply(kMsgChecked);
			int32 count = 0;

			entry_ref ref;
			int32 index;
			for (index = 0; message->FindRef("source", index, &ref) == B_OK;
					index++) {
				entry_ref targetRef;
				if (message->FindRef("target", index, &targetRef) == B_OK) {
					if (!_CheckRef(ref, targetRef)) {
						reply.AddRef("refs", &ref);
						count++;
					}
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


bool
RenameChecker::_CheckRef(const entry_ref& ref, const entry_ref& targetRef)
{
	BEntry entry(&targetRef);
	return !entry.Exists();
}


//	#pragma mark -


extern "C" void
process_refs(entry_ref directoryRef, BMessage* msg, void*)
{
	readSettings();

	RenameWindow* window = new RenameWindow(gWindowFrame);

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
	if (useUI) {
		readSettings();
		window = new RenameWindow(gWindowFrame);
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
