/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */


#include "RefModel.h"


//	#pragma mark - RefFilter


RefFilter::~RefFilter()
{
}


//	#pragma mark - RefModel


RefModel::RefModel(const BMessenger& target)
	:
	fTarget(target),
	fFilter(NULL),
	fRecursive(false)
{
}


RefModel::~RefModel()
{
}


void
RefModel::AddRef(const entry_ref& ref)
{
}


void
RefModel::RemoveRef(const entry_ref& ref)
{
}


void
RefModel::SetFilter(RefFilter* filter)
{
}

