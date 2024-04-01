/* batchrename - Batch rename Tracker add-on
 *
 * Copyright (c) 2019-2024 pinc Software. All Rights Reserved.
 */


#include "batchrename.h"

#include "RenameSettings.h"
#include "RenameWindow.h"

#include <Application.h>
#include <Entry.h>
#include <File.h>
#include <Directory.h>
#include <Path.h>

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>


extern const char* __progname;
const char* kProgramName = __progname;


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
	printf("Copyright (c) 2019-2024 pinc software.\n"
		"Usage: %s [-vrmfictds] <list of directories>\n"
		"  -v\tverbose mode\n"
		"  -u\tshow UI\n",
		kProgramName);
}


int
main(int argc, char** argv)
{
	BApplication app("application/x-vnd.pinc.rename");

	// $TERM is not defined when launched from Tracker
	bool useUI = getenv("TERM") == NULL;

	if (argc == 1 && !useUI) {
		printUsage();
		return 1;
	}

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
