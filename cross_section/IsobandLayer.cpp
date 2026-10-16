#include "IsobandLayer.h"
#include "Config.h"
#include "Isoband.h"
#include "GridTools.h"
#include "Layer.h"
#include "State.h"
#include <boost/move/unique_ptr.hpp>
#include <boost/timer/timer.hpp>
#include <ctpp2/CDT.hpp>
#include <engines/contour/Engine.h>
#include <gis/Box.h>
#include <gis/OGR.h>
#include <grid-files/common/ImagePaint.h>
#include <macgyver/StringConversion.h>
#include <macgyver/TimeFormatter.h>
#include <macgyver/TimeParser.h>
#include <timeseries/ParameterFactory.h>
#include <trax/InterpolationType.h>
#include <algorithm>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
namespace
{
// Map an isoband interpolation name to the Trax interpolation type
Trax::InterpolationType parse_interpolation(const std::string& interpolation)
{
  if (interpolation == "linear")
    return Trax::InterpolationType::Linear;
  if (interpolation == "nearest" || interpolation == "discrete" || interpolation == "midpoint")
    return Trax::InterpolationType::Midpoint;
  throw Fmi::Exception(BCP, "Unknown isoband interpolation method '" + interpolation + "'");
}

// Build the template hash for one generated isoband
CTPP::CDT make_isoband_hash(CTPP::CDT& theGlobals,
                            State& theState,
                            const Isoband& isoband,
                            const OGRGeometryPtr& geom)
{
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
  return hash;
}

}  // namespace

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
        throw Fmi::Exception(BCP, "Isoband-layer does not have a setting named '" + name + "'");
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
    if (theState.query().source && *theState.query().source == "grid")
      generate_gridEngine(theGlobals, theState);
    else
      generate_qEngine(theGlobals, theState);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

