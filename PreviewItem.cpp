/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */


#include "PreviewItem.h"

#include "RenameAction.h"

#include <ControlLook.h>
#include <Directory.h>
#include <Path.h>
#include <View.h>


static rgb_color kGroupColor[] = {
	make_color(100, 200, 100),
	make_color(200, 100, 200),
	make_color(100, 200, 200),
	make_color(150, 200, 150),
	make_color(100, 250, 100),
};
static const uint32 kGroupColorCount = 5;


PreviewItem::PreviewItem(const entry_ref& ref)
	:
	BStringItem(ref.name),
	fRef(ref),
	fGroups(10, true),
	fError(NO_ERROR)
{
}


void
PreviewItem::SetTarget(const BString& target)
{
	fTarget = target;
}


void
PreviewItem::UpdateProcessed(int32 from, int32 to, const BString& replace)
{
	// Update text
	fTarget.Remove(from, to - from);
	fTarget.Insert(replace, from);

	// Update groups
	int32 diff = replace.Length() - to + from;

	for (int32 index = 0; index < fRenameGroups.CountItems(); index++) {
		Group& group = *fRenameGroups.ItemAt(index);

		if (group.start == from)
			group.end = from + replace.Length();
		else if (group.end > to)
			group.end += diff;

		if (group.start > from)
			group.start += diff;
	}
}


void
PreviewItem::SetRenameAction(const RenameAction& action)
{
	fGroups.MakeEmpty();
	fRenameGroups.MakeEmpty();
	fError = NO_ERROR;

	BString newName = action.Rename(fGroups, fRenameGroups, fRef.name);
	if (newName != fRef.name)
		fTarget = newName;
	else
		fTarget = "";

	if (HasTarget() && !_IsValidName(fTarget))
		fError = INVALID_NAME;
}


status_t
PreviewItem::Rename()
{
	if (!HasTarget())
		return B_ENTRY_NOT_FOUND;

	BEntry entry(&fRef);
	status_t status = entry.InitCheck();
	if (status != B_OK)
		return status;

	// Make sure all sub directories exist
	int32 index = fTarget.FindLast('/');
	if (index >= 0) {
		BPath path;
		status = path.SetTo(&fRef);
		if (status == B_OK)
			status = path.GetParent(&path);
		if (status == B_OK)
			status = path.Append(BString().SetTo(fTarget, index));
		if (status == B_OK)
			status = create_directory(path.Path(), 0755);
		if (status != B_OK)
			return status;
	}

	status = entry.Rename(fTarget.String());
	if (status != B_OK)
		return status;

	entry.GetRef(&fRef);
	fTarget = "";

	return B_OK;
}


const char*
PreviewItem::ErrorMessage() const
{
	switch (fError) {
		default:
		case NO_ERROR:
			return NULL;
		case INVALID_NAME:
			return "Invalid name!";
		case DUPLICATE:
			return "Duplicated name!";
		case EXISTS:
			return "Already exists!";
	}
}


