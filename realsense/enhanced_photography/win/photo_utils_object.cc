// Copyright (c) 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "realsense/enhanced_photography/win/photo_utils_object.h"

#include <string>

#include "realsense/common/win/common_utils.h"
#include "realsense/enhanced_photography/win/common_utils.h"
#include "realsense/enhanced_photography/win/depth_photo_object.h"

namespace realsense {
using namespace realsense::common;  // NOLINT
namespace enhanced_photography {

PhotoUtilsObject::PhotoUtilsObject(EnhancedPhotographyInstance* instance,
                                   bool isRSSDKInstalled)
    : instance_(instance),
      isRSSDKInstalled_(isRSSDKInstalled),
      photo_utils_(nullptr),
      session_(nullptr) {
  handler_.Register("colorResize",
                    base::Bind(&PhotoUtilsObject::OnColorResize,
                               base::Unretained(this)));
  handler_.Register("commonFOV",
                    base::Bind(&PhotoUtilsObject::OnCommonFOV,
                               base::Unretained(this)));
  handler_.Register("depthResize",
                    base::Bind(&PhotoUtilsObject::OnDepthResize,
                               base::Unretained(this)));
  handler_.Register("enhanceDepth",
                    base::Bind(&PhotoUtilsObject::OnEnhanceDepth,
                               base::Unretained(this)));
  handler_.Register("getDepthQuality",
                    base::Bind(&PhotoUtilsObject::OnGetDepthQuality,
                               base::Unretained(this)));
  handler_.Register("photoCrop",
                    base::Bind(&PhotoUtilsObject::OnPhotoCrop,
                               base::Unretained(this)));
  handler_.Register("photoRotate",
                    base::Bind(&PhotoUtilsObject::OnPhotoRotate,
                               base::Unretained(this)));

  if (isRSSDKInstalled) {
    session_ = PXCSession::CreateInstance();
    photo_utils_ = PXCEnhancedPhoto::PhotoUtils::CreateInstance(session_);
  }
}

PhotoUtilsObject::~PhotoUtilsObject() {
  if (photo_utils_) {
    photo_utils_->Release();
    photo_utils_ = nullptr;
  }
  if (session_) {
    session_->Release();
    session_ = nullptr;
  }
}

void PhotoUtilsObject::OnColorResize(
    scoped_ptr<XWalkExtensionFunctionInfo> info) {
  if (!isRSSDKInstalled_) {
    info->PostResult(CreateErrorResult(ERROR_CODE_INIT_FAILED,
                                       "The RSSDK is uninstalled"));
    return;
  }

  jsapi::depth_photo::Photo photo;
  scoped_ptr<ColorResize::Params> params(
      ColorResize::Params::Create(*info->arguments()));
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

  DCHECK(photo_utils_);
  int width = params->width;
  PXCPhoto* pxcphoto =
      photo_utils_->ColorResize(depthPhotoObject->GetPhoto(), width);

  if (!pxcphoto) {
    info->PostResult(CreateErrorResult(ERROR_CODE_EXEC_FAILED));
    return;
  }

  CreateDepthPhotoObject(instance_, pxcphoto, &photo);
  info->PostResult(ColorResize::Results::Create(photo, std::string()));
}

void PhotoUtilsObject::OnCommonFOV(
    scoped_ptr<XWalkExtensionFunctionInfo> info) {
  if (!isRSSDKInstalled_) {
    info->PostResult(CreateErrorResult(ERROR_CODE_INIT_FAILED,
                                       "The RSSDK is uninstalled"));
    return;
  }

  jsapi::depth_photo::Photo photo;
  scoped_ptr<CommonFOV::Params> params(
      CommonFOV::Params::Create(*info->arguments()));
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

  DCHECK(photo_utils_);
  PXCPhoto* pxcphoto = photo_utils_->CommonFOV(depthPhotoObject->GetPhoto());

  if (!pxcphoto) {
    info->PostResult(CreateErrorResult(ERROR_CODE_EXEC_FAILED));
    return;
  }

  CreateDepthPhotoObject(instance_, pxcphoto, &photo);
  info->PostResult(CommonFOV::Results::Create(photo, std::string()));
}

void PhotoUtilsObject::OnDepthResize(
    scoped_ptr<XWalkExtensionFunctionInfo> info) {
  if (!isRSSDKInstalled_) {
    info->PostResult(CreateErrorResult(ERROR_CODE_INIT_FAILED,
                                       "The RSSDK is uninstalled"));
    return;
  }

  jsapi::depth_photo::Photo photo;
  scoped_ptr<DepthResize::Params> params(
      DepthResize::Params::Create(*info->arguments()));
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

  DCHECK(photo_utils_);
  int width = params->width;

  PXCPhoto* pxcphoto;
  if (params->quality) {
    PXCEnhancedPhoto::PhotoUtils::DepthFillQuality pxcquality;
    if (params->quality == DepthFillQuality::DEPTH_FILL_QUALITY_HIGH) {
      pxcquality = PXCEnhancedPhoto::PhotoUtils::DepthFillQuality::HIGH;
    } else {
      pxcquality = PXCEnhancedPhoto::PhotoUtils::DepthFillQuality::LOW;
    }
    pxcphoto = photo_utils_->DepthResize(depthPhotoObject->GetPhoto(),
                                         width,
                                         pxcquality);
  } else {
    pxcphoto = photo_utils_->DepthResize(depthPhotoObject->GetPhoto(), width);
  }

  if (!pxcphoto) {
    info->PostResult(CreateErrorResult(ERROR_CODE_EXEC_FAILED));
    return;
  }

  CreateDepthPhotoObject(instance_, pxcphoto, &photo);
  info->PostResult(DepthResize::Results::Create(photo, std::string()));
}

void PhotoUtilsObject::OnEnhanceDepth(
  scoped_ptr<XWalkExtensionFunctionInfo> info) {
  if (!isRSSDKInstalled_) {
    info->PostResult(CreateErrorResult(ERROR_CODE_INIT_FAILED,
                                       "The RSSDK is uninstalled"));
    return;
  }

  jsapi::depth_photo::Photo photo;
  scoped_ptr<EnhanceDepth::Params> params(
      EnhanceDepth::Params::Create(*info->arguments()));
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

  DCHECK(photo_utils_);
  DepthFillQuality quality = params->quality;
  PXCEnhancedPhoto::PhotoUtils::DepthFillQuality pxcquality;
  if (quality == DepthFillQuality::DEPTH_FILL_QUALITY_HIGH)
    pxcquality = PXCEnhancedPhoto::PhotoUtils::DepthFillQuality::HIGH;
  else
    pxcquality = PXCEnhancedPhoto::PhotoUtils::DepthFillQuality::LOW;

  PXCPhoto* pxcphoto = photo_utils_->EnhanceDepth(depthPhotoObject->GetPhoto(),
                                                  pxcquality);
  if (!pxcphoto) {
    info->PostResult(CreateErrorResult(ERROR_CODE_EXEC_FAILED));
    return;
  }

  CreateDepthPhotoObject(instance_, pxcphoto, &photo);
  info->PostResult(EnhanceDepth::Results::Create(photo, std::string()));
}

void PhotoUtilsObject::OnGetDepthQuality(
    scoped_ptr<XWalkExtensionFunctionInfo> info) {
  if (!isRSSDKInstalled_) {
    info->PostResult(CreateErrorResult(ERROR_CODE_INIT_FAILED,
                                       "The RSSDK is uninstalled"));
    return;
  }

  jsapi::photo_utils::DepthMapQuality depth_quality;
  scoped_ptr<GetDepthQuality::Params> params(
      GetDepthQuality::Params::Create(*info->arguments()));
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

  DCHECK(photo_utils_);
  PXCEnhancedPhoto::PhotoUtils::DepthMapQuality qulity =
      photo_utils_->GetDepthQuality(
          depthPhotoObject->GetPhoto()->QueryDepth());
  if (qulity == PXCEnhancedPhoto::PhotoUtils::DepthMapQuality::BAD) {
    depth_quality = DepthMapQuality::DEPTH_MAP_QUALITY_BAD;
  } else if (qulity == PXCEnhancedPhoto::PhotoUtils::DepthMapQuality::FAIR) {
    depth_quality = DepthMapQuality::DEPTH_MAP_QUALITY_FAIR;
  } else if (qulity == PXCEnhancedPhoto::PhotoUtils::DepthMapQuality::GOOD) {
    depth_quality = DepthMapQuality::DEPTH_MAP_QUALITY_GOOD;
  } else {
    info->PostResult(CreateErrorResult(ERROR_CODE_EXEC_FAILED));
    return;
  }

  info->PostResult(GetDepthQuality::Results::Create(
      depth_quality, std::string()));
}

void PhotoUtilsObject::OnPhotoCrop(
    scoped_ptr<XWalkExtensionFunctionInfo> info) {
  if (!isRSSDKInstalled_) {
    info->PostResult(CreateErrorResult(ERROR_CODE_INIT_FAILED,
                                       "The RSSDK is uninstalled"));
    return;
  }

  jsapi::depth_photo::Photo photo;
  scoped_ptr<PhotoCrop::Params> params(
      PhotoCrop::Params::Create(*info->arguments()));
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

  DCHECK(photo_utils_);
  PXCRectI32 pxcrect;
  pxcrect.x = params->rect.x;
  pxcrect.y = params->rect.y;
  pxcrect.w = params->rect.w;
  pxcrect.h = params->rect.h;

  PXCPhoto* pxcphoto = photo_utils_->PhotoCrop(depthPhotoObject->GetPhoto(),
                                               pxcrect);
  if (!pxcphoto) {
    info->PostResult(CreateErrorResult(ERROR_CODE_EXEC_FAILED));
    return;
  }

  CreateDepthPhotoObject(instance_, pxcphoto, &photo);
  info->PostResult(PhotoCrop::Results::Create(photo, std::string()));
}

void PhotoUtilsObject::OnPhotoRotate(
    scoped_ptr<XWalkExtensionFunctionInfo> info) {
  if (!isRSSDKInstalled_) {
    info->PostResult(CreateErrorResult(ERROR_CODE_INIT_FAILED,
                                       "The RSSDK is uninstalled"));
    return;
  }

  jsapi::depth_photo::Photo photo;
  scoped_ptr<PhotoRotate::Params> params(
      PhotoRotate::Params::Create(*info->arguments()));
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

  DCHECK(photo_utils_);

  pxcF32 rotation = params->rotation;
  PXCPhoto* pxcphoto = photo_utils_->PhotoRotate(depthPhotoObject->GetPhoto(),
                                                 rotation);
  if (!pxcphoto) {
    info->PostResult(CreateErrorResult(ERROR_CODE_EXEC_FAILED));
    return;
  }

  CreateDepthPhotoObject(instance_, pxcphoto, &photo);
  info->PostResult(PhotoRotate::Results::Create(photo, std::string()));
}

}  // namespace enhanced_photography
}  // namespace realsense
