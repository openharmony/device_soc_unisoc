/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <gtest/gtest.h>

#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <new>
#include <string>
#include <unistd.h>
#include <vector>
#include <sys/mman.h>
#include <sys/syscall.h>

#include "ashmem.h"
#include "innrt_device_vdi.h"
#include "prepared_model_impl_2_0.h"

namespace OHOS::HDI::Nnrt::V2_0 {
static int32_t CallDeviceToNnrtResult(unisoc::Status status)
{
    return internal::ToNnrtResult(status);
}

static int32_t CallPreparedToNnrtResult(unisoc::Status status)
{
    return internal::ToNnrtResult(status);
}

static size_t CallElementSizeOfUniAI(unisoc::DataType type)
{
    return internal::ElementSizeOfUniAI(type);
}

static int32_t CallWriteUniAIOutputToAshmem(const IOTensor& outDesc, const unisoc::UniAITensor& outTensor,
    const sptr<Ashmem>& ash)
{
    return internal::WriteUniAIOutputToAshmem(outDesc, outTensor, ash);
}

static int32_t CallAppendUniAITensorFromIoTensor(const IOTensor& ioTensor, bool writable,
    std::vector<sptr<Ashmem>>& ashmems, std::vector<unisoc::UniAITensor>& out)
{
    return internal::AppendUniAITensorFromIoTensor(ioTensor, writable, ashmems, out);
}
} // namespace OHOS::HDI::Nnrt::V2_0

using OHOS::HDI::Nnrt::V2_0::INnrtDeviceVdi;
using OHOS::HDI::Nnrt::V2_0::NNRT_ReturnCode;
using OHOS::HDI::Nnrt::V2_0::SharedBuffer;
using OHOS::HDI::Nnrt::V2_0::DeviceStatus;
using OHOS::HDI::Nnrt::V2_0::DeviceType;
using OHOS::HDI::Nnrt::V2_0::ModelConfig;
using OHOS::HDI::Nnrt::V2_0::IPreparedModel;
using OHOS::HDI::Nnrt::V2_0::PreparedModelImpl;
using OHOS::HDI::Nnrt::V2_0::sptr;
using OHOS::Ashmem;

static std::atomic<bool> g_forceDupFail { false };
static std::atomic<bool> g_forceMapReadWriteFail { false };
static std::atomic<bool> g_forceMapReadOnlyFail { false };
static std::atomic<bool> g_forceMapReadOnlyNoMap { false };
static std::atomic<bool> g_forceMapReadWriteNoMap { false };
static std::atomic<int> g_failNothrowNewCount { 0 };
static std::atomic<int> g_nothrowNewCallIndex { 0 };
static std::atomic<int> g_failNothrowNewOnCall { 0 };

static std::atomic<bool> g_uniAIReturnNull { false };
static std::atomic<int> g_uniAIBackendState { static_cast<int>(unisoc::BackendState::DEVICE_AVAILABLE) };
static std::atomic<int> g_uniAILoadStatus { static_cast<int>(unisoc::Status::AI_SUCCESS) };
static std::atomic<int> g_uniAICompileStatus { static_cast<int>(unisoc::Status::AI_SUCCESS) };

static std::string g_lastUniAICreateBackendPath;
static void* g_lastCreatedUniAI = nullptr;

