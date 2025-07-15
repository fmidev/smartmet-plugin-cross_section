#include "IsobandLayer.h"
#include "Config.h"
#include "Isoband.h"
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
    bool raw = false;
    auto pos = pName.find(".raw");
    if (pos != std::string::npos)
    {
      raw = true;
      pName.erase(pos, 4);
    }

    std::string key = theState.query().producer + ";" + pName;

    Engine::Grid::ParameterDetails_vec parameterDetails;
    gridEngine.getParameterDetails(theState.query().producer, pName, parameterDetails);
    // gridEngine.mapParameterDetails(parameterDetails);

    if (parameterDetails.size() != 1 || parameterDetails[0].mProducerName != key)
    {
      for (auto& rec : parameterDetails)
      {
        QueryServer::ParameterMapping_vec mappings;
        std::string pn = rec.mProducerName;
        // if (pn == key)
        pn = rec.mOriginalProducer;

        if (rec.mLevelId > " " || rec.mLevel > " ")
        {
          if (rec.mGeometryId > " ")
            gridEngine.getParameterMappings(rec.mProducerName,
                                            rec.mOriginalParameter,
                                            std::stoi(rec.mGeometryId),
                                            std::stoi(rec.mLevelId),
                                            std::stoi(rec.mLevel),
                                            false,
                                            mappings);
          else
            gridEngine.getParameterMappings(rec.mProducerName,
                                            rec.mOriginalParameter,
                                            std::stoi(rec.mLevelId),
                                            std::stoi(rec.mLevel),
                                            false,
                                            mappings);

          if (mappings.empty() && rec.mLevel < " ")
          {
            if (rec.mGeometryId > " ")
              gridEngine.getParameterMappings(rec.mProducerName,
                                              rec.mOriginalParameter,
                                              std::stoi(rec.mGeometryId),
                                              std::stoi(rec.mLevelId),
                                              -1,
                                              false,
                                              mappings);
            else
              gridEngine.getParameterMappings(rec.mProducerName,
                                              rec.mOriginalParameter,
                                              std::stoi(rec.mLevelId),
                                              -1,
                                              false,
                                              mappings);
            // getParameterMappings(producerInfo.mName, parameterKey, producerGeometryId,
            // T::ParamLevelIdTypeValue::ANY, paramLevelId, -1, false, mappings);
          }
        }
        else
        {
          if (rec.mGeometryId > " ")
            gridEngine.getParameterMappings(rec.mProducerName,
                                            rec.mOriginalParameter,
                                            std::stoi(rec.mGeometryId),
                                            true,
                                            mappings);
          else
            gridEngine.getParameterMappings(
                rec.mProducerName, rec.mOriginalParameter, true, mappings);

          // getParameterMappings(producerInfo.mName, parameterKey, producerGeometryId, true,
          // mappings);
        }
        if (!mappings.empty())
        {
          Engine::Grid::MappingDetails details;
          details.mMapping = mappings[0];
          rec.mMappings.push_back(details);
        }
        // rec.print(std::cout,0,0);
      }

      if (parameterDetails.empty() || parameterDetails[0].mMappings.empty())
      {
        Fmi::Exception exception(BCP, "Parameter mappings not found");
        exception.addParameter("Parameter", *parameter);
        exception.addParameter("Producer", theState.query().producer);
        throw exception;
      }
    }

    std::string zkey = *theState.query().zproducer + ";" + *zparameter;

    Engine::Grid::ParameterDetails_vec zParameterDetails;
    gridEngine.getParameterDetails(*theState.query().zproducer, *zparameter, zParameterDetails);
    // gridEngine.mapParameterDetails(zParameterDetails);

    if (zParameterDetails.size() != 1 || zParameterDetails[0].mProducerName != zkey)
    {
      for (auto& rec : zParameterDetails)
      {
        std::string pn = rec.mProducerName;
        if (pn == zkey)
          pn = rec.mOriginalProducer;

        QueryServer::ParameterMapping_vec mappings;
        if (rec.mLevelId > " " || rec.mLevel > " ")
        {
          if (rec.mGeometryId > " ")
            gridEngine.getParameterMappings(pn,
                                            rec.mOriginalParameter,
                                            std::stoi(rec.mGeometryId),
                                            std::stoi(rec.mLevelId),
                                            std::stoi(rec.mLevel),
                                            false,
                                            mappings);
          else
            gridEngine.getParameterMappings(pn,
                                            rec.mOriginalParameter,
                                            std::stoi(rec.mLevelId),
                                            std::stoi(rec.mLevel),
                                            false,
                                            mappings);

          if (mappings.empty() && rec.mLevel < " ")
          {
            if (rec.mGeometryId > " ")
              gridEngine.getParameterMappings(pn,
                                              rec.mOriginalParameter,
                                              std::stoi(rec.mGeometryId),
                                              std::stoi(rec.mLevelId),
                                              -1,
                                              false,
                                              mappings);
            else
              gridEngine.getParameterMappings(
                  pn, rec.mOriginalParameter, std::stoi(rec.mLevelId), -1, false, mappings);
            // getParameterMappings(producerInfo.mName, parameterKey, producerGeometryId,
            // T::ParamLevelIdTypeValue::ANY, paramLevelId, -1, false, mappings);
          }
        }
        else
        {
          if (rec.mGeometryId > " ")
            gridEngine.getParameterMappings(
                pn, rec.mOriginalParameter, std::stoi(rec.mGeometryId), true, mappings);
          else
            gridEngine.getParameterMappings(pn, rec.mOriginalParameter, true, mappings);
          // getParameterMappings(producerInfo.mName, parameterKey, producerGeometryId, true,
          // mappings);
        }

        if (!mappings.empty())
        {
          Engine::Grid::MappingDetails details;
          details.mMapping = mappings[0];
          rec.mMappings.push_back(details);
        }

        // rec.print(std::cout,0,0);
      }

      if (zParameterDetails.empty() || zParameterDetails[0].mMappings.empty())
      {
        Fmi::Exception exception(BCP, "Z-Parameter mappings not found");
        exception.addParameter("Z-Parameter", *zparameter);
        exception.addParameter("Producer", *theState.query().zproducer);
        throw exception;
      }
    }

    double lon1 = theState.query().longitude1;
    double lat1 = theState.query().latitude1;
    double lon2 = theState.query().longitude2;
    double lat2 = theState.query().latitude2;
    int steps = theState.query().steps;

    std::string valueProducerName = parameterDetails[0].mOriginalProducer;
    std::string valueParameter = parameterDetails[0].mOriginalParameter;
    int geometryId = -1;
    short areaInterpolationMethod = T::AreaInterpolationMethod::Linear;
    short timeInterpolationMethod = T::TimeInterpolationMethod::Linear;
    std::string heightProducerName = zParameterDetails[0].mOriginalProducer;
    std::string heightParameter = zParameterDetails[0].mOriginalParameter;

    if (!parameterDetails[0].mMappings.empty())
    {
      valueProducerName = parameterDetails[0].mMappings[0].mMapping.mProducerName;
      valueParameter = parameterDetails[0].mMappings[0].mMapping.mParameterName;
      geometryId = parameterDetails[0].mMappings[0].mMapping.mGeometryId;
      if (raw)
        areaInterpolationMethod = T::AreaInterpolationMethod::Linear;
      else
        areaInterpolationMethod =
            parameterDetails[0].mMappings[0].mMapping.mAreaInterpolationMethod;

      timeInterpolationMethod = parameterDetails[0].mMappings[0].mMapping.mTimeInterpolationMethod;
    }

    if (!zParameterDetails[0].mMappings.empty())
    {
      heightProducerName = zParameterDetails[0].mMappings[0].mMapping.mProducerName;
      heightParameter = zParameterDetails[0].mMappings[0].mMapping.mParameterName;
    }

    int forecastType = -1;
    int forecastNumber = -1;
    std::vector<float> contourLowValues;
    std::vector<float> contourHighValues;
    std::vector<T::Coordinate> coordinates;
    std::vector<float> gridData;
    uint gridWidth = 0;
    uint gridHeight = 0;
    size_t smooth_size = 0;
    size_t smooth_degree = 1;
    T::ByteData_vec contours;

    std::string utcTime = Fmi::to_iso_string(theState.time().utc_time());

    gridEngine.getVerticalGrid(lon1,
                               lat1,
                               lon2,
                               lat2,
                               steps,
                               utcTime,
                               valueProducerName,
                               valueParameter,
                               heightProducerName,
                               heightParameter,
                               geometryId,
                               forecastType,
                               forecastNumber,
                               areaInterpolationMethod,
                               timeInterpolationMethod,
                               coordinates,
                               gridData,
                               gridWidth,
                               gridHeight);

    double maxHeight = 0;
    double maxDistance = 0;
    double maxValue = -1000000000;
    double minValue = 1000000000;

    for (auto value : gridData)
    {
      if (value > maxValue)
        maxValue = value;

      if (value != ParamValueMissing && value < minValue)
        minValue = value;
    }

    for (const auto& coord : coordinates)
    {
      if (coord.y() > maxHeight)
        maxHeight = coord.y();

      if (coord.x() > maxDistance)
        maxDistance = coord.x();
    }

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

    getIsobands(gridData,
                &coordinates,
                gridWidth,
                gridHeight,
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

    if (interpolation == "linear")
      options.interpolation = Trax::InterpolationType::Linear;
    else if (interpolation == "nearest" || interpolation == "discrete" ||
             interpolation == "midpoint")
      options.interpolation = Trax::InterpolationType::Midpoint;
    else
      throw Fmi::Exception(BCP, "Unknown isoband interpolation method '" + interpolation + "'");

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
