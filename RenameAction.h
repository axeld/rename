#ifndef RENAME_ACTION_H
#define RENAME_ACTION_H


#include <ObjectList.h>
#include <String.h>


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

	virtual BString				Rename(BObjectList<Group>& sourceGroups,
									BObjectList<Group>& targetGroups,
									const char* string) const = 0;
};


#endif	// RENAME_ACTION_H