void IsobandLayer::generate_gridEngine(CTPP::CDT& theGlobals, State& theState)
{
  try
  {
    const auto& gridEngine = theState.getGridEngine();
    if (!gridEngine.isEnabled())
      throw Fmi::Exception(BCP, "The grid-engine is disabled!");

    std::unique_ptr<boost::timer::auto_cpu_timer> timer;
    if (theState.query().timer)
    {
      std::string report = "IsobandLayer::generate finished in %t sec CPU, %w sec real\n";
      timer = std::make_unique<boost::timer::auto_cpu_timer>(2, report);
    }

    if (parameter == std::nullopt)
      throw Fmi::Exception(BCP, "Parameter not set for isoband-layer");

    if (zparameter == std::nullopt)
      throw Fmi::Exception(BCP, "Z-Parameter not set for isoband-layer");

    std::string pName = *parameter;
    bool raw = remove_raw_suffix(pName);

    auto parameterDetails = get_value_parameter_details(
        gridEngine, theState.query().producer, pName, *parameter);

    auto zParameterDetails =
        get_height_parameter_details(gridEngine, *theState.query().zproducer, *zparameter);

    auto grid = get_vertical_grid(gridEngine, theState, parameterDetails, zParameterDetails, raw);
    const std::string& utcTime = grid.utcTime;

    std::vector<float> contourLowValues;
    std::vector<float> contourHighValues;
    size_t smooth_size = 0;
    size_t smooth_degree = 1;
    T::ByteData_vec contours;

    for (const auto& isoband : isobands)
    {
      if (isoband.lolimit)
        contourLowValues.push_back(*isoband.lolimit);
      else
        contourLowValues.push_back(-1000000000);

      if (isoband.hilimit)
        contourHighValues.push_back(*isoband.hilimit);
      else
        contourHighValues.push_back(1000000000);
    }

    getIsobands(grid.gridData,
                &grid.coordinates,
                grid.gridWidth,
                grid.gridHeight,
                contourLowValues,
                contourHighValues,
                T::AreaInterpolationMethod::Linear,
                smooth_size,
                smooth_degree,
                contours);

    if (!contours.empty())
    {
      std::vector<OGRGeometryPtr> geoms;
      uint c = 0;
      for (const auto& wkb : contours)
      {
        const auto* cwkb = reinterpret_cast<const unsigned char*>(wkb.data());
        OGRGeometry* geom = nullptr;
        OGRGeometryFactory::createFromWkb(cwkb, nullptr, &geom, wkb.size());
        auto geomPtr = OGRGeometryPtr(geom);
        geoms.push_back(geomPtr);

        theState.updateEnvelope(geomPtr);
        const Isoband& isoband = isobands[c];

        CTPP::CDT hash = make_isoband_hash(theGlobals, theState, isoband, geomPtr);

        theGlobals["layers"][utcTime]["isobands"][*parameter].PushBack(hash);

        c++;
      }
    }
    /*
        int imageWidth = 1200;
        int imageHeight = 1100;
        bool rotate = true;
        double mpy = (double)imageHeight / maxHeight;
        double mpx = (double)imageWidth / maxDistance;
        double ms = 2;
        float m = minValue;

        ImagePaint imagePaint(imageWidth,imageHeight,0xFFFFFFFF,false,rotate);

        int sz = imageWidth * imageHeight;
        unsigned long *image = new unsigned long[sz];
        for (int t=0; t<sz; t++)
          image[t] = 0xFF0000;

        // ### Painting contours into the image:

        if (contours.size() > 0)
        {
          uint c = 250;
          uint step = 250 / contours.size();

          uint t = 0;
          for (auto it = contours.begin(); it != contours.end(); ++it)
          {
            uint col = (c << 16) + (c << 8) + c;

            imagePaint.paintWkb(mpx,mpy,0,0,*it,col);
            c = c - step;
            t++;
          }
        }
        char fname[200];
        sprintf(fname,"/tmp/contour.png");
        imagePaint.savePngImage(fname);
    */
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

void IsobandLayer::generate_qEngine(CTPP::CDT& theGlobals, State& theState)
{
  try
  {
    std::unique_ptr<boost::timer::auto_cpu_timer> timer;
    if (theState.query().timer)
    {
      std::string report = "IsobandLayer::generate finished in %t sec CPU, %w sec real\n";
      timer = std::make_unique<boost::timer::auto_cpu_timer>(2, report);
    }

    // Establish the data
    auto q = theState.producer();

    // Establish the parameter

    if (parameter == std::nullopt)
      throw Fmi::Exception(BCP, "Parameter not set for isoband-layer");
    auto param = SmartMet::TimeSeries::ParameterFactory::instance().parse(*parameter);

    // Establish z-parameter

    auto zparam = param;
    if (zparameter)
      zparam = SmartMet::TimeSeries::ParameterFactory::instance().parse(*zparameter);

    // Generate isobands and store them into the template engine

    std::unique_ptr<Fmi::TimeFormatter> timeformatter(Fmi::TimeFormatter::create("iso"));
    std::string timekey = timeformatter->format(theState.time());

    const auto& contourer = theState.getContourEngine();
    std::vector<SmartMet::Engine::Contour::Range> limits;
    limits.reserve(isobands.size());
    for (const auto& isoband : isobands)
      limits.emplace_back(isoband.lolimit, isoband.hilimit);
    SmartMet::Engine::Contour::Options options(param, theState.time().utc_time(), limits);

    if (multiplier || offset)
      options.transformation(multiplier ? *multiplier : 1.0, offset ? *offset : 0.0);

    options.interpolation = parse_interpolation(interpolation);

    auto qInfo = q->info();

    std::vector<OGRGeometryPtr> geoms;
    if (!zparameter)
      geoms = contourer.crossection(*qInfo,
                                    options,
                                    theState.query().longitude1,
                                    theState.query().latitude1,
                                    theState.query().longitude2,
                                    theState.query().latitude2,
                                    theState.query().steps);
    else
      geoms = contourer.crossection(*qInfo,
                                    zparam,
                                    options,
                                    theState.query().longitude1,
                                    theState.query().latitude1,
                                    theState.query().longitude2,
                                    theState.query().latitude2,
                                    theState.query().steps);
    for (unsigned int i = 0; i < isobands.size(); i++)
    {
      const OGRGeometryPtr& geom = geoms[i];

      theState.updateEnvelope(geom);

      // Add the layer
      CTPP::CDT hash = make_isoband_hash(theGlobals, theState, isobands[i], geom);

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
