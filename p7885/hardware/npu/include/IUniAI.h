/**
 * @file IUniAI.h
 * @brief UniAI interface declaration file
 * @date 2024-01-03
 * @copyright Copyright (c) 2024 UNISOC Technologies Co.,Ltd. All Rights
 * Reserved
 */

#ifndef NPU_INCLUDE_IUNI_AI_H
#define NPU_INCLUDE_IUNI_AI_H

#include <string>
#include <vector>
#include <memory>
#include <array>

namespace unisoc {

/**
 * @brief Supported Frontend ML FW, Use to load different network
 */
enum class FrontendId {
    TFlite = 0,     ///< TFlite Framework
    Onnx = 1,       ///< Onnx Framework
    TF = 2,
};

/**
 * @brief backend device state
 */
enum class BackendState {
    DEVICE_AVAILABLE = 0,
    DEVICE_UNAVAILABLE = 1,
    DEVICE_BUSY = 2,
    DEVICE_INVALID = 3
};

/**
 * @brief Supported model type for inference
 */
enum class ModelType {
    UniAI_IR = 0,
    UniAI_NativeModel = 1,  ///< Currently only IR & NativeModel model is supported,
};

/**
 * @brief TensorShape
 * \li m_Dims    : Dim data of each Dims
 * \li m_NumDims : Dim num
 */
struct TensorShape {
    std::array<unsigned int, 5U> m_Dims;
    unsigned int m_NumDims;
};

/**
 * @brief The priority of Inference
 */
enum class Priority {
    UNDEFINED = 0, ///< reserved, not use now
    LOW       = 1, ///< Inference with low priority
    MEDIUM    = 2, ///< Inference with medium priority
    HIGH      = 3  ///< Inference with high priority
};

/**
 * @brief Supported backends for inference
 */
enum class UniAIBackends {
    CPU = 0,            ///< CPU backend
    NPU = 1,            ///< NPU backend
    GPU = 2,            ///< GPU backend
    VDSP = 3            ///< VDSP backend
};

/**
 * @brief Model file data type
 */
enum class DataType {
    Float16 = 0,
    Float32 = 1,
    QuantisedAsymm8 = 2,
    QuantisedSymm8 = 3,
    QSymmS16 = 4,
    Signed32 = 5,
    Signed64 = 6,
    Boolean = 7,
};

/**
 * @brief data layout of the picture
 */
enum class DataLayout {
    UNDEFINED = 0, ///< reserved, not use now
    NCHW = 1,      ///< [batch, in_channels, in_height, in_weight]
    NHWC = 2       ///< [batch, in_height, in_weight, in_channels]
};

/**
 * @brief Input / Output info of the network
 */
struct IoInfo {
    TensorShape m_Shape;
    DataLayout m_Layout;
    DataType m_DataType;
    std::vector<float> m_Scales;
    int32_t m_Offset;
};

/**
 * @brief Status
 */
enum class Status {
    AI_FAILURE, ///< some errors occur, can see log details, see log details
    AI_SUCCESS, ///< all operations work successfully
    AI_UNKNOWN_FRONTEND_ID, ///< the given frontend id is invalid
    AI_UNKNOWN_BACKEND_ID,  ///< the given backend id is invalid
    AI_OPTIMIZE_ERROR,      ///< occur optimize error when NetworkCompile
    AI_SMALL_INPUT_SIZE,    ///< the given input buffer is too small
    AI_SMALL_OUTPUT_SIZE,   ///< the given output buffer is too small
    AI_FILE_NOT_FOUND,      ///< the given file path is not found
    AI_INVALID_NETWORK_ID,  ///< the given network id is invalid
    AI_INVALID_ARGUMENT,    ///< the given argument is invalid, see log details
    AI_INVALID_QUANTIZE_DATA_TYPE, ///< the given quantize dtaa type id is invalid
    AI_INVALID_DATA_TYPE,          ///< the given data type id is invalid
    AI_PARSE_ERROR, ///< parse error occur, should check input/output nodename
};

/**
 * @brief UniAI Tensor for inference
 */
class UniAITensor {
public:
    /**
     * @brief Create UniAITensor
     *
     * @param inputShape      Dimensions of tensor
     * @param dataLayout      Arrangement of tensor data @see { DataLayout }
     * @param dataType        Data type of tensor data @see { DataType }
     * @param data            The address of tensor data
     */
    UniAITensor(TensorShape inputShape, DataLayout dataLayout, DataType dataType, void *data = nullptr);

