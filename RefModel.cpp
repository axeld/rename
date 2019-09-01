/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */


#include "RefModel.h"

#include <Autolock.h>
#include <Directory.h>

#include <stdio.h>
#include <string.h>


#define REMOVED_CHANGED 	0x01
#define FILTER_CHANGED		0x02
#define RECURSIVE_CHANGED	0x04
#define REMOVED_CLEARED 	0x08
#define REFS_UPDATED		0x10


RefModel::RefModel(const BMessenger& target)
	:
	fTarget(target),
	fThread(-1),
	fFilter(NULL),
	fOldFilter(NULL),
	fRecursive(false),
	fAddedLock("added lock"),
	fPendingLock("pending lock"),
	fRemovedLock("removed lock"),
	fOptionsLock("options lock"),
	fChanges(0)
{
	fChangeSem = create_sem(0, "changes");
	fThread = spawn_thread(&RefModel::_Work, "ref model", B_NORMAL_PRIORITY,
		this);
	if (fThread >= 0)
		resume_thread(fThread);
}


RefModel::~RefModel()
{
	delete_sem(fChangeSem);
	wait_for_thread(fThread, NULL);
}


status_t
RefModel::InitCheck()
{
	if (fChangeSem < 0)
		return fChangeSem;
	if (fThread < 0)
		return fThread;

	return B_OK;
}


void
RefModel::AddRef(const entry_ref& ref)
{
	BAutolock locker(fAddedLock);
	fAdded.insert(ref);
	BAutolock pendingLocker(fPendingLock);
	fPending.insert(ref);

	release_sem_etc(fChangeSem, 1, B_DO_NOT_RESCHEDULE);
}


void
RefModel::RemoveRef(const entry_ref& ref)
{
	BAutolock locker(fRemovedLock);
	fRemoved.insert(ref);

	atomic_or(&fChanges, REMOVED_CHANGED);
	release_sem_etc(fChangeSem, 1, B_DO_NOT_RESCHEDULE);
}


void
RefModel::UpdateRef(const entry_ref& from, const entry_ref& to)
{
	BAutolock locker(fAddedLock);

	if (fAdded.erase(from)) {
		fAdded.insert(to);

		BAutolock pendingLocker(fPendingLock);
		fPending.insert(to);
	} else {
		atomic_or(&fChanges, REFS_UPDATED);
	}

	release_sem_etc(fChangeSem, 1, B_DO_NOT_RESCHEDULE);
}


void
RefModel::SetRecursive(bool recursive)
{
	if (recursive == IsRecursive())
		return;

	BAutolock locker(fOptionsLock);
	fRecursive = recursive;

	atomic_or(&fChanges, RECURSIVE_CHANGED);
	release_sem(fChangeSem);
}


void
RefModel::SetFilter(RefFilter* filter)
{
	if (filter == fFilter)
		return;

	BAutolock locker(fOptionsLock);
	delete fOldFilter;
	fOldFilter = fFilter;
	fFilter = filter;

	atomic_or(&fChanges, FILTER_CHANGED);
	release_sem(fChangeSem);
}


void
RefModel::ResetRemoved()
{
	BAutolock locker(fRemovedLock);
	fRemoved.clear();

	atomic_or(&fChanges, REMOVED_CLEARED);
	release_sem(fChangeSem);
}


/*static*/ status_t
RefModel::_Work(void* _self)
{
	RefModel* self = (RefModel*)_self;
	return self->_Work();
}


