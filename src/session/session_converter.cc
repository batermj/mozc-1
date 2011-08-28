// Copyright 2010-2011, Google Inc.
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

// A class handling the converter on the session layer.

#include "session/session_converter.h"

#include <vector>

#include "base/text_normalizer.h"
#include "base/util.h"
#include "converter/converter_interface.h"
#include "converter/segments.h"
#include "composer/composer.h"
#include "session/internal/candidate_list.h"
#include "session/internal/session_output.h"
#include "transliteration/transliteration.h"

namespace mozc {
namespace session {

namespace {
const size_t kDefaultMaxHistorySize = 3;
}

SessionConverter::SessionConverter(const ConverterInterface *converter)
    : SessionConverterInterface(),
      state_(COMPOSITION),
      composer_(NULL),
      converter_(converter),
      segments_(new Segments),
      segment_index_(0),
      candidate_list_(new CandidateList(true)),
      candidate_list_visible_(false) {
  conversion_preferences_.use_history = true;
  conversion_preferences_.max_history_size = kDefaultMaxHistorySize;
  operation_preferences_.use_cascading_window = true;
  operation_preferences_.candidate_shortcuts.clear();
}

SessionConverter::~SessionConverter() {}

void SessionConverter::SetOperationPreferences(
    const OperationPreferences &preferences) {
  operation_preferences_.use_cascading_window =
      preferences.use_cascading_window;
  operation_preferences_.candidate_shortcuts =
      preferences.candidate_shortcuts;
}

bool SessionConverter::CheckState(
    SessionConverterInterface::States states) const {
  return ((state_ & states) != NO_STATE);
}

bool SessionConverter::IsActive() const {
  return CheckState(SUGGESTION | PREDICTION | CONVERSION);
}

const ConversionPreferences &SessionConverter::conversion_preferences() const {
  return conversion_preferences_;
}

bool SessionConverter::Convert(const composer::Composer *composer) {
  return ConvertWithPreferences(composer, conversion_preferences_);
}

bool SessionConverter::ConvertWithPreferences(
    const composer::Composer *composer,
    const ConversionPreferences &preferences) {
  DCHECK(CheckState(COMPOSITION | SUGGESTION | CONVERSION));
  if (composer == NULL) {
    LOG(ERROR) << "Composer is NULL";
    return false;
  }
  composer_ = composer;

  segments_->set_request_type(Segments::CONVERSION);
  SetConversionPreferences(preferences, segments_.get());

  if (!converter_->StartConversionWithComposer(segments_.get(), composer_)) {
    LOG(WARNING) << "StartConversionWithComposer() failed";
    return false;
  }

  segment_index_ = 0;
  state_ = CONVERSION;
  candidate_list_visible_ = false;
  UpdateCandidateList();
  GetPreeditAndConversion(0, segments_->conversion_segments_size(),
                          &composition_, &default_result_);
  return true;
}

bool SessionConverter::ConvertReverse(const string &source_text,
                                      composer::Composer *composer) {
  Segments reverse_segments;
  if (!converter_->StartReverseConversion(&reverse_segments, source_text)) {
    return false;
  }
  if (reverse_segments.segments_size() == 0) {
    LOG(WARNING) << "no segments from reverse conversion";
    return false;
  }
  string reading;
  for (int i = 0; i < reverse_segments.segments_size(); ++i) {
    const mozc::Segment &segment = reverse_segments.segment(i);
    if (segment.candidates_size() == 0) {
      LOG(WARNING) << "got an empty segment from reverse conversion";
      return false;
    }
    reading.append(segment.candidate(0).value);
  }

  composer->Reset();
  vector<string> reading_characters;
  // TODO(hsumita): Currently, InsertCharacterPreedit can't deal multiple
  // characters at the same time.
  // So we should split query to UTF-8 chars to avoid DCHECK failure.
  // http://b/3437358
  //
  // See also http://b/5094684, http://b/5094642
  Util::SplitStringToUtf8Chars(reading, &reading_characters);
  for (size_t i = 0; i < reading_characters.size(); ++i) {
    composer->InsertCharacterPreedit(reading_characters[i]);
  }
  composer->set_source_text(source_text);
  // start conversion here.
  if (!Convert(composer)) {
    LOG(ERROR) << "Failed to start conversion for reverse conversion";
    return false;
  }
  return true;
}

namespace {
Attributes GetT13nAttributes(const transliteration::TransliterationType type) {
  Attributes attributes = NO_ATTRIBUTES;
  switch (type) {
    case transliteration::HIRAGANA:  // "ひらがな"
      attributes = HIRAGANA;
      break;
    case transliteration::FULL_KATAKANA:  // "カタカナ"
      attributes = (FULL_WIDTH | KATAKANA);
      break;
    case transliteration::HALF_ASCII:  // "ascII"
      attributes = (HALF_WIDTH | ASCII);
      break;
    case transliteration::HALF_ASCII_UPPER:  // "ASCII"
      attributes = (HALF_WIDTH | ASCII | UPPER);
      break;
    case transliteration::HALF_ASCII_LOWER:  // "ascii"
      attributes = (HALF_WIDTH | ASCII | LOWER);
      break;
    case transliteration::HALF_ASCII_CAPITALIZED:  // "Ascii"
      attributes = (HALF_WIDTH | ASCII | CAPITALIZED);
      break;
    case transliteration::FULL_ASCII:  // "ａｓｃＩＩ"
      attributes = (FULL_WIDTH | ASCII);
      break;
    case transliteration::FULL_ASCII_UPPER:  // "ＡＳＣＩＩ"
      attributes = (FULL_WIDTH | ASCII | UPPER);
      break;
    case transliteration::FULL_ASCII_LOWER:  // "ａｓｃｉｉ"
      attributes = (FULL_WIDTH | ASCII | LOWER);
      break;
    case transliteration::FULL_ASCII_CAPITALIZED:  // "Ａｓｃｉｉ"
      attributes = (FULL_WIDTH | ASCII | CAPITALIZED);
      break;
    case transliteration::HALF_KATAKANA:  // "ｶﾀｶﾅ"
      attributes = (HALF_WIDTH | KATAKANA);
      break;
    default:
      LOG(ERROR) << "Unknown type: " << type;
      break;
  }
  return attributes;
}
}  // namespace

bool SessionConverter::ConvertToTransliteration(
    const composer::Composer *composer,
    const transliteration::TransliterationType type) {
  DCHECK(CheckState(COMPOSITION | SUGGESTION | PREDICTION | CONVERSION));
  if (CheckState(PREDICTION)) {
    // TODO(komatsu): A better way is to transliterate the key of the
    // focused candidate.  However it takes a long time.
    Cancel();
    DCHECK(CheckState(COMPOSITION));
  }

  Attributes query_attr =
      (GetT13nAttributes(type) &
       (HALF_WIDTH | FULL_WIDTH | ASCII | HIRAGANA | KATAKANA));

  if (CheckState(COMPOSITION | SUGGESTION)) {
    if (!Convert(composer)) {
      LOG(ERROR) << "Conversion failed";
      return false;
    }

    // TODO(komatsu): This is a workaround to transliterate the whole
    // preedit as a single segment.  We should modify
    // converter/converter.cc to enable to accept mozc::Segment::FIXED
    // from the session layer.
    if (segments_->conversion_segments_size() != 1) {
      converter_->ResizeSegment(segments_.get(), 0,
                                Util::CharsLen(composition_));
      UpdateCandidateList();
    }

    DCHECK(CheckState(CONVERSION));
    candidate_list_->MoveToAttributes(query_attr);
  } else {
    DCHECK(CheckState(CONVERSION));
    const Attributes current_attr =
        candidate_list_->GetDeepestFocusedCandidate().attributes();

    if ((query_attr & current_attr & ASCII) &&
        ((((query_attr & HALF_WIDTH) && (current_attr & FULL_WIDTH))) ||
         (((query_attr & FULL_WIDTH) && (current_attr & HALF_WIDTH))))) {
      query_attr |= (current_attr & (UPPER | LOWER | CAPITALIZED));
    }

    candidate_list_->MoveNextAttributes(query_attr);
  }
  candidate_list_visible_ = false;
  SegmentFocus();
  return true;
}

bool SessionConverter::ConvertToHalfWidth(const composer::Composer *composer) {
  DCHECK(CheckState(COMPOSITION | SUGGESTION | PREDICTION | CONVERSION));
  if (CheckState(PREDICTION)) {
    // TODO(komatsu): A better way is to transliterate the key of the
    // focused candidate.  However it takes a long time.
    Cancel();
    DCHECK(CheckState(COMPOSITION));
  }

  const string *composition = NULL;
  if (CheckState(COMPOSITION | SUGGESTION)) {
    if (!Convert(composer)) {
      LOG(ERROR) << "Conversion failed";
      return false;
    }
    // TODO(komatsu): This is a workaround to transliterate the whole
    // preedit as a single segment.  We should modify
    // converter/converter.cc to enable to accept mozc::Segment::FIXED
    // from the session layer.
    if (segments_->conversion_segments_size() != 1) {
      converter_->ResizeSegment(segments_.get(), 0,
                                Util::CharsLen(composition_));
      UpdateCandidateList();
    }

    composition = &composition_;
  } else {
    composition = &(GetSelectedCandidate(segment_index_).value);
  }

  DCHECK(CheckState(CONVERSION));
  Attributes attributes = HALF_WIDTH;
  // TODO(komatsu): make a function to return a logical sum of ScriptType.
  // If composition_ is "あｂｃ", it should be treated as Katakana.
  if (Util::ContainsScriptType(*composition, Util::KATAKANA) ||
      Util::ContainsScriptType(*composition, Util::HIRAGANA) ||
      Util::ContainsScriptType(*composition, Util::KANJI) ||
      Util::IsKanaSymbolContained(*composition)
  ) {
    attributes |= KATAKANA;
  } else {
    attributes |= ASCII;
    attributes |= (candidate_list_->GetDeepestFocusedCandidate().attributes() &
                   (UPPER | LOWER | CAPITALIZED));
  }
  candidate_list_->MoveNextAttributes(attributes);
  candidate_list_visible_ = false;
  SegmentFocus();
  return true;
}

bool SessionConverter::SwitchKanaType(const composer::Composer *composer) {
  DCHECK(CheckState(COMPOSITION | SUGGESTION | PREDICTION | CONVERSION));
  if (CheckState(PREDICTION)) {
    // TODO(komatsu): A better way is to transliterate the key of the
    // focused candidate.  However it takes a long time.
    Cancel();
    DCHECK(CheckState(COMPOSITION));
  }

  Attributes attributes = NO_ATTRIBUTES;
  if (CheckState(COMPOSITION | SUGGESTION)) {
    if (!Convert(composer)) {
      LOG(ERROR) << "Conversion failed";
      return false;
    }

    // TODO(komatsu): This is a workaround to transliterate the whole
    // preedit as a single segment.  We should modify
    // converter/converter.cc to enable to accept mozc::Segment::FIXED
    // from the session layer.
    if (segments_->conversion_segments_size() != 1) {
      converter_->ResizeSegment(segments_.get(), 0,
                                Util::CharsLen(composition_));
      UpdateCandidateList();
    }

    attributes = (FULL_WIDTH | KATAKANA);
  } else {
    const Attributes current_attributes =
        candidate_list_->GetDeepestFocusedCandidate().attributes();
    // "漢字" -> "かんじ" -> "カンジ" -> "ｶﾝｼﾞ" -> "かんじ" -> ...
    if (current_attributes & HIRAGANA) {
      attributes = (FULL_WIDTH | KATAKANA);
    } else if ((current_attributes & KATAKANA) &&
               (current_attributes & FULL_WIDTH)) {
      attributes = (HALF_WIDTH | KATAKANA);
    } else {
      attributes = HIRAGANA;
    }
  }

  DCHECK(CheckState(CONVERSION));
  candidate_list_->MoveNextAttributes(attributes);
  candidate_list_visible_ = false;
  SegmentFocus();
  return true;
}

namespace {

// Prepend the candidates to the first conversion segment.
void PrependCandidates(const Segment &previous_segment,
                       const string &preedit,
                       Segments *segments) {
  DCHECK(segments);

  // TODO(taku) want to have a method in converter to make an empty segment
  if (segments->conversion_segments_size() == 0) {
    segments->clear_conversion_segments();
    Segment *segment = segments->add_segment();
    segment->Clear();
    segment->set_key(preedit);
  }

  DCHECK_EQ(1, segments->conversion_segments_size());
  Segment *segment = segments->mutable_conversion_segment(0);
  DCHECK(segment);

  const size_t cands_size = previous_segment.candidates_size();
  for (size_t i = 0; i < cands_size; ++i) {
    Segment::Candidate *candidate = segment->push_front_candidate();
    *candidate = previous_segment.candidate(cands_size - i - 1);  // copy
  }
  *(segment->mutable_meta_candidates()) = previous_segment.meta_candidates();
}
}  // namespace


bool SessionConverter::Suggest(const composer::Composer *composer) {
  return SuggestWithPreferences(composer, conversion_preferences_);
}

bool SessionConverter::SuggestWithPreferences(
    const composer::Composer *composer,
    const ConversionPreferences &preferences) {
  DCHECK(CheckState(COMPOSITION | SUGGESTION));
  candidate_list_visible_ = false;
  if (composer == NULL) {
    LOG(ERROR) << "Composer is NULL";
    return false;
  }

  // Normalize the current state by resetting the previous state.
  ResetState();
  composer_ = composer;

  // Initialize the segments for suggestion.
  segments_->set_request_type(Segments::SUGGESTION);
  SetConversionPreferences(preferences, segments_.get());

  string preedit;
  composer_->GetQueryForPrediction(&preedit);
  if (!converter_->StartSuggestion(segments_.get(), preedit)) {
    // TODO(komatsu): Because suggestion is a prefix search, once
    // StartSuggestion returns false, this GetSuggestion always
    // returns false.  Refactor it.
    VLOG(1) << "StartSuggestion() returns no suggestions.";

    // Clear segments and keep the context
    converter_->CancelConversion(segments_.get());
    return false;
  }
  DCHECK_EQ(1, segments_->conversion_segments_size());

  // Copy current suggestions so that we can merge
  // prediction/suggestions later
  previous_suggestions_.CopyFrom(segments_->conversion_segment(0));

  // TODO(komatsu): the next line can be deleted.
  segment_index_ = 0;
  state_ = SUGGESTION;
  UpdateCandidateList();
  candidate_list_visible_ = true;
  return true;
}


bool SessionConverter::Predict(const composer::Composer *composer) {
  return PredictWithPreferences(composer, conversion_preferences_);
}

bool SessionConverter::IsEmptySegment(const Segment &segment) const {
  return ((segment.candidates_size() == 0) &&
          (segment.meta_candidates_size() == 0));
}

bool SessionConverter::PredictWithPreferences(
    const composer::Composer *composer,
    const ConversionPreferences &preferences) {
  // TODO(komatsu): DCHECK should be
  // DCHECK(CheckState(COMPOSITION | SUGGESTION | PREDICTION));
  DCHECK(CheckState(COMPOSITION | SUGGESTION | CONVERSION | PREDICTION));
  ResetResult();

  if (composer == NULL) {
    LOG(ERROR) << "Composer is NULL";
    return false;
  }
  composer_ = composer;

  // Initialize the segments for prediction
  segments_->set_request_type(Segments::PREDICTION);
  SetConversionPreferences(preferences, segments_.get());

  const bool predict_first =
      !CheckState(PREDICTION) && IsEmptySegment(previous_suggestions_);

  const bool predict_expand =
      (CheckState(PREDICTION) &&
       !IsEmptySegment(previous_suggestions_) &&
       candidate_list_->size() > 0 &&
       candidate_list_->focused() &&
       candidate_list_->focused_index() == candidate_list_->last_index());

  string preedit;
  composer_->GetQueryForPrediction(&preedit);
  segments_->clear_conversion_segments();

  if (predict_expand || predict_first) {
    if (!converter_->StartPrediction(segments_.get(), preedit)) {
      LOG(WARNING) << "StartPrediction() failed";

      // TODO(komatsu): Perform refactoring after checking the stability test.
      //
      // If predict_expand is true, it means we have prevous_suggestions_.
      // So we can use it as the result of this prediction.
      if (predict_first) {
        ResetState();
        composer_ = composer;
        return false;
      }
    }
  }

  // Merge suggestions and prediction
  PrependCandidates(previous_suggestions_, preedit, segments_.get());

  segment_index_ = 0;
  state_ = PREDICTION;
  UpdateCandidateList();
  candidate_list_visible_ = true;
  GetPreeditAndConversion(0, segments_->conversion_segments_size(),
                          &composition_, &default_result_);

  return true;
}

void SessionConverter::MaybeExpandPrediction() {
  DCHECK(CheckState(PREDICTION | CONVERSION));

  // Expand the current suggestions and fill with Prediction results.
  if (!CheckState(PREDICTION) ||
      IsEmptySegment(previous_suggestions_) ||
      !candidate_list_->focused() ||
      candidate_list_->focused_index() != candidate_list_->last_index()) {
    return;
  }

  DCHECK(CheckState(PREDICTION));
  ResetResult();

  const int previous_index = static_cast<int>(candidate_list_->focused_index());
  PredictWithPreferences(composer_, conversion_preferences_);

  // Move to the last suggestion if expand mode
  if (previous_index >= 0) {
    candidate_list_->MoveToId(candidate_list_->candidate(previous_index).id());
  }
}

void SessionConverter::Cancel() {
  DCHECK(CheckState(PREDICTION | CONVERSION));
  ResetResult();

  // Clear segments and keep the context
  converter_->CancelConversion(segments_.get());
  ResetState();
}

void SessionConverter::Reset() {
  DCHECK(CheckState(COMPOSITION | SUGGESTION | PREDICTION | CONVERSION));

  // Even if composition mode, call ResetConversion
  // in order to clear history segments. If the current conversion
  // segments are not empty, don't call ResetConversion just in case.
  if (segments_->conversion_segments_size() == 0) {
    converter_->ResetConversion(segments_.get());
  }

  if (CheckState(COMPOSITION)) {
    return;
  }

  ResetResult();
  // Reset segments and context
  ResetState();
}

void SessionConverter::Commit() {
  DCHECK(CheckState(PREDICTION | CONVERSION));
  ResetResult();
  UpdateResult(0, segments_->conversion_segments_size());

  for (size_t i = 0; i < segments_->conversion_segments_size(); ++i) {
    converter_->CommitSegmentValue(segments_.get(),
                                   i,
                                   GetCandidateIndexForConverter(i));
  }
  converter_->FinishConversion(segments_.get());
  ResetState();
}

void SessionConverter::CommitSuggestion(const size_t index) {
  DCHECK(CheckState(SUGGESTION));
  if (index >= candidate_list_->size()) {
    LOG(ERROR) << "index is out of the range: " << index;
    return;
  }

  ResetResult();
  candidate_list_->MoveToPageIndex(index);
  UpdateResult(0, segments_->conversion_segments_size());
  converter_->FinishConversion(segments_.get());
  ResetState();
}

void SessionConverter::CommitFirstSegment(composer::Composer *composer) {
  DCHECK(CheckState(PREDICTION | CONVERSION));
  DCHECK_EQ(composer, composer_);
  ResetResult();
  candidate_list_visible_ = false;

  // If the number of segments is one, just call Commit.
  if (segments_->conversion_segments_size() == 1) {
    Commit();
    return;
  }

  // Store the first conversion segment to the result.
  UpdateResult(0, 1);

  // Get the first conversion segment and the selected candidate.
  Segment *first_segment = segments_->mutable_conversion_segment(0);
  if (first_segment == NULL) {
    LOG(ERROR) << "There is no segment.";
    return;
  }

  // Delete the key characters of the first segment from the preedit.
  for (size_t i = 0; i < Util::CharsLen(first_segment->key()); ++i) {
    composer->DeleteAt(0);
  }
  // The number of segments should be more than one.
  DCHECK_GT(composer->GetLength(), 0);

  // Adjust the segment_index, since the first segment disappeared.
  if (segment_index_ > 0) {
    --segment_index_;
  }

  // Commit the first conversion segment only.
  converter_->SubmitFirstSegment(segments_.get(),
                                 candidate_list_->focused_id());
  UpdateCandidateList();
}

void SessionConverter::CommitPreedit(const composer::Composer &composer) {
  // If the conversion is active, composer should be equal to composer_.
  DCHECK(!IsActive() || &composer == composer_);

  string key, preedit, normalized_preedit;
  composer.GetQueryForConversion(&key);
  composer.GetStringForSubmission(&preedit);
  TextNormalizer::NormalizePreeditText(preedit, &normalized_preedit);
  SessionOutput::FillPreeditResult(preedit, &result_);

  ConverterUtil::InitSegmentsFromString(key, normalized_preedit,
                                        segments_.get());

  converter_->FinishConversion(segments_.get());
  ResetState();
}

void SessionConverter::CommitHead(
    size_t count, composer::Composer *composer) {
  // If the conversion is active, composer should be equal to composer_.
  DCHECK(!IsActive() || composer == composer_);

  string preedit, normalized_preedit;
  composer->GetStringForSubmission(&preedit);
  if (count > preedit.length()) {
    count = preedit.length();
  }
  preedit = Util::SubString(preedit, 0, count);
  TextNormalizer::NormalizePreeditText(preedit, &normalized_preedit);
  SessionOutput::FillPreeditResult(normalized_preedit, &result_);
  for (size_t i = 0; i < count; ++i) {
    composer->DeleteAt(0);
  }
}

void SessionConverter::Revert() {
  converter_->RevertConversion(segments_.get());
}

void SessionConverter::SegmentFocusRight() {
  DCHECK(CheckState(PREDICTION | CONVERSION));
  candidate_list_visible_ = false;
  if (CheckState(PREDICTION)) {
    return;  // Do nothing.
  }
  ResetResult();
  SegmentFix();

  if (segment_index_ + 1 >= segments_->conversion_segments_size()) {
    // If |segment_index_| is at the tail of the segments,
    // focus on the head.
    segment_index_ = 0;
  } else {
    ++segment_index_;
  }
  UpdateCandidateList();
}

void SessionConverter::SegmentFocusLast() {
  DCHECK(CheckState(PREDICTION | CONVERSION));
  candidate_list_visible_ = false;
  if (CheckState(PREDICTION)) {
    return;  // Do nothing.
  }
  ResetResult();

  const size_t r_edge = segments_->conversion_segments_size() - 1;
  if (segment_index_ >= r_edge) {
    return;
  }

  SegmentFix();
  segment_index_ = r_edge;
  UpdateCandidateList();
}

void SessionConverter::SegmentFocusLeft() {
  DCHECK(CheckState(PREDICTION | CONVERSION));
  candidate_list_visible_ = false;
  if (CheckState(PREDICTION)) {
    return;  // Do nothing.
  }
  ResetResult();
  SegmentFix();

  if (segment_index_ <= 0) {
    // If |segment_index_| is at the head of the segments,
    // focus on the tail.
    segment_index_ = segments_->conversion_segments_size() - 1;
  } else {
    --segment_index_;
  }

  UpdateCandidateList();
}

void SessionConverter::SegmentFocusLeftEdge() {
  DCHECK(CheckState(PREDICTION | CONVERSION));
  candidate_list_visible_ = false;
  if (CheckState(PREDICTION)) {
    return;  // Do nothing.
  }
  ResetResult();

  if (segment_index_ <= 0) {
    return;
  }

  SegmentFix();
  segment_index_ = 0;
  UpdateCandidateList();
}

bool SessionConverter::IsLastSegmentFocused() const {
  return (segment_index_ + 1 >= segments_->conversion_segments_size());
}

void SessionConverter::SegmentWidthExpand() {
  DCHECK(CheckState(PREDICTION | CONVERSION));
  candidate_list_visible_ = false;
  if (CheckState(PREDICTION)) {
    return;  // Do nothing.
  }
  ResetResult();

  if (!converter_->ResizeSegment(segments_.get(), segment_index_, 1)) {
    return;
  }

  UpdateCandidateList();
}

void SessionConverter::SegmentWidthShrink() {
  DCHECK(CheckState(PREDICTION | CONVERSION));
  candidate_list_visible_ = false;
  if (CheckState(PREDICTION)) {
    return;  // Do nothing.
  }
  ResetResult();

  if (!converter_->ResizeSegment(segments_.get(), segment_index_, -1)) {
    return;
  }

  UpdateCandidateList();
}

void SessionConverter::CandidateNext() {
  DCHECK(CheckState(PREDICTION | CONVERSION));
  ResetResult();

  MaybeExpandPrediction();
  candidate_list_->MoveNext();
  candidate_list_visible_ = true;
  SegmentFocus();
}

void SessionConverter::CandidateNextPage() {
  DCHECK(CheckState(PREDICTION | CONVERSION));
  ResetResult();

  candidate_list_->MoveNextPage();
  candidate_list_visible_ = true;
  SegmentFocus();
}

void SessionConverter::CandidatePrev() {
  DCHECK(CheckState(PREDICTION | CONVERSION));
  ResetResult();

  candidate_list_->MovePrev();
  candidate_list_visible_ = true;
  SegmentFocus();
}

void SessionConverter::CandidatePrevPage() {
  DCHECK(CheckState(PREDICTION | CONVERSION));
  ResetResult();

  candidate_list_->MovePrevPage();
  candidate_list_visible_ = true;
  SegmentFocus();
}

void SessionConverter::CandidateMoveToId(const int id) {
  DCHECK(CheckState(SUGGESTION | PREDICTION | CONVERSION));
  ResetResult();

  if (CheckState(SUGGESTION)) {
    Predict(composer_);
  }
  DCHECK(CheckState(PREDICTION | CONVERSION));

  candidate_list_->MoveToId(id);
  candidate_list_visible_ = false;
  SegmentFocus();
}

void SessionConverter::CandidateMoveToPageIndex(const size_t index) {
  DCHECK(CheckState(PREDICTION | CONVERSION));
  ResetResult();

  candidate_list_->MoveToPageIndex(index);
  candidate_list_visible_ = false;
  SegmentFocus();
}

bool SessionConverter::CandidateMoveToShortcut(const char shortcut) {
  DCHECK(CheckState(PREDICTION | CONVERSION));

  if (!candidate_list_visible_) {
    VLOG(1) << "Candidate list is not displayed.";
    return false;
  }

  const string &shortcuts = operation_preferences_.candidate_shortcuts;
  if (shortcuts.empty()) {
    VLOG(1) << "No shortcuts";
    return false;
  }

  // Check if the input character is in the shortcut.
  // TODO(komatsu): Support non ASCII characters such as Unicode and
  // special keys.
  const string::size_type index = shortcuts.find(shortcut);
  if (index == string::npos) {
    VLOG(1) << "shortcut is not a member of shortcuts.";
    return false;
  }

  if (!candidate_list_->MoveToPageIndex(index)) {
    VLOG(1) << "shortcut is out of the range.";
    return false;
  }
  ResetResult();
  SegmentFocus();
  return true;
}

bool SessionConverter::IsCandidateListVisible() const {
  return candidate_list_visible_;
}

void SessionConverter::SetCandidateListVisible(bool visible) {
  candidate_list_visible_ = visible;
}

void SessionConverter::PopOutput(commands::Output *output) {
  FillOutput(output);
  ResetResult();
}

void SessionConverter::FillOutput(commands::Output *output) const {
  if (output == NULL) {
    LOG(ERROR) << "output is NULL.";
    return;
  }
  if (result_.has_value()) {
    FillResult(output->mutable_result());
  }
  if (CheckState(COMPOSITION)) {
    if (composer_ && !composer_->Empty()) {
      session::SessionOutput::FillPreedit(*composer_,
                                          output->mutable_preedit());
    }
  }
  if (!IsActive()) {
    return;
  }

  // Composition on Suggestion
  if (CheckState(SUGGESTION)) {
    DCHECK(composer_);
    // When the suggestion comes from zero query suggestion, the
    // composer is empty.  In that case, preedit is not rendered.
    if (!composer_->Empty()) {
      session::SessionOutput::FillPreedit(*composer_,
                                          output->mutable_preedit());
    }
  } else if (CheckState(PREDICTION | CONVERSION)) {
    // Conversion on Prediction or Conversion
    FillConversion(output->mutable_preedit());
  }
  // Candidate list
  if (CheckState(SUGGESTION | PREDICTION | CONVERSION) &&
      candidate_list_visible_) {
    FillCandidates(output->mutable_candidates());
  }
  // All candidate words
  if (CheckState(SUGGESTION | PREDICTION | CONVERSION)) {
    FillAllCandidateWords(output->mutable_all_candidate_words());
  }
}

void SessionConverter::FillContext(commands::Context *context) const {
  if (context->has_preceding_text()) {
    // Client has set the information of surrounding text, so do nothing.
    return;
  }
  if (segments_->history_segments_size() == 0) {
    return;
  }

  // Set preceding text using history segments.
  string *preceding_text = context->mutable_preceding_text();
  for (size_t i = 0; i < segments_->history_segments_size(); ++i) {
    *preceding_text += segments_->history_segment(i).candidate(0).value;
  }
}

void SessionConverter::GetSegments(Segments *dest) const {
  DCHECK(dest);
  dest->Clear();
  dest->CopyFrom(*segments_.get());
}

void SessionConverter::SetSegments(const Segments &src) {
  segments_->Clear();
  segments_->CopyFrom(src);
}

void SessionConverter::RemoveTailOfHistorySegments(size_t num_of_characters) {
  segments_->RemoveTailOfHistorySegments(num_of_characters);
}

const commands::Result &SessionConverter::GetResult() const {
  return result_;
}

const string &SessionConverter::GetDefaultResult() const {
  return default_result_;
}

const string &SessionConverter::GetComposition() const {
  return composition_;
}

const CandidateList &SessionConverter::GetCandidateList() const {
  return *candidate_list_;
}

const OperationPreferences &SessionConverter::GetOperationPreferences() const {
  return operation_preferences_;
}

SessionConverterInterface::State SessionConverter::GetState() const {
  return state_;
}

size_t SessionConverter::GetSegmentIndex() const {
  return segment_index_;
}

const Segment &SessionConverter::GetPreviousSuggestions()
    const {
  return previous_suggestions_;
}

// static
void SessionConverter::SetConversionPreferences(
    const ConversionPreferences &preferences,
    Segments *segments) {
  segments->set_user_history_enabled(preferences.use_history);
  segments->set_max_history_segments_size(preferences.max_history_size);
}

void SessionConverter::CopyFrom(const SessionConverterInterface &src) {
  Reset();

  // TODO(hsumita): copy composer_ and converter_
  Segments segments;
  src.GetSegments(&segments);
  SetSegments(segments);

  state_ = src.GetState();
  composition_ = src.GetComposition();
  segment_index_ = src.GetSegmentIndex();
  conversion_preferences_ = src.conversion_preferences();
  operation_preferences_ = src.GetOperationPreferences();
  result_.CopyFrom(src.GetResult());
  default_result_ = src.GetDefaultResult();

  const Segment &previous_suggestions =
      src.GetPreviousSuggestions();
  previous_suggestions_.CopyFrom(previous_suggestions);

  if (CheckState(SUGGESTION | PREDICTION | CONVERSION)) {
    UpdateCandidateList();
    candidate_list_->MoveToId(src.GetCandidateList().focused_id());
    SetCandidateListVisible(src.IsCandidateListVisible());
  }
}

void SessionConverter::ResetResult() {
  result_.Clear();
}

void SessionConverter::ResetState() {
  state_ = COMPOSITION;
  composer_ = NULL;
  segment_index_ = 0;
  previous_suggestions_.clear();
  candidate_list_visible_ = false;
  candidate_list_->Clear();
  composition_.clear();
  default_result_.clear();
}

void SessionConverter::SegmentFocus() {
  DCHECK(CheckState(SUGGESTION | PREDICTION | CONVERSION));
  converter_->FocusSegmentValue(segments_.get(),
                                segment_index_,
                                GetCandidateIndexForConverter(segment_index_));
}

void SessionConverter::SegmentFix() {
  DCHECK(CheckState(SUGGESTION | PREDICTION | CONVERSION));
  converter_->CommitSegmentValue(segments_.get(),
                                 segment_index_,
                                 GetCandidateIndexForConverter(segment_index_));
}

void SessionConverter::GetPreeditAndConversion(const size_t index,
                                               const size_t size,
                                               string *preedit,
                                               string *conversion) const {
  DCHECK(CheckState(SUGGESTION | PREDICTION | CONVERSION));
  DCHECK(index + size <= segments_->conversion_segments_size());

  for (size_t i = index; i < size; ++i) {
    if (CheckState(CONVERSION)) {
      // In conversion mode, all the key of candidates is same.
      preedit->append(segments_->conversion_segment(i).key());
    } else {
      DCHECK(CheckState(SUGGESTION | PREDICTION));
      // In suggestion or prediction modes, each key may have
      // different keys, so content_key is used although it is
      // possibly droped the conjugational word (ex. the content_key
      // of "はしる" is "はし").
      preedit->append(GetSelectedCandidate(i).content_key);
    }
    conversion->append(GetSelectedCandidate(i).value);
  }
}

void SessionConverter::UpdateResult(const size_t index, const size_t size) {
  DCHECK(CheckState(SUGGESTION | PREDICTION | CONVERSION));

  string preedit, conversion;
  GetPreeditAndConversion(index, size, &preedit, &conversion);
  SessionOutput::FillConversionResult(preedit, conversion, &result_);
}

namespace {
// Convert transliteration::TransliterationType to id used in the
// converter.  The id number are negative values, and 0 of
// transliteration::TransliterationType is bound for -1 of the id.
int GetT13nId(const transliteration::TransliterationType type) {
  return -(type + 1);
}
}  // namespace

void SessionConverter::UpdateCandidateList() {
  DCHECK(CheckState(SUGGESTION | PREDICTION | CONVERSION));
  candidate_list_->Clear();

  const Segment &segment = segments_->conversion_segment(segment_index_);
  for (size_t i = 0; i < segment.candidates_size(); ++i) {
    candidate_list_->AddCandidate(i, segment.candidate(i).value);
    // if candidate has spelling correction attribute,
    // always display the candidate to let user know the
    // miss spelled candidate.
    if (i < 10 &&
        (segment.candidate(i).attributes &
         Segment::Candidate::SPELLING_CORRECTION)) {
      candidate_list_visible_ = true;
    }
  }

  const bool focused = (segments_->request_type() != Segments::SUGGESTION);
  candidate_list_->set_focused(focused);

  if (segment.meta_candidates_size() == 0) {
    LOG(WARNING) << "T13N is not initialized: " << segment.key();
    return;
  }

  // Set transliteration candidates
  CandidateList *transliterations;
  if (operation_preferences_.use_cascading_window) {
    const bool kNoRotate = false;
    transliterations = candidate_list_->AllocateSubCandidateList(kNoRotate);
    transliterations->set_focused(true);

    const char kT13nLabel[] =
      // "そのほかの文字種";
      "\xe3\x81\x9d\xe3\x81\xae\xe3\x81\xbb\xe3\x81\x8b\xe3\x81\xae"
      "\xe6\x96\x87\xe5\xad\x97\xe7\xa8\xae";
    transliterations->set_name(kT13nLabel);
  } else {
    transliterations = candidate_list_.get();
  }

  // Add transliterations.
  for (size_t i = 0; i < transliteration::NUM_T13N_TYPES; ++i) {
    const transliteration::TransliterationType type =
      transliteration::TransliterationTypeArray[i];
    transliterations->AddCandidateWithAttributes(
        GetT13nId(type),
        segment.meta_candidate(i).value,
        GetT13nAttributes(type));
  }
}

int SessionConverter::GetCandidateIndexForConverter(
    const size_t segment_index) const {
  DCHECK(CheckState(SUGGESTION | PREDICTION | CONVERSION));
  // If segment_index does not point to the focused segment, the value
  // should be always zero.
  if (segment_index != segment_index_) {
    return 0;
  }
  return candidate_list_->focused_id();
}

const Segment::Candidate &SessionConverter::GetSelectedCandidate(
    const size_t segment_index) const {
  DCHECK(CheckState(SUGGESTION | PREDICTION | CONVERSION));
  const int id = GetCandidateIndexForConverter(segment_index);
  return segments_->conversion_segment(segment_index).candidate(id);
}

void SessionConverter::FillConversion(commands::Preedit *preedit) const {
  DCHECK(CheckState(PREDICTION | CONVERSION));
  SessionOutput::FillConversion(*segments_,
                                segment_index_,
                                candidate_list_->focused_id(),
                                preedit);
}

void SessionConverter::FillResult(commands::Result *result) const {
  result->CopyFrom(result_);
}

void SessionConverter::FillCandidates(commands::Candidates *candidates) const {
  DCHECK(CheckState(SUGGESTION | PREDICTION | CONVERSION));
  if (!candidate_list_visible_) {
    return;
  }

  // The position to display the candidate window.
  size_t position = 0;
  string conversion;
  for (size_t i = 0; i < segment_index_; ++i) {
    position += Util::CharsLen(GetSelectedCandidate(i).value);
  }

  const Segment &segment = segments_->conversion_segment(segment_index_);
  SessionOutput::FillCandidates(
      segment, *candidate_list_, position, candidates);

  // Shortcut keys
  if (CheckState(PREDICTION | CONVERSION)) {
    SessionOutput::FillShortcuts(operation_preferences_.candidate_shortcuts,
                                 candidates);
  }

  // Store category
  switch (segments_->request_type()) {
    case Segments::CONVERSION:
      candidates->set_category(commands::CONVERSION);
      break;
    case Segments::PREDICTION:
      candidates->set_category(commands::PREDICTION);
      break;
    case Segments::SUGGESTION:
      candidates->set_category(commands::SUGGESTION);
      break;
    default:
      LOG(WARNING) << "Unkown request type: " << segments_->request_type();
      candidates->set_category(commands::CONVERSION);
      break;
  }

  if (candidates->has_usages()) {
    candidates->mutable_usages()->set_category(commands::USAGE);
  }
  if (candidates->has_subcandidates()) {
    // TODO(komatsu): Subcandidate is not always for transliterations.
    // The category of the subcandidates should be checked.
    candidates->mutable_subcandidates()->set_category(
        commands::TRANSLITERATION);
  }

  // Store display type
  candidates->set_display_type(commands::MAIN);
  if (candidates->has_usages()) {
    candidates->mutable_usages()->set_display_type(commands::CASCADE);
  }
  if (candidates->has_subcandidates()) {
    // TODO(komatsu): Subcandidate is not always for transliterations.
    // The category of the subcandidates should be checked.
    candidates->mutable_subcandidates()->set_display_type(commands::CASCADE);
  }

  // Store footer.
  SessionOutput::FillFooter(candidates->category(), candidates);
}


void SessionConverter::FillAllCandidateWords(
    commands::CandidateList *candidates) const {
  DCHECK(CheckState(SUGGESTION | PREDICTION | CONVERSION));
  commands::Category category;
  switch (segments_->request_type()) {
    case Segments::CONVERSION:
      category = commands::CONVERSION;
      break;
    case Segments::PREDICTION:
      category = commands::PREDICTION;
      break;
    case Segments::SUGGESTION:
      category = commands::SUGGESTION;
      break;
    default:
      LOG(WARNING) << "Unkown request type: " << segments_->request_type();
      category = commands::CONVERSION;
      break;
  }

  const Segment &segment = segments_->conversion_segment(segment_index_);
  SessionOutput::FillAllCandidateWords(
      segment, *candidate_list_, category, candidates);
}

}  // namespace session
}  // namespace mozc
