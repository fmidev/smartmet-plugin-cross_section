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
#include <boost/lexical_cast.hpp>
#include <boost/move/unique_ptr.hpp>
#include <boost/timer/timer.hpp>
#include <ctpp2/CDT.hpp>
#include <engines/geonames/Engine.h>
#include <json/json.h>
#include <json/reader.h>
#include <macgyver/AnsiEscapeCodes.h>
#include <spine/Convenience.h>
#include <macgyver/Exception.h>
#include <spine/OptionParsers.h>
#include <spine/SmartMet.h>
#include <spine/TimeSeriesGeneratorOptions.h>
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

const std::string &check_attack(const std::string &theName)
{
  try
  {
    if (theName.find("./") == std::string::npos)
      return theName;

    throw Fmi::Exception(
        BCP, "Attack IRI detected, relative paths upwards are not safe: '" + theName + "'");
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
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

std::string Plugin::query(SmartMet::Spine::Reactor &theReactor,
                          const SmartMet::Spine::HTTP::Request &theRequest,
                          SmartMet::Spine::HTTP::Response &theResponse)
{
  try
  {
    // Establish debugging related variables

    bool print_hash = SmartMet::Spine::optional_bool(theRequest.getParameter("hash"), false);

    bool print_json = SmartMet::Spine::optional_bool(theRequest.getParameter("json"), false);

    // Generate the query variables

    Query query;
    query.timer = SmartMet::Spine::optional_bool(theRequest.getParameter("timer"), false);

    query.customer = SmartMet::Spine::optional_string(theRequest.getParameter("customer"),
                                                      itsConfig.defaultCustomer());

    query.producer = SmartMet::Spine::required_string(
        theRequest.getParameter("producer"), "Product configuration option 'producer' not given");

    query.steps = SmartMet::Spine::required_unsigned_long(
        theRequest.getParameter("steps"),
        "Configuration option 'steps' must be given to determine "
        "how many parts the isocircle is divided into");

    query.timezone = SmartMet::Spine::optional_string(theRequest.getParameter("timezone"),
                                                      itsConfig.defaultTimeZone());

    // We require exactly two locations

    SmartMet::Engine::Geonames::LocationOptions loptions = itsGeoEngine->parseLocations(theRequest);

    if (loptions.size() != 2)
      throw Fmi::Exception(BCP,
                                       "Exactly two locations are required for a cross-section");

    const auto &locs = loptions.locations();
    query.longitude1 = locs.front().loc->longitude;
    query.latitude1 = locs.front().loc->latitude;
    query.longitude2 = locs.back().loc->longitude;
    query.latitude2 = locs.back().loc->latitude;

    // State variable

    State state(*this);
    state.query(query);

    // And timeseries options now that state (and querydata) is established

    SmartMet::Spine::TimeSeriesGeneratorOptions toptions =
        SmartMet::Spine::OptionParsers::parseTimes(theRequest);
    toptions.setDataTimes(state.producer()->validTimes(), state.producer()->isClimatology());

    auto tz = itsGeoEngine->getTimeZones().time_zone_from_string(query.timezone);
    auto times = SmartMet::Spine::TimeSeriesGenerator::generate(toptions, tz);

    // Product JSON

    auto product_name = SmartMet::Spine::required_string(
        theRequest.getParameter("product"), "Product configuration option 'product' not given");
    auto product = getProduct(query.customer, product_name, print_json);

    // The template to fill

    auto format_name = SmartMet::Spine::optional_string(theRequest.getParameter("format"),
                                                        itsConfig.defaultTemplate());

    auto tmpl = getTemplate(format_name);

    // Build the response CDT
    CTPP::CDT hash(CTPP::CDT::HASH_VAL);
    {
      std::string report = "Product::generate finished in %t sec CPU, %w sec real\n";
      boost::movelib::unique_ptr<boost::timer::auto_cpu_timer> mytimer;
      if (query.timer)
        mytimer = boost::movelib::make_unique<boost::timer::auto_cpu_timer>(2, report);
      product.generate(hash, state, times);
    }

    if (print_hash)
    {
      std::cout << "Generated CDT for " << query.customer << " " << product_name << std::endl
                << hash.RecursiveDump() << std::endl;
    }

    std::ostringstream output, log;
    try
    {
      std::string report = "Template processing finished in %t sec CPU, %w sec real\n";
      std::unique_ptr<boost::timer::auto_cpu_timer> mytimer;
      if (query.timer)
        mytimer.reset(new boost::timer::auto_cpu_timer(2, report));
      tmpl->process(hash, output, log);
    }
    catch (const CTPP::CTPPException &e)
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

    return output.str();
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
      std::cout << "Expanded " << theName << " JSON:" << std::endl << json << std::endl;

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
    std::string tmpl_path = (itsConfig.templateDirectory() + "/" + check_attack(theName) + ".c2t");

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

    using boost::posix_time::ptime;

    const bool isdebug = SmartMet::Spine::optional_bool(theRequest.getParameter("debug"), false);

    // Default expiration time

    const int expires_seconds = 60;

    // Now

    ptime t_now = boost::posix_time::second_clock::universal_time();

    try
    {
      std::string response = query(theReactor, theRequest, theResponse);

      theResponse.setStatus(SmartMet::Spine::HTTP::Status::ok);

      // Build cache expiration time info

      ptime t_expires = t_now + boost::posix_time::seconds(expires_seconds);

      // The headers themselves

      std::shared_ptr<Fmi::TimeFormatter> tformat(Fmi::TimeFormatter::create("http"));

      std::string cachecontrol =
          "public, max-age=" + boost::lexical_cast<std::string>(expires_seconds);
      std::string expiration = tformat->format(t_expires);
      std::string modification = tformat->format(t_now);

      theResponse.setHeader("Cache-Control", cachecontrol);
      theResponse.setHeader("Expires", expiration);
      theResponse.setHeader("Last-Modified", modification);

      if (response.size() == 0)
      {
        std::cerr << "Warning: Empty input for request " << theRequest.getQueryString() << " from "
                  << theRequest.getClientIP() << std::endl;
      }
      else
      {
        theResponse.setContent(response);
        theResponse.setHeader("Content-type", "application/json; charset=UTF-8");
#ifdef MYDEBUG
        std::cout << "Output:" << std::endl << response << std::endl;
#endif
      }
    }

    catch (...)
    {
      Fmi::Exception exception(BCP, "Request processing exception!");
      exception.addParameter("URI", theRequest.getURI());
      exception.addParameter("ClientIP", theRequest.getClientIP());
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
      msg = msg.substr(0, 100);
      theResponse.setHeader("X-CSection-Error", msg.c_str());
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
    : SmartMetPlugin(),
      itsModuleName("CrossSection"),
      itsConfig(theConfig),
      itsReactor(theReactor),
      itsQEngine(nullptr),
      itsContourEngine(nullptr),
      itsGeoEngine(nullptr),
      itsTemplateFactory()
{
  try
  {
    if (theReactor->getRequiredAPIVersion() != SMARTMET_API_VERSION)
    {
      std::cerr << ANSI_BOLD_ON << ANSI_FG_RED
                << "*** CrossSection Plugin and Server API version mismatch ***" << ANSI_FG_DEFAULT
                << ANSI_BOLD_OFF << std::endl;
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
  try
  {
    /* QEngine */

    auto engine = itsReactor->getSingleton("Querydata", nullptr);
    if (!engine)
      throw Fmi::Exception(BCP, "Querydata engine unavailable");
    itsQEngine = reinterpret_cast<SmartMet::Engine::Querydata::Engine *>(engine);

    /* Contour */
    engine = itsReactor->getSingleton("Contour", nullptr);
    if (!engine)
      throw Fmi::Exception(BCP, "Contour engine unavailable");
    itsContourEngine = reinterpret_cast<SmartMet::Engine::Contour::Engine *>(engine);

    /* GeoEngine */
    engine = itsReactor->getSingleton("Geonames", nullptr);
    if (!engine)
      throw Fmi::Exception(BCP, "Geonames engine unavailable");
    itsGeoEngine = reinterpret_cast<SmartMet::Engine::Geonames::Engine *>(engine);

    /* Register handler */

    if (!itsReactor->addContentHandler(this,
                                       itsConfig.defaultUrl(),
                                       boost::bind(&Plugin::callRequestHandler, this, _1, _2, _3)))
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
 * \brief Destructor
 */
// ----------------------------------------------------------------------

Plugin::~Plugin() {}
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
