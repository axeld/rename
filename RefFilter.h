/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */
#ifndef REF_FILTER_H
#define REF_FILTER_H


#include <Entry.h>
#include <ObjectList.h>
#include <String.h>


class RefFilter {
public:
	virtual						~RefFilter();

	virtual	bool				Accept(const entry_ref& ref,
									bool directory) const = 0;
};


class FilesOnlyFilter : public RefFilter {
public:
								FilesOnlyFilter();
	virtual						~FilesOnlyFilter();

	virtual	bool				Accept(const entry_ref& ref,
									bool directory) const;
};


class FoldersOnlyFilter : public RefFilter {
public:
								FoldersOnlyFilter();
	virtual						~FoldersOnlyFilter();

	virtual	bool				Accept(const entry_ref& ref,
									bool directory) const;
};


class TextFilter : public RefFilter {
public:
								TextFilter(const char* text);
	virtual						~TextFilter();

	virtual	bool				Accept(const entry_ref& ref,
									bool directory) const;

private:
			BString				fSearchText;
};


class ReverseFilter : public RefFilter {
public:
								ReverseFilter(RefFilter* filter);
	virtual						~ReverseFilter();

	virtual	bool				Accept(const entry_ref& ref,
									bool directory) const;

private:
			RefFilter*			fFilter;
};


class AndFilter : public RefFilter {
public:
								AndFilter();
	virtual						~AndFilter();

			void				AddFilter(RefFilter* filter);
			bool				IsEmpty() const
									{ return fFilters.IsEmpty(); }

	virtual	bool				Accept(const entry_ref& ref,
									bool directory) const;

private:
			BObjectList<RefFilter> fFilters;
};


#endif	// REF_FILTER_H
