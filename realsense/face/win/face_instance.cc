// Copyright (c) 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "realsense/face/win/face_instance.h"

#include <string>

// This file is auto-generated by face_module.idl
#include "face_module.h"  // NOLINT

#include "base/json/json_string_value_serializer.h"
#include "realsense/face/win/face_module_object.h"

namespace realsense {
namespace face {

using namespace xwalk::common; // NOLINT
using realsense::jsapi::face_module::FaceModuleConstructor::Params;

FaceInstance::FaceInstance()
    : handler_(this),
      store_(&handler_),
      ft_ext_thread_("FTExtensionThread") {
  ft_ext_thread_.Start();
  handler_.Register("faceModuleConstructor",
      base::Bind(&FaceInstance::OnFaceModuleConstructor,
                 base::Unretained(this)));
}

FaceInstance::~FaceInstance() {
  ft_ext_thread_.Stop();
}

void FaceInstance::HandleMessage(const char* msg) {
  JSONStringValueDeserializer str_deserializer(msg);

  int error_code = 0;
  std::string error_message;
  scoped_ptr<base::Value> value(
      str_deserializer.Deserialize(&error_code, &error_message));
  CHECK(value.get());
  CHECK_EQ(0, error_code);
  CHECK(error_message.empty());

  ft_ext_thread_.message_loop()->PostTask(
      FROM_HERE,
      base::Bind(&FaceInstance::OnHandleMessage,
                 base::Unretained(this),
                 base::Passed(&value)));
}

void FaceInstance::OnHandleMessage(scoped_ptr<base::Value> msg) {
  DCHECK_EQ(ft_ext_thread_.message_loop(), base::MessageLoop::current());
  handler_.HandleMessage(msg.Pass());
}

void FaceInstance::OnFaceModuleConstructor(
    scoped_ptr<XWalkExtensionFunctionInfo> info) {
  DCHECK_EQ(ft_ext_thread_.message_loop(), base::MessageLoop::current());
  scoped_ptr<Params> params(Params::Create(*info->arguments()));

  scoped_ptr<BindingObject> obj(new FaceModuleObject());
  store_.AddBindingObject(params->object_id, obj.Pass());
}

}  // namespace face
}  // namespace realsense