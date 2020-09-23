#include "IsobandLayer.h"
#include "Config.h"
#include "Isoband.h"
#include "Layer.h"
#include "State.h"
#include <boost/move/unique_ptr.hpp>
#include <boost/timer/timer.hpp>
#include <ctpp2/CDT.hpp>
#include <engines/contour/Engine.h>
#include <engines/contour/Interpolation.h>
#include <gis/Box.h>
#include <gis/OGR.h>
#include <macgyver/TimeFormatter.h>
#include <spine/ParameterFactory.h>

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

IsobandLayer::IsobandLayer() : interpolation("linear") {}

// ----------------------------------------------------------------------
/*!
 * \brief Initialize from JSON
 */
// ----------------------------------------------------------------------

void IsobandLayer::init(const Json::Value& theJson, const Config& theConfig)
{
  try
  {
    if (!theJson.isObject())
      throw Fmi::Exception(BCP, "Isoband-layer JSON is not a JSON object");

    // Iterate through all the members

    const auto members = theJson.getMemberNames();
    for (const auto& name : members)
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
      else if (name == "isobands")
      {
        if (!json.isArray())
          throw Fmi::Exception(
              BCP, "isobands setting must be an array got instead:\n" + json.toStyledString());
        for (const auto& isoband_json : json)
        {
          Isoband isoband;
          isoband.init(isoband_json, theConfig);
          isobands.push_back(isoband);
        }
      }
      else
        throw Fmi::Exception(
            BCP, "Isoband-layer does not have a setting named '" + name + "'");
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Generate the layer details into the template hash
 */
// ----------------------------------------------------------------------

void IsobandLayer::generate(CTPP::CDT& theGlobals, State& theState)
{
  try
  {
    std::string report = "IsobandLayer::generate finished in %t sec CPU, %w sec real\n";
    boost::movelib::unique_ptr<boost::timer::auto_cpu_timer> timer;
    if (theState.query().timer)
      timer = boost::movelib::make_unique<boost::timer::auto_cpu_timer>(2, report);

    // Establish the data
    auto q = theState.producer();

    // Establish the parameter

    if (parameter == boost::none)
      throw Fmi::Exception(BCP, "Parameter not set for isoband-layer");
    auto param = SmartMet::Spine::ParameterFactory::instance().parse(*parameter);

    // Establish z-parameter

    auto zparam = param;
    if (zparameter)
      zparam = SmartMet::Spine::ParameterFactory::instance().parse(*zparameter);

    // Generate isobands and store them into the template engine

    auto timeformatter = Fmi::TimeFormatter::create("iso");
    std::string timekey = timeformatter->format(theState.time());

    const auto& contourer = theState.getContourEngine();
    std::vector<SmartMet::Engine::Contour::Range> limits;
    for (const auto& isoband : isobands)
      limits.emplace_back(SmartMet::Engine::Contour::Range(isoband.lolimit, isoband.hilimit));
    SmartMet::Engine::Contour::Options options(param, theState.time().utc_time(), limits);

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
      throw std::runtime_error("Unknown isoband interpolation method '" + interpolation + "'");

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
    for (unsigned int i = 0; i < isobands.size(); i++)
    {
      OGRGeometryPtr geom = geoms[i];

      theState.updateEnvelope(geom);

      // Add the layer
      const Isoband& isoband = isobands[i];

      CTPP::CDT hash(CTPP::CDT::HASH_VAL);
      if (isoband.lolimit)
        hash["lolimit"] = *isoband.lolimit;
      if (isoband.hilimit)
        hash["hilimit"] = *isoband.hilimit;
      if (geom != nullptr && geom->IsEmpty() == 0)
        hash["path"] = Fmi::OGR::exportToSvg(*geom, Fmi::Box::identity(), 1);
      else
        hash["path"] = "";
      theState.addAttributes(theGlobals, hash, isoband.attributes);

      theGlobals["layers"][timekey]["isobands"][*parameter].PushBack(hash);
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