extern "C" int Dup(int oldfd)
{
    if (g_forceDupFail.exchange(false)) {
        errno = EBADF;
        return -1;
    }
    return static_cast<int>(syscall(SYS_dup, oldfd));
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept
{
    int callIndex = g_nothrowNewCallIndex.fetch_add(1) + 1;
    if (g_failNothrowNewCount.load() > 0) {
        if (g_failNothrowNewCount.fetch_sub(1) > 0) {
            return nullptr;
        }
    }
    int failOnCall = g_failNothrowNewOnCall.load();
    if (failOnCall > 0 && callIndex == failOnCall) {
        return nullptr;
    }
    return std::malloc(size);
}

void operator delete(void* ptr) noexcept
{
    std::free(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept
{
    std::free(ptr);
}

namespace OHOS {
bool Ashmem::MapReadAndWriteAshmem()
{
    if (g_forceMapReadWriteFail.exchange(false)) {
        return false;
    }
    if (g_forceMapReadWriteNoMap.exchange(false)) {
        return true;
    }
    return MapAshmem(PROT_READ | PROT_WRITE);
}

bool Ashmem::MapReadOnlyAshmem()
{
    if (g_forceMapReadOnlyFail.exchange(false)) {
        return false;
    }
    if (g_forceMapReadOnlyNoMap.exchange(false)) {
        return true;
    }
    return MapAshmem(PROT_READ);
}
}

namespace unisoc {
UniAITensor::UniAITensor(TensorShape inputShape, DataLayout dataLayout, DataType dataType, void *data)
    : m_DataLayout(dataLayout), m_DataType(dataType), m_Shape(inputShape), m_Data(data)
{
}

DataLayout UniAITensor::GetLayout() const
{
    return m_DataLayout;
}

DataType UniAITensor::GetDataType() const
{
    return m_DataType;
}

TensorShape UniAITensor::GetShape() const
{
    return m_Shape;
}
}

struct StubUniAI : public unisoc::IUniAI {
    virtual ~StubUniAI() = default;
    std::vector<unisoc::IoInfo> inputInfos;
    unisoc::Status inferStatus { unisoc::Status::AI_SUCCESS };
    std::vector<unisoc::UniAITensor> inferOutputs;
    std::vector<unisoc::BackendId> supportedBackends;
    std::vector<unisoc::DataType> supportedDataTypes;
    std::vector<std::string> inputNames;
    std::vector<std::string> outputNames;
    std::string sdkVersion { "nnrt-ut" };
    int sdkVersionQueryCount { 0 };

    unisoc::BackendState backendState { unisoc::BackendState::DEVICE_AVAILABLE };
    unisoc::Status loadNetworkStatus { unisoc::Status::AI_SUCCESS };
    unisoc::Status networkCompileStatus { unisoc::Status::AI_SUCCESS };
    bool lastSaveCache { false };
    std::string lastCachePath;
    std::vector<unisoc::BackendId> lastPreferentBackendList;

    void DestroyNetwork(unisoc::NetworkId) override {}
    std::vector<unisoc::BackendId> GetSupportBackendId() override { return supportedBackends; }
    unisoc::BackendState GetBackendState([[maybe_unused]] unisoc::BackendId) override { return backendState; }
    std::vector<unisoc::DataType> GetSupportedDataType() override { return supportedDataTypes; }
    unisoc::Status LoadNetwork(const char*, unisoc::NetworkId&, unisoc::ModelType&) override
    {
        return unisoc::Status::AI_SUCCESS;
    }
    unisoc::Status NetworkCompile(unisoc::NetworkId& networkid, unisoc::ModelType modelType,
        const std::vector<unisoc::BackendId>& preferentBackendList, bool saveCache, const char* cachePath) override
    {
        (void)networkid;
        (void)modelType;
        lastPreferentBackendList = preferentBackendList;
        lastSaveCache = saveCache;
        lastCachePath = cachePath == nullptr ? std::string() : std::string(cachePath);
        return networkCompileStatus;
    }
    std::vector<std::string> GetNetworkInputNames(unisoc::NetworkId) override { return inputNames; }
    std::vector<std::string> GetNetworkOutputNames(unisoc::NetworkId) override { return outputNames; }
    unisoc::Status Inference(unisoc::NetworkId, std::vector<unisoc::UniAITensor>&,
        std::vector<unisoc::UniAITensor>& outputs) override
    {
        outputs = inferOutputs;
        return inferStatus;
    }
    std::string GetUniAISdkVersion() override
    {
        ++sdkVersionQueryCount;
        return sdkVersion;
    }
    std::vector<unisoc::IoInfo> GetInputAttributeInfo(unisoc::NetworkId) override { return inputInfos; }
    std::vector<unisoc::IoInfo> GetOutputAttributeInfo(unisoc::NetworkId) override { return {}; }
    unisoc::Status NetworkCompile(unisoc::CompilationParameter&) override { return unisoc::Status::AI_SUCCESS; }
    unisoc::Status LoadNetwork(const void* modelBuffer, const size_t bufferSize, unisoc::NetworkId& networkid,
        unisoc::ModelType& modelType) override
    {
        (void)modelBuffer;
        (void)bufferSize;
        networkid = 1;
        modelType = unisoc::ModelType::UniAI_IR;
        return loadNetworkStatus;
    }
};

namespace unisoc {
IUniAIPtr IUniAI::Create(bool isProfiling)
{
    return IUniAI::Create(static_cast<const char*>(nullptr), isProfiling);
}

IUniAIPtr IUniAI::Create(const char* dynamicBackendPath, bool)
{
    g_lastUniAICreateBackendPath = dynamicBackendPath == nullptr ? std::string() : std::string(dynamicBackendPath);
    if (g_uniAIReturnNull.load()) {
        g_lastCreatedUniAI = nullptr;
        return IUniAIPtr(nullptr, &IUniAI::Destroy);
    }

    auto* raw = new StubUniAI();
    raw->backendState = static_cast<BackendState>(g_uniAIBackendState.load());
    raw->loadNetworkStatus = static_cast<Status>(g_uniAILoadStatus.load());
    raw->networkCompileStatus = static_cast<Status>(g_uniAICompileStatus.load());
    g_lastCreatedUniAI = raw;
    return IUniAIPtr(raw, &IUniAI::Destroy);
}

void IUniAI::Destroy(IUniAI* UniAI)
{
    delete static_cast<StubUniAI*>(UniAI);
}
}

namespace {
static unisoc::IUniAIPtr MakeStubUniAI()
{
    return unisoc::IUniAIPtr(new StubUniAI(), [](unisoc::IUniAI* ptr) { delete static_cast<StubUniAI*>(ptr); });
}

static unisoc::IUniAIPtr MakeNullUniAI()
{
    return unisoc::IUniAIPtr(nullptr, [](unisoc::IUniAI*) {});
}

template <typename T>
class ScopedAtomicStore final {
public:
    ScopedAtomicStore(std::atomic<T>& target, T value) : target_(target), old_(target.load())
    {
        target_.store(value);
    }
    ~ScopedAtomicStore()
    {
        target_.store(old_);
    }
    ScopedAtomicStore(const ScopedAtomicStore&) = delete;
    ScopedAtomicStore& operator=(const ScopedAtomicStore&) = delete;

private:
    std::atomic<T>& target_;
    T old_;
};

static OHOS::HDI::Nnrt::V2_0::SharedBuffer MakeBuf(int fd, uint32_t size, uint32_t offset, uint32_t dataSize)
{
    OHOS::HDI::Nnrt::V2_0::SharedBuffer b {};
    b.fd = fd;
    b.bufferSize = size;
    b.offset = offset;
    b.dataSize = dataSize;
    return b;
}

static unisoc::UniAITensor MakeTensor(const std::vector<int32_t>& dims,
    unisoc::DataType dt, unisoc::DataLayout dl, void* ptr)
{
    unisoc::TensorShape shape {};
    shape.m_NumDims = static_cast<unsigned int>(dims.size());
    shape.m_Dims.fill(1U);
    for (size_t i = 0; i < dims.size() && i < shape.m_Dims.size(); ++i) {
        shape.m_Dims[i] = static_cast<unsigned int>(dims[i]);
    }
    return unisoc::UniAITensor(shape, dl, dt, ptr);
}

struct AshmemBuffer {
    sptr<Ashmem> ash;
    SharedBuffer buffer;
};

static AshmemBuffer MakeAshmemBuffer(uint32_t size, const void* data, uint32_t dataSize)
{
    AshmemBuffer out {};
    out.ash = Ashmem::CreateAshmem("nnrt_test", static_cast<int32_t>(size));
    if (!out.ash) {
        out.buffer = MakeBuf(-1, size, 0, dataSize);
        return out;
    }
    out.ash->MapReadAndWriteAshmem();
    if (data != nullptr && dataSize > 0) {
        out.ash->WriteToAshmem(data, static_cast<int32_t>(dataSize), 0);
    }
    out.buffer.fd = out.ash->GetAshmemFd();
    out.buffer.bufferSize = static_cast<uint32_t>(out.ash->GetAshmemSize());
    out.buffer.offset = 0;
    out.buffer.dataSize = dataSize;
    return out;
}

static std::string DirName(const std::string& path)
{
    auto pos = path.find_last_of('/');
    if (pos == std::string::npos) {
        return {};
    }
    return path.substr(0, pos);
}

static bool ReadFileToVector(const std::string& path, std::vector<uint8_t>& out)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    file.seekg(0, std::ios::end);
    std::streamoff size = file.tellg();
    if (size <= 0) {
        return false;
    }
    out.resize(static_cast<size_t>(size));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(out.data()), size);
    return file.good() || file.eof();
}

static bool LoadModelData(std::vector<uint8_t>& out)
{
    std::string baseDir = DirName(__FILE__);
    std::vector<std::string> candidates {
        baseDir.empty() ? std::string() : baseDir + "/model.uir"
    };
    for (const auto& path : candidates) {
        if (!path.empty() && ReadFileToVector(path, out)) {
            return true;
        }
    }
    return false;
}
} // namespace

TEST(NnrtVdiImplTest, CreateDestroy)
{
    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);
    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(NnrtVdiImplTest, GetNames)
{
    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);

    std::string deviceName;
    EXPECT_EQ(vdi->GetDeviceName(deviceName), static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
    EXPECT_FALSE(deviceName.empty());

    std::string vendorName;
    EXPECT_EQ(vdi->GetVendorName(vendorName), static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
    EXPECT_FALSE(vendorName.empty());

    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(NnrtVdiImplTest, GetDeviceTypeAndSupports)
{
    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);

    DeviceType type {};
    EXPECT_EQ(vdi->GetDeviceType(type), static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
    EXPECT_EQ(type, DeviceType::ACCELERATOR);

    bool supported = false;
    EXPECT_EQ(vdi->IsFloat16PrecisionSupported(supported), static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
    EXPECT_TRUE(supported);
    EXPECT_EQ(vdi->IsPerformanceModeSupported(supported), static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
    EXPECT_TRUE(supported);
    EXPECT_EQ(vdi->IsPrioritySupported(supported), static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
    EXPECT_TRUE(supported);
    EXPECT_EQ(vdi->IsDynamicInputSupported(supported), static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
    EXPECT_TRUE(supported);

    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(NnrtVdiImplTest, GetSupportedOperationSize)
{
    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);

    OHOS::HDI::Nnrt::V2_0::Model model {};
    model.nodes.resize(3U);
    std::vector<bool> ops;
    EXPECT_EQ(vdi->GetSupportedOperation(model, ops), static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
    EXPECT_EQ(ops.size(), model.nodes.size());
    for (bool f : ops) {
        EXPECT_FALSE(f);
    }

    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(NnrtVdiImplTest, GetDeviceStatusCreateUniAIFail)
{
    ScopedAtomicStore<bool> guard(g_uniAIReturnNull, true);
    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);

    DeviceStatus status {};
    EXPECT_EQ(vdi->GetDeviceStatus(status), static_cast<int32_t>(NNRT_ReturnCode::NNRT_DEVICE_ERROR));
    EXPECT_EQ(status, DeviceStatus::UNKNOWN);

    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(NnrtVdiImplTest, GetDeviceStatusBackendStates)
{
    ScopedAtomicStore<bool> guard(g_uniAIReturnNull, false);
    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);

    DeviceStatus status {};

    g_uniAIBackendState.store(static_cast<int>(unisoc::BackendState::DEVICE_AVAILABLE));
    EXPECT_EQ(vdi->GetDeviceStatus(status), static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
    EXPECT_EQ(status, DeviceStatus::AVAILABLE);

    g_uniAIBackendState.store(static_cast<int>(unisoc::BackendState::DEVICE_BUSY));
    EXPECT_EQ(vdi->GetDeviceStatus(status), static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
    EXPECT_EQ(status, DeviceStatus::BUSY);

    g_uniAIBackendState.store(static_cast<int>(unisoc::BackendState::DEVICE_UNAVAILABLE));
    EXPECT_EQ(vdi->GetDeviceStatus(status), static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
    EXPECT_EQ(status, DeviceStatus::OFFLINE);

    g_uniAIBackendState.store(static_cast<int>(unisoc::BackendState::DEVICE_INVALID));
    EXPECT_EQ(vdi->GetDeviceStatus(status), static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
    EXPECT_EQ(status, DeviceStatus::OFFLINE);

    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(NnrtVdiImplTest, AllocateBufferZeroLength)
{
    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);

    SharedBuffer buffer {};
    EXPECT_EQ(vdi->AllocateBuffer(0, buffer), static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_BUFFER_SIZE));

    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(NnrtVdiImplTest, AllocateBufferCreateAshmemFail)
{
    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);

    SharedBuffer buffer {};
    EXPECT_EQ(vdi->AllocateBuffer(0xFFFFFFFFu, buffer), static_cast<int32_t>(NNRT_ReturnCode::NNRT_OUT_OF_MEMORY));

    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(NnrtVdiImplTest, AllocateBufferMapFail)
{
    ScopedAtomicStore<bool> guard(g_forceMapReadWriteFail, true);
    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);

    SharedBuffer buffer {};
    EXPECT_EQ(vdi->AllocateBuffer(4096, buffer), static_cast<int32_t>(NNRT_ReturnCode::NNRT_MEMORY_ERROR));

    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(NnrtVdiImplTest, AllocateAndReleaseBuffer)
{
    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);

    SharedBuffer buffer {};
    constexpr uint32_t kLen = 4096;
    EXPECT_EQ(vdi->AllocateBuffer(kLen, buffer), static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
    EXPECT_GE(buffer.fd, 0);
    EXPECT_GE(buffer.bufferSize, kLen);
    EXPECT_EQ(buffer.offset, 0u);
    EXPECT_EQ(buffer.dataSize, kLen);

    EXPECT_EQ(vdi->ReleaseBuffer(buffer), static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));

    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(NnrtVdiImplTest, DestructorReleasesBuffers)
{
    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);
    SharedBuffer buffer {};
    EXPECT_EQ(vdi->AllocateBuffer(1024, buffer), static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
    EXPECT_GE(buffer.fd, 0);
    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(NnrtVdiImplTest, ReleaseBufferNotExisting)
{
    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);
    SharedBuffer buf {};
    buf.fd = -1;
    EXPECT_EQ(vdi->ReleaseBuffer(buf), static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(NnrtVdiImplTest, PrepareModelApisMinimal)
{
    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);

    OHOS::HDI::Nnrt::V2_0::Model dummyModel {};
    ModelConfig config {};
    sptr<IPreparedModel> prepared;

    EXPECT_EQ(vdi->PrepareModel(dummyModel, config, prepared),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_NOT_SUPPORT));

    bool cacheSupported = false;
    EXPECT_EQ(vdi->IsModelCacheSupported(cacheSupported), static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
    EXPECT_TRUE(cacheSupported);

    std::vector<SharedBuffer> emptyCache;
    EXPECT_EQ(vdi->PrepareModelFromModelCache(emptyCache, config, prepared),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_MODEL));

    std::vector<SharedBuffer> emptyOffline;
    EXPECT_EQ(vdi->PrepareOfflineModel(emptyOffline, config, prepared),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_MODEL));

    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(NnrtVdiImplTest, PrepareModelFromModelCacheNonEmptyCallsOfflineModel)
{
    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);

    ModelConfig config {};
    sptr<IPreparedModel> prepared;
    std::vector<SharedBuffer> modelCache;
    modelCache.push_back(MakeBuf(-1, 100, 0, 100));

    EXPECT_EQ(vdi->PrepareModelFromModelCache(modelCache, config, prepared),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_BUFFER));

    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(NnrtVdiImplTest, PrepareOfflineModelDupFail)
{
    ScopedAtomicStore<bool> dupGuard(g_forceDupFail, true);
    ScopedAtomicStore<bool> createGuard(g_uniAIReturnNull, false);
    ScopedAtomicStore<int> loadGuard(g_uniAILoadStatus, static_cast<int>(unisoc::Status::AI_SUCCESS));
    ScopedAtomicStore<int> compileGuard(g_uniAICompileStatus, static_cast<int>(unisoc::Status::AI_SUCCESS));

    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);

    uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    auto modelBuf = MakeAshmemBuffer(sizeof(data), data, sizeof(data));
    ASSERT_NE(modelBuf.buffer.fd, -1);

    ModelConfig config {};
    sptr<IPreparedModel> prepared;
    std::vector<SharedBuffer> offline { modelBuf.buffer };
    EXPECT_EQ(vdi->PrepareOfflineModel(offline, config, prepared),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_BUFFER));

    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(NnrtVdiImplTest, PrepareOfflineModelMapReadOnlyFail)
{
    ScopedAtomicStore<bool> mapGuard(g_forceMapReadOnlyFail, true);
    ScopedAtomicStore<bool> createGuard(g_uniAIReturnNull, false);
    ScopedAtomicStore<int> loadGuard(g_uniAILoadStatus, static_cast<int>(unisoc::Status::AI_SUCCESS));
    ScopedAtomicStore<int> compileGuard(g_uniAICompileStatus, static_cast<int>(unisoc::Status::AI_SUCCESS));

    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);

    uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    auto modelBuf = MakeAshmemBuffer(sizeof(data), data, sizeof(data));
    ASSERT_NE(modelBuf.buffer.fd, -1);

    ModelConfig config {};
    sptr<IPreparedModel> prepared;
    std::vector<SharedBuffer> offline { modelBuf.buffer };
    EXPECT_EQ(vdi->PrepareOfflineModel(offline, config, prepared),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_MEMORY_ERROR));

    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(NnrtVdiImplTest, PrepareOfflineModelReadFromAshmemNull)
{
    ScopedAtomicStore<bool> mapGuard(g_forceMapReadOnlyNoMap, true);
    ScopedAtomicStore<bool> createGuard(g_uniAIReturnNull, false);
    ScopedAtomicStore<int> loadGuard(g_uniAILoadStatus, static_cast<int>(unisoc::Status::AI_SUCCESS));
    ScopedAtomicStore<int> compileGuard(g_uniAICompileStatus, static_cast<int>(unisoc::Status::AI_SUCCESS));

    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);

    uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    auto modelBuf = MakeAshmemBuffer(sizeof(data), data, sizeof(data));
    ASSERT_NE(modelBuf.buffer.fd, -1);

    ModelConfig config {};
    sptr<IPreparedModel> prepared;
    std::vector<SharedBuffer> offline { modelBuf.buffer };
    EXPECT_EQ(vdi->PrepareOfflineModel(offline, config, prepared),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_BUFFER));

    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(NnrtVdiImplTest, PrepareOfflineModelCreateUniAIFail)
{
    ScopedAtomicStore<bool> createGuard(g_uniAIReturnNull, true);

    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);

    uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    auto modelBuf = MakeAshmemBuffer(sizeof(data), data, sizeof(data));
    ASSERT_NE(modelBuf.buffer.fd, -1);

    ModelConfig config {};
    sptr<IPreparedModel> prepared;
    std::vector<SharedBuffer> offline { modelBuf.buffer };
    EXPECT_EQ(vdi->PrepareOfflineModel(offline, config, prepared),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_DEVICE_ERROR));

    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(NnrtVdiImplTest, PrepareOfflineModelReadsMissingExtensions)
{
    ScopedAtomicStore<bool> createGuard(g_uniAIReturnNull, false);
    ScopedAtomicStore<int> loadGuard(g_uniAILoadStatus, static_cast<int>(unisoc::Status::AI_SUCCESS));
    ScopedAtomicStore<int> compileGuard(g_uniAICompileStatus, static_cast<int>(unisoc::Status::AI_SUCCESS));

    g_lastUniAICreateBackendPath.clear();

    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);

    uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    auto modelBuf = MakeAshmemBuffer(sizeof(data), data, sizeof(data));
    ASSERT_NE(modelBuf.buffer.fd, -1);

    ModelConfig config {};
    sptr<IPreparedModel> prepared;
    std::vector<SharedBuffer> offline { modelBuf.buffer };
    EXPECT_EQ(vdi->PrepareOfflineModel(offline, config, prepared),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
    ASSERT_NE(prepared, nullptr);
    EXPECT_TRUE(g_lastUniAICreateBackendPath.empty());

    auto* raw = static_cast<StubUniAI*>(g_lastCreatedUniAI);
    ASSERT_NE(raw, nullptr);
    EXPECT_FALSE(raw->lastSaveCache);
    EXPECT_TRUE(raw->lastCachePath.empty());

    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(NnrtVdiImplTest, PrepareOfflineModelReadsExtensionsWithTrailingZeros)
{
    ScopedAtomicStore<bool> createGuard(g_uniAIReturnNull, false);
    ScopedAtomicStore<int> loadGuard(g_uniAILoadStatus, static_cast<int>(unisoc::Status::AI_SUCCESS));
    ScopedAtomicStore<int> compileGuard(g_uniAICompileStatus, static_cast<int>(unisoc::Status::AI_SUCCESS));

    g_lastUniAICreateBackendPath.clear();

    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);

    uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    auto modelBuf = MakeAshmemBuffer(sizeof(data), data, sizeof(data));
    ASSERT_NE(modelBuf.buffer.fd, -1);

    ModelConfig config {};
    config.extensions["uniai.backend_path"] = {'/', 'v', 'e', 'n', 'd', 'o', 'r', '/', 'l', 'i', 'b', '6', '4', '/',
        0, 0};
    config.extensions["uniai.enable_cache"] = {1, 0};
    config.extensions["uniai.cache_path"] = {'c', 'a', 'c', 'h', 'e', '/', 'd', 'i', 'r', 0, 0};

    sptr<IPreparedModel> prepared;
    std::vector<SharedBuffer> offline { modelBuf.buffer };
    EXPECT_EQ(vdi->PrepareOfflineModel(offline, config, prepared),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
    ASSERT_NE(prepared, nullptr);
    EXPECT_EQ(g_lastUniAICreateBackendPath, "/vendor/lib64/");

    auto* raw = static_cast<StubUniAI*>(g_lastCreatedUniAI);
    ASSERT_NE(raw, nullptr);
    EXPECT_TRUE(raw->lastSaveCache);
    EXPECT_EQ(raw->lastCachePath, "cache/dir");
    ASSERT_GE(raw->lastPreferentBackendList.size(), 2U);
    EXPECT_EQ(raw->lastPreferentBackendList[0], unisoc::UniAIBackends::NPU);
    EXPECT_EQ(raw->lastPreferentBackendList[1], unisoc::UniAIBackends::CPU);

    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(NnrtVdiImplTest, PrepareOfflineModelReadsEmptyExtensionsUseDefaults)
{
    ScopedAtomicStore<bool> createGuard(g_uniAIReturnNull, false);
    ScopedAtomicStore<int> loadGuard(g_uniAILoadStatus, static_cast<int>(unisoc::Status::AI_SUCCESS));
    ScopedAtomicStore<int> compileGuard(g_uniAICompileStatus, static_cast<int>(unisoc::Status::AI_SUCCESS));

    g_lastUniAICreateBackendPath.clear();

    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);

    uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    auto modelBuf = MakeAshmemBuffer(sizeof(data), data, sizeof(data));
    ASSERT_NE(modelBuf.buffer.fd, -1);

    ModelConfig config {};
    config.extensions["uniai.backend_path"] = {};
    config.extensions["uniai.enable_cache"] = {};
    config.extensions["uniai.cache_path"] = {};

    sptr<IPreparedModel> prepared;
    std::vector<SharedBuffer> offline { modelBuf.buffer };
    EXPECT_EQ(vdi->PrepareOfflineModel(offline, config, prepared),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
    ASSERT_NE(prepared, nullptr);
    EXPECT_TRUE(g_lastUniAICreateBackendPath.empty());

    auto* raw = static_cast<StubUniAI*>(g_lastCreatedUniAI);
    ASSERT_NE(raw, nullptr);
    EXPECT_FALSE(raw->lastSaveCache);
    EXPECT_TRUE(raw->lastCachePath.empty());

    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(NnrtVdiImplTest, PrepareOfflineModelLoadNetworkStatusMappings)
{
    struct Case {
        unisoc::Status status;
        NNRT_ReturnCode expected;
    };
    std::vector<Case> cases = {
        { unisoc::Status::AI_SMALL_INPUT_SIZE, NNRT_ReturnCode::NNRT_INSUFFICIENT_BUFFER },
        { unisoc::Status::AI_SMALL_OUTPUT_SIZE, NNRT_ReturnCode::NNRT_INSUFFICIENT_BUFFER },
        { unisoc::Status::AI_INVALID_ARGUMENT, NNRT_ReturnCode::NNRT_INVALID_PARAMETER },
        { unisoc::Status::AI_FILE_NOT_FOUND, NNRT_ReturnCode::NNRT_INVALID_FILE },
        { unisoc::Status::AI_PARSE_ERROR, NNRT_ReturnCode::NNRT_INVALID_MODEL },
        { unisoc::Status::AI_FAILURE, NNRT_ReturnCode::NNRT_FAILED },
    };

    for (const auto& item : cases) {
        ScopedAtomicStore<bool> createGuard(g_uniAIReturnNull, false);
        ScopedAtomicStore<int> loadGuard(g_uniAILoadStatus, static_cast<int>(item.status));
        ScopedAtomicStore<int> compileGuard(g_uniAICompileStatus, static_cast<int>(unisoc::Status::AI_SUCCESS));

        INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
        ASSERT_NE(vdi, nullptr);

        uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
        auto modelBuf = MakeAshmemBuffer(sizeof(data), data, sizeof(data));
        ASSERT_NE(modelBuf.buffer.fd, -1);

        ModelConfig config {};
        sptr<IPreparedModel> prepared;
        std::vector<SharedBuffer> offline { modelBuf.buffer };
        EXPECT_EQ(vdi->PrepareOfflineModel(offline, config, prepared), static_cast<int32_t>(item.expected));

        OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
    }
}

TEST(NnrtVdiImplTest, PrepareOfflineModelCompileStatusMappings)
{
    struct Case {
        unisoc::Status status;
        NNRT_ReturnCode expected;
    };
    std::vector<Case> cases = {
        { unisoc::Status::AI_SMALL_INPUT_SIZE, NNRT_ReturnCode::NNRT_INSUFFICIENT_BUFFER },
        { unisoc::Status::AI_SMALL_OUTPUT_SIZE, NNRT_ReturnCode::NNRT_INSUFFICIENT_BUFFER },
        { unisoc::Status::AI_INVALID_ARGUMENT, NNRT_ReturnCode::NNRT_INVALID_PARAMETER },
        { unisoc::Status::AI_FILE_NOT_FOUND, NNRT_ReturnCode::NNRT_INVALID_FILE },
        { unisoc::Status::AI_PARSE_ERROR, NNRT_ReturnCode::NNRT_INVALID_MODEL },
        { unisoc::Status::AI_FAILURE, NNRT_ReturnCode::NNRT_FAILED },
    };

    for (const auto& item : cases) {
        ScopedAtomicStore<bool> createGuard(g_uniAIReturnNull, false);
        ScopedAtomicStore<int> loadGuard(g_uniAILoadStatus, static_cast<int>(unisoc::Status::AI_SUCCESS));
        ScopedAtomicStore<int> compileGuard(g_uniAICompileStatus, static_cast<int>(item.status));

        INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
        ASSERT_NE(vdi, nullptr);

        uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
        auto modelBuf = MakeAshmemBuffer(sizeof(data), data, sizeof(data));
        ASSERT_NE(modelBuf.buffer.fd, -1);

        ModelConfig config {};
        sptr<IPreparedModel> prepared;
        std::vector<SharedBuffer> offline { modelBuf.buffer };
        EXPECT_EQ(vdi->PrepareOfflineModel(offline, config, prepared), static_cast<int32_t>(item.expected));

        OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
    }
}

TEST(PreparedModelImplTest, ExportModelCacheNotSupport)
{
    PreparedModelImpl model(MakeNullUniAI(), unisoc::NetworkId {}, nullptr);
    std::vector<SharedBuffer> modelCache;
    EXPECT_EQ(model.ExportModelCache(modelCache),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_NOT_SUPPORT));
}

TEST(PreparedModelImplTest, GetInputDimRangesWithNullUniAI)
{
    PreparedModelImpl model(MakeNullUniAI(), unisoc::NetworkId {}, nullptr);
    std::vector<std::vector<uint32_t>> minDims;
    std::vector<std::vector<uint32_t>> maxDims;
    EXPECT_EQ(model.GetInputDimRanges(minDims, maxDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_NULL_PTR));
}

TEST(PreparedModelImplTest, RunWithNullUniAI)
{
    PreparedModelImpl model(MakeNullUniAI(), unisoc::NetworkId {}, nullptr);
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs;
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs;
    std::vector<std::vector<int32_t>> outputDims;
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_NULL_PTR));
}

TEST(PreparedModelImplTest, GetInputDimRangesEmptyInfos)
{
    auto stub = MakeStubUniAI();
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    std::vector<std::vector<uint32_t>> minDims;
    std::vector<std::vector<uint32_t>> maxDims;
    EXPECT_EQ(model.GetInputDimRanges(minDims, maxDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_FAILED));
}

TEST(PreparedModelImplTest, GetInputDimRangesInvalidNumDimsZero)
{
    auto stub = MakeStubUniAI();
    auto raw = static_cast<StubUniAI*>(stub.get());
    unisoc::IoInfo info {};
    info.m_Shape.m_NumDims = 0;
    raw->inputInfos = { info };
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    std::vector<std::vector<uint32_t>> minDims;
    std::vector<std::vector<uint32_t>> maxDims;
    EXPECT_EQ(model.GetInputDimRanges(minDims, maxDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_SHAPE));
}

TEST(PreparedModelImplTest, GetInputDimRangesInvalidNumDimsTooLarge)
{
    auto stub = MakeStubUniAI();
    auto raw = static_cast<StubUniAI*>(stub.get());
    unisoc::IoInfo info {};
    info.m_Shape.m_Dims.fill(1U);
    info.m_Shape.m_NumDims = static_cast<unsigned int>(info.m_Shape.m_Dims.size() + 1U);
    raw->inputInfos = { info };
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    std::vector<std::vector<uint32_t>> minDims;
    std::vector<std::vector<uint32_t>> maxDims;
    EXPECT_EQ(model.GetInputDimRanges(minDims, maxDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_SHAPE));
}

TEST(PreparedModelImplTest, GetInputDimRangesInvalidZeroDim)
{
    auto stub = MakeStubUniAI();
    auto raw = static_cast<StubUniAI*>(stub.get());
    unisoc::IoInfo info {};
    info.m_Shape.m_Dims.fill(0U);
    info.m_Shape.m_NumDims = 1U;
    raw->inputInfos = { info };
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    std::vector<std::vector<uint32_t>> minDims;
    std::vector<std::vector<uint32_t>> maxDims;
    EXPECT_EQ(model.GetInputDimRanges(minDims, maxDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_SHAPE));
}

TEST(PreparedModelImplTest, GetInputDimRangesSuccess)
{
    auto stub = MakeStubUniAI();
    auto raw = static_cast<StubUniAI*>(stub.get());
    unisoc::IoInfo info {};
    info.m_Shape.m_Dims.fill(1U);
    info.m_Shape.m_NumDims = 3U;
    raw->inputInfos = { info };
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    std::vector<std::vector<uint32_t>> minDims;
    std::vector<std::vector<uint32_t>> maxDims;
    EXPECT_EQ(model.GetInputDimRanges(minDims, maxDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
    ASSERT_EQ(minDims.size(), 1U);
    ASSERT_EQ(maxDims.size(), 1U);
    ASSERT_EQ(minDims[0].size(), info.m_Shape.m_NumDims);
    ASSERT_EQ(maxDims[0].size(), info.m_Shape.m_NumDims);
}

TEST(PreparedModelImplTest, RunInvalidInputBufferFd)
{
    auto stub = MakeStubUniAI();
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor in {};
    in.data = MakeBuf(-1, 100, 0, 100);
    in.dimensions = {1};
    in.dataType = OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_UINT8;
    in.format = OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NHWC;
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs { in };
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs;
    std::vector<std::vector<int32_t>> outputDims;
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_BUFFER));
}

TEST(PreparedModelImplTest, RunInvalidOutputBufferSize)
{
    auto stub = MakeStubUniAI();
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor in {};
    uint8_t inputData[4] = {1, 2, 3, 4};
    auto inBuf = MakeAshmemBuffer(4, inputData, sizeof(inputData));
    ASSERT_NE(inBuf.buffer.fd, -1);
    in.data = inBuf.buffer;
    in.dimensions = {1};
    in.dataType = OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_UINT8;
    in.format = OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NHWC;
    OHOS::HDI::Nnrt::V2_0::IOTensor out {};
    out.data = MakeBuf(-1, 100, 0, 100);
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs { in };
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs { out };
    std::vector<std::vector<int32_t>> outputDims;
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_BUFFER));
}

TEST(PreparedModelImplTest, RunInferenceFailed)
{
    auto stub = MakeStubUniAI();
    auto raw = static_cast<StubUniAI*>(stub.get());
    raw->inferStatus = unisoc::Status::AI_INVALID_ARGUMENT;
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor in {};
    uint8_t inputData[4] = {1, 2, 3, 4};
    auto inBuf = MakeAshmemBuffer(4, inputData, sizeof(inputData));
    auto outBuf = MakeAshmemBuffer(4, nullptr, 4);
    ASSERT_NE(inBuf.buffer.fd, -1);
    ASSERT_NE(outBuf.buffer.fd, -1);
    in.data = inBuf.buffer;
    in.dimensions = {1};
    in.dataType = OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_UINT8;
    in.format = OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NHWC;
    OHOS::HDI::Nnrt::V2_0::IOTensor out {};
    out.data = outBuf.buffer;
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs { in };
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs { out };
    std::vector<std::vector<int32_t>> outputDims;
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_PARAMETER));
}

TEST(PreparedModelImplTest, RunOutputSizeMismatch)
{
    auto stub = MakeStubUniAI();
    auto raw = static_cast<StubUniAI*>(stub.get());
    raw->inferOutputs = {};
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor in {};
    uint8_t inputData[4] = {1, 2, 3, 4};
    auto inBuf = MakeAshmemBuffer(4, inputData, sizeof(inputData));
    auto outBuf = MakeAshmemBuffer(4, nullptr, 4);
    ASSERT_NE(inBuf.buffer.fd, -1);
    ASSERT_NE(outBuf.buffer.fd, -1);
    in.data = inBuf.buffer;
    in.dimensions = {1};
    in.dataType = OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_UINT8;
    in.format = OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NHWC;
    OHOS::HDI::Nnrt::V2_0::IOTensor out {};
    out.data = outBuf.buffer;
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs { in };
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs { out };
    std::vector<std::vector<int32_t>> outputDims;
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_FAILED));
}

TEST(PreparedModelImplTest, RunWriteOutputInsufficientBuffer)
{
    auto stub = MakeStubUniAI();
    auto raw = static_cast<StubUniAI*>(stub.get());
    std::vector<float> storage(4, 0.5f);
    auto big = MakeTensor({4}, unisoc::DataType::Float32, unisoc::DataLayout::NHWC, storage.data());
    raw->inferOutputs = { big };
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor in {};
    uint8_t inputData[16] = {};
    auto inBuf = MakeAshmemBuffer(16, inputData, sizeof(inputData));
    auto outBuf = MakeAshmemBuffer(4, nullptr, 4);
    ASSERT_NE(inBuf.buffer.fd, -1);
    ASSERT_NE(outBuf.buffer.fd, -1);
    in.data = inBuf.buffer;
    in.dimensions = {1};
    in.dataType = OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_UINT8;
    in.format = OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NHWC;
    OHOS::HDI::Nnrt::V2_0::IOTensor out {};
    out.data = outBuf.buffer;
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs { in };
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs { out };
    std::vector<std::vector<int32_t>> outputDims;
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_INSUFFICIENT_BUFFER));
}

