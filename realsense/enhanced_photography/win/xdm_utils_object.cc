// Copyright (c) 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "realsense/enhanced_photography/win/xdm_utils_object.h"

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "realsense/common/win/common_utils.h"
#include "realsense/enhanced_photography/win/common_utils.h"
#include "realsense/enhanced_photography/win/depth_photo_object.h"

namespace realsense {
using namespace realsense::common;  // NOLINT
namespace enhanced_photography {

XDMUtilsObject::XDMUtilsObject(EnhancedPhotographyInstance* instance,
                               bool isRSSDKInstalled)
      : instance_(instance),
        isRSSDKInstalled_(isRSSDKInstalled),
        session_(nullptr),
        binary_message_size_(0) {
  handler_.Register("isXDM",
      base::Bind(&XDMUtilsObject::OnIsXDM,
                 base::Unretained(this)));
  handler_.Register("loadXDM",
      base::Bind(&XDMUtilsObject::OnLoadXDM,
                 base::Unretained(this)));
  handler_.Register("saveXDM",
      base::Bind(&XDMUtilsObject::OnSaveXDM,
                 base::Unretained(this)));

  if (isRSSDKInstalled) {
    session_ = PXCSession::CreateInstance();
  }
}

XDMUtilsObject::~XDMUtilsObject() {
  if (session_) {
    session_->Release();
    session_ = nullptr;
  }
}

void XDMUtilsObject::OnIsXDM(
    scoped_ptr<XWalkExtensionFunctionInfo> info) {
  if (!isRSSDKInstalled_) {
    info->PostResult(CreateErrorResult(ERROR_CODE_INIT_FAILED,
                                       "The RSSDK is uninstalled"));
    return;
  }

  base::BinaryValue* binary_value = nullptr;
  GetBinaryValueFromArgs(info->arguments(), &binary_value);
  if (!binary_value)
    info->PostResult(CreateErrorResult(ERROR_CODE_PARAM_UNSUPPORTED));

  base::ScopedTempDir tmp_dir;
  tmp_dir.CreateUniqueTempDir();
  base::FilePath tmp_file = tmp_dir.path().Append(
      FILE_PATH_LITERAL("tmp_img.jpg"));

  CreateFileWithBinaryValue(*binary_value, &tmp_file);

  wchar_t* wfile = const_cast<wchar_t*>(tmp_file.value().c_str());
  PXCPhoto* pxcphoto = session_->CreatePhoto();
  if (pxcphoto->IsXDM(wfile)) {
    info->PostResult(IsXDM::Results::Create(true, std::string()));
  } else {
    info->PostResult(IsXDM::Results::Create(false, std::string()));
  }
  pxcphoto->Release();
}

void XDMUtilsObject::OnLoadXDM(
    scoped_ptr<XWalkExtensionFunctionInfo> info) {
  if (!isRSSDKInstalled_) {
    info->PostResult(CreateErrorResult(ERROR_CODE_INIT_FAILED,
                                       "The RSSDK is uninstalled"));
    return;
  }

  base::BinaryValue* binary_value = nullptr;
  GetBinaryValueFromArgs(info->arguments(), &binary_value);
  if (!binary_value)
    info->PostResult(CreateErrorResult(ERROR_CODE_PARAM_UNSUPPORTED));

  base::ScopedTempDir tmp_dir;
  tmp_dir.CreateUniqueTempDir();
  base::FilePath tmp_file = tmp_dir.path().Append(
      FILE_PATH_LITERAL("tmp_img.jpg"));

  CreateFileWithBinaryValue(*binary_value, &tmp_file);

  wchar_t* wfile = const_cast<wchar_t*>(tmp_file.value().c_str());
  PXCPhoto* pxcphoto = session_->CreatePhoto();
  if (pxcphoto->LoadXDM(wfile) < PXC_STATUS_NO_ERROR) {
    info->PostResult(CreateErrorResult(ERROR_CODE_EXEC_FAILED));
    return;
  }
  jsapi::depth_photo::Photo photo;
  CreateDepthPhotoObject(instance_, pxcphoto, &photo);
  info->PostResult(LoadXDM::Results::Create(photo, std::string()));
}

void XDMUtilsObject::OnSaveXDM(
    scoped_ptr<XWalkExtensionFunctionInfo> info) {
  if (!isRSSDKInstalled_) {
    info->PostResult(CreateErrorResult(ERROR_CODE_INIT_FAILED,
                                       "The RSSDK is uninstalled"));
    return;
  }

  scoped_ptr<SaveXDM::Params> params(
      SaveXDM::Params::Create(*info->arguments()));
  if (!params) {
    info->PostResult(CreateErrorResult(ERROR_CODE_PARAM_UNSUPPORTED));
    return;
  }

  std::string object_id = params->photo.object_id;
  DepthPhotoObject* depthPhotoObject = static_cast<DepthPhotoObject*>(
      instance_->GetBindingObjectById(object_id));
  if (!depthPhotoObject || !depthPhotoObject->GetPhoto()) {
    info->PostResult(CreateErrorResult(ERROR_CODE_PHOTO_INVALID));
    return;
  }

  base::ScopedTempDir tmp_dir;
  tmp_dir.CreateUniqueTempDir();
  base::FilePath tmp_file = tmp_dir.path().Append(
      FILE_PATH_LITERAL("tmp_img.jpg"));
  wchar_t* wfile = const_cast<wchar_t*>(tmp_file.value().c_str());
  if (depthPhotoObject->GetPhoto()->SaveXDM(wfile) < PXC_STATUS_NO_ERROR) {
    info->PostResult(CreateErrorResult(ERROR_CODE_EXEC_FAILED));
    return;
  }

  base::File file(tmp_file, base::File::FLAG_OPEN | base::File::FLAG_READ);
  int64 file_length = file.GetLength();
  binary_message_size_ = file_length + sizeof(int);
  binary_message_.reset(new uint8[binary_message_size_]);
  // the first sizeof(int) bytes will be used for callback id.
  char* data = reinterpret_cast<char*>(binary_message_.get() + 1 * sizeof(int));
  file.Read(0, data, file_length);
  file.Close();

  scoped_ptr<base::ListValue> result(new base::ListValue());
  result->Append(base::BinaryValue::CreateWithCopiedBuffer(
      reinterpret_cast<const char*>(binary_message_.get()),
      binary_message_size_));
  info->PostResult(result.Pass());
}

void XDMUtilsObject::CreateFileWithBinaryValue(
    const base::BinaryValue& value, base::FilePath* file_path) {
  const char* data = value.GetBuffer();

  base::File file(*file_path,
                  base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  file.created();
  file.Write(0, data, static_cast<int>(value.GetSize()));
  file.Close();
}

}  // namespace enhanced_photography
}  // namespace realsense
