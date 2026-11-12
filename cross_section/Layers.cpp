#include "Layers.h"
#include "Layer.h"
#include "LayerFactory.h"
#include <macgyver/Exception.h>
#include <string>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
// ----------------------------------------------------------------------
/*!
 * \brief Initialize the layers from JSON
 */
// ----------------------------------------------------------------------

void Layers::init(const Json::Value& theJson, const Config& theConfig)
{
  try
  {
    if (!theJson.isArray())
      throw Fmi::Exception(BCP, "Layers JSON is not a JSON array");

    for (const auto& json : theJson)
    {
      std::shared_ptr<Layer> layer(LayerFactory::create(json));
      layer->init(json, theConfig);
      layers.push_back(layer);
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Generate the definitions into the template hash tables
 */
// ----------------------------------------------------------------------

void Layers::generate(CTPP::CDT& theGlobals, State& theState)
{
  try
  {
    for (auto& layer : layers)
    {
      layer->generate(theGlobals, theState);
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
