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
#ifndef VIDEO_API_H_
#define VIDEO_API_H_

#include <cstdint>
#include <cstring>

namespace OHOS {
namespace OMX {
/**
 * Legacy image layout descriptor.
 *
 * This type is kept for backward compatibility.
 * New code should use MediaImage2.
 */
struct MediaImage {
    enum Type {
        kTypeUnknown = 0,
        kTypeYuv,
    };
    enum PlaneIndex {
        kPlaneY = 0,
        kPlaneU,
        kPlaneV,
        kMaxNumPlanes
    };
    Type imageType;
    uint32_t planeCount;              // Count of initialized planes.
    uint32_t frameWidth;              // Unpadded width of the reference plane.
    uint32_t frameHeight;             // Unpadded height of the reference plane.
    uint32_t validBitDepth;           // Effective sample bit depth.
    struct PlaneInfo {
        uint32_t byteOffset;          // Plane origin offset from buffer base.
        uint32_t columnStrideBytes;   // Byte delta for one pixel step on X.
        uint32_t rowStrideBytes;      // Byte delta for one row step on Y.
        uint32_t horizontalSubsample; // Horizontal subsampling vs reference plane.
        uint32_t verticalSubsample;   // Vertical subsampling vs reference plane.
    };
    PlaneInfo planes[kMaxNumPlanes];
};
/**
 * Extended image descriptor with explicit storage-depth metadata.
 */
struct __attribute__ ((__packed__)) MediaImage2 {
    enum Type : uint32_t {
        kTypeUnknown = 0,
        kTypeYuv,
        kTypeYuva,
        kTypeRgb,
        kTypeRgba,
        kTypeY,
    };
    enum PlaneIndex : uint32_t {
        kPlaneY = 0,
        kPlaneU = 1,
        kPlaneV = 2,
        kPlaneR = 0,
        kPlaneG = 1,
        kPlaneB = 2,
        kPlaneA = 3,
        kMaxNumPlanes = 4,
    };
    Type imageType;
    uint32_t planeCount;              // Count of initialized planes.
    uint32_t frameWidth;              // Unpadded width of the reference plane.
    uint32_t frameHeight;             // Unpadded height of the reference plane.
    uint32_t validBitDepth;           // Effective payload bit depth (MSB aligned).
    uint32_t storageBitDepth;         // Stored bits per component, typically 8 or 16.
    struct __attribute__ ((__packed__)) PlaneInfo {
        uint32_t byteOffset;          // Plane origin offset from buffer base.
        int32_t columnStrideBytes;    // Byte delta for one pixel step on X.
        int32_t rowStrideBytes;       // Byte delta for one row step on Y.
        uint32_t horizontalSubsample; // Horizontal subsampling vs reference plane.
        uint32_t verticalSubsample;   // Vertical subsampling vs reference plane.
    };
    PlaneInfo planes[kMaxNumPlanes];
    void initFromV1(const MediaImage&);  // Internal bridge from legacy layout.
};
static_assert(sizeof(MediaImage2::PlaneInfo) == 20, "wrong struct size");
static_assert(sizeof(MediaImage2) == 104, "wrong struct size");
/**
 * Color signaling payload exchanged between OMX-facing and codec-facing code.
 *
 * ABI rules:
 * 1) Future fields must be appended at the tail.
 * 2) Callers should accept payload sizes greater than or equal to known size.
 * 3) Existing offsets are fixed for compatibility.
 */
struct __attribute__ ((__packed__, aligned(alignof(uint32_t)))) SprdColorAspects {
    // Pixel range category.
    enum Range : uint32_t {
        kRangeUnspecified,
        kRangeFull,
        kRangeLimited,
        kRangeOther = 0xff,
    };

    // Source/display primaries category.
    enum Primaries : uint32_t {
        kPrimariesUnspecified,
        kPrimariesBt709_5,      // ITU-R BT.709-5 profile.
        kPrimariesBt470_6M,     // ITU-R BT.470-6 system M profile.
        kPrimariesBt601_6_625,  // ITU-R BT.601-6 (625-line) profile.
        kPrimariesBt601_6_525,  // ITU-R BT.601-6 (525-line) profile.
        kPrimariesGenericFilm,  // Generic film profile.
        kPrimariesBt2020,       // ITU-R BT.2020 profile.
        kPrimariesOther = 0xff,
    };

