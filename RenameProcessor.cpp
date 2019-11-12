/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */


#include "RenameProcessor.h"

#include <Entry.h>
#include <Node.h>
#include <Path.h>
#include <String.h>

#include <fs_attr.h>

#include <map>
#include <new>
#include <set>


RenameProcessor::RenameProcessor()
	:
	BLooper("Rename processor")
{
}


void
RenameProcessor::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgProcessAndCheckRename:
		{
			BMessage reply(kMsgProcessed);

			entry_ref ref;
			int32 index;
			for (index = 0; message->FindRef("source", index, &ref) == B_OK;
					index++) {
				BString target;
				if (message->FindString("target", index, &target) == B_OK) {
					_ProcessRef(reply, ref, target);
					if (!_CheckRef(ref, target))
						reply.AddRef("exists", &ref);
				}
			}

			message->SendReply(&reply);
			break;
		}

		default:
			BLooper::MessageReceived(message);
			break;
	}
}


//!	Evaluate expressions in the target name.
void
RenameProcessor::_ProcessRef(BMessage& update, const entry_ref& ref,
	const BString& target)
{
	int length = target.Length();
	const char* buffer = target.String();
	int diff = 0;

	for (int index = 0; index < length - 3; index++) {
		if (buffer[index] == '$') {
			char open = buffer[index + 1];
			BString expression;
			int expressionLength = -1;
			if (open == '(' || open == '[') {
				expressionLength = _Extract(buffer + index + 2,
					length - index - 2, open, expression);
			}

			BString result;
			bool changed = false;

			if (expressionLength > 0 && open == '[') {
				// shell script
				result = _ExecuteShell(ref, expression.String());
				changed = true;
			} else if (expressionLength > 0 && open == '(') {
				// Attribute replacement
				result = _ReadAttribute(ref, expression.String());
				changed = true;
			}

			if (changed) {
				update.AddRef("ref", &ref);
				update.AddInt32("from", index + diff);
				update.AddInt32("to", index + expressionLength + 3 + diff);
				update.AddString("replace", result);

				diff += result.Length() - expressionLength - 3;
				index += expressionLength + 2;
			}
		}
	}
}


bool
RenameProcessor::_CheckRef(const entry_ref& ref, const BString& target)
{
	entry_ref targetRef = ref;
	int32 index = target.FindFirst('/');
	if (index >= 0) {
		BPath path(&ref);
		if (path.InitCheck() != B_OK
			|| path.GetParent(&path) != B_OK
			|| path.Append(target) != B_OK
			|| get_ref_for_path(path.Path(), &targetRef) != B_OK)
			return true;
	} else
		targetRef.set_name(target.String());

	BEntry entry(&targetRef);
	return !entry.Exists();
}


BString
RenameProcessor::_ReadAttribute(const entry_ref& ref, const char* name)
{
	BNode node(&ref);
	status_t status = node.InitCheck();
	if (status != B_OK)
		return "";

	attr_info info;
	status = node.GetAttrInfo(name, &info);
	if (status != B_OK)
		return "";

	char buffer[1024];
	off_t size = info.size;
	if (size > sizeof(buffer))
		size = sizeof(buffer);

	ssize_t bytesRead = node.ReadAttr(name, info.type, 0, buffer, size);
	if (bytesRead != size)
		return "";

	// Taken over from Haiku's listattr.cpp
	BString result;
	switch (info.type) {
		case B_INT8_TYPE:
			result.SetToFormat("%" B_PRId8, *((int8 *)buffer));
			break;
		case B_UINT8_TYPE:
			result.SetToFormat("%" B_PRIu8, *((uint8 *)buffer));
			break;
		case B_INT16_TYPE:
			result.SetToFormat("%" B_PRId16, *((int16 *)buffer));
			break;
		case B_UINT16_TYPE:
			result.SetToFormat("%" B_PRIu16, *((uint16 *)buffer));
			break;
		case B_INT32_TYPE:
			result.SetToFormat("%" B_PRId32, *((int32 *)buffer));
			break;
		case B_UINT32_TYPE:
			result.SetToFormat("%" B_PRIu32, *((uint32 *)buffer));
			break;
		case B_INT64_TYPE:
			result.SetToFormat("%" B_PRId64, *((int64 *)buffer));
			break;
		case B_UINT64_TYPE:
			result.SetToFormat("%" B_PRIu64, *((uint64 *)buffer));
			break;
		case B_FLOAT_TYPE:
			result.SetToFormat("%f", *((float *)buffer));
			break;
		case B_DOUBLE_TYPE:
			result.SetToFormat("%f", *((double *)buffer));
			break;
		case B_BOOL_TYPE:
			result.SetToFormat("%d", *((unsigned char *)buffer));
			break;
		case B_TIME_TYPE:
		{
			char stringBuffer[256];
			struct tm timeInfo;
			localtime_r((time_t *)buffer, &timeInfo);
			strftime(stringBuffer, sizeof(stringBuffer), "%c", &timeInfo);
			result.SetToFormat("%s", stringBuffer);
			break;
		}
		case B_STRING_TYPE:
		case B_MIME_STRING_TYPE:
		case 'MSIG':
		case 'MSDC':
		case 'MPTH':
			result.SetToFormat("%s", buffer);
			break;
	}
	return result;
}


BString
RenameProcessor::_ExecuteShell(const entry_ref& ref, const char* script)
{
	BPath path(&ref);
	if (path.InitCheck() != B_OK)
		return "";

	BString output;

	setenv("file", path.Path(), true);

	BString scriptBuffer = script;
	scriptBuffer.ReplaceAll("'", "\'");

	BString commandBuffer = BString().SetToFormat("bash -c '%s'",
		scriptBuffer.String());

	FILE* pipe = popen(commandBuffer.String(), "r");
	if (pipe == NULL)
		return "";

	while (true) {
		char buffer[4096];
		const char* line = fgets(buffer, sizeof(buffer), pipe);
		if (line == NULL)
			break;

		output += line;

		// Cut off trailing newline
		if (output.ByteAt(output.Length() - 1) == '\n')
			output.Truncate(output.Length() - 1);
	}
	pclose(pipe);

	return output;
}


int
RenameProcessor::_Extract(const char* buffer, int length, char open,
	BString& expression)
{
	// Find end
	char close = open == '(' ? ')' : ']';
	int level = 0;
	for (int index = 0; index < length; index++) {
		if (buffer[index] == open)
			level++;
		else if (buffer[index] == close) {
			level--;
			if (level < 0) {
				// Extract expression
				expression.SetTo(buffer, index);
				return index;
			}
		}
	}

	return -1;
}
