// ======================================================================
/*!
 * \brief Factory for layers
 *
 * Create a layer based on the type specified in the JSON
 */
// ======================================================================

// TODO: Add caching

#pragma once

#include "Layer.h"
#include <json/json.h>
#include <string>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
namespace LayerFactory
{
Layer* create(const Json::Value& theJson);
}  // namespace LayerFactory
}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
