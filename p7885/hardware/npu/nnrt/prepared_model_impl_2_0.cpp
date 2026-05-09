/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "prepared_model_impl_2_0.h"

#include <unistd.h>

#include "nnrt_common.h"

namespace OHOS {
namespace HDI {
namespace Nnrt {
namespace V2_0 {
namespace {
constexpr int32_t K_MIN_VALID_FD = 0;
constexpr uint32_t K_ZERO_U32 = 0;

bool FillTensorShape(const std::vector<int32_t>& dims, unisoc::TensorShape& out)
{
    if (dims.size() > out.m_Dims.size()) {
        return false;
    }
    out.m_NumDims = static_cast<unsigned int>(dims.size());
    out.m_Dims.fill(1U);
    for (size_t i = 0; i < dims.size(); ++i) {
        if (dims[i] <= 0) {
            return false;
        }
        out.m_Dims[i] = static_cast<unsigned int>(dims[i]);
    }
    return true;
}

bool ToUniAIDataType(DataType type, unisoc::DataType& out)
{
    switch (type) {
        case DataType::DATA_TYPE_FLOAT16:
            out = unisoc::DataType::Float16;
            return true;
        case DataType::DATA_TYPE_FLOAT32:
            out = unisoc::DataType::Float32;
            return true;
        case DataType::DATA_TYPE_UINT8:
            out = unisoc::DataType::QuantisedAsymm8;
            return true;
        case DataType::DATA_TYPE_INT8:
            out = unisoc::DataType::QuantisedSymm8;
            return true;
        case DataType::DATA_TYPE_INT32:
            out = unisoc::DataType::Signed32;
            return true;
        case DataType::DATA_TYPE_INT64:
            out = unisoc::DataType::Signed64;
            return true;
        default:
            return false;
    }
}

unisoc::DataLayout ToUniAIDataLayout(Format format)
{
    switch (format) {
        case Format::FORMAT_NCHW:
            return unisoc::DataLayout::NCHW;
        case Format::FORMAT_NHWC:
            return unisoc::DataLayout::NHWC;
        default:
            return unisoc::DataLayout::UNDEFINED;
    }
}

size_t ElementCountOfUniAI(const unisoc::TensorShape& shape)
{
    size_t count = 1;
    for (unsigned int i = 0; i < shape.m_NumDims; i++) {
        count *= static_cast<size_t>(shape.m_Dims[i]);
    }
    return count;
}

const void* GetUniAITensorDataPtr(const unisoc::UniAITensor& tensor)
{
    switch (tensor.GetDataType()) {
        case unisoc::DataType::QuantisedAsymm8:
        case unisoc::DataType::QuantisedSymm8:
            return tensor.buffer<uint8_t>();
        case unisoc::DataType::Signed32:
            return tensor.buffer<int32_t>();
        case unisoc::DataType::Signed64:
            return tensor.buffer<int64_t>();
        case unisoc::DataType::Float16:
            return tensor.buffer<uint16_t>();
        case unisoc::DataType::Float32:
            return tensor.buffer<float>();
        default:
            return nullptr;
    }
}

bool IsValidSharedBuffer(const SharedBuffer& buffer)
{
    if (buffer.fd < K_MIN_VALID_FD) {
        return false;
    }
    if (buffer.bufferSize == K_ZERO_U32 || buffer.dataSize == K_ZERO_U32) {
        return false;
    }
    if (buffer.offset + buffer.dataSize > buffer.bufferSize) {
        return false;
    }
    return true;
}

int32_t CreateMappedAshmem(const SharedBuffer& buffer, bool writable, std::vector<sptr<Ashmem>>& ashmems,
    sptr<Ashmem>& out)
{
    if (!IsValidSharedBuffer(buffer)) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_BUFFER);
    }

    int dupFd = dup(buffer.fd);
    if (dupFd < 0) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_BUFFER);
    }

    sptr<Ashmem> ash = new (std::nothrow) Ashmem(dupFd, static_cast<int32_t>(buffer.bufferSize));
    if (ash == nullptr) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_OUT_OF_MEMORY);
    }

    const bool ok = writable ? ash->MapReadAndWriteAshmem() : ash->MapReadOnlyAshmem();
    if (!ok) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_MEMORY_ERROR);
    }

    ashmems.emplace_back(ash);
    out = ash;
    return static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS);
}