TEST(PreparedModelImplTest, RunSuccess)
{
    auto stub = MakeStubUniAI();
    auto raw = static_cast<StubUniAI*>(stub.get());
    std::vector<float> storage(4, 0.5f);
    auto outTensor = MakeTensor({4}, unisoc::DataType::Float32, unisoc::DataLayout::NHWC, storage.data());
    raw->inferOutputs = { outTensor };
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor in {};
    uint8_t inputData[16] = {};
    auto inBuf = MakeAshmemBuffer(16, inputData, sizeof(inputData));
    auto outBuf = MakeAshmemBuffer(16, nullptr, 16);
    ASSERT_NE(inBuf.buffer.fd, -1);
    ASSERT_NE(outBuf.buffer.fd, -1);
    in.data = inBuf.buffer;
    in.dimensions = {1};
    in.dataType = OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_UINT8;
    in.format = OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NHWC;
    OHOS::HDI::Nnrt::V2_0::IOTensor out {};
    out.data = outBuf.buffer;
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs { in };
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs { out };
    std::vector<std::vector<int32_t>> outputDims;
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
    ASSERT_EQ(outputDims.size(), 1U);
}

TEST(NnrtVdiImplTest, PrepareOfflineModelInvalidBuffers)
{
    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);
    OHOS::HDI::Nnrt::V2_0::ModelConfig cfg {};
    OHOS::HDI::Nnrt::V2_0::sptr<OHOS::HDI::Nnrt::V2_0::IPreparedModel> pm;
    std::vector<SharedBuffer> bufs;
    bufs.push_back(MakeBuf(-1, 100, 0, 100));
    EXPECT_EQ(vdi->PrepareOfflineModel(bufs, cfg, pm), static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_BUFFER));
    bufs[0] = MakeBuf(0, 0, 0, 100);
    EXPECT_EQ(vdi->PrepareOfflineModel(bufs, cfg, pm), static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_BUFFER));
    bufs[0] = MakeBuf(0, 100, 0, 0);
    EXPECT_EQ(vdi->PrepareOfflineModel(bufs, cfg, pm), static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_BUFFER));
    bufs[0] = MakeBuf(0, 100, 80, 50);
    EXPECT_EQ(vdi->PrepareOfflineModel(bufs, cfg, pm), static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_BUFFER));
    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(NnrtVdiImplTest, PrepareOfflineModelWithActualModel)
{
    std::vector<uint8_t> modelData;
    if (!LoadModelData(modelData)) {
        SUCCEED();
        return;
    }
    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);
    auto modelBuf = MakeAshmemBuffer(static_cast<uint32_t>(modelData.size()), modelData.data(),
        static_cast<uint32_t>(modelData.size()));
    ASSERT_NE(modelBuf.buffer.fd, -1);
    ModelConfig cfg {};
    sptr<IPreparedModel> pm;
    std::vector<SharedBuffer> bufs { modelBuf.buffer };
    int32_t ret = vdi->PrepareOfflineModel(bufs, cfg, pm);
    EXPECT_NE(ret, static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_BUFFER));
    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(PreparedModelImplTest, RunInvalidInputDimsNegative)
{
    auto stub = MakeStubUniAI();
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor in {};
    uint8_t inputData[4] = {1, 2, 3, 4};
    auto inBuf = MakeAshmemBuffer(4, inputData, sizeof(inputData));
    ASSERT_NE(inBuf.buffer.fd, -1);
    in.data = inBuf.buffer;
    in.dimensions = {-1};
    in.dataType = OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_UINT8;
    in.format = OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NHWC;
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs { in };
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs;
    std::vector<std::vector<int32_t>> outputDims;
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_SHAPE));
}