void
PreviewItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	rgb_color lowColor = owner->LowColor();
	rgb_color highColor = owner->HighColor();
	BRect bounds = owner->Bounds();
	float half = bounds.Width() / 2.f;
	float width = half + be_control_look->DefaultLabelSpacing();

	if (IsSelected() || complete) {
		rgb_color color;
		if (IsSelected())
			color = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
		else
			color = owner->ViewColor();

		owner->SetLowColor(color);
		owner->FillRect(frame, B_SOLID_LOW);
	} else
		owner->SetLowColor(owner->ViewColor());

	float x = frame.left + be_control_look->DefaultLabelSpacing();
	BRect rect = bounds;
	rect.right = half - 1;

	owner->PushState();
	owner->ClipToRect(rect);

	if (fGroups.IsEmpty()) {
		// Text does not match
		owner->SetLowColor(200, 100, 100);
		owner->FillRect(BRect(x, frame.top, x + owner->StringWidth(fRef.name),
			frame.bottom), B_SOLID_LOW);

		owner->MovePenTo(x, frame.top + BaselineOffset());
		owner->DrawString(fRef.name);
	} else {
		// Text does match, fill groups in different colors
		int startIndex = 0;//fGroups.CountItems() == 1 ? 0 : 1;
		_DrawGroupedText(owner, frame, x, fRef.name, fGroups, startIndex);
	}

	owner->PopState();

	if (fError != NO_ERROR && HasTarget())
		owner->SetHighColor(200, 50, 50);

	if (fRenameGroups.IsEmpty()) {
		owner->MovePenTo(x + width, frame.top + BaselineOffset());
		owner->DrawString(fTarget);
	} else {
		_DrawGroupedText(owner, frame, x + width, fTarget.String(),
			fRenameGroups, 0);
	}

	if (fError != NO_ERROR && HasTarget()) {
		const char* errorText = ErrorMessage();
		float width = owner->StringWidth(errorText);

		owner->MovePenBy(10, 0);
		BPoint location = owner->PenLocation();
		owner->SetLowColor(255, 0, 0);
		owner->FillRect(BRect(location.x - 2, frame.top,
			location.x + width + 2, frame.bottom), B_SOLID_LOW);
		owner->SetHighColor(255, 255, 255);
		owner->DrawString(errorText);
	}

	owner->SetLowColor(lowColor);
	owner->SetHighColor(highColor);
}


/*static*/ int
PreviewItem::Compare(const void* _a, const void* _b)
{
	const PreviewItem* a = *static_cast<const PreviewItem* const*>(_a);
	const PreviewItem* b = *static_cast<const PreviewItem* const*>(_b);
	const entry_ref& refA = a->Ref();
	const entry_ref& refB = b->Ref();

	return strcasecmp(refA.name, refB.name);
}


void
PreviewItem::_DrawGroupedText(BView* owner, BRect frame, float x,
	const BString& text, const BObjectList<Group>& groups, int32 first)
{
	uint32 count = text.CountChars();
	BPoint sizes[count];
	be_plain_font->GetEscapements(text.String(), count, NULL, sizes);

	float fontSize = be_plain_font->Size();
	int32 byteIndex = 0;
	bool inGroup = false;
	uint32 groupIndex = first;
	const Group* group = groups.ItemAt(groupIndex);
	float startOffset = 0;
	float endOffset = 0;

	for (uint32 i = 0; i < count; i++) {
		int32 characterBytes = text.CountBytes(i, 1);
		float offset = endOffset;
		endOffset += sizes[i].x * fontSize;

		if (byteIndex == group->start) {
			startOffset = offset;
			inGroup = true;
		}

		byteIndex += characterBytes;

		if (byteIndex == group->end) {
			// End group
			if (inGroup) {
				_DrawGroup(owner, group->index, frame, x + startOffset,
					x + endOffset);
				inGroup = false;
			}

			// Select next group, if any
			if (++groupIndex == (uint32)groups.CountItems())
				break;
			group = groups.ItemAt(groupIndex);
		}
	}
	if (inGroup) {
		// End last group
		_DrawGroup(owner, group->index, frame, x + startOffset,
			x + endOffset);
	}

	owner->SetDrawingMode(B_OP_OVER);
	owner->MovePenTo(x, frame.top + BaselineOffset());
	owner->DrawString(text.String());
}


void
PreviewItem::_DrawGroup(BView* owner, uint32 groupIndex, BRect frame,
	float start, float end)
{
	owner->SetLowColor(kGroupColor[groupIndex % kGroupColorCount]);
	owner->FillRect(BRect(start, frame.top, end, frame.bottom), B_SOLID_LOW);
}


bool
PreviewItem::_IsValidName(const char* name) const
{
	// TODO: move could be optional
//	return strchr(name, '/') == NULL;
	return true;
}
