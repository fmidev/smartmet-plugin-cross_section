// ======================================================================

#include "Product.h"
#include "Config.h"
#include "State.h"
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <ctpp2/CDT.hpp>
#include <macgyver/TimeParser.h>
#include <spine/Exception.h>
#include <spine/HTTP.h>

namespace
{
// ----------------------------------------------------------------------
/*!
 * \brief Distance between two points along earth surface
 *
 * \param theLon1 Longitude of point 1
 * \param theLat1 Latitude of point 1
 * \param theLon2 Longitude of point 2
 * \param theLat2 Latitude of point 2
 * \return The distance in kilometers
 *
 *  Haversine Formula (from R.W. Sinnott, "Virtues of the Haversine",
 *  Sky and Telescope, vol. 68, no. 2, 1984, p. 159)
 *  will give mathematically and computationally exact results. The
 *  intermediate result c is the great circle distance in radians. The
 *  great circle distance d will be in the same units as R.
 *
 *  When the two points are antipodal (on opposite sides of the Earth),
 *  the Haversine Formula is ill-conditioned, but the error, perhaps
 *  as large as 2 km (1 mi), is in the context of a distance near
 *  20,000 km (12,000 mi). Further, there is a possibility that roundoff
 *  errors might cause the value of sqrt(a) to exceed 1.0, which would
 *  cause the inverse sine to crash without the bulletproofing provided by
 *  the min() function.
 *
 * The code was taken from NFmiLocation::Distance
 */
// ----------------------------------------------------------------------

double torad(double theValue)
{
  return theValue * 3.14159265358979323846 / 180.0;
}
double geodistance(double theLon1, double theLat1, double theLon2, double theLat2)
{
  using namespace std;

  double lo1 = torad(theLon1);
  double la1 = torad(theLat1);

  double lo2 = torad(theLon2);
  double la2 = torad(theLat2);

  double dlon = lo2 - lo1;
  double dlat = la2 - la1;
  double sindlat = sin(dlat / 2);
  double sindlon = sin(dlon / 2);

  double a = sindlat * sindlat + cos(la1) * cos(la2) * sindlon * sindlon;
  double help1 = sqrt(a);
  double c = 2. * asin(std::min(1., help1));

  return 6371.220 * c;
}
}  // namespace

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
// ----------------------------------------------------------------------
/*!
 * \brief Initialize the product from JSON
 */
// ----------------------------------------------------------------------

void Product::init(const Json::Value& theJson, const Config& theConfig)
{
  try
  {
    if (!theJson.isObject())
      throw SmartMet::Spine::Exception(BCP, "Product JSON is not a JSON object (name-value pairs)");

    // Iterate through all the members

    const auto members = theJson.getMemberNames();
    BOOST_FOREACH (const auto& name, members)
    {
      const Json::Value& json = theJson[name];

      if (name == "layers")
        layers.init(json, theConfig);
      else
        throw SmartMet::Spine::Exception(BCP,
                                         "Product does not have a setting named '" + name + "'");
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Generate the product into the template hash tables
 *
 */
// ----------------------------------------------------------------------

void Product::generate(CTPP::CDT& theGlobals,
                       State& theState,
                       const SmartMet::Spine::TimeSeriesGenerator::LocalTimeList& theTimes)
{
  try
  {
    // Initialize the structure

    theGlobals["layers"] = CTPP::CDT(CTPP::CDT::HASH_VAL);

    // Process all times

    BOOST_FOREACH (const auto& time, theTimes)
    {
      theState.time(time);
      layers.generate(theGlobals, theState);
    }

    // Generate bounding box

    const auto& env = theState.envelope();
    if (env.IsInit())
    {
      theGlobals["bbox"] = CTPP::CDT(CTPP::CDT::HASH_VAL);
      theGlobals["bbox"]["xmin"] = env.MinX;
      theGlobals["bbox"]["xmax"] = env.MaxX;
      theGlobals["bbox"]["ymin"] = env.MinY;
      theGlobals["bbox"]["ymax"] = env.MaxY;
    }

    // Distance between the two points

    theGlobals["distance"] = geodistance(theState.query().longitude1,
                                         theState.query().latitude1,
                                         theState.query().longitude2,
                                         theState.query().latitude2);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
