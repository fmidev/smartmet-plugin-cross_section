#include "Layers.h"
#include "Layer.h"
#include "LayerFactory.h"
#include "State.h"
#include <ctpp2/CDT.hpp>
#include <boost/foreach.hpp>
#include <spine/Exception.h>

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
      throw SmartMet::Spine::Exception(BCP, "Layers JSON is not a JSON array");

    for (unsigned int i = 0; i < theJson.size(); i++)
    {
      const Json::Value& json = theJson[i];
      boost::shared_ptr<Layer> layer(LayerFactory::create(json));
      layer->init(json, theConfig);
      layers.push_back(layer);
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
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
    BOOST_FOREACH (auto& layer, layers)
    {
      layer->generate(theGlobals, theState);
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
