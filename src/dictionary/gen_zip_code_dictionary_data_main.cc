// Copyright 2010, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Calculate scores and output dictionary for zip code.

#include <cmath>
#include <string>
#include <vector>

#include "base/base.h"
#include "base/util.h"
#include "base/file_stream.h"
#include "converter/pos_matcher.h"

DEFINE_string(input, "", "input seed file");
DEFINE_string(output, "", "output dictionary text file");

namespace {
const uint32 kOffset = 30000;
const uint32 kScoreMax = 32767;

uint32 GetScore(int64 freq) {
  if (freq <= 0) {
    return kOffset;
  }
  uint32 score = kOffset - log(static_cast<double>(freq));
  if (score > kScoreMax) {  // cost should be within 15 bits
    score = kScoreMax;
  }
  return score;
}
}  // namespace

int main(int argc, char **argv) {
  InitGoogle(argv[0], &argc, &argv, false);

  const uint16 zip_code_pos = mozc::POSMatcher::GetZipcodeId();

  mozc::InputFileStream ifs(FLAGS_input.c_str());
  CHECK(ifs);
  string line;
  vector<string> tokens;

  mozc::OutputFileStream ofs(FLAGS_output.c_str());
  CHECK(ofs);

  while (getline(ifs, line)) {
    if (line.size() <= 0 || line[0] == '#') {
      continue;
    }
    tokens.clear();
    mozc::Util::SplitStringUsing(line, "\t", &tokens);
    if (tokens.size() < 3) {
      LOG(ERROR) << "format error: " << line;
      continue;
    }
    const string &key = tokens[0];
    const string &value = tokens[1];
    const int64 freq = static_cast<int64>(strtod(tokens[2].c_str(), NULL));
    const uint32 score = GetScore(freq);

    ofs << key << "\t" << zip_code_pos << "\t" << zip_code_pos << "\t"
        << score << "\t" << value << endl;
  }
}
