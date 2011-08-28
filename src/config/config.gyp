# Copyright 2010-2011, Google Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

{
  'variables': {
    'relative_dir': 'config',
  },
  'targets': [
    {
      'target_name': 'config_handler',
      'type': 'static_library',
      'sources': [
        'config_handler.cc',
      ],
      'dependencies': [
        '../base/base.gyp:base',
        '../base/base.gyp:config_file_stream',
        'genproto_config',
        'config_protocol',
      ],
    },
    {
      'target_name': 'genproto_config',
      'type': 'none',
      'sources': [
        'config.proto',
      ],
      'includes': [
        '../protobuf/genproto.gypi',
      ],
    },
    {
      'target_name': 'config_protocol',
      'type': 'static_library',
      'sources': [
        '<(proto_out_dir)/<(relative_dir)/config.pb.cc',
      ],
      'dependencies': [
        '../protobuf/protobuf.gyp:protobuf',
        'genproto_config'
      ],
    },
    {
      'target_name': 'stats_config_util',
      'type': 'static_library',
      'sources': [
        'stats_config_util.cc',
      ],
      'dependencies': [
        'config_handler',
        'genproto_config'
      ],
    },
  ],
}
