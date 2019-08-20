#ifndef REF_MODEL_H
#define REF_MODEL_H


#include <Entry.h>
#include <Messenger.h>
#include <ObjectList.h>


static const uint32 kMsgAddRef = 'adRf';
static const uint32 kMsgRemoveRef = 'rmRf';


class RefFilter {
public:
	virtual						~RefFilter();

	virtual	bool				Accept(const entry_ref& ref) const = 0;
};


class RefModel {
public:
								RefModel(const BMessenger& target);
								~RefModel();

			void				AddRef(const entry_ref& ref);
			void				RemoveRef(const entry_ref& ref);

			bool				IsRecursive() const
									{ return fRecursive; }

			void				SetRecursive(bool recursive);
			void				SetFilter(RefFilter* filter);

private:
	static	status_t			_Work(int32* _self);
			status_t			_Work();

private:
			BMessenger			fTarget;
			RefFilter*			fFilter;
			bool				fRecursive;
};


#endif	// REF_MODEL_H
