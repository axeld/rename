/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */
#ifndef RENAME_WINDOW_H
#define RENAME_WINDOW_H


#include <Messenger.h>
#include <Window.h>


class PreviewList;
class RefModel;
class RenameSettings;
class RenameView;

class BCardView;
class BCheckBox;
class BMenuField;
class BPopUpMenu;
class BTextControl;


static const uint32 kMsgRefsRemoved = 'rfrm';


class RenameWindow : public BWindow {
public:
								RenameWindow(RenameSettings& settings);
	virtual						~RenameWindow();

			void				AddRef(const entry_ref& ref);
			int32				AddRefs(const BMessage& message);

	virtual	void				MessageReceived(BMessage* message);

private:
			void				_HandleProcessed(BMessage* message);
			void				_UpdatePreviewItems();
			void				_UpdateFilter();
			void				_RenameFiles();

private:
			RenameSettings&		fSettings;
			BMenuField*			fActionMenuField;
			BPopUpMenu*			fActionMenu;
			BCardView*			fCardView;
			RenameView*			fView;
			BButton*			fOkButton;
			BButton*			fResetRemovedButton;
			BButton*			fRemoveUnchangedButton;
			BTextControl*		fFilterControl;
			BCheckBox*			fRegExpFilterCheckBox;
			BCheckBox*			fReverseFilterCheckBox;
			BCheckBox*			fRecursiveCheckBox;
			BMenuField*			fTypeMenuField;
			BPopUpMenu*			fTypeMenu;
			BMenuField*			fReplacementMenuField;
			BPopUpMenu*			fReplacementMenu;
			PreviewList*		fPreviewList;
			RefModel*			fRefModel;
			BMessenger			fRenameProcessor;
};


#endif	// RENAME_WINDOW_H
