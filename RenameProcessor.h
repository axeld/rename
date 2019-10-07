/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */
#ifndef RENAME_PROCESSOR_H
#define RENAME_PROCESSOR_H


#include <Looper.h>


static const uint32 kMsgProcessAndCheckRename = 'pchR';
static const uint32 kMsgProcessed = 'prcd';


class RenameProcessor : public BLooper {
public:
								RenameProcessor();

	virtual	void				MessageReceived(BMessage* message);

private:
			void				_ProcessRef(BMessage& update,
									const entry_ref& ref,
									const BString& target);
			bool				_CheckRef(const entry_ref& ref,
									const BString& target);
			BString				_ReadAttribute(const entry_ref& ref,
									const char* name);
			int					_Extract(const char* buffer, int length,
									char open, BString& expression);
};


#endif	// RENAME_PROCESSOR_H
