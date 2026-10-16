#include "GridTools.h"
#include "State.h"
#include <macgyver/Exception.h>
#include <macgyver/StringConversion.h>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
namespace
{
// Add the first parameter mapping found for the parameter details of the producer
void add_parameter_mapping(const Engine::Grid::Engine& gridEngine,
                           Engine::Grid::ParameterDetails& rec,
                           const std::string& pn)
{
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

// Throw if no mappings were found for the parameter
void require_mappings(const Engine::Grid::ParameterDetails_vec& parameterDetails,
                      const char* message,
                      const char* parameterLabel,
                      const std::string& parameterName,
                      const std::string& producer)
{
  if (parameterDetails.empty() || parameterDetails[0].mMappings.empty())
  {
    Fmi::Exception exception(BCP, message);
    exception.addParameter(parameterLabel, parameterName);
    exception.addParameter("Producer", producer);
    throw exception;
  }
}
}  // namespace

bool remove_raw_suffix(std::string& theParameter)
{
  auto pos = theParameter.find(".raw");
  if (pos == std::string::npos)
    return false;
  theParameter.erase(pos, 4);
  return true;
}

Engine::Grid::ParameterDetails_vec get_value_parameter_details(
    const Engine::Grid::Engine& theGridEngine,
    const std::string& theProducer,
    const std::string& theParameter,
    const std::string& theParameterName)
{
  std::string key = theProducer + ";" + theParameter;

  Engine::Grid::ParameterDetails_vec parameterDetails;
  theGridEngine.getParameterDetails(theProducer, theParameter, parameterDetails);
  // gridEngine.mapParameterDetails(parameterDetails);

  if (parameterDetails.size() != 1 || parameterDetails[0].mProducerName != key)
  {
    for (auto& rec : parameterDetails)
      add_parameter_mapping(theGridEngine, rec, rec.mProducerName);

    require_mappings(
        parameterDetails, "Parameter mappings not found", "Parameter", theParameterName, theProducer);
  }
  return parameterDetails;
}

Engine::Grid::ParameterDetails_vec get_height_parameter_details(
    const Engine::Grid::Engine& theGridEngine,
    const std::string& theProducer,
    const std::string& theParameter)
{
  std::string zkey = theProducer + ";" + theParameter;

  Engine::Grid::ParameterDetails_vec zParameterDetails;
  theGridEngine.getParameterDetails(theProducer, theParameter, zParameterDetails);
  // gridEngine.mapParameterDetails(zParameterDetails);

  if (zParameterDetails.size() != 1 || zParameterDetails[0].mProducerName != zkey)
  {
    for (auto& rec : zParameterDetails)
    {
      std::string pn = rec.mProducerName;
      if (pn == zkey)
        pn = rec.mOriginalProducer;
      add_parameter_mapping(theGridEngine, rec, pn);
    }

    require_mappings(zParameterDetails,
                     "Z-Parameter mappings not found",
                     "Z-Parameter",
                     theParameter,
                     theProducer);
  }
  return zParameterDetails;
}

VerticalGrid get_vertical_grid(const Engine::Grid::Engine& theGridEngine,
                               const State& theState,
                               const Engine::Grid::ParameterDetails_vec& theParameterDetails,
                               const Engine::Grid::ParameterDetails_vec& theZParameterDetails,
                               bool theRawFlag)
{
  double lon1 = theState.query().longitude1;
  double lat1 = theState.query().latitude1;
  double lon2 = theState.query().longitude2;
  double lat2 = theState.query().latitude2;
  int steps = theState.query().steps;

  std::string valueProducerName = theParameterDetails[0].mOriginalProducer;
  std::string valueParameter = theParameterDetails[0].mOriginalParameter;
  int geometryId = -1;
  short areaInterpolationMethod = T::AreaInterpolationMethod::Linear;
  short timeInterpolationMethod = T::TimeInterpolationMethod::Linear;
  std::string heightProducerName = theZParameterDetails[0].mOriginalProducer;
  std::string heightParameter = theZParameterDetails[0].mOriginalParameter;

  if (!theParameterDetails[0].mMappings.empty())
  {
    const auto& mapping = theParameterDetails[0].mMappings[0].mMapping;
    valueProducerName = mapping.mProducerName;
    valueParameter = mapping.mParameterName;
    geometryId = mapping.mGeometryId;
    if (theRawFlag)
      areaInterpolationMethod = T::AreaInterpolationMethod::Linear;
    else
      areaInterpolationMethod = mapping.mAreaInterpolationMethod;

    timeInterpolationMethod = mapping.mTimeInterpolationMethod;
  }

  if (!theZParameterDetails[0].mMappings.empty())
  {
    heightProducerName = theZParameterDetails[0].mMappings[0].mMapping.mProducerName;
    heightParameter = theZParameterDetails[0].mMappings[0].mMapping.mParameterName;
  }

  int forecastType = -1;
  int forecastNumber = -1;

  VerticalGrid grid;
  grid.utcTime = Fmi::to_iso_string(theState.time().utc_time());

  theGridEngine.getVerticalGrid(lon1,
                                lat1,
                                lon2,
                                lat2,
                                steps,
                                grid.utcTime,
                                valueProducerName,
                                valueParameter,
                                heightProducerName,
                                heightParameter,
                                geometryId,
                                forecastType,
                                forecastNumber,
                                areaInterpolationMethod,
                                timeInterpolationMethod,
                                grid.coordinates,
                                grid.gridData,
                                grid.gridWidth,
                                grid.gridHeight);
  return grid;
}

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
