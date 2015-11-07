# Copyright 2015 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'targets': [
    {
      'target_name': 'weave_daemon_sample',
      'type': 'executable',
      'sources': [
        'sample.cc',
      ],
      'dependencies': [
        '<@(DEPTH)/libweave_standalone.gyp:libweave',
        '<@(DEPTH)/examples/provider/provider.gyp:libweave_provider',
      ]
    }
  ]
}
