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


//	#pragma mark - RefFilter


RefFilter::~RefFilter()
{
}


//	#pragma mark - FilesOnlyFilter


FilesOnlyFilter::FilesOnlyFilter()
{
}


FilesOnlyFilter::~FilesOnlyFilter()
{
}


bool
FilesOnlyFilter::Accept(const entry_ref& ref, bool directory) const
{
	return !directory;
}


//	#pragma mark - FoldersOnlyFilter


FoldersOnlyFilter::FoldersOnlyFilter()
{
}


FoldersOnlyFilter::~FoldersOnlyFilter()
{
}


bool
FoldersOnlyFilter::Accept(const entry_ref& ref, bool directory) const
{
	return directory;
}


//	#pragma mark - TextFilter


TextFilter::TextFilter(const char* text)
	:
	fSearchText(text)
{
}


TextFilter::~TextFilter()
{
}


bool
TextFilter::Accept(const entry_ref& ref, bool directory) const
{
	return strcasestr(ref.name, fSearchText.String()) != NULL;
}


//	#pragma mark - RegularExpressionFilter


RegularExpressionFilter::RegularExpressionFilter(const char* pattern)
	:
	fValidPattern(false)
{
	if (regcomp(&fCompiledPattern, pattern, REG_EXTENDED) == 0)
		fValidPattern = true;
}


RegularExpressionFilter::~RegularExpressionFilter()
{
	if (fValidPattern)
		regfree(&fCompiledPattern);
}


bool
RegularExpressionFilter::Accept(const entry_ref& ref, bool directory) const
{
	if (!fValidPattern)
		return true;

	return regexec(&fCompiledPattern, ref.name, 0, NULL, 0) == 0;
}


//	#pragma mark - ReverseFilter


ReverseFilter::ReverseFilter(RefFilter* filter)
	:
	fFilter(filter)
{
}


ReverseFilter::~ReverseFilter()
{
	delete fFilter;
}


bool
ReverseFilter::Accept(const entry_ref& ref, bool directory) const
{
	return !fFilter->Accept(ref, directory);
}


//	#pragma mark - AndFilter


AndFilter::AndFilter()
	:
	fFilters(5, true)
{
}


AndFilter::~AndFilter()
{
}


void
AndFilter::AddFilter(RefFilter* filter)
{
	if (filter != NULL)
		fFilters.AddItem(filter);
}


bool
AndFilter::Accept(const entry_ref& ref, bool directory) const
{
	for (int32 index = 0; index < fFilters.CountItems(); index++) {
		if (!fFilters.ItemAt(index)->Accept(ref, directory))
			return false;
	}
	return true;
}
