/* rename - Batch rename Tracker add-on
 *
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */


#include <Application.h>
#include <Alert.h>
#include <Catalog.h>
#include <CardView.h>
#include <ListView.h>
#include <String.h>
#include <TextControl.h>

#include <Path.h>
#include <File.h>
#include <NodeInfo.h>
#include <Node.h>
#include <Directory.h>
#include <FindDirectory.h>

#include <kernel/fs_info.h>
#include <kernel/fs_attr.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


static const uint32 kMsgRenameSettings = 'pReS';

static const uint32 kMsgUpdateReplace = 'upRe';
static const uint32 kMsgUpdateSearch = 'upSe';


#define B_TRANSLATION_CONTEXT "Rename"


class RenameWindow : public BWindow {
public:
								RenameWindow(BRect rect);
	virtual						~RenameWindow();

	virtual	void				MessageReceived(BMessage* message);

private:
			BCardView*			fCardView;
			BTextControl*		fPatternControl;
			BTextControl*		fRenameControl;
			BButton*			fOkButton;
			BButton*			fCancelButton;
			BListView*			fPreviewList;
};


extern const char* __progname;
static const char* kProgramName = __progname;

BRect gWindowFrame(150, 150, 200, 200);
bool gRecursive = true;
bool gFromShell = false;
bool gVerbose = false;


status_t
getSettingsPath(BPath &path)
{
	status_t status;
	if ((status = find_directory(B_USER_SETTINGS_DIRECTORY, &path)) != B_OK)
		return status;

	path.Append("pinc.albumattr settings");
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
	if (load.FindRect("window position", &rect) == B_OK)
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


//	#pragma mark -


RenameWindow::RenameWindow(BRect rect)
	:
	BWindow(rect, B_TRANSLATE("Rename files"), B_DOCUMENT_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS, B_ASYNCHRONOUS_CONTROLS)
{
	fPatternControl = new BTextControl("Regular expression", NULL, NULL);
	fPatternControl->SetModificationMessage(new BMessage(kMsgUpdateSearch));

	fRenameControl = new BTextControl("Replace with", NULL, NULL);
	fRenameControl->SetModificationMessage(new BMessage(kMsgUpdateReplace));
}


RenameWindow::~RenameWindow()
{
	gWindowFrame = Frame();

	saveSettings();
}


void
RenameWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgUpdateSearch:
		case kMsgUpdateReplace:
			puts("update!");
			break;
	}
}


//	#pragma mark -


extern "C" void
process_refs(entry_ref directoryRef, BMessage* msg, void*)
{
	readSettings();

	RenameWindow* window = new RenameWindow(gWindowFrame);
	window->Show();

	wait_for_thread(window->Thread(), NULL);

/*	entry_ref ref;
	int32 index;
	for (index = 0; msg->FindRef("refs", index, &ref) == B_OK; index ++) {
		BEntry entry(&ref);
		if (entry.InitCheck() == B_OK)
			handleDirectory(entry, 0);
	}

	if (index == 0) {
		BEntry entry(&directoryRef);
		if (entry.InitCheck() == B_OK)
			handleDirectory(entry, 0);
	}*/
}


void
printUsage()
{
	printf("Copyright (c) 2019 pinc software.\n"
		"Usage: %s [-vrmfictds] <list of directories>\n"
		"  -v\tverbose mode\n"
		"  -r\tenter directories recursively\n"
		"  -m\tdon't use the media kit: retrieve song length from attributes only\n"
		"  -f\tforces updates even if the directories already have attributes\n"
		"  -i\tinstalls the extra application/x-vnd.Be-directory-album MIME type\n"
		"  -c\tfinds a cover image and set their thumbnail as directory icon\n"
		"  -t\tdon't use the thumbnail from the image, always create a new one\n"
		"  -d\tallows different artists in one album (i.e. for samplers, soundtracks, ...)\n"
		"  -s\tread options from standard settings file\n",
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

	gFromShell = true;

	while (*++argv && **argv == '-') {
		for (int i = 1; (*argv)[i]; i++) {
			switch ((*argv)[i]) {
				case 'v':
					gVerbose = true;
					break;
				default:
					printUsage();
					return 1;
			}
		}
	}

	argv--;

	while (*++argv) {
		BEntry entry(*argv);

		if (entry.InitCheck() == B_OK)
			handleDirectory(entry, 0);
		else
			fprintf(stderr, "could not find \"%s\".\n", *argv);
	}
	return 0;
}