    // Transfer-function category.
    enum Transfer : uint32_t {
        kTransferUnspecified,
        kTransferLinear,        // Linear response.
        kTransferSrgb,          // sRGB response.
        kTransferSmpte170M,     // SMPTE 170M family.
        kTransferGamma22,       // Approximate gamma 2.2.
        kTransferGamma28,       // Approximate gamma 2.8.
        kTransferSt2084,        // SMPTE ST 2084 (PQ).
        kTransferHlg,           // ARIB STD-B67 HLG.
        // Extended or legacy values that may still appear.
        kTransferSmpte240M = 0x40, // SMPTE 240M.
        kTransferXvycc,         // IEC 61966-2-4 xvYCC.
        kTransferBt1361,        // ITU-R BT.1361 extended gamut.
        kTransferSt428,         // SMPTE ST 428-1.
        kTransferOther = 0xff,
    };

    // Matrix-coefficient category for color conversion.
    enum MatrixCoeffs : uint32_t {
        kMatrixUnspecified,
        kMatrixBt709_5,         // ITU-R BT.709-5 matrix.
        kMatrixBt470_6M,        // KR=0.30, KB=0.11 matrix.
        kMatrixBt601_6,         // ITU-R BT.601-6 matrix.
        kMatrixSmpte240M,       // SMPTE 240M matrix.
        kMatrixBt2020,          // BT.2020 non-constant luminance matrix.
        kMatrixBt2020Constant,  // BT.2020 constant luminance matrix.
        kMatrixOther = 0xff,
    };

    // Convenience standard grouping for primaries+matrix combinations.
    enum Standard : uint32_t {
        kStandardUnspecified,
        kStandardBt709,                 // Bt709 primaries + Bt709 matrix.
        kStandardBt601_625,             // Bt601(625) primaries + Bt601 matrix.
        kStandardBt601_625Unadjusted,   // Bt601(625) primaries + adjusted KR/KB matrix.
        kStandardBt601_525,             // Bt601(525) primaries + Bt601 matrix.
        kStandardBt601_525Unadjusted,   // Bt601(525) primaries + Smpte240M matrix.
        kStandardBt2020,                // Bt2020 primaries + Bt2020 matrix.
        kStandardBt2020Constant,        // Bt2020 primaries + Bt2020 constant matrix.
        kStandardBt470M,                // Bt470M primaries + Bt470M matrix.
        kStandardFilm,                  // Generic-film primaries + film KR/KB tuning.
        kStandardOther = 0xff,
    };