status_t
RefModel::_Work()
{
	while (true) {
		int32 count;
		status_t status = get_sem_count(fChangeSem, &count);
		if (status < B_OK)
			return status;

		if (count < 1)
			count = 1;

		status = acquire_sem_etc(fChangeSem, count, 0, 0);
		if (status < B_OK)
			return status;

		// Determine changes and configuration
		int32 changes = atomic_and(&fChanges, 0);

		EntrySet pending;
		{
			BAutolock locker(fPendingLock);
			pending = fPending;
			fPending.clear();
		}

		bool recursive;
		RefFilter* filter;
		{
			BAutolock locker(fOptionsLock);
			recursive = fRecursive;
			filter = fFilter;

			delete fOldFilter;
			fOldFilter = NULL;
		}

		// Rebuild transformed, if needed
		if ((changes & (RECURSIVE_CHANGED | REFS_UPDATED)) != 0) {
			fTransformedDirs.clear();
			fTransformedFiles.clear();

			{
				BAutolock locker(fAddedLock);
				pending = fAdded;
			}
		}

		BMessage update(kMsgUpdateRefs);

		// Rebuild filter, if needed
		if ((changes & (FILTER_CHANGED | REMOVED_CHANGED
				| REMOVED_CLEARED)) != 0) {
			_Transform(update, pending, recursive, NULL, false);
			_Filter(update, fTransformedDirs, filter, true);
			_Filter(update, fTransformedFiles, filter, false);
		} else
			_Transform(update, pending, recursive, filter, true);

		if ((changes & RECURSIVE_CHANGED) != 0 && !recursive)
			_RemoveStale(update);

		if (!update.IsEmpty())
			fTarget.SendMessage(&update);
	}
}


void
RefModel::_Transform(BMessage& update, const EntrySet& entries, bool recursive,
	RefFilter* filter, bool processFilter)
{
	EntrySet transformedDirs;
	EntrySet transformedFiles;

	EntrySet::const_iterator iterator = entries.begin();
	for (; iterator != entries.end(); iterator++) {
		const entry_ref& ref = *iterator;
		_ProcessRef(transformedDirs, transformedFiles, recursive, ref,
			processFilter);
	}
	if (processFilter) {
		_Filter(update, transformedDirs, filter, true);
		_Filter(update, transformedFiles, filter, false);
	}
}


void
RefModel::_ProcessRef(EntrySet& dirs, EntrySet& files, bool recursive,
	const entry_ref& ref, bool useSets)
{
	BEntry entry(&ref);
	if (entry.InitCheck() != B_OK) {
		fprintf(stderr, "Cannot access %s: %s\n", ref.name,
			strerror(entry.InitCheck()));
		return;
	}

	if (entry.IsDirectory()) {
		if (useSets)
			dirs.insert(ref);
		fTransformedDirs.insert(ref);

		if (recursive) {
			BDirectory directory(&ref);
			entry_ref childRef;
			while (directory.GetNextRef(&childRef) == B_OK) {
				_ProcessRef(dirs, files, true, childRef, useSets);
			}
		}
	} else {
		if (useSets)
			files.insert(ref);
		fTransformedFiles.insert(ref);
	}
}


void
RefModel::_Filter(BMessage& update, const EntrySet& entries, RefFilter* filter,
	bool directory)
{
	EntrySet::const_iterator iterator = entries.begin();
	for (; iterator != entries.end(); iterator++) {
		const entry_ref& ref = *iterator;
		{
			BAutolock locker(fRemovedLock);
			if (fRemoved.find(ref) != fRemoved.end()) {
				_RemoveFromFilter(update, ref);
				continue;
			}
		}
		if (fFilter != NULL && !filter->Accept(ref, directory)) {
			_RemoveFromFilter(update, ref);
			continue;
		}
		_AddToFilter(update, ref);
	}
}


void
RefModel::_RemoveStale(BMessage& update)
{
	EntrySet::iterator iterator = fFiltered.begin();
	while (iterator != fFiltered.end()) {
		const entry_ref& ref = *iterator;
		if (fTransformedDirs.find(ref) == fTransformedDirs.end()
			&& fTransformedFiles.find(ref) == fTransformedFiles.end()) {
			update.AddRef("remove", &ref);
			iterator = fFiltered.erase(iterator);
		} else
			iterator++;
	}
}


void
RefModel::_RemoveFromFilter(BMessage& update, const entry_ref& ref)
{
	if (fFiltered.find(ref) == fFiltered.end())
		return;

	fFiltered.erase(ref);
	update.AddRef("remove", &ref);
}


void
RefModel::_AddToFilter(BMessage& update, const entry_ref& ref)
{
	if (fFiltered.find(ref) != fFiltered.end())
		return;

	fFiltered.insert(ref);
	update.AddRef("add", &ref);
}