    /**
     * @brief Get the data layout of the current UniAITensor
     *
     * @return UniAITensor data layout
     */
    DataLayout GetLayout() const;

    /**
     * @brief Get the data type of the current UniAITensor
     *
     * @return UniAITensor data type
     */
    DataType GetDataType() const;

    /**
     * @brief Get the shape of the current UniAITensor
     *
     * @return std::vector<int> the shape of UniAITensor
     */
    TensorShape GetShape() const;

    /**
     * @brief Get a pointer to UniAITensor data of the specified type
     *
     * @return a pointer to data of the specified type
     */
    template <typename T>
    T* buffer() const
    {
        T* data = reinterpret_cast<T*>(m_Data);
        return data;
    }

protected:
    /**
     * @brief UniAITensor data layout
     */
    DataLayout m_DataLayout;

    /**
     * @brief UniAITensor data type
     */
    DataType m_DataType;

    /**
     * @brief UniAITensor shape
     */
    TensorShape m_Shape;

    /**
     * @brief UniAITensor data pointer
     */
    void *m_Data;
};

/**
 * @brief IUniAI class (Pre-declaration for IUniAIPtr)
 */
class IUniAI;

/**
 * @brief using IUniAIPtr as std::unique_ptr<IUniAI, void (*)(IUniAI
 * *UniAI)>
 */
using IUniAIPtr = std::unique_ptr<IUniAI, void (*)(IUniAI *UniAI)>;

/**
 * @brief using NetworkId as int to record the current network id, it is relate
 * to the model file
 */
using NetworkId = int;

/**
 * @brief backend device id, "CPU; NPU"
 */
using BackendId = unisoc::UniAIBackends;

/**
 * @brief Compilation Parameter struct of NetworkCompile()
 */
struct CompilationParameter {
    CompilationParameter()
        : networkid(),
          modelType(),
          preferentBackendList({UniAIBackends::CPU}),
          saveCache(false),
          cachePath(nullptr),
          udoLibPath(nullptr)
    {}

    CompilationParameter(const NetworkId& pNetworkid, const ModelType& pModeType,
                         std::vector<BackendId> pBackendList, bool pSaveCache = false,
                         const char* pCachePath = nullptr, const char* pUdoLibPath = nullptr)
        : networkid(pNetworkid),
          modelType(pModeType),
          preferentBackendList(std::move(pBackendList)),
          saveCache(pSaveCache),
          cachePath(pCachePath),
          udoLibPath(pUdoLibPath)
    {}

    NetworkId networkid;
    ModelType modelType;
    std::vector<BackendId> preferentBackendList;
    bool saveCache;
    const char *cachePath;
    const char *udoLibPath;
};

class UniAI;

/**
 * @brief UniAI class, contains all the public interfaces of UniAI
 */
class IUniAI {
public:
    static IUniAIPtr Create(bool isProfiling = false);

    /**
     * @brief Create UniAI init dynamic backend path
     *
     * @param dynamicBackendPath      dynamic backends path
     * @return IUniAIPtr          the pointer to IUniAI @see {
     * IUniAIPtr }
     */
    static IUniAIPtr Create(const char *dynamicBackendPath, bool isProfiling = false);

    /**
     * @brief Destory UniAI env
     *
     * @param UniAI the pointer to IUniAI @see { IUniAI }
     */
    static void Destroy(IUniAI *UniAI);

    /**
     * @brief Destory network
     *
     * @param networkid : network id, it is relate to the model file @see {
     * NetworkId }
     */
    void DestroyNetwork(NetworkId networkid);

    /**
     * @brief Get backend id list supported by UniAI
     *
     * @return std::vector<BackendId> the backend id list supported by UniAI
     */
    std::vector<BackendId> GetSupportBackendId();