    // Bidirectional signaling values shared between client and component.
    Range rangeMode;             // Pixel range category.
    Primaries primaryId;         // Primaries category.
    Transfer transferId;         // Transfer-function category.
    MatrixCoeffs matrixCoeffId;  // Matrix-coefficients category.
};
static_assert(sizeof(SprdColorAspects) == 16, "wrong struct size");
/**
 * Static HDR metadata container (CTA-861-3 style).
 */
struct __attribute__ ((__packed__)) HDRStaticInfo {
    // Descriptor selector from CTA metadata block.
    enum ID : uint8_t {
        kType1 = 0, // Static metadata descriptor type 1.
    } descriptorId;
    struct __attribute__ ((__packed__)) Primaries1 {
        // Coordinate units are 0.00002 in normalized CIE space.
        uint16_t chromaX;
        uint16_t chromaY;
    };
    // Payload definition for descriptor type 1.
    struct __attribute__ ((__packed__)) Type1 {
        Primaries1 displayPrimaryRed;
        Primaries1 displayPrimaryGreen;
        Primaries1 displayPrimaryBlue;
        Primaries1 whitePoint;
        uint16_t maxDisplayLuminanceCdM2;
        uint16_t minDisplayLuminanceCdM2x1e4;
        uint16_t maxContentLightLevelCdM2;
        uint16_t maxFrameAverageLightLevelCdM2;
    };
    union {
        Type1 type1Payload;
    };
};
static_assert(sizeof(HDRStaticInfo::Primaries1) == 4, "wrong struct size");
static_assert(sizeof(HDRStaticInfo::Type1) == 24, "wrong struct size");
static_assert(sizeof(HDRStaticInfo) == 25, "wrong struct size");
#ifdef STRINGIFY_ENUMS
namespace {
template <typename EnumT>
struct EnumNameItem {
    EnumT value;
    const char *name;
};

template <typename EnumT, size_t N>
inline const char *LookupSparseEnumName(
    EnumT value, const EnumNameItem<EnumT> (&table)[N], const char *fallback)
{
    for (size_t i = 0; i < N; ++i) {
        if (table[i].value == value) {
            return table[i].name;
        }
    }
    return fallback;
}

template <size_t N>
inline const char *LookupDenseName(uint32_t index, const char *const (&table)[N], const char *fallback)
{
    return (index < N) ? table[index] : fallback;
}

static const char *const kMediaImageTypeNames[] = {"MI_UNKNOWN", "MI_YUV"};
static const char *const kMediaImagePlaneNames[] = {"MI_Y", "MI_U", "MI_V"};
static const char *const kMediaImage2TypeNames[] = {
    "MI2_UNKNOWN", "MI2_YUV", "MI2_YUVA", "MI2_RGB", "MI2_RGBA", "MI2_Y"};
static const EnumNameItem<SprdColorAspects::Range> kRangeNames[] = {
    {SprdColorAspects::kRangeUnspecified, "RANGE_UNSPEC"},
    {SprdColorAspects::kRangeFull, "RANGE_FULL"},
    {SprdColorAspects::kRangeLimited, "RANGE_LIMITED"},
    {SprdColorAspects::kRangeOther, "RANGE_OTHER"},
};
static const EnumNameItem<SprdColorAspects::Primaries> kPrimariesNames[] = {
    {SprdColorAspects::kPrimariesUnspecified, "PRI_UNSPEC"},
    {SprdColorAspects::kPrimariesBt709_5, "PRI_BT709_5"},
    {SprdColorAspects::kPrimariesBt470_6M, "PRI_BT470_6M"},
    {SprdColorAspects::kPrimariesBt601_6_625, "PRI_BT601_6_625"},
    {SprdColorAspects::kPrimariesBt601_6_525, "PRI_BT601_6_525"},
    {SprdColorAspects::kPrimariesGenericFilm, "PRI_GENERIC_FILM"},
    {SprdColorAspects::kPrimariesBt2020, "PRI_BT2020"},
    {SprdColorAspects::kPrimariesOther, "PRI_OTHER"},
};
static const EnumNameItem<SprdColorAspects::Transfer> kTransferNames[] = {
    {SprdColorAspects::kTransferUnspecified, "TR_UNSPEC"},
    {SprdColorAspects::kTransferLinear, "TR_LINEAR"},
    {SprdColorAspects::kTransferSrgb, "TR_SRGB"},
    {SprdColorAspects::kTransferSmpte170M, "TR_SMPTE170M"},
    {SprdColorAspects::kTransferGamma22, "TR_GAMMA22"},
    {SprdColorAspects::kTransferGamma28, "TR_GAMMA28"},
    {SprdColorAspects::kTransferSt2084, "TR_ST2084"},
    {SprdColorAspects::kTransferHlg, "TR_HLG"},
    {SprdColorAspects::kTransferSmpte240M, "TR_SMPTE240M"},
    {SprdColorAspects::kTransferXvycc, "TR_XVYCC"},
    {SprdColorAspects::kTransferBt1361, "TR_BT1361"},
    {SprdColorAspects::kTransferSt428, "TR_ST428"},
    {SprdColorAspects::kTransferOther, "TR_OTHER"},
};
static const EnumNameItem<SprdColorAspects::MatrixCoeffs> kMatrixNames[] = {
    {SprdColorAspects::kMatrixUnspecified, "MTX_UNSPEC"},
    {SprdColorAspects::kMatrixBt709_5, "MTX_BT709_5"},
    {SprdColorAspects::kMatrixBt470_6M, "MTX_BT470_6M"},
    {SprdColorAspects::kMatrixBt601_6, "MTX_BT601_6"},
    {SprdColorAspects::kMatrixSmpte240M, "MTX_SMPTE240M"},
    {SprdColorAspects::kMatrixBt2020, "MTX_BT2020"},
    {SprdColorAspects::kMatrixBt2020Constant, "MTX_BT2020_CONST"},
    {SprdColorAspects::kMatrixOther, "MTX_OTHER"},
};
static const EnumNameItem<SprdColorAspects::Standard> kStandardNames[] = {
    {SprdColorAspects::kStandardUnspecified, "STD_UNSPEC"},
    {SprdColorAspects::kStandardBt709, "STD_BT709"},
    {SprdColorAspects::kStandardBt601_625, "STD_BT601_625"},
    {SprdColorAspects::kStandardBt601_625Unadjusted, "STD_BT601_625_UNADJ"},
    {SprdColorAspects::kStandardBt601_525, "STD_BT601_525"},
    {SprdColorAspects::kStandardBt601_525Unadjusted, "STD_BT601_525_UNADJ"},
    {SprdColorAspects::kStandardBt2020, "STD_BT2020"},
    {SprdColorAspects::kStandardBt2020Constant, "STD_BT2020_CONST"},
    {SprdColorAspects::kStandardBt470M, "STD_BT470M"},
    {SprdColorAspects::kStandardFilm, "STD_FILM"},
    {SprdColorAspects::kStandardOther, "STD_OTHER"},
};

inline const char *DebugLabelForMediaImageType(MediaImage::Type value, const char *fallback)
{
    return LookupDenseName(static_cast<uint32_t>(value), kMediaImageTypeNames, fallback);
}

inline const char *DebugLabelForMediaImagePlane(MediaImage::PlaneIndex value, const char *fallback)
{
    return LookupDenseName(static_cast<uint32_t>(value), kMediaImagePlaneNames, fallback);
}

inline const char *DebugLabelForMediaImage2Type(MediaImage2::Type value, const char *fallback)
{
    return LookupDenseName(static_cast<uint32_t>(value), kMediaImage2TypeNames, fallback);
}

inline const char *DebugLabelForColorRange(SprdColorAspects::Range value, const char *fallback)
{
    return LookupSparseEnumName(value, kRangeNames, fallback);
}

inline const char *DebugLabelForColorPrimaries(SprdColorAspects::Primaries value, const char *fallback)
{
    return LookupSparseEnumName(value, kPrimariesNames, fallback);
}

inline const char *DebugLabelForColorTransfer(SprdColorAspects::Transfer value, const char *fallback)
{
    return LookupSparseEnumName(value, kTransferNames, fallback);
}

inline const char *DebugLabelForColorMatrix(SprdColorAspects::MatrixCoeffs value, const char *fallback)
{
    return LookupSparseEnumName(value, kMatrixNames, fallback);
}

inline const char *DebugLabelForColorStandard(SprdColorAspects::Standard value, const char *fallback)
{
    return LookupSparseEnumName(value, kStandardNames, fallback);
}
}  // anonymous helper namespace

// Compatibility bridge: keep external helper names unchanged while routing
// to project-local debug-label providers.
static const char *asString(MediaImage::Type i, const char *def = "??")
{
    return DebugLabelForMediaImageType(i, def);
}

static const char *asString(MediaImage::PlaneIndex i, const char *def = "??")
{
    return DebugLabelForMediaImagePlane(i, def);
}

static const char *asString(MediaImage2::Type i, const char *def = "??")
{
    return DebugLabelForMediaImage2Type(i, def);
}

static char asChar2(
    MediaImage2::PlaneIndex i, MediaImage2::Type j, char def = '?')
{
    const char *planes = asString(j, nullptr);
    // Use fallback when the type is unknown or index exceeds label width.
    if (j == MediaImage2::kTypeUnknown || planes == nullptr ||
        static_cast<size_t>(i) >= std::strlen(planes)) {
        return def;
    }
    return planes[i];
}

static const char *asString(SprdColorAspects::Range i, const char *def = "??")
{
    return DebugLabelForColorRange(i, def);
}

static const char *asString(SprdColorAspects::Primaries i, const char *def = "??")
{
    return DebugLabelForColorPrimaries(i, def);
}

static const char *asString(SprdColorAspects::Transfer i, const char *def = "??")
{
    return DebugLabelForColorTransfer(i, def);
}

static const char *asString(SprdColorAspects::MatrixCoeffs i, const char *def = "??")
{
    return DebugLabelForColorMatrix(i, def);
}

static const char *asString(SprdColorAspects::Standard i, const char *def = "??")
{
    return DebugLabelForColorStandard(i, def);
}
#endif
}  // namespace OMX (OHOS media layer)
}  // namespace OHOS root scope
#endif  // VIDEO_API_H_
