// ======================================================================
/*!
 * \brief A factory for thread safe template formatting
 *
 * TODO: Should this be in macgyver instead of something as trivial
 *       as TemplateFormatterMT ?
 */
// ======================================================================

#pragma once

#include <filesystem>
#include <boost/thread.hpp>
#include <macgyver/TemplateFormatter.h>
#include <memory>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
using SharedFormatter = std::shared_ptr<Fmi::TemplateFormatter>;

class TemplateFactory
{
 public:
  TemplateFactory() = default;
  ~TemplateFactory() = default;

  TemplateFactory(const TemplateFactory& other) = delete;
  TemplateFactory& operator=(const TemplateFactory& other) = delete;
  TemplateFactory(TemplateFactory&& other) = delete;
  TemplateFactory& operator=(TemplateFactory&& other) = delete;

  SharedFormatter get(const std::filesystem::path& theFilename) const;

 private:
  struct TemplateInfo
  {
    std::time_t modtime = 0;
    SharedFormatter formatter;

    TemplateInfo() = default;
  };

  // CT++ may not be thread safe - but using a thread specific
  // storage for cached copies makes using it thread safe
  using TemplateMap = std::map<std::filesystem::path, TemplateInfo>;
  using Templates = boost::thread_specific_ptr<TemplateMap>;
  mutable Templates itsTemplates;

};  // class TemplateFactory

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
