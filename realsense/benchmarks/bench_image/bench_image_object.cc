// Copyright (c) 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <sstream>

#include "third_party/modp_b64/modp_b64.h"
#include "realsense/benchmarks/bench_image/bench_image_object.h"
#include "bench_image.h" // NOLINT

namespace realsense {
namespace bench_image {

using namespace realsense::jsapi::bench_image; // NOLINT
using namespace xwalk::common; // NOLINT

BenchImageObject::BenchImageObject() {
  handler_.Register("getSampleLong",
    base::Bind(&BenchImageObject::OnGetSampleLong,
                               base::Unretained(this)));
  handler_.Register("getSampleString",
    base::Bind(&BenchImageObject::OnGetSampleString,
                             base::Unretained(this)));
  frame_count = 0;
}

BenchImageObject::~BenchImageObject() {
}

void BenchImageObject::OnGetSampleLong(
  scoped_ptr<XWalkExtensionFunctionInfo> info) {
  scoped_ptr<SampleLong> sample(new SampleLong());
  scoped_ptr<GetSampleLong::Params>
    params(GetSampleLong::Params::Create(*info->arguments()));

  int size = params->width * params->height;
  if (size != 0) {
    uint32 pixel = GeneratePixel();
    sample->color.data.resize(size, pixel);
    sample->color.width = params->width;
    sample->color.height = params->height;

    info->PostResult(GetSampleLong::Results::Create(*sample, std::string()));
  } else {
    info->PostResult(GetSampleLong::Results::Create(
      *sample, std::string("image size = 0")));
  }
}

void BenchImageObject::OnGetSampleString(
  scoped_ptr<XWalkExtensionFunctionInfo> info) {
  scoped_ptr<SampleString> sample(new SampleString());
  scoped_ptr<GetSampleString::Params>
    params(GetSampleString::Params::Create(*info->arguments()));

  int size = params->width * params->height * 4;
  if (size != 0) {
    int encode_len = modp_b64_encode_len(size);
    scoped_ptr<char> raw_data(new char[size]);
    scoped_ptr<char> encode_data(new char[encode_len]);
    uint32 pixel = GeneratePixel();
    for (int i = 0; i < size / 4; i++)
      reinterpret_cast<uint32*>(raw_data.get())[i] = pixel;
    modp_b64_encode(encode_data.get(), raw_data.get(), size);

    sample->color.data.assign(encode_data.get());
    sample->color.height = params->height;
    sample->color.width = params->width;

    info->PostResult(GetSampleString::Results::Create(*sample, std::string()));
  } else {
    info->PostResult(GetSampleString::Results::Create(
      *sample, std::string("image size = 0")));
  }
}

uint32 BenchImageObject::GeneratePixel() {
  return (0xff << ((frame_count++ % 3) * 8)) + 0x80000000;
}

}  // namespace bench_image
}  // namespace realsense
