#include "IsolineLayer.h"
#include "Config.h"
#include "Isoline.h"
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

void IsolineLayer::generate_gridEngine(CTPP::CDT& theGlobals, State& theState)
{
  try
  {
    const auto& gridEngine = theState.getGridEngine();
    if (!gridEngine.isEnabled())
      throw Fmi::Exception(BCP, "The grid-engine is disabled!");

    std::string report = "IsolineLayer::generate finished in %t sec CPU, %w sec real\n";
    boost::movelib::unique_ptr<boost::timer::auto_cpu_timer> timer;
    if (theState.query().timer)
      timer = boost::movelib::make_unique<boost::timer::auto_cpu_timer>(2, report);

    if (parameter == boost::none)
      throw Fmi::Exception(BCP, "Parameter not set for isoband-layer");

    if (zparameter == boost::none)
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

    if (!(parameterDetails.size() == 1 && parameterDetails[0].mProducerName == key))
    {
      for (auto rec = parameterDetails.begin(); rec != parameterDetails.end(); ++rec)
      {
        QueryServer::ParameterMapping_vec mappings;
        std::string pn = rec->mProducerName;
        // if (pn == key)
        pn = rec->mOriginalProducer;

        if (rec->mLevelId > " " || rec->mLevel > " ")
        {
          if (rec->mGeometryId > " ")
            gridEngine.getParameterMappings(rec->mProducerName,
                                            rec->mOriginalParameter,
                                            atoi(rec->mGeometryId.c_str()),
                                            atoi(rec->mLevelId.c_str()),
                                            atoi(rec->mLevel.c_str()),
                                            false,
                                            mappings);
          else
            gridEngine.getParameterMappings(rec->mProducerName,
                                            rec->mOriginalParameter,
                                            atoi(rec->mLevelId.c_str()),
                                            atoi(rec->mLevel.c_str()),
                                            false,
                                            mappings);

          if (mappings.size() == 0 && rec->mLevel < " ")
          {
            if (rec->mGeometryId > " ")
              gridEngine.getParameterMappings(rec->mProducerName,
                                              rec->mOriginalParameter,
                                              atoi(rec->mGeometryId.c_str()),
                                              atoi(rec->mLevelId.c_str()),
                                              -1,
                                              false,
                                              mappings);
            else
              gridEngine.getParameterMappings(rec->mProducerName,
                                              rec->mOriginalParameter,
                                              atoi(rec->mLevelId.c_str()),
                                              -1,
                                              false,
                                              mappings);
            // getParameterMappings(producerInfo.mName, parameterKey, producerGeometryId,
            // T::ParamLevelIdTypeValue::ANY, paramLevelId, -1, false, mappings);
          }
        }
        else
        {
          if (rec->mGeometryId > " ")
            gridEngine.getParameterMappings(rec->mProducerName,
                                            rec->mOriginalParameter,
                                            atoi(rec->mGeometryId.c_str()),
                                            true,
                                            mappings);
          else
            gridEngine.getParameterMappings(
                rec->mProducerName, rec->mOriginalParameter, true, mappings);

          // getParameterMappings(producerInfo.mName, parameterKey, producerGeometryId, true,
          // mappings);
        }
        if (mappings.size() > 0)
        {
          Engine::Grid::MappingDetails details;
          details.mMapping = mappings[0];
          rec->mMappings.push_back(details);
        }
        // rec->print(std::cout,0,0);
      }

      if (parameterDetails.size() == 0 || parameterDetails[0].mMappings.size() == 0)
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

    if (!(zParameterDetails.size() == 1 && zParameterDetails[0].mProducerName == zkey))
    {
      for (auto rec = zParameterDetails.begin(); rec != zParameterDetails.end(); ++rec)
      {
        std::string pn = rec->mProducerName;
        if (pn == zkey)
          pn = rec->mOriginalProducer;

        QueryServer::ParameterMapping_vec mappings;
        if (rec->mLevelId > " " || rec->mLevel > " ")
        {
          if (rec->mGeometryId > " ")
            gridEngine.getParameterMappings(pn,
                                            rec->mOriginalParameter,
                                            atoi(rec->mGeometryId.c_str()),
                                            atoi(rec->mLevelId.c_str()),
                                            atoi(rec->mLevel.c_str()),
                                            false,
                                            mappings);
          else
            gridEngine.getParameterMappings(pn,
                                            rec->mOriginalParameter,
                                            atoi(rec->mLevelId.c_str()),
                                            atoi(rec->mLevel.c_str()),
                                            false,
                                            mappings);

          if (mappings.size() == 0 && rec->mLevel < " ")
          {
            if (rec->mGeometryId > " ")
              gridEngine.getParameterMappings(pn,
                                              rec->mOriginalParameter,
                                              atoi(rec->mGeometryId.c_str()),
                                              atoi(rec->mLevelId.c_str()),
                                              -1,
                                              false,
                                              mappings);
            else
              gridEngine.getParameterMappings(
                  pn, rec->mOriginalParameter, atoi(rec->mLevelId.c_str()), -1, false, mappings);
            // getParameterMappings(producerInfo.mName, parameterKey, producerGeometryId,
            // T::ParamLevelIdTypeValue::ANY, paramLevelId, -1, false, mappings);
          }
        }
        else
        {
          if (rec->mGeometryId > " ")
            gridEngine.getParameterMappings(
                pn, rec->mOriginalParameter, atoi(rec->mGeometryId.c_str()), true, mappings);
          else
            gridEngine.getParameterMappings(pn, rec->mOriginalParameter, true, mappings);
          // getParameterMappings(producerInfo.mName, parameterKey, producerGeometryId, true,
          // mappings);
        }

        if (mappings.size() > 0)
        {
          Engine::Grid::MappingDetails details;
          details.mMapping = mappings[0];
          rec->mMappings.push_back(details);
        }

        // rec->print(std::cout,0,0);
      }

      if (zParameterDetails.size() == 0 || zParameterDetails[0].mMappings.size() == 0)
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

    if (parameterDetails[0].mMappings.size() > 0)
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

    if (zParameterDetails[0].mMappings.size() > 0)
    {
      heightProducerName = zParameterDetails[0].mMappings[0].mMapping.mProducerName;
      heightParameter = zParameterDetails[0].mMappings[0].mMapping.mParameterName;
    }

    int forecastType = -1;
    int forecastNumber = -1;
    std::vector<float> contourValues;
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

    for (auto it = gridData.begin(); it != gridData.end(); ++it)
    {
      if (*it > maxValue)
        maxValue = *it;

      if (*it != ParamValueMissing && *it < minValue)
        minValue = *it;
    }

    for (auto it = coordinates.begin(); it != coordinates.end(); ++it)
    {
      if (it->y() > maxHeight)
        maxHeight = it->y();

      if (it->x() > maxDistance)
        maxDistance = it->x();
    }

    for (const auto& isoline : isolines)
    {
      if (isoline.value)
        contourValues.push_back(isoline.value);
    }

    getIsolines(gridData,
                &coordinates,
                gridWidth,
                gridHeight,
                contourValues,
                T::AreaInterpolationMethod::Linear,
                smooth_size,
                smooth_degree,
                contours);

    std::vector<OGRGeometryPtr> geoms;

    if (contours.size() > 0)
    {
      uint c = 0;
      for (auto wkb = contours.begin(); wkb != contours.end(); ++wkb)
      {
        unsigned char* cwkb = reinterpret_cast<unsigned char*>(wkb->data());
        OGRGeometry* geom = nullptr;
        OGRGeometryFactory::createFromWkb(cwkb, nullptr, &geom, wkb->size());
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
    std::string report = "IsolineLayer::generate finished in %t sec CPU, %w sec real\n";
    boost::movelib::unique_ptr<boost::timer::auto_cpu_timer> timer;
    if (theState.query().timer)
      timer = boost::movelib::make_unique<boost::timer::auto_cpu_timer>(2, report);

    // Establish the data
    auto q = theState.producer();

    // Establish the desired direction parameter

    if (parameter == boost::none)
      throw Fmi::Exception(BCP, "Parameter not set for isoband-layer");

    auto param = SmartMet::TimeSeries::ParameterFactory::instance().parse(*parameter);

    // Establish z-parameter

    auto zparam = param;
    if (zparameter)
      zparam = SmartMet::TimeSeries::ParameterFactory::instance().parse(*zparameter);

    // Generate isolines and store them into the template engine

    auto timeformatter = Fmi::TimeFormatter::create("iso");
    std::string timekey = timeformatter->format(theState.time());

    const auto& contourer = theState.getContourEngine();
    std::vector<double> isoline_options;
    for (const auto& isoline : isolines)
      isoline_options.push_back(isoline.value);

    SmartMet::Engine::Contour::Options options(param, theState.time().utc_time(), isoline_options);

    if (multiplier || offset)
      options.transformation(multiplier ? *multiplier : 1.0, offset ? *offset : 0.0);

    if (interpolation == "linear")
      options.interpolation = Trax::InterpolationType::Linear;
    else if (interpolation == "nearest")
      options.interpolation = Trax::InterpolationType::Midpoint;
    else if (interpolation == "discrete")
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
      OGRGeometryPtr geom = geoms[i];

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