TEST(PreparedModelImplTest, RunInvalidInputDimsTooLarge)
{
    auto stub = MakeStubUniAI();
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor in {};
    uint8_t inputData[16] = {};
    auto inBuf = MakeAshmemBuffer(16, inputData, sizeof(inputData));
    ASSERT_NE(inBuf.buffer.fd, -1);
    in.data = inBuf.buffer;
    in.dimensions = {1, 1, 1, 1, 1, 1};
    in.dataType = OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_UINT8;
    in.format = OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NHWC;
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs { in };
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs;
    std::vector<std::vector<int32_t>> outputDims;
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_SHAPE));
}

TEST(PreparedModelImplTest, RunInvalidInputDupFailed)
{
    auto stub = MakeStubUniAI();
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor in {};
    uint8_t inputData[4] = {1, 2, 3, 4};
    auto inBuf = MakeAshmemBuffer(4, inputData, sizeof(inputData));
    ASSERT_NE(inBuf.buffer.fd, -1);
    close(inBuf.buffer.fd);
    in.data = inBuf.buffer;
    in.dimensions = {1};
    in.dataType = OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_UINT8;
    in.format = OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NHWC;
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs { in };
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs;
    std::vector<std::vector<int32_t>> outputDims;
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_BUFFER));
}

TEST(PreparedModelImplTest, RunInvalidInputAshmemAllocFail)
{
    auto stub = MakeStubUniAI();
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor in {};
    uint8_t inputData[4] = {1, 2, 3, 4};
    auto inBuf = MakeAshmemBuffer(4, inputData, sizeof(inputData));
    ASSERT_NE(inBuf.buffer.fd, -1);
    in.data = inBuf.buffer;
    in.dimensions = {1};
    in.dataType = OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_UINT8;
    in.format = OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NHWC;
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs { in };
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs;
    std::vector<std::vector<int32_t>> outputDims;
    ScopedAtomicStore<int> newFailGuard(g_failNothrowNewCount, 1);
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_OUT_OF_MEMORY));
}

