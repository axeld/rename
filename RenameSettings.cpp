/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */


#include "RenameSettings.h"

#include <File.h>
#include <FindDirectory.h>
#include <Path.h>


static const uint32 kMsgRenameSettings = 'pReS';

static BRect sDefaultWindowFrame(150, 150, 600, 500);


RenameSettings::RenameSettings()
{
}


RenameSettings::~RenameSettings()
{
}


status_t
RenameSettings::Load()
{
	BPath path;
	status_t status = _GetPath(path);
	if (status != B_OK)
		return status;

	BFile file;
	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status != B_OK)
		return status;

	status = fSettings.Unflatten(&file);
	if (status != B_OK)
		return status;

	if (fSettings.what != kMsgRenameSettings) {
		fSettings.MakeEmpty();
		return B_ENTRY_NOT_FOUND;
	}

	return B_OK;
}


status_t
RenameSettings::Save()
{
	BPath path;
	status_t status = _GetPath(path);
	if (status != B_OK)
		return status;

	BFile file;
	status = file.SetTo(path.Path(),
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (status != B_OK)
		return status;

	fSettings.what = kMsgRenameSettings;

	return fSettings.Flatten(&file);
}


BRect
RenameSettings::WindowFrame() const
{
	return fSettings.GetRect("window frame", sDefaultWindowFrame);
}


void
RenameSettings::SetWindowFrame(BRect frame)
{
	fSettings.SetRect("window frame", frame);
}


bool
RenameSettings::Recursive() const
{
	return fSettings.GetBool("recursive", false);
}


void
RenameSettings::SetRecursive(bool recursive)
{
	fSettings.SetBool("recursive", recursive);
}


::FileTypeMode
RenameSettings::FileTypeMode() const
{
	return (::FileTypeMode)fSettings.GetUInt32("file type mode",
		(uint32)FILES_AND_FOLDERS);
}


void
RenameSettings::SetFileTypeMode(::FileTypeMode mode)
{
	fSettings.SetUInt32("file type mode", (uint32)mode);
}


::ReplacementMode
RenameSettings::ReplacementMode() const
{
	return (::ReplacementMode)fSettings.GetUInt32("replacement mode",
		(uint32)NEED_ALL_REPLACEMENTS);
}


void
RenameSettings::SetReplacementMode(::ReplacementMode mode)
{
	fSettings.SetUInt32("replacement mode", (uint32)mode);
}


status_t
RenameSettings::Get(const char* name, BMessage& settings)
{
	return fSettings.FindMessage(name, &settings);
}


void
RenameSettings::Set(const char* name, const BMessage& settings)
{
	fSettings.RemoveName(name);
	fSettings.AddMessage(name, &settings);
}


status_t
RenameSettings::_GetPath(BPath& path)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	return path.Append("pinc.rename settings");
}
