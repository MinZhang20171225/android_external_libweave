// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BUFFET_STATES_MOCK_STATE_CHANGE_QUEUE_INTERFACE_H_
#define BUFFET_STATES_MOCK_STATE_CHANGE_QUEUE_INTERFACE_H_

#include <vector>

#include <gmock/gmock.h>

#include "buffet/states/state_change_queue_interface.h"

namespace buffet {

class MockStateChangeQueueInterface : public StateChangeQueueInterface {
 public:
  MOCK_CONST_METHOD0(IsEmpty, bool());
  MOCK_METHOD1(NotifyPropertiesUpdated, bool(const StateChange&));
  MOCK_METHOD0(GetAndClearRecordedStateChanges, std::vector<StateChange>());
};

}  // namespace buffet

#endif  // BUFFET_STATES_MOCK_STATE_CHANGE_QUEUE_INTERFACE_H_