TEST(PreparedModelImplTest, RunInvalidInputMapReadOnlyFailed)
{
    auto stub = MakeStubUniAI();
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor in {};
    uint8_t inputData[4] = {1, 2, 3, 4};
    auto inBuf = MakeAshmemBuffer(4, inputData, sizeof(inputData));
    ASSERT_NE(inBuf.buffer.fd, -1);
    in.data = inBuf.buffer;
    in.dimensions = {1};
    in.dataType = OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_UINT8;
    in.format = OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NHWC;
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs { in };
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs;
    std::vector<std::vector<int32_t>> outputDims;
    ScopedAtomicStore<bool> mapFailGuard(g_forceMapReadOnlyFail, true);
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_MEMORY_ERROR));
}

TEST(PreparedModelImplTest, RunInvalidInputReadFromAshmemNull)
{
    auto stub = MakeStubUniAI();
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor in {};
    uint8_t inputData[4] = {1, 2, 3, 4};
    auto inBuf = MakeAshmemBuffer(4, inputData, sizeof(inputData));
    ASSERT_NE(inBuf.buffer.fd, -1);
    in.data = inBuf.buffer;
    in.dimensions = {1};
    in.dataType = OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_UINT8;
    in.format = OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NHWC;
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs { in };
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs;
    std::vector<std::vector<int32_t>> outputDims;
    ScopedAtomicStore<bool> noMapGuard(g_forceMapReadOnlyNoMap, true);
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_BUFFER));
}

