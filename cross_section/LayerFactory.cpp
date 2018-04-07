// ======================================================================

#include "LayerFactory.h"
#include "IsobandLayer.h"
#include "IsolineLayer.h"
#include <spine/Exception.h>
#include <fstream>
#include <stdexcept>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
namespace LayerFactory
{
// ----------------------------------------------------------------------
/*!
 * \brief Create a new layer
 */
//----------------------------------------------------------------------

Layer* create(const Json::Value& theJson)
{
  try
  {
    if (!theJson.isObject())
      throw SmartMet::Spine::Exception(BCP, "Layer JSON must be an object");

    auto name = theJson.get("layer_type", "isoband").asString();

    if (name == "isoband")
      return new IsobandLayer;
    else if (name == "isoline")
      return new IsolineLayer;
    else
      throw SmartMet::Spine::Exception(BCP, "Unknown layer type '" + name + "'");
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

}  // namespace LayerFactory
}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
