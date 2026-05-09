/*
 * Copyright 2016-2026 Unisoc (Shanghai) Technologies Co., Ltd.
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
#ifndef ERRORS_H
#define ERRORS_H
#pragma once
#include <cerrno>
#include <cstdint>
#include <string>
namespace OHOS {
namespace OMX {
using status_t = std::int32_t;
namespace ErrorStatus {
static constexpr status_t kUnknownBase = static_cast<status_t>(INT32_MIN);
static constexpr status_t kOk = 0;
static constexpr status_t kNoMemory = -ENOMEM;
static constexpr status_t kInvalidOperation = -ENOSYS;
static constexpr status_t kBadValue = -EINVAL;
static constexpr status_t kBadType = kUnknownBase + 1;
static constexpr status_t kNameNotFound = -ENOENT;
static constexpr status_t kPermissionDenied = -EPERM;
static constexpr status_t kNoInit = -ENODEV;
static constexpr status_t kAlreadyExists = -EEXIST;
static constexpr status_t kDeadObject = -EPIPE;
static constexpr status_t kFailedTransaction = kUnknownBase + 2;
#if !defined(_WIN32)
static constexpr status_t kBadIndex = -EOVERFLOW;
static constexpr status_t kNotEnoughData = -ENODATA;
static constexpr status_t kWouldBlock = -EWOULDBLOCK;
static constexpr status_t kTimedOut = -ETIMEDOUT;
static constexpr status_t kUnknownTransaction = -EBADMSG;
#else
static constexpr status_t kBadIndex = -E2BIG;
static constexpr status_t kNotEnoughData = kUnknownBase + 3;
static constexpr status_t kWouldBlock = kUnknownBase + 4;
static constexpr status_t kTimedOut = kUnknownBase + 5;
static constexpr status_t kUnknownTransaction = kUnknownBase + 6;
#endif
static constexpr status_t kFdsNotAllowed = kUnknownBase + 7;
static constexpr status_t kUnexpectedNull = kUnknownBase + 8;
}  // namespace ErrorStatus

static constexpr status_t STATUS_OK = ErrorStatus::kOk;
#ifndef NO_ERROR
static constexpr status_t STATUS_NO_ERROR = STATUS_OK;
#endif
static constexpr status_t STATUS_UNKNOWN_ERROR = ErrorStatus::kUnknownBase;
static constexpr status_t STATUS_NO_MEMORY = ErrorStatus::kNoMemory;
static constexpr status_t STATUS_INVALID_OPERATION = ErrorStatus::kInvalidOperation;
static constexpr status_t STATUS_BAD_VALUE = ErrorStatus::kBadValue;
static constexpr status_t STATUS_BAD_TYPE = ErrorStatus::kBadType;
static constexpr status_t STATUS_NAME_NOT_FOUND = ErrorStatus::kNameNotFound;
static constexpr status_t STATUS_PERMISSION_DENIED = ErrorStatus::kPermissionDenied;
static constexpr status_t STATUS_NO_INIT = ErrorStatus::kNoInit;
static constexpr status_t STATUS_ALREADY_EXISTS = ErrorStatus::kAlreadyExists;
static constexpr status_t STATUS_DEAD_OBJECT = ErrorStatus::kDeadObject;
static constexpr status_t STATUS_FAILED_TRANSACTION = ErrorStatus::kFailedTransaction;
static constexpr status_t STATUS_BAD_INDEX = ErrorStatus::kBadIndex;
static constexpr status_t STATUS_NOT_ENOUGH_DATA = ErrorStatus::kNotEnoughData;
static constexpr status_t STATUS_WOULD_BLOCK = ErrorStatus::kWouldBlock;
static constexpr status_t STATUS_TIMED_OUT = ErrorStatus::kTimedOut;
static constexpr status_t STATUS_UNKNOWN_TRANSACTION = ErrorStatus::kUnknownTransaction;
static constexpr status_t STATUS_FDS_NOT_ALLOWED = ErrorStatus::kFdsNotAllowed;
static constexpr status_t STATUS_UNEXPECTED_NULL = ErrorStatus::kUnexpectedNull;
[[maybe_unused]] inline constexpr bool StatusSucceeded(status_t code)
{
    return code >= 0;
}
[[maybe_unused]] inline constexpr bool StatusFailed(status_t code)
{
    return !StatusSucceeded(code);
}
std::string statusToString(status_t status);
}  // namespace OMX
}  // namespace OHOS
#endif
