/*
 * Copyright 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "fields/packet_field.h"
#include "fields/size_field.h"
#include "parse_location.h"

class BodyField : public PacketField {
 public:
  BodyField(ParseLocation loc);

  virtual PacketField::Type GetFieldType() const override;

  virtual Size GetSize() const override;

  virtual std::string GetType() const override;

  virtual void GenGetter(std::ostream&, Size, Size) const override;

  virtual bool GenBuilderParameter(std::ostream&) const override;

  virtual bool HasParameterValidator() const override;

  virtual void GenParameterValidator(std::ostream&) const override;

  virtual void GenInserter(std::ostream&) const override;

  virtual void GenValidator(std::ostream&) const override;
};
