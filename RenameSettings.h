/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */
#ifndef RENAME_SETTINGS_H
#define RENAME_SETTINGS_H


#include <Message.h>


class BPath;


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

			status_t			Get(const char* name, BMessage& settings);
			void				Set(const char* name, const BMessage& settings);

private:
			status_t			_GetPath(BPath& path);

private:
			BMessage			fSettings;
};


#endif	// RENAME_SETTINGS_H