TEST(PreparedModelImplTest, RunInvalidInputDatatype)
{
    auto stub = MakeStubUniAI();
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor in {};
    uint8_t inputData[4] = {1, 2, 3, 4};
    auto inBuf = MakeAshmemBuffer(4, inputData, sizeof(inputData));
    ASSERT_NE(inBuf.buffer.fd, -1);
    in.data = inBuf.buffer;
    in.dimensions = {1};
    in.dataType = OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_FLOAT64;
    in.format = OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NHWC;
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs { in };
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs;
    std::vector<std::vector<int32_t>> outputDims;
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_DATATYPE));
}

TEST(PreparedModelImplTest, RunInvalidOutputDataSizeZero)
{
    auto stub = MakeStubUniAI();
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor in {};
    uint8_t inputData[4] = {1, 2, 3, 4};
    auto inBuf = MakeAshmemBuffer(4, inputData, sizeof(inputData));
    auto outBuf = MakeAshmemBuffer(16, nullptr, 16);
    ASSERT_NE(inBuf.buffer.fd, -1);
    ASSERT_NE(outBuf.buffer.fd, -1);
    in.data = inBuf.buffer;
    in.dimensions = {1};
    in.dataType = OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_UINT8;
    in.format = OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NHWC;
    OHOS::HDI::Nnrt::V2_0::IOTensor out {};
    out.data = MakeBuf(outBuf.buffer.fd, outBuf.buffer.bufferSize, outBuf.buffer.offset, 0);
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs { in };
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs { out };
    std::vector<std::vector<int32_t>> outputDims;
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_BUFFER));
}

TEST(PreparedModelImplTest, RunInvalidOutputOffsetOverflow)
{
    auto stub = MakeStubUniAI();
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor in {};
    uint8_t inputData[4] = {1, 2, 3, 4};
    auto inBuf = MakeAshmemBuffer(4, inputData, sizeof(inputData));
    auto outBuf = MakeAshmemBuffer(16, nullptr, 16);
    ASSERT_NE(inBuf.buffer.fd, -1);
    ASSERT_NE(outBuf.buffer.fd, -1);
    in.data = inBuf.buffer;
    in.dimensions = {1};
    in.dataType = OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_UINT8;
    in.format = OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NHWC;
    OHOS::HDI::Nnrt::V2_0::IOTensor out {};
    out.data = MakeBuf(outBuf.buffer.fd, outBuf.buffer.bufferSize, 8, 16);
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs { in };
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs { out };
    std::vector<std::vector<int32_t>> outputDims;
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_BUFFER));
}

TEST(PreparedModelImplTest, RunInvalidOutputDupFailed)
{
    auto stub = MakeStubUniAI();
    auto raw = static_cast<StubUniAI*>(stub.get());
    std::vector<float> storage(4, 0.5f);
    raw->inferOutputs = { MakeTensor({4}, unisoc::DataType::Float32, unisoc::DataLayout::NHWC, storage.data()) };

    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor out {};
    auto outBuf = MakeAshmemBuffer(16, nullptr, 16);
    ASSERT_NE(outBuf.buffer.fd, -1);
    out.data = outBuf.buffer;

    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs;
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs { out };
    std::vector<std::vector<int32_t>> outputDims;
    ScopedAtomicStore<bool> dupFailGuard(g_forceDupFail, true);
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_INVALID_BUFFER));
}

TEST(PreparedModelImplTest, RunInvalidOutputAshmemAllocFail)
{
    auto stub = MakeStubUniAI();
    auto raw = static_cast<StubUniAI*>(stub.get());
    std::vector<float> storage(4, 0.5f);
    raw->inferOutputs = { MakeTensor({4}, unisoc::DataType::Float32, unisoc::DataLayout::NHWC, storage.data()) };

    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor out {};
    auto outBuf = MakeAshmemBuffer(16, nullptr, 16);
    ASSERT_NE(outBuf.buffer.fd, -1);
    out.data = outBuf.buffer;

    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs;
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs { out };
    std::vector<std::vector<int32_t>> outputDims;
    ScopedAtomicStore<int> newFailGuard(g_failNothrowNewCount, 1);
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_OUT_OF_MEMORY));
}

TEST(PreparedModelImplTest, RunInvalidOutputMapReadWriteFailed)
{
    auto stub = MakeStubUniAI();
    auto raw = static_cast<StubUniAI*>(stub.get());
    std::vector<float> storage(4, 0.5f);
    raw->inferOutputs = { MakeTensor({4}, unisoc::DataType::Float32, unisoc::DataLayout::NHWC, storage.data()) };

    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor out {};
    auto outBuf = MakeAshmemBuffer(16, nullptr, 16);
    ASSERT_NE(outBuf.buffer.fd, -1);
    out.data = outBuf.buffer;

    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs;
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs { out };
    std::vector<std::vector<int32_t>> outputDims;
    ScopedAtomicStore<bool> mapFailGuard(g_forceMapReadWriteFail, true);
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_MEMORY_ERROR));
}

