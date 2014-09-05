// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "buffet/commands/command_instance.h"

#include <base/values.h>
#include <chromeos/errors/error.h>
#include <chromeos/errors/error_codes.h>

#include "buffet/commands/command_definition.h"
#include "buffet/commands/command_dictionary.h"
#include "buffet/commands/command_proxy_interface.h"
#include "buffet/commands/command_queue.h"
#include "buffet/commands/schema_constants.h"
#include "buffet/commands/schema_utils.h"

namespace buffet {

const char CommandInstance::kStatusQueued[] = "queued";
const char CommandInstance::kStatusInProgress[] = "inProgress";
const char CommandInstance::kStatusPaused[] = "paused";
const char CommandInstance::kStatusError[] = "error";
const char CommandInstance::kStatusDone[] = "done";
const char CommandInstance::kStatusCanceled[] = "canceled";
const char CommandInstance::kStatusAborted[] = "aborted";
const char CommandInstance::kStatusExpired[] = "expired";

CommandInstance::CommandInstance(const std::string& name,
                                 const std::string& category,
                                 const native_types::Object& parameters)
    : name_(name), category_(category), parameters_(parameters) {
}

std::shared_ptr<const PropValue> CommandInstance::FindParameter(
    const std::string& name) const {
  std::shared_ptr<const PropValue> value;
  auto p = parameters_.find(name);
  if (p != parameters_.end())
    value = p->second;
  return value;
}

namespace {

// Helper method to retrieve command parameters from the command definition
// object passed in as |json| and corresponding command definition schema
// specified in |command_def|.
// On success, returns |true| and the validated parameters and values through
// |parameters|. Otherwise returns |false| and additional error information in
// |error|.
bool GetCommandParameters(const base::DictionaryValue* json,
                          const CommandDefinition* command_def,
                          native_types::Object* parameters,
                          chromeos::ErrorPtr* error) {
  // Get the command parameters from 'parameters' property.
  base::DictionaryValue no_params;  // Placeholder when no params are specified.
  const base::DictionaryValue* params = nullptr;
  const base::Value* params_value = nullptr;
  if (json->GetWithoutPathExpansion(commands::attributes::kCommand_Parameters,
                                    &params_value)) {
    // Make sure the "parameters" property is actually an object.
    if (!params_value->GetAsDictionary(&params)) {
      chromeos::Error::AddToPrintf(error, chromeos::errors::json::kDomain,
                                   chromeos::errors::json::kObjectExpected,
                                   "Property '%s' must be a JSON object",
                                   commands::attributes::kCommand_Parameters);
      return false;
    }
  } else {
    // "parameters" are not specified. Assume empty param list.
    params = &no_params;
  }

  // Now read in the parameters and validate their values against the command
  // definition schema.
  if (!TypedValueFromJson(params, command_def->GetParameters().get(),
                          parameters, error)) {
    return false;
  }
  return true;
}

}  // anonymous namespace

std::unique_ptr<CommandInstance> CommandInstance::FromJson(
    const base::Value* value,
    const CommandDictionary& dictionary,
    chromeos::ErrorPtr* error) {
  std::unique_ptr<CommandInstance> instance;
  // Get the command JSON object from the value.
  const base::DictionaryValue* json = nullptr;
  if (!value->GetAsDictionary(&json)) {
    chromeos::Error::AddTo(error, chromeos::errors::json::kDomain,
                           chromeos::errors::json::kObjectExpected,
                           "Command instance is not a JSON object");
    return instance;
  }

  // Get the command name from 'name' property.
  std::string command_name;
  if (!json->GetStringWithoutPathExpansion(commands::attributes::kCommand_Name,
                                           &command_name)) {
    chromeos::Error::AddTo(error, errors::commands::kDomain,
                           errors::commands::kPropertyMissing,
                           "Command name is missing");
    return instance;
  }
  // Make sure we know how to handle the command with this name.
  const CommandDefinition* command_def = dictionary.FindCommand(command_name);
  if (!command_def) {
    chromeos::Error::AddToPrintf(error, errors::commands::kDomain,
                                 errors::commands::kInvalidCommandName,
                                 "Unknown command received: %s",
                                 command_name.c_str());
    return instance;
  }

  native_types::Object parameters;
  if (!GetCommandParameters(json, command_def, &parameters, error)) {
    chromeos::Error::AddToPrintf(error, errors::commands::kDomain,
                                 errors::commands::kCommandFailed,
                                 "Failed to validate command '%s'",
                                 command_name.c_str());
    return instance;
  }

  instance.reset(new CommandInstance(command_name,
                                     command_def->GetCategory(),
                                     parameters));
  return instance;
}

bool CommandInstance::SetProgress(int progress) {
  if (progress < 0 || progress > 100)
    return false;
  if (progress != progress_) {
    progress_ = progress;
    SetStatus(kStatusInProgress);
    if (proxy_)
      proxy_->OnProgressChanged(progress_);
  }
  return true;
}

void CommandInstance::Abort() {
  SetStatus(kStatusAborted);
  RemoveFromQueue();
  // The command will be destroyed after that, so do not access any members.
}

void CommandInstance::Cancel() {
  SetStatus(kStatusCanceled);
  RemoveFromQueue();
  // The command will be destroyed after that, so do not access any members.
}

void CommandInstance::Done() {
  SetProgress(100);
  SetStatus(kStatusDone);
  RemoveFromQueue();
  // The command will be destroyed after that, so do not access any members.
}

void CommandInstance::SetStatus(const std::string& status) {
  if (status != status_) {
    status_ = status;
    if (proxy_)
      proxy_->OnStatusChanged(status_);
  }
}

void CommandInstance::RemoveFromQueue() {
  if (queue_) {
    // Store this instance in unique_ptr until the end of this method,
    // otherwise it will be destroyed as soon as CommandQueue::Remove() returns.
    std::unique_ptr<CommandInstance> this_instance = queue_->Remove(GetID());
    // The command instance will survive till the end of this scope.
  }
}


}  // namespace buffet