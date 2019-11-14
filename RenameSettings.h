/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */
#ifndef RENAME_SETTINGS_H
#define RENAME_SETTINGS_H


#include <Message.h>


class BPath;


enum FileTypeMode {
	FILES_AND_FOLDERS	= 0,
	FILES_ONLY			= 1,
	FOLDERS_ONLY		= 2
};

enum ReplacementMode {
	NEED_NO_REPLACEMENTS	= 0,
	NEED_ANY_REPLACEMENTS	= 1,
	NEED_ALL_REPLACEMENTS	= 2
};


class RenameSettings {
public:
								RenameSettings();
								~RenameSettings();

			status_t			Load();
			status_t			Save();

			BRect				WindowFrame() const;
			void				SetWindowFrame(BRect frame);

			bool				Recursive() const;
			void				SetRecursive(bool recursive);

			::FileTypeMode		FileTypeMode() const;
			void				SetFileTypeMode(::FileTypeMode mode);

			::ReplacementMode	ReplacementMode() const;
			void				SetReplacementMode(::ReplacementMode mode);

			status_t			Get(const char* name, BMessage& settings);
			void				Set(const char* name, const BMessage& settings);

private:
			status_t			_GetPath(BPath& path);

private:
			BMessage			fSettings;
};


#endif	// RENAME_SETTINGS_H
