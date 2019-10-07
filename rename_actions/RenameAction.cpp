/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */


#include "RenameAction.h"


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