void BuildOutputDimsFromUniAI(const std::vector<unisoc::UniAITensor>& outputTensors,
    std::vector<std::vector<int32_t>>& outputDims)
{
    outputDims.clear();
    outputDims.reserve(outputTensors.size());
    for (size_t i = 0; i < outputTensors.size(); i++) {
        const auto shape = outputTensors[i].GetShape();
        std::vector<int32_t> dims;
        dims.reserve(shape.m_NumDims);
        for (unsigned int d = 0; d < shape.m_NumDims; d++) {
            dims.emplace_back(static_cast<int32_t>(shape.m_Dims[d]));
        }
        outputDims.emplace_back(std::move(dims));
    }
}
} // namespace

namespace internal {
int32_t ToNnrtResult(unisoc::Status status)
{
    if (status == unisoc::Status::AI_SUCCESS) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS);
    }
    if (status == unisoc::Status::AI_SMALL_INPUT_SIZE || status == unisoc::Status::AI_SMALL_OUTPUT_SIZE) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_INSUFFICIENT_BUFFER);
    }
    if (status == unisoc::Status::AI_INVALID_ARGUMENT) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_PARAMETER);
    }
    if (status == unisoc::Status::AI_FILE_NOT_FOUND) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_FILE);
    }
    if (status == unisoc::Status::AI_PARSE_ERROR) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_MODEL);
    }
    return static_cast<int32_t>(NNRT_ReturnCode::NNRT_FAILED);
}

size_t ElementSizeOfUniAI(unisoc::DataType type)
{
    constexpr size_t kBytesU8 = sizeof(uint8_t);
    constexpr size_t kBytesU16 = sizeof(uint16_t);
    constexpr size_t kBytesI32 = sizeof(int32_t);
    constexpr size_t kBytesI64 = sizeof(int64_t);
    constexpr size_t kBytesF32 = sizeof(float);
    constexpr size_t kInvalidSize = 0;
    switch (type) {
        case unisoc::DataType::QuantisedAsymm8:
        case unisoc::DataType::QuantisedSymm8:
            return kBytesU8;
        case unisoc::DataType::Signed32:
            return kBytesI32;
        case unisoc::DataType::Signed64:
            return kBytesI64;
        case unisoc::DataType::Float16:
            return kBytesU16;
        case unisoc::DataType::Float32:
            return kBytesF32;
        default:
            return kInvalidSize;
    }
}

int32_t WriteUniAIOutputToAshmem(const IOTensor& outDesc, const unisoc::UniAITensor& outTensor, const sptr<Ashmem>& ash)
{
    if (ash == nullptr) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_NULL_PTR);
    }

    const void* src = GetUniAITensorDataPtr(outTensor);
    if (src == nullptr) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_FAILED);
    }

    const size_t elemSize = ElementSizeOfUniAI(outTensor.GetDataType());
    const size_t elemCount = ElementCountOfUniAI(outTensor.GetShape());
    if (elemSize == 0 || elemCount == 0) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_FAILED);
    }

    const size_t bytes = elemSize * elemCount;
    if (bytes > static_cast<size_t>(outDesc.data.dataSize)) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_INSUFFICIENT_BUFFER);
    }

    const int32_t offset = static_cast<int32_t>(outDesc.data.offset);
    const int32_t bytesI32 = static_cast<int32_t>(bytes);
    if (!ash->WriteToAshmem(src, bytesI32, offset)) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_MEMORY_ERROR);
    }
    return static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS);
}

int32_t AppendUniAITensorFromIoTensor(const IOTensor& ioTensor, bool writable, std::vector<sptr<Ashmem>>& ashmems,
    std::vector<unisoc::UniAITensor>& out)
{
    sptr<Ashmem> ash;
    int32_t ret = CreateMappedAshmem(ioTensor.data, writable, ashmems, ash);
    if (ret != static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS)) {
        return ret;
    }

    const void* dataPtr = ash->ReadFromAshmem(static_cast<int32_t>(ioTensor.data.dataSize),
        static_cast<int32_t>(ioTensor.data.offset));
    if (dataPtr == nullptr) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_BUFFER);
    }

    unisoc::TensorShape shape {};
    if (!FillTensorShape(ioTensor.dimensions, shape)) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_SHAPE);
    }

    unisoc::DataType dataType {};
    if (!ToUniAIDataType(ioTensor.dataType, dataType)) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_DATATYPE);
    }

    out.emplace_back(unisoc::UniAITensor(shape,
        ToUniAIDataLayout(ioTensor.format), dataType, const_cast<void*>(dataPtr)));
    return static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS);
}
} // namespace internal