TEST(PreparedModelImplTest, RunWriteOutputToAshmemFailed)
{
    auto stub = MakeStubUniAI();
    auto raw = static_cast<StubUniAI*>(stub.get());
    std::vector<float> storage(4, 0.5f);
    raw->inferOutputs = { MakeTensor({4}, unisoc::DataType::Float32, unisoc::DataLayout::NHWC, storage.data()) };

    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor out {};
    auto outBuf = MakeAshmemBuffer(16, nullptr, 16);
    ASSERT_NE(outBuf.buffer.fd, -1);
    out.data = outBuf.buffer;

    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs;
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs { out };
    std::vector<std::vector<int32_t>> outputDims;
    ScopedAtomicStore<bool> noMapGuard(g_forceMapReadWriteNoMap, true);
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_MEMORY_ERROR));
}

TEST(PreparedModelImplTest, RunInferenceSmallOutputSize)
{
    auto stub = MakeStubUniAI();
    auto raw = static_cast<StubUniAI*>(stub.get());
    raw->inferStatus = unisoc::Status::AI_SMALL_OUTPUT_SIZE;
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor in {};
    uint8_t inputData[4] = {1, 2, 3, 4};
    auto inBuf = MakeAshmemBuffer(4, inputData, sizeof(inputData));
    auto outBuf = MakeAshmemBuffer(4, nullptr, 4);
    ASSERT_NE(inBuf.buffer.fd, -1);
    ASSERT_NE(outBuf.buffer.fd, -1);
    in.data = inBuf.buffer;
    in.dimensions = {1};
    in.dataType = OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_UINT8;
    in.format = OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NHWC;
    OHOS::HDI::Nnrt::V2_0::IOTensor out {};
    out.data = outBuf.buffer;
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs { in };
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs { out };
    std::vector<std::vector<int32_t>> outputDims;
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_INSUFFICIENT_BUFFER));
}

TEST(PreparedModelImplTest, RunInferenceStatusMappings)
{
    struct Case {
        unisoc::Status status;
        NNRT_ReturnCode expected;
    };
    std::vector<Case> cases = {
        { unisoc::Status::AI_FILE_NOT_FOUND, NNRT_ReturnCode::NNRT_INVALID_FILE },
        { unisoc::Status::AI_PARSE_ERROR, NNRT_ReturnCode::NNRT_INVALID_MODEL },
        { unisoc::Status::AI_FAILURE, NNRT_ReturnCode::NNRT_FAILED },
    };
    for (const auto& item : cases) {
        auto stub = MakeStubUniAI();
        auto raw = static_cast<StubUniAI*>(stub.get());
        raw->inferStatus = item.status;
        PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
        OHOS::HDI::Nnrt::V2_0::IOTensor in {};
        uint8_t inputData[4] = {1, 2, 3, 4};
        auto inBuf = MakeAshmemBuffer(4, inputData, sizeof(inputData));
        auto outBuf = MakeAshmemBuffer(16, nullptr, 16);
        ASSERT_NE(inBuf.buffer.fd, -1);
        ASSERT_NE(outBuf.buffer.fd, -1);
        in.data = inBuf.buffer;
        in.dimensions = {1};
        in.dataType = OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_UINT8;
        in.format = OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NHWC;
        OHOS::HDI::Nnrt::V2_0::IOTensor out {};
        out.data = outBuf.buffer;
        std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs { in };
        std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs { out };
        std::vector<std::vector<int32_t>> outputDims;
        EXPECT_EQ(model.Run(inputs, outputs, outputDims), static_cast<int32_t>(item.expected));
    }
}

TEST(PreparedModelImplTest, RunWithVariousInputTypesAndLayouts)
{
    struct Case {
        OHOS::HDI::Nnrt::V2_0::DataType type;
        OHOS::HDI::Nnrt::V2_0::Format format;
    };
    std::vector<Case> cases = {
        { OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_FLOAT16, OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NCHW },
        { OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_FLOAT32, OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NHWC },
        { OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_INT8, OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NONE },
        { OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_INT32, OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NCHW },
        { OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_INT64, OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NONE },
    };
    for (const auto& item : cases) {
        auto stub = MakeStubUniAI();
        auto raw = static_cast<StubUniAI*>(stub.get());
        std::vector<float> storage(4, 0.25f);
        auto outTensor = MakeTensor({4}, unisoc::DataType::Float32, unisoc::DataLayout::NHWC, storage.data());
        raw->inferOutputs = { outTensor };
        PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
        OHOS::HDI::Nnrt::V2_0::IOTensor in {};
        uint8_t inputData[16] = {};
        auto inBuf = MakeAshmemBuffer(16, inputData, sizeof(inputData));
        auto outBuf = MakeAshmemBuffer(32, nullptr, 32);
        ASSERT_NE(inBuf.buffer.fd, -1);
        ASSERT_NE(outBuf.buffer.fd, -1);
        in.data = inBuf.buffer;
        in.dimensions = {1};
        in.dataType = item.type;
        in.format = item.format;
        OHOS::HDI::Nnrt::V2_0::IOTensor out {};
        out.data = outBuf.buffer;
        std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs { in };
        std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs { out };
        std::vector<std::vector<int32_t>> outputDims;
        EXPECT_EQ(model.Run(inputs, outputs, outputDims),
            static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
        ASSERT_EQ(outputDims.size(), 1U);
    }
}

TEST(PreparedModelImplTest, RunWriteOutputVariousTypes)
{
    struct Case {
        unisoc::DataType type;
        size_t elemSize;
    };
    std::vector<Case> cases = {
        { unisoc::DataType::QuantisedAsymm8, sizeof(uint8_t) },
        { unisoc::DataType::Signed32, sizeof(int32_t) },
        { unisoc::DataType::Signed64, sizeof(int64_t) },
        { unisoc::DataType::Float16, sizeof(uint16_t) },
    };
    for (const auto& item : cases) {
        auto stub = MakeStubUniAI();
        auto raw = static_cast<StubUniAI*>(stub.get());
        std::vector<uint8_t> storage(32, 1);
        auto outTensor = MakeTensor({4}, item.type, unisoc::DataLayout::NHWC, storage.data());
        raw->inferOutputs = { outTensor };
        PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
        OHOS::HDI::Nnrt::V2_0::IOTensor in {};
        uint8_t inputData[16] = {};
        auto inBuf = MakeAshmemBuffer(16, inputData, sizeof(inputData));
        size_t outBytes = 4 * item.elemSize;
        auto outBuf = MakeAshmemBuffer(static_cast<uint32_t>(outBytes), nullptr, static_cast<uint32_t>(outBytes));
        ASSERT_NE(inBuf.buffer.fd, -1);
        ASSERT_NE(outBuf.buffer.fd, -1);
        in.data = inBuf.buffer;
        in.dimensions = {1};
        in.dataType = OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_UINT8;
        in.format = OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NHWC;
        OHOS::HDI::Nnrt::V2_0::IOTensor out {};
        out.data = outBuf.buffer;
        std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs { in };
        std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs { out };
        std::vector<std::vector<int32_t>> outputDims;
        EXPECT_EQ(model.Run(inputs, outputs, outputDims),
            static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
        ASSERT_EQ(outputDims.size(), 1U);
    }
}

TEST(PreparedModelImplTest, RunWriteOutputInvalidDataPtrType)
{
    auto stub = MakeStubUniAI();
    auto raw = static_cast<StubUniAI*>(stub.get());
    std::vector<uint16_t> storage(4, 1);
    auto outTensor = MakeTensor({4}, unisoc::DataType::QSymmS16, unisoc::DataLayout::NHWC, storage.data());
    raw->inferOutputs = { outTensor };
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor in {};
    uint8_t inputData[16] = {};
    auto inBuf = MakeAshmemBuffer(16, inputData, sizeof(inputData));
    auto outBuf = MakeAshmemBuffer(16, nullptr, 16);
    ASSERT_NE(inBuf.buffer.fd, -1);
    ASSERT_NE(outBuf.buffer.fd, -1);
    in.data = inBuf.buffer;
    in.dimensions = {1};
    in.dataType = OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_UINT8;
    in.format = OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NHWC;
    OHOS::HDI::Nnrt::V2_0::IOTensor out {};
    out.data = outBuf.buffer;
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs { in };
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs { out };
    std::vector<std::vector<int32_t>> outputDims;
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_FAILED));
}

