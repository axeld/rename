/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */
#ifndef RENAME_VIEW_H
#define RENAME_VIEW_H


#include <View.h>


class RenameAction;


static const uint32 kMsgUpdatePreview = 'upPv';


class RenameView : public BView {
public:
								RenameView(const char* name);
	virtual						~RenameView();

	virtual	RenameAction*		Action() const = 0;
	virtual void				RequestFocus() const;
};


#endif	// RENAME_VIEW_H
