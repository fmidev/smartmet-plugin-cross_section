// ======================================================================
/*!
 * \brief SmartMet CrossSection plugin implementation
 */
// ======================================================================

#include "Plugin.h"
#include "Json.h"
#include "Product.h"
#include "Query.h"
#include "State.h"
#include <boost/move/unique_ptr.hpp>
#include <boost/timer/timer.hpp>
#include <ctpp2/CDT.hpp>
#include <engines/geonames/Engine.h>
#include <json/json.h>
#include <json/reader.h>
#include <macgyver/AnsiEscapeCodes.h>
#include <macgyver/Exception.h>
#include <spine/Convenience.h>
#include <spine/HostInfo.h>
#include <spine/SmartMet.h>
#include <timeseries/OptionParsers.h>
#include <timeseries/TimeSeriesGeneratorOptions.h>
#include <stdexcept>

namespace
{
// ----------------------------------------------------------------------
/*!
 * \brief Validate name for inclusion
 *
 * A valid name does not lead upwards in a path if inserted into
 * a path. If the name looks valid, we return it so that we do not
 * have to use if statements when this check is done. If the name
 * does not look valid, we throw.
 */
// ----------------------------------------------------------------------

void check_attack(const std::string &theName)
{
  if (theName.find("./") == std::string::npos)
    return;

  throw Fmi::Exception(
      BCP, "Attack IRI detected, relative paths upwards are not safe: '" + theName + "'");
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
 * \brief Perform a CSection query
 */
// ----------------------------------------------------------------------

std::string Plugin::query(SmartMet::Spine::Reactor & /* theReactor */,
                          const SmartMet::Spine::HTTP::Request &theRequest,
                          SmartMet::Spine::HTTP::Response & /* theResponse */)
{
  try
  {
    // Establish debugging related variables

    bool print_hash = SmartMet::Spine::optional_bool(theRequest.getParameter("hash"), false);

    bool print_json = SmartMet::Spine::optional_bool(theRequest.getParameter("json"), false);

    // Generate the query variables

    Query q;
    q.timer = SmartMet::Spine::optional_bool(theRequest.getParameter("timer"), false);

    q.customer = SmartMet::Spine::optional_string(theRequest.getParameter("customer"),
                                                  itsConfig.defaultCustomer());

    q.producer = SmartMet::Spine::required_string(
        theRequest.getParameter("producer"), "Product configuration option 'producer' not given");

    q.zproducer =
        SmartMet::Spine::optional_string(theRequest.getParameter("zproducer"), q.producer);
    q.source = SmartMet::Spine::optional_string(theRequest.getParameter("source"), "querydata");

    q.steps = SmartMet::Spine::required_unsigned_long(
        theRequest.getParameter("steps"),
        "Configuration option 'steps' must be given to determine "
        "how many parts the isocircle is divided into");

    q.timezone = SmartMet::Spine::optional_string(theRequest.getParameter("timezone"),
                                                  itsConfig.defaultTimeZone());

    // We require exactly two locations

    SmartMet::Engine::Geonames::LocationOptions loptions = itsGeoEngine->parseLocations(theRequest);

    if (loptions.size() != 2)
      throw Fmi::Exception(BCP, "Exactly two locations are required for a cross-section");

    const auto &locs = loptions.locations();
    q.longitude1 = locs.front().loc->longitude;
    q.latitude1 = locs.front().loc->latitude;
    q.longitude2 = locs.back().loc->longitude;
    q.latitude2 = locs.back().loc->latitude;

    // State variable

    State state(*this);
    state.query(q);

    // And timeseries options now that state (and querydata) is established

    TimeSeries::TimeSeriesGeneratorOptions toptions = SmartMet::TimeSeries::parseTimes(theRequest);

    if (!q.source || *q.source != "grid")
      toptions.setDataTimes(state.producer()->validTimes(), state.producer()->isClimatology());

    auto tz = itsGeoEngine->getTimeZones().time_zone_from_string(q.timezone);
    auto times = TimeSeries::TimeSeriesGenerator::generate(toptions, tz);

    // Product JSON

    auto product_name = SmartMet::Spine::required_string(
        theRequest.getParameter("product"), "Product configuration option 'product' not given");
    auto product = getProduct(q.customer, product_name, print_json);

    // The template to fill

    auto format_name = SmartMet::Spine::optional_string(theRequest.getParameter("format"),
                                                        itsConfig.defaultTemplate());

    auto tmpl = getTemplate(format_name);

    // Build the response CDT
    CTPP::CDT hash(CTPP::CDT::HASH_VAL);
    {
      std::unique_ptr<boost::timer::auto_cpu_timer> mytimer;
      if (q.timer)
      {
        std::string report = "Product::generate finished in %t sec CPU, %w sec real\n";
        mytimer = std::make_unique<boost::timer::auto_cpu_timer>(2, report);
      }
      product.generate(hash, state, times);
    }

    if (print_hash)
    {
      std::cout << "Generated CDT for " << q.customer << " " << product_name << '\n'
                << hash.RecursiveDump() << '\n';
    }

    std::string output;
    try
    {
      std::string log;
      std::unique_ptr<boost::timer::auto_cpu_timer> mytimer;
      if (q.timer)
      {
        std::string report = "Template processing finished in %t sec CPU, %w sec real\n";
        mytimer.reset(new boost::timer::auto_cpu_timer(2, report));
      }
      tmpl->process(hash, output, log);
    }
    catch (const CTPP::CTPPException & /* ex */)
    {
      throw Fmi::Exception(BCP, "Template processing failed!")
          .addParameter("Product", product_name)
          .addParameter("Format name", format_name);
    }
    catch (...)
    {
      throw Fmi::Exception(BCP, "Template processing failed!")
          .addParameter("Product", product_name)
          .addParameter("Format name", format_name);
    }

    return output;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Get product definition from the plugin cache
 */
// ----------------------------------------------------------------------

Product Plugin::getProduct(const std::string &theCustomer,
                           const std::string &theName,
                           bool theDebugFlag) const
{
  try
  {
    std::string cache_name = theCustomer + "/" + theName;

    // TODO: Should check file modification time too

    // Try the cache first
    {
      SmartMet::Spine::ReadLock lock(itsProductCacheMutex);
      auto tmp = itsProductCache.find(cache_name);
      if (tmp != itsProductCache.end())
        return tmp->second;
    }

    // Establish the path to the JSON file.

    std::string customer_root = (itsConfig.rootDirectory() + "/customers/" + theCustomer);

    std::string product_path = customer_root + "/products/" + theName + ".json";

    // Read the JSON

    Json::Value json;
    Json::Reader reader;
    std::string json_text = itsFileCache.get(product_path);
    bool json_ok = reader.parse(json_text, json);

    if (!json_ok)
      throw Fmi::Exception(
          BCP, "Failed to parse '" + product_path + "': " + reader.getFormattedErrorMessages());

    // Expand the JSON

    std::string layers_root = customer_root + "/layers/";

    JSON::expand(json, itsConfig.rootDirectory(), layers_root, itsFileCache);

    // Expand paths

    JSON::dereference(json);

    // Debugging

    if (theDebugFlag)
      std::cout << "Expanded " << theName << " JSON:\n" << json << '\n';

    // And initialize the product specs from the JSON

    Product product;
    product.init(json, itsConfig);

    // Cache the result and return it
    {
      SmartMet::Spine::WriteLock lock(itsProductCacheMutex);
      itsProductCache.insert(std::make_pair(theName, product));
    }

    return product;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Get template from the plugin cache
 */
// ----------------------------------------------------------------------

SharedFormatter Plugin::getTemplate(const std::string &theName) const
{
  try
  {
    check_attack(theName);
    std::string tmpl_path = (itsConfig.templateDirectory() + "/" + theName + ".c2t");

    return itsTemplateFactory.get(tmpl_path);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Get the configuration object - layers need to know defaults
 */
// ----------------------------------------------------------------------

const Config &Plugin::getConfig() const
{
  return itsConfig;
}
// ----------------------------------------------------------------------
/*!
 * \brief Main content handler
 */
// ----------------------------------------------------------------------

void Plugin::requestHandler(SmartMet::Spine::Reactor &theReactor,
                            const SmartMet::Spine::HTTP::Request &theRequest,
                            SmartMet::Spine::HTTP::Response &theResponse)
{
  try
  {
    theResponse.setHeader("Access-Control-Allow-Origin", "*");

    using Fmi::DateTime;

    const bool isdebug = SmartMet::Spine::optional_bool(theRequest.getParameter("debug"), false);

    // Default expiration time

    const int expires_seconds = 60;

    // Now

    Fmi::DateTime t_now = Fmi::SecondClock::universal_time();

    try
    {
      std::string response = query(theReactor, theRequest, theResponse);

      theResponse.setStatus(SmartMet::Spine::HTTP::Status::ok);

      // Build cache expiration time info

      Fmi::DateTime t_expires = t_now + Fmi::Seconds(expires_seconds);

      // The headers themselves

      std::shared_ptr<Fmi::TimeFormatter> tformat(Fmi::TimeFormatter::create("http"));

      std::string cachecontrol = "public, max-age=" + std::to_string(expires_seconds);
      std::string expiration = tformat->format(t_expires);
      std::string modification = tformat->format(t_now);

      theResponse.setHeader("Cache-Control", cachecontrol);
      theResponse.setHeader("Expires", expiration);
      theResponse.setHeader("Last-Modified", modification);

      if (response.empty())
      {
        std::cerr << "Warning: Empty input for request " << theRequest.getQueryString() << " from "
                  << theRequest.getClientIP() << '\n';
      }
      else
      {
        theResponse.setContent(response);
        theResponse.setHeader("Content-type", "application/json; charset=UTF-8");
#ifdef MYDEBUG
        std::cout << "Output:\n" << response << '\n';
#endif
      }
    }

    catch (...)
    {
      Fmi::Exception exception(BCP, "Request processing exception!", nullptr);
      exception.addParameter("URI", theRequest.getURI());
      exception.addParameter("ClientIP", theRequest.getClientIP());
      exception.addParameter("HostName", Spine::HostInfo::getHostName(theRequest.getClientIP()));
      exception.printError();

      if (isdebug)
      {
        std::string msg = exception.getHtmlStackTrace();
        theResponse.setContent(msg);
        theResponse.setStatus(SmartMet::Spine::HTTP::Status::ok);
      }
      else
      {
        theResponse.setStatus(SmartMet::Spine::HTTP::Status::service_unavailable);
      }
      // Remove newlines, make sure length is reasonable
      std::string msg = exception.what();
      boost::algorithm::replace_all(msg, "\n", " ");
      if (msg.size() > 100)
        msg.resize(100);
      theResponse.setHeader("X-CSection-Error", msg);
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Plugin constructor
 */
// ----------------------------------------------------------------------

Plugin::Plugin(SmartMet::Spine::Reactor *theReactor, const char *theConfig)
    : itsModuleName("CrossSection"), itsConfig(theConfig), itsReactor(theReactor)
{
  try
  {
    if (theReactor->getRequiredAPIVersion() != SMARTMET_API_VERSION)
    {
      std::cerr << ANSI_BOLD_ON << ANSI_FG_RED
                << "*** CrossSection Plugin and Server API version mismatch ***" << ANSI_FG_DEFAULT
                << ANSI_BOLD_OFF << '\n';
      return;
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Initialize
 */
// ----------------------------------------------------------------------

void Plugin::init()
{
  using namespace boost::placeholders;

  try
  {
    /* QEngine */

    itsQEngine = itsReactor->getEngine<SmartMet::Engine::Querydata::Engine>("Querydata", nullptr);

    /* GridEngine */
    itsGridEngine = itsReactor->getEngine<SmartMet::Engine::Grid::Engine>("grid", nullptr);

    /* Contour */
    itsContourEngine = itsReactor->getEngine<SmartMet::Engine::Contour::Engine>("Contour", nullptr);

    /* GeoEngine */
    itsGeoEngine = itsReactor->getEngine<SmartMet::Engine::Geonames::Engine>("Geonames", nullptr);

    /* Register handler */

    if (!itsReactor->addContentHandler(
            this,
            itsConfig.defaultUrl(),
            [this](Spine::Reactor &theReactor,
                   const Spine::HTTP::Request &theRequest,
                   Spine::HTTP::Response &theResponse)
            { callRequestHandler(theReactor, theRequest, theResponse); }))
      throw Fmi::Exception(BCP, "Failed to register CSection content handler");
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Shutdown the plugin
 */
// ----------------------------------------------------------------------

void Plugin::shutdown()
{
  std::cout << "  -- Shutdown requested (csection)\n";
}

// ----------------------------------------------------------------------
/*!
 * \brief Return the plugin name
 */
// ----------------------------------------------------------------------

const std::string &Plugin::getPluginName() const
{
  return itsModuleName;
}
// ----------------------------------------------------------------------
/*!
 * \brief Return the required version
 */
// ----------------------------------------------------------------------

int Plugin::getRequiredAPIVersion() const
{
  return SMARTMET_API_VERSION;
}

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet

/*
 * Server knows us through the 'SmartMetPlugin' virtual interface, which
 * the 'Plugin' class implements.
 */

extern "C" SmartMetPlugin *create(SmartMet::Spine::Reactor *them, const char *config)
{
  return new SmartMet::Plugin::CrossSection::Plugin(them, config);
}

extern "C" void destroy(SmartMetPlugin *us)
{
  // This will call 'Plugin::~Plugin()' since the destructor is virtual
  delete us;
}
