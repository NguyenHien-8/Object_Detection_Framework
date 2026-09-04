#pragma once

#ifndef ODF_VERSION_MAJOR
#define ODF_VERSION_MAJOR 0
#define ODF_VERSION_MINOR 1
#define ODF_VERSION_PATCH 0
#endif

namespace odf {

inline constexpr int kVersionMajor = ODF_VERSION_MAJOR;
inline constexpr int kVersionMinor = ODF_VERSION_MINOR;
inline constexpr int kVersionPatch = ODF_VERSION_PATCH;

}  // namespace odf

