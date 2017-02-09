// ======================================================================

#include "Layer.h"
#include "Config.h"
#include "State.h"
#include "LayerFactory.h"
#include "Layer.h"
#include <spine/Exception.h>
#include <spine/HTTP.h>
#include <spine/ParameterFactory.h>
#include <ctpp2/CDT.hpp>
#include <boost/foreach.hpp>
#include <stdexcept>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
// ----------------------------------------------------------------------
/*!
 * \brief Initialize from JSON
 */
// ----------------------------------------------------------------------

bool Layer::init(const std::string& theName, const Json::Value& theJson, const Config& theConfig)
{
  try
  {
    if (theName == "layer_type")
      ;
    else if (theName == "attributes")
      attributes.init(theJson, theConfig);
    else
      return false;

    return true;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
