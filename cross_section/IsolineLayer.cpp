#include "IsolineLayer.h"
#include "Config.h"
#include "Isoline.h"
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
#include <algorithm>

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

IsolineLayer::IsolineLayer() : interpolation("linear") {}

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
      throw Fmi::Exception(BCP, "Isoline-layer JSON is not a JSON object");

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
      else if (name == "isolines")
      {
        if (!json.isArray())
          throw Fmi::Exception(BCP, "isolines setting must be an array");

        for (const auto& isoline_json : json)
        {
          Isoline isoline;
          isoline.init(isoline_json, theConfig);
          isolines.push_back(isoline);
        }
      }
      else
        throw Fmi::Exception(BCP, "Isoline-layer does not have a setting named '" + name + "'");
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

void IsolineLayer::generate(CTPP::CDT& theGlobals, State& theState)
{
  try
  {
    if (theState.query().source && theState.query().source && *theState.query().source == "grid")
      generate_gridEngine(theGlobals, theState);
    else
      generate_qEngine(theGlobals, theState);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

void IsolineLayer::generate_gridEngine(CTPP::CDT& theGlobals, State& theState)
{
  try
  {
    const auto& gridEngine = theState.getGridEngine();
    if (!gridEngine.isEnabled())
      throw Fmi::Exception(BCP, "The grid-engine is disabled!");

    std::unique_ptr<boost::timer::auto_cpu_timer> timer;
    if (theState.query().timer)
    {
      std::string report = "IsolineLayer::generate finished in %t sec CPU, %w sec real\n";
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

    std::vector<float> contourValues;
    size_t smooth_size = 0;
    size_t smooth_degree = 1;
    T::ByteData_vec contours;

    for (const auto& isoline : isolines)
      contourValues.push_back(isoline.value);

    getIsolines(grid.gridData,
                &grid.coordinates,
                grid.gridWidth,
                grid.gridHeight,
                contourValues,
                T::AreaInterpolationMethod::Linear,
                smooth_size,
                smooth_degree,
                contours);

    if (!contours.empty())
    {
      uint c = 0;
      std::vector<OGRGeometryPtr> geoms;
      for (const auto& wkb : contours)
      {
        const auto* cwkb = reinterpret_cast<const unsigned char*>(wkb.data());
        OGRGeometry* geom = nullptr;
        OGRGeometryFactory::createFromWkb(cwkb, nullptr, &geom, wkb.size());
        auto geomPtr = OGRGeometryPtr(geom);
        geoms.push_back(geomPtr);

        theState.updateEnvelope(geomPtr);
        const Isoline& isoline = isolines[c];

        CTPP::CDT hash(CTPP::CDT::HASH_VAL);
        if (isoline.value)
          hash["value"] = isoline.value;
        if (geom != nullptr && geom->IsEmpty() == 0)
          hash["path"] = Fmi::OGR::exportToSvg(*geom, Fmi::Box::identity(), 1);
        else
          hash["path"] = "";
        theState.addAttributes(theGlobals, hash, isoline.attributes);

        theGlobals["layers"][utcTime]["isolines"][*parameter].PushBack(hash);

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

        ImagePaint imagePaint(imageWidth,imageHeight,0xFFFFFF,false,rotate);

        int sz = imageWidth * imageHeight;
        unsigned long *image = new unsigned long[sz];
        for (int t=0; t<sz; t++)
          image[t] = 0xFF0000;

        // ### Painting contours into the image:

        if (contours.size() > 0)
        {
          for (auto it = contours.begin(); it != contours.end(); ++it)
          {
            imagePaint.paintWkb(mpx,mpy,0,0,*it,0x000000);
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

void IsolineLayer::generate_qEngine(CTPP::CDT& theGlobals, State& theState)
{
  try
  {
    std::unique_ptr<boost::timer::auto_cpu_timer> timer;
    if (theState.query().timer)
    {
      std::string report = "IsolineLayer::generate finished in %t sec CPU, %w sec real\n";
      timer = std::make_unique<boost::timer::auto_cpu_timer>(2, report);
    }

    // Establish the data
    auto q = theState.producer();

    // Establish the desired direction parameter

    if (parameter == std::nullopt)
      throw Fmi::Exception(BCP, "Parameter not set for isoband-layer");

    auto param = SmartMet::TimeSeries::ParameterFactory::instance().parse(*parameter);

    // Establish z-parameter

    auto zparam = param;
    if (zparameter)
      zparam = SmartMet::TimeSeries::ParameterFactory::instance().parse(*zparameter);

    // Generate isolines and store them into the template engine

    std::unique_ptr<Fmi::TimeFormatter> timeformatter(Fmi::TimeFormatter::create("iso"));
    std::string timekey = timeformatter->format(theState.time());

    const auto& contourer = theState.getContourEngine();
    std::vector<double> isoline_options;
    isoline_options.reserve(isolines.size());
    for (const auto& isoline : isolines)
      isoline_options.push_back(isoline.value);

    SmartMet::Engine::Contour::Options options(param, theState.time().utc_time(), isoline_options);

    if (multiplier || offset)
      options.transformation(multiplier ? *multiplier : 1.0, offset ? *offset : 0.0);

    if (interpolation == "linear")
      options.interpolation = Trax::InterpolationType::Linear;
    else if (interpolation == "nearest" || interpolation == "discrete" ||
             interpolation == "midpoint")
      options.interpolation = Trax::InterpolationType::Midpoint;
    else
      throw Fmi::Exception(BCP, "Unknown isoline interpolation method '" + interpolation + "'");

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

    for (unsigned int i = 0; i < isolines.size(); i++)
    {
      const OGRGeometryPtr& geom = geoms[i];

      theState.updateEnvelope(geom);

      // Add the layer
      const Isoline& isoline = isolines[i];

      CTPP::CDT hash(CTPP::CDT::HASH_VAL);
      if (isoline.value != 0)
        hash["value"] = isoline.value;
      if (geom != nullptr && geom->IsEmpty() == 0)
        hash["path"] = Fmi::OGR::exportToSvg(*geom, Fmi::Box::identity(), 1);
      else
        hash["path"] = "";
      theState.addAttributes(theGlobals, hash, isoline.attributes);

      theGlobals["layers"][timekey]["isolines"][*parameter].PushBack(hash);
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
