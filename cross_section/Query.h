// ======================================================================
/*!
 * \brief Query parameters
 */
// ======================================================================

#pragma once

#include <boost/optional.hpp>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
struct Query
{
  std::string producer;  // the producer
  std::string customer;  // the customer
  boost::optional<std::string> zproducer;  // the z-producer
  boost::optional<std::string> source;

  double longitude1;  // cross section start point
  double latitude1;
  double longitude2;  // cross section end point
  double latitude2;
  std::size_t steps;  // how many steps to take on the iso circle

  std::string timezone;  // timezone for the timestamps

  bool timer;  // print debugging information on timings
};

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
