// ======================================================================

#include "LayerFactory.h"
#include "IsobandLayer.h"
#include "IsolineLayer.h"
#include <macgyver/Exception.h>
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
      throw Fmi::Exception(BCP, "Layer JSON must be an object");

    auto name = theJson.get("layer_type", "isoband").asString();

    if (name == "isoband")
      return new IsobandLayer;
    if (name == "isoline")
      return new IsolineLayer;

    throw Fmi::Exception(BCP, "Unknown layer type '" + name + "'");
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace LayerFactory
}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