TEST(PreparedModelImplTest, RunWriteOutputInvalidElementCountZero)
{
    auto stub = MakeStubUniAI();
    auto raw = static_cast<StubUniAI*>(stub.get());
    std::vector<float> storage(4, 0.0f);
    auto outTensor = MakeTensor({0}, unisoc::DataType::Float32, unisoc::DataLayout::NHWC, storage.data());
    raw->inferOutputs = { outTensor };
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor in {};
    uint8_t inputData[16] = {};
    auto inBuf = MakeAshmemBuffer(16, inputData, sizeof(inputData));
    auto outBuf = MakeAshmemBuffer(16, nullptr, 16);
    ASSERT_NE(inBuf.buffer.fd, -1);
    ASSERT_NE(outBuf.buffer.fd, -1);
    in.data = inBuf.buffer;
    in.dimensions = {1};
    in.dataType = OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_UINT8;
    in.format = OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NHWC;
    OHOS::HDI::Nnrt::V2_0::IOTensor out {};
    out.data = outBuf.buffer;
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs { in };
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs { out };
    std::vector<std::vector<int32_t>> outputDims;
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_FAILED));
}

TEST(PreparedModelImplTest, RunInputFloat16Nchw)
{
    auto stub = MakeStubUniAI();
    auto raw = static_cast<StubUniAI*>(stub.get());
    std::vector<uint16_t> storage(4, 1);
    auto outTensor = MakeTensor({4}, unisoc::DataType::Float16, unisoc::DataLayout::NHWC, storage.data());
    raw->inferOutputs = { outTensor };
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor in {};
    uint8_t inputData[16] = {};
    auto inBuf = MakeAshmemBuffer(16, inputData, sizeof(inputData));
    auto outBuf = MakeAshmemBuffer(8, nullptr, 8);
    ASSERT_NE(inBuf.buffer.fd, -1);
    ASSERT_NE(outBuf.buffer.fd, -1);
    in.data = inBuf.buffer;
    in.dimensions = {1};
    in.dataType = OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_FLOAT16;
    in.format = OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NCHW;
    OHOS::HDI::Nnrt::V2_0::IOTensor out {};
    out.data = outBuf.buffer;
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs { in };
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs { out };
    std::vector<std::vector<int32_t>> outputDims;
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
    ASSERT_EQ(outputDims.size(), 1U);
}

TEST(PreparedModelImplTest, RunInputFloat32UnknownLayout)
{
    auto stub = MakeStubUniAI();
    auto raw = static_cast<StubUniAI*>(stub.get());
    std::vector<float> storage(4, 0.5f);
    auto outTensor = MakeTensor({4}, unisoc::DataType::Float32, unisoc::DataLayout::NHWC, storage.data());
    raw->inferOutputs = { outTensor };
    PreparedModelImpl model(std::move(stub), unisoc::NetworkId {}, nullptr);
    OHOS::HDI::Nnrt::V2_0::IOTensor in {};
    uint8_t inputData[16] = {};
    auto inBuf = MakeAshmemBuffer(16, inputData, sizeof(inputData));
    auto outBuf = MakeAshmemBuffer(16, nullptr, 16);
    ASSERT_NE(inBuf.buffer.fd, -1);
    ASSERT_NE(outBuf.buffer.fd, -1);
    in.data = inBuf.buffer;
    in.dimensions = {1};
    in.dataType = OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_FLOAT32;
    in.format = OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NONE;
    OHOS::HDI::Nnrt::V2_0::IOTensor out {};
    out.data = outBuf.buffer;
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> inputs { in };
    std::vector<OHOS::HDI::Nnrt::V2_0::IOTensor> outputs { out };
    std::vector<std::vector<int32_t>> outputDims;
    EXPECT_EQ(model.Run(inputs, outputs, outputDims),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
    ASSERT_EQ(outputDims.size(), 1U);
}

TEST(NnrtDeviceVdiImplCoverageTest, ToNnrtResultSuccess)
{
    EXPECT_EQ(OHOS::HDI::Nnrt::V2_0::CallDeviceToNnrtResult(unisoc::Status::AI_SUCCESS),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
}

TEST(PreparedModelImplCoverageTest, ToNnrtResultSuccess)
{
    EXPECT_EQ(OHOS::HDI::Nnrt::V2_0::CallPreparedToNnrtResult(unisoc::Status::AI_SUCCESS),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_SUCCESS));
}

TEST(PreparedModelImplCoverageTest, ElementSizeInvalidTypeReturnsZero)
{
    EXPECT_EQ(OHOS::HDI::Nnrt::V2_0::CallElementSizeOfUniAI(unisoc::DataType::QSymmS16), 0U);
}

TEST(PreparedModelImplCoverageTest, WriteUniAIOutputToAshmemWithNullAshmem)
{
    OHOS::HDI::Nnrt::V2_0::IOTensor outDesc {};
    outDesc.data.fd = 0;
    outDesc.data.bufferSize = 1;
    outDesc.data.offset = 0;
    outDesc.data.dataSize = 1;

    float value = 0.0f;
    unisoc::TensorShape shape {};
    shape.m_NumDims = 1;
    shape.m_Dims.fill(1U);
    shape.m_Dims[0] = 1U;
    unisoc::UniAITensor outTensor(shape, unisoc::DataLayout::NHWC, unisoc::DataType::Float32, &value);

    EXPECT_EQ(OHOS::HDI::Nnrt::V2_0::CallWriteUniAIOutputToAshmem(outDesc, outTensor, sptr<Ashmem>()),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_NULL_PTR));
}

TEST(PreparedModelImplCoverageTest, AppendUniAITensorWritableMapReadWriteFailed)
{
    uint8_t inputData[4] = {1, 2, 3, 4};
    auto inBuf = MakeAshmemBuffer(4, inputData, sizeof(inputData));
    ASSERT_NE(inBuf.buffer.fd, -1);

    OHOS::HDI::Nnrt::V2_0::IOTensor io {};
    io.data = inBuf.buffer;
    io.dimensions = {1};
    io.dataType = OHOS::HDI::Nnrt::V2_0::DataType::DATA_TYPE_UINT8;
    io.format = OHOS::HDI::Nnrt::V2_0::Format::FORMAT_NHWC;

    std::vector<sptr<Ashmem>> ashmems;
    std::vector<unisoc::UniAITensor> tensors;
    ScopedAtomicStore<bool> mapFailGuard(g_forceMapReadWriteFail, true);
    EXPECT_EQ(OHOS::HDI::Nnrt::V2_0::CallAppendUniAITensorFromIoTensor(io, true, ashmems, tensors),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_MEMORY_ERROR));
}

TEST(NnrtVdiImplTest, PrepareOfflineModelAshmemAllocFailOutOfMemory)
{
    ScopedAtomicStore<bool> createGuard(g_uniAIReturnNull, false);
    ScopedAtomicStore<int> loadGuard(g_uniAILoadStatus, static_cast<int>(unisoc::Status::AI_SUCCESS));
    ScopedAtomicStore<int> compileGuard(g_uniAICompileStatus, static_cast<int>(unisoc::Status::AI_SUCCESS));

    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);

    uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    auto modelBuf = MakeAshmemBuffer(sizeof(data), data, sizeof(data));
    ASSERT_NE(modelBuf.buffer.fd, -1);

    ModelConfig config {};
    sptr<IPreparedModel> prepared;
    std::vector<SharedBuffer> offline { modelBuf.buffer };
    ScopedAtomicStore<int> callIndexGuard(g_nothrowNewCallIndex, 0);
    ScopedAtomicStore<int> failOnCallGuard(g_failNothrowNewOnCall, 1);
    EXPECT_EQ(vdi->PrepareOfflineModel(offline, config, prepared),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_OUT_OF_MEMORY));

    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}

TEST(NnrtVdiImplTest, PrepareOfflineModelPreparedModelAllocFailOutOfMemory)
{
    ScopedAtomicStore<bool> createGuard(g_uniAIReturnNull, false);
    ScopedAtomicStore<int> loadGuard(g_uniAILoadStatus, static_cast<int>(unisoc::Status::AI_SUCCESS));
    ScopedAtomicStore<int> compileGuard(g_uniAICompileStatus, static_cast<int>(unisoc::Status::AI_SUCCESS));

    INnrtDeviceVdi* vdi = OHOS::HDI::Nnrt::V2_0::CreateNnrtDeviceVdi();
    ASSERT_NE(vdi, nullptr);

    uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    auto modelBuf = MakeAshmemBuffer(sizeof(data), data, sizeof(data));
    ASSERT_NE(modelBuf.buffer.fd, -1);

    ModelConfig config {};
    sptr<IPreparedModel> prepared;
    std::vector<SharedBuffer> offline { modelBuf.buffer };
    ScopedAtomicStore<int> callIndexGuard(g_nothrowNewCallIndex, 0);
    ScopedAtomicStore<int> failOnCallGuard(g_failNothrowNewOnCall, 2);
    EXPECT_EQ(vdi->PrepareOfflineModel(offline, config, prepared),
        static_cast<int32_t>(NNRT_ReturnCode::NNRT_OUT_OF_MEMORY));

    OHOS::HDI::Nnrt::V2_0::DestroyNnrtDeviceVdi(vdi);
}
