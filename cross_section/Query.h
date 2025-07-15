// ======================================================================
/*!
 * \brief Query parameters
 */
// ======================================================================

#pragma once

#include <optional>
#include <string>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
struct Query
{
  std::string producer;                  // the producer
  std::string customer;                  // the customer
  std::optional<std::string> zproducer;  // the z-producer
  std::optional<std::string> source;

  double longitude1 = 0;  // cross section start point
  double latitude1 = 0;
  double longitude2 = 0;  // cross section end point
  double latitude2 = 0;
  std::size_t steps = 0;  // how many steps to take on the iso circle

  std::string timezone;  // timezone for the timestamps

  bool timer = false;  // print debugging information on timings
};

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
