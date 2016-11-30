//======================================================================

#include "IsolineLayer.h"
#include "Config.h"
#include "Isoline.h"
#include "Layer.h"
#include "State.h"
#include <gis/Box.h>
#include <gis/OGR.h>
#include <ctpp2/CDT.hpp>
#include <spine/ParameterFactory.h>
#include <engines/contour/Engine.h>
#include <boost/foreach.hpp>

// TODO:
#include <boost/timer/timer.hpp>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
// ----------------------------------------------------------------------
/*!
 * \brief Constructor
 */
// ----------------------------------------------------------------------

IsolineLayer::IsolineLayer() : interpolation("linear")
{
}
// ----------------------------------------------------------------------
/*!
 * \brief We need a vtable for the class
 */
// ----------------------------------------------------------------------

IsolineLayer::~IsolineLayer()
{
}
// ----------------------------------------------------------------------
/*!
 * \brief Initialize from JSON
 */
// ----------------------------------------------------------------------

void IsolineLayer::init(const Json::Value& theJson, const Config& theConfig)
{
  try
  {
    if (!theJson.isObject())
      throw SmartMet::Spine::Exception(BCP, "Isoline-layer JSON is not a JSON object");

    // Iterate through all the members

    const auto members = theJson.getMemberNames();
    BOOST_FOREACH (const auto& name, members)
    {
      const Json::Value& json = theJson[name];

      if (Layer::init(name, json, theConfig))
        ;
      else if (name == "parameter")
        parameter = json.asString();
      else if (name == "zparameter")
        zparameter = json.asString();
      else if (name == "interpolation")
        interpolation = json.asString();
      else if (name == "multiplier")
        multiplier = json.asDouble();
      else if (name == "offset")
        offset = json.asDouble();
      else if (name == "isolines")
      {
        if (!json.isArray())
          throw SmartMet::Spine::Exception(BCP, "isolines setting must be an array");

        for (unsigned int i = 0; i < json.size(); i++)
        {
          const Json::Value& isoline_json = json[i];
          Isoline isoline;
          isoline.init(isoline_json, theConfig);
          isolines.push_back(isoline);
        }
      }
      else
        throw SmartMet::Spine::Exception(
            BCP, "Isoline-layer does not have a setting named '" + name + "'");
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Generate the layer details into the template hash
 */
// ----------------------------------------------------------------------

void IsolineLayer::generate(CTPP::CDT& theGlobals, State& theState)
{
  try
  {
    std::string report = "IsolineLayer::generate finished in %t sec CPU, %w sec real\n";
    std::unique_ptr<boost::timer::auto_cpu_timer> timer;
    if (theState.query().timer)
      timer.reset(new boost::timer::auto_cpu_timer(2, report));

    // Establish the data
    auto q = theState.producer();

    // Establish the desired direction parameter

    if (!parameter)
      throw SmartMet::Spine::Exception(BCP, "Parameter not set for isoband-layer");

    auto param = SmartMet::Spine::ParameterFactory::instance().parse(*parameter);

    // Establish z-parameter

    auto zparam = param;
    if (zparameter)
      zparam = SmartMet::Spine::ParameterFactory::instance().parse(*zparameter);

    // Generate isolines and store them into the template engine

    auto timeformatter = Fmi::TimeFormatter::create("iso");
    std::string timekey = timeformatter->format(theState.time());

    const auto& contourer = theState.getContourEngine();
    std::vector<double> isoline_options;
    for (auto isoline : isolines)
      isoline_options.push_back(isoline.value);

    SmartMet::Engine::Contour::Options options(param, theState.time().utc_time(), isoline_options);

    if (multiplier || offset)
      options.transformation(multiplier ? *multiplier : 1.0, offset ? *offset : 0.0);

    if (interpolation == "linear")
      options.interpolation = SmartMet::Engine::Contour::Linear;
    else if (interpolation == "nearest")
      options.interpolation = SmartMet::Engine::Contour::Nearest;
    else if (interpolation == "discrete")
      options.interpolation = SmartMet::Engine::Contour::Discrete;
    else if (interpolation == "loglinear")
      options.interpolation = SmartMet::Engine::Contour::LogLinear;
    else
      throw SmartMet::Spine::Exception(
          BCP, "Unknown isoline interpolation method '" + interpolation + "'");

    boost::shared_ptr<NFmiFastQueryInfo> qInfo = q->info();

    std::vector<OGRGeometryPtr> geoms;
    if (!zparameter)
      geoms = contourer.crossection(qInfo,
                                    options,
                                    theState.query().longitude1,
                                    theState.query().latitude1,
                                    theState.query().longitude2,
                                    theState.query().latitude2,
                                    theState.query().steps);
    else
      geoms = contourer.crossection(qInfo,
                                    zparam,
                                    options,
                                    theState.query().longitude1,
                                    theState.query().latitude1,
                                    theState.query().longitude2,
                                    theState.query().latitude2,
                                    theState.query().steps);

    for (unsigned int i = 0; i < isolines.size(); i++)
    {
      OGRGeometryPtr geom = geoms[i];

      theState.updateEnvelope(geom);

      // Add the layer
      const Isoline& isoline = isolines[i];

      CTPP::CDT hash(CTPP::CDT::HASH_VAL);
      if (isoline.value)
        hash["value"] = isoline.value;
      if (geom && !geom->IsEmpty())
        hash["path"] = Fmi::OGR::exportToSvg(*geom, Fmi::Box::identity(), 1);
      else
        hash["path"] = "";
      theState.addAttributes(theGlobals, hash, isoline.attributes);

      theGlobals["layers"][timekey]["isolines"][*parameter].PushBack(hash);
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
