/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */
#ifndef REF_MODEL_H
#define REF_MODEL_H


#include "RefFilter.h"

#include <Entry.h>
#include <Locker.h>
#include <Messenger.h>
#include <ObjectList.h>

#include <set>


static const uint32 kMsgUpdateRefs = 'upRf';


typedef std::set<entry_ref> EntrySet;


class RefModel {
public:
								RefModel(const BMessenger& target);
								~RefModel();

			status_t			InitCheck();

			void				AddRef(const entry_ref& ref);
			void				RemoveRef(const entry_ref& ref);

			bool				IsRecursive() const
									{ return fRecursive; }

			void				SetRecursive(bool recursive);
			void				SetFilter(RefFilter* filter);
			void				ResetRemoved();

private:
	static	status_t			_Work(void* _self);
			status_t			_Work();

			void				_Transform(BMessage& update,
									const EntrySet& entries, bool recursive,
									RefFilter* filter, bool processFilter);
			void				_ProcessRef(EntrySet& dirs, EntrySet& files,
									bool recursive, const entry_ref& ref,
									bool useSets);
			void				_Filter(BMessage& update,
									const EntrySet& entries,
									RefFilter* filter, bool directory);
			void				_RemoveStale(BMessage& update);
			void				_RemoveFromFilter(BMessage& update,
									const entry_ref& ref);
			void				_AddToFilter(BMessage& update,
									const entry_ref& ref);

private:
			BMessenger			fTarget;
			thread_id			fThread;
			RefFilter*			fFilter;
			RefFilter*			fOldFilter;
			bool				fRecursive;
			//!	Guards fAdded.
			BLocker				fAddedLock;
			//!	Guards fPending.
			BLocker				fPendingLock;
			//!	Guards fRemoved.
			BLocker				fRemovedLock;
			//!	Guards fRecursive and fFilter.
			BLocker				fOptionsLock;

			//!	All new entries will be added here, and never removed.
			EntrySet			fAdded;

			/*!	All not-yet transformed entries will end up here, and will
				be picked up by the worker thread.
			 */
			EntrySet			fPending;

			//!	Contains all entries after applying recursive setting.
			EntrySet			fTransformedDirs;
			EntrySet			fTransformedFiles;

			/*!	Contains all entries that where removed by the user.
				They will be removed from the filtered set.
			*/
			EntrySet			fRemoved;

			/*! Contains the filtered entries out of fTransformed. Changes
				to this set will be reported to the target.
			*/
			EntrySet			fFiltered;

			sem_id				fChangeSem;
			int32				fChanges;
};


#endif	// REF_MODEL_H