    /**
     * @brief Get the backend state selected by the user
     *
     * @param id            : backend id supported by UniAI @see { BackendId }
     * @return BackendState : backend state @see { BackendState }
     */
    BackendState GetBackendState(const BackendId id);

    /**
     * @brief Get Data Type supported by UniAI
     *
     * @return data type list supported by UniAI
     */
    std::vector<DataType> GetSupportedDataType();

    /**
     * @brief LoadNetwork from binary file
     *
     * @param modelPath              The storage path of the model file
     * @param networkid              network id, it is relate to the model file
     * @see { NetworkId }
     * @param modelType              modelType, type of the modle
     * @see { ModelType }
     * @return Status                Success will return 1, Failure will return 0
     * @see { Status }
     */
    Status LoadNetwork(const char* modelPath, NetworkId &networkid, ModelType& modelType);

    /**
     * @brief compile network
     *
     * @param networkid              network id, it is relate to the model file
     * @see { NetworkId }
     * @param modelType              modelType, type of the modle
     * @see { ModelType }
     * @param preferentBackendList   Compile network with the backend list
     * @return Status                Success will return 1, Failure will return 0
     * @see { Status }
     */
    Status NetworkCompile(NetworkId& networkid, ModelType modelType,
                          const std::vector<BackendId> &preferentBackendList,
                          bool saveCache = false, const char* cachePath = nullptr);

    /**
     * @brief get network input names
     *
     * @param networkid              network id
     * @see { NetworkId }
     * @return vector of Network input names
     */
    std::vector<std::string> GetNetworkInputNames(const NetworkId networkid);

    /**
     * @brief get network output names
     *
     * @param networkid              network id
     * @see { NetworkId }
     * @return vector of Network output names
     */
    std::vector<std::string> GetNetworkOutputNames(const NetworkId networkid);

    /**
     * @brief Run Model file
     *
     * @param networkid  network id, it is relate to the model file @see {
     * NetworkId }
     * @param inputData  Input data @see { UniAITensor }
     * @param outputData The result of Inference @see { UniAITensor }
     * @param priority   The priority of Inference @see { priority }
     * @return           Status Success will return 1, Failure will return 0 @see
     * { Status }
     */
    Status Inference(const NetworkId networkid, std::vector<UniAITensor> &inputTensors,
                     std::vector<UniAITensor> &outputTensors);

    /**
     * @brief Get AISDK VERSION
     *
     * @return  AISDK VERSION string
     */
    std::string GetUniAISdkVersion();

    /**
     * @brief record avaliable network ids
     */
    std::vector<NetworkId> m_NetworkIds;

    /**
     * @brief Get Inputs info
     *
     * @return std::vector<IoInfo> the inputs info list
     */
    std::vector<IoInfo> GetInputAttributeInfo(NetworkId id);

    /**
     * @brief Get Outputs info
     *
     * @return std::vector<IoInfo> the outputs info list
     */
    std::vector<IoInfo> GetOutputAttributeInfo(NetworkId id);

    /**
     * @brief compile network
     *
     * @param CompilationParameter  Compilation Parameter struct to avoid too much input Parameter.
     */
    Status NetworkCompile(CompilationParameter& compilationParameter);

    /**
     * @brief LoadNetwork from memory buffer
     *
     * @param modelBuffer            The memory buffer of the model file
     * @param bufferSize             The size of model memory buffer
     * @param networkid              network id, it is relate to the model file
     * @see { NetworkId }
     * @param modelType              modelType, type of the modle
     * @see { ModelType }
     * @return Status                Success will return 1, Failure will return 0
     * @see { Status }
     */
    Status LoadNetwork(const void* modelBuffer, const size_t bufferSize, NetworkId &networkid, ModelType& modelType);

protected:
    ~IUniAI();

    IUniAI(bool isProfiling);

    IUniAI(const char *dynamicBackendPath, bool isProfiling);

    std::unique_ptr<UniAI> pUniAIImpl;
};

} // namespace unisoc

#endif // NPU_INCLUDE_IUNI_AI_H
