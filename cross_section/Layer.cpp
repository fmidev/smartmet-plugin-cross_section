// ======================================================================

#include "Layer.h"
#include "Config.h"
#include "Layer.h"
#include "LayerFactory.h"
#include "State.h"
#include <boost/foreach.hpp>
#include <ctpp2/CDT.hpp>
#include <spine/Exception.h>
#include <spine/HTTP.h>
#include <spine/ParameterFactory.h>
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
