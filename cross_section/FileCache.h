// ======================================================================
/*!
 * \brief Generic file content cache
 *
 * This cache stores permanently all requested files, and does not
 * check if files have been modified or deleted. Files may be accessed
 * from multiple threads. We do not return references since they
 * may be invalidated by a new insertion into the cache. We do not
 * return a shared pointer either, since the value is almost always
 * going to be copied into a CDT structure anyway.
 */
// ======================================================================

#pragma once

#include <boost/filesystem/path.hpp>
#include <spine/Thread.h>
#include <map>
#include <string>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
class FileCache
{
 public:
  std::string get(const boost::filesystem::path& thePath) const;

 private:
  struct FileContents
  {
    std::time_t modification_time = 0UL;
    std::string content;

    FileContents() = default;
    FileContents(const std::time_t& theTime, std::string theContent)
        : modification_time(theTime), content(std::move(theContent))
    {
    }
  };

  using Cache = std::map<boost::filesystem::path, FileContents>;
  mutable SmartMet::Spine::MutexType itsMutex;
  mutable Cache itsCache;

};  // class FileCache

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
