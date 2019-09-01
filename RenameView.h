/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */
#ifndef RENAME_VIEW_H
#define RENAME_VIEW_H


#include <View.h>


class RenameAction;


class RenameView : public BView {
public:
								RenameView(const char* name);
	virtual						~RenameView();

	virtual	RenameAction*		Action() const = 0;
	virtual void				RequestFocus() const = 0;
};


#endif	// RENAME_VIEW_H