PreparedModelImpl::PreparedModelImpl(unisoc::IUniAIPtr uniAI, unisoc::NetworkId networkId, sptr<Ashmem> modelAshmem)
    : uniAI_(std::move(uniAI)), networkId_(networkId), modelAshmem_(std::move(modelAshmem))
{
}

PreparedModelImpl::~PreparedModelImpl()
{
    if (uniAI_) {
        uniAI_->DestroyNetwork(networkId_);
    }
    if (modelAshmem_) {
        modelAshmem_->UnmapAshmem();
        modelAshmem_->CloseAshmem();
        modelAshmem_ = nullptr;
    }
}

int32_t PreparedModelImpl::ExportModelCache(std::vector<SharedBuffer>& modelCache)
{
    (void)modelCache;
    return static_cast<int32_t>(NNRT_ReturnCode::NNRT_NOT_SUPPORT);
}

int32_t PreparedModelImpl::GetInputDimRanges(std::vector<std::vector<uint32_t>>& minInputDims,
    std::vector<std::vector<uint32_t>>& maxInputDims)
{
    if (!uniAI_) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_NULL_PTR);
    }

    std::vector<unisoc::IoInfo> inputInfos = uniAI_->GetInputAttributeInfo(networkId_);
    if (inputInfos.empty()) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_FAILED);
    }

    minInputDims.clear();
    maxInputDims.clear();
    minInputDims.reserve(inputInfos.size());
    maxInputDims.reserve(inputInfos.size());

    for (const auto& inputInfo : inputInfos) {
        if (inputInfo.m_Shape.m_NumDims == 0 || inputInfo.m_Shape.m_NumDims > inputInfo.m_Shape.m_Dims.size()) {
            return static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_SHAPE);
        }

        std::vector<uint32_t> dims;
        dims.reserve(inputInfo.m_Shape.m_NumDims);
        for (size_t i = 0; i < inputInfo.m_Shape.m_NumDims; ++i) {
            uint32_t dim = inputInfo.m_Shape.m_Dims[i];
            if (dim == 0) {
                return static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_SHAPE);
            }
            dims.emplace_back(dim);
        }
        minInputDims.emplace_back(dims);
        maxInputDims.emplace_back(dims);
    }

    return static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS);
}

int32_t PreparedModelImpl::Run(const std::vector<IOTensor>& inputs, const std::vector<IOTensor>& outputs,
    std::vector<std::vector<int32_t>>& outputDims)
{
    if (!uniAI_) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_NULL_PTR);
    }

    std::vector<sptr<Ashmem>> ashmems;
    std::vector<unisoc::UniAITensor> inputTensors;
    inputTensors.reserve(inputs.size());

    for (const auto& input : inputs) {
        int32_t ret = internal::AppendUniAITensorFromIoTensor(input, false, ashmems, inputTensors);
        if (ret != static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS)) {
            return ret;
        }
    }

    std::vector<sptr<Ashmem>> outputAshmems;
    outputAshmems.reserve(outputs.size());
    for (const auto& output : outputs) {
        sptr<Ashmem> ash;
        int32_t ret = CreateMappedAshmem(output.data, true, ashmems, ash);
        if (ret != static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS)) {
            return ret;
        }
        outputAshmems.emplace_back(std::move(ash));
    }

    std::vector<unisoc::UniAITensor> outputTensors;
    auto ret = uniAI_->Inference(networkId_, inputTensors, outputTensors);
    if (ret != unisoc::Status::AI_SUCCESS) {
        return internal::ToNnrtResult(ret);
    }

    if (outputTensors.size() != outputs.size()) {
        return static_cast<int32_t>(NNRT_ReturnCode::NNRT_FAILED);
    }

    for (size_t i = 0; i < outputs.size(); i++) {
        int32_t wret = internal::WriteUniAIOutputToAshmem(outputs[i], outputTensors[i], outputAshmems[i]);
        if (wret != static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS)) {
            return wret;
        }
    }

    BuildOutputDimsFromUniAI(outputTensors, outputDims);

    return static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS);
}

} // namespace V2_0
} // namespace Nnrt
} // namespace HDI
} // namespace OHOS
