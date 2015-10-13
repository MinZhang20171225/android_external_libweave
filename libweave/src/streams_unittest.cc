// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/streams.h"

#include <functional>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <weave/provider/test/fake_task_runner.h>

#include <src/bind_lambda.h>

namespace weave {

TEST(Stream, CopyStreams) {
  provider::test::FakeTaskRunner task_runner;
  std::vector<uint8_t> test_data(1024 * 1024);
  for (size_t i = 0; i < test_data.size(); ++i)
    test_data[i] = static_cast<uint8_t>(std::hash<size_t>()(i));
  MemoryStream source{test_data, &task_runner};
  MemoryStream destination{{}, &task_runner};

  bool done = false;

  auto on_success = base::Bind([&test_data, &done, &destination](size_t size) {
    done = true;
    EXPECT_EQ(test_data, destination.GetData());
  });
  auto on_error = base::Bind([](ErrorPtr error) { ADD_FAILURE(); });
  StreamCopier copier{&source, &destination};
  copier.Copy(on_success, on_error);

  task_runner.Run(test_data.size());
  EXPECT_TRUE(done);
}

}  // namespace weave
