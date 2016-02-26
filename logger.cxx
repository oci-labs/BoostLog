//  logger.cxx

#include "logger.hxx"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/core.hpp>
#include <boost/log/common.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/expressions/keyword.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/utility/exception_handler.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/filter_parser.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/utility/setup/from_stream.hpp>
#include <boost/log/utility/setup/settings.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <fstream>
#include <string>

BOOST_LOG_GLOBAL_LOGGER_CTOR_ARGS(sysLogger,
  boost::log::sources::severity_channel_logger_mt<boost::log::trivial::severity_level>,
  (boost::log::keywords::channel = "SYSLF"));

BOOST_LOG_GLOBAL_LOGGER_CTOR_ARGS(dataLogger,
  boost::log::sources::severity_channel_logger_mt<boost::log::trivial::severity_level>,
  (boost::log::keywords::channel = "DATALF"));

// Custom formatter factory to add TimeStamp format support in config ini file.
// Allows %TimeStamp(format=\"%Y.%m.%d %H:%M:%S.%f\")% to be used in ini config file for property Format.
class TimeStampFormatterFactory :
  public boost::log::basic_formatter_factory<char, boost::posix_time::ptime>
{
public:
  formatter_type create_formatter(const boost::log::attribute_name& name, const args_map& args) {
    args_map::const_iterator it = args.find("format");
    if (it != args.end()) {
      return boost::log::expressions::stream 
        << boost::log::expressions::format_date_time<boost::posix_time::ptime>(
             boost::log::expressions::attr<boost::posix_time::ptime>(name), it->second);
    } else {
      return boost::log::expressions::stream 
        << boost::log::expressions::attr<boost::posix_time::ptime>(name);
    }
  }
};

// Custom formatter factory to add Uptime format support in config ini file.
// Allows %Uptime(format=\"%O:%M:%S.%f\")% to be used in ini config file for property Format.
// boost::log::attributes::timer value type is boost::posix_time::time_duration
class UptimeFormatterFactory :
  public boost::log::basic_formatter_factory<char, boost::posix_time::time_duration>
{
public:
  formatter_type create_formatter(const boost::log::attribute_name& name, const args_map& args)
  {
    args_map::const_iterator it = args.find("format");
    if (it != args.end()) {
      return boost::log::expressions::stream
        << boost::log::expressions::format_date_time<boost::posix_time::time_duration>(
        boost::log::expressions::attr<boost::posix_time::time_duration>(name), it->second);
    } else {
      return boost::log::expressions::stream
        << boost::log::expressions::attr<boost::posix_time::time_duration>(name);
    }
  }
};

void
Logger::init() {
  initFromConfig("");
}

void
Logger::initFromConfig(const std::string& configFileName) {
  // Disable all exceptions
  boost::log::core::get()->set_exception_handler(boost::log::make_exception_suppressor());

  // Add common attributes: LineID, TimeStamp, ProcessID, ThreadID
  boost::log::add_common_attributes();
  // Add boost log timer as global attribute Uptime
  boost::log::core::get()->add_global_attribute("Uptime", boost::log::attributes::timer());
  // Allows %Severity% to be used in ini config file for property Filter.
  boost::log::register_simple_filter_factory<boost::log::trivial::severity_level, char>("Severity");
  // Allows %Severity% to be used in ini config file for property Format.
  boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char>("Severity");
  // Allows %TimeStamp(format=\"%Y.%m.%d %H:%M:%S.%f\")% to be used in ini config file for property Format.
  boost::log::register_formatter_factory("TimeStamp", boost::make_shared<TimeStampFormatterFactory>());
  // Allows %Uptime(format=\"%O:%M:%S.%f\")% to be used in ini config file for property Format.
  boost::log::register_formatter_factory("Uptime", boost::make_shared<UptimeFormatterFactory>());

  if (configFileName.empty()) {
    // Make sure we log to console if nothing specified.
    // Severity logger logs to console by default.
  } else {
    std::ifstream ifs(configFileName);
    if (!ifs.is_open()) {
      // We can log this because console is setup by default
      LOG_WARN("Unable to open logging config file: " << configFileName);
    } else {
      try {
        // Still can throw even with the exception suppressor above.
        boost::log::init_from_stream(ifs);
      } catch (std::exception& e) {
        std::string err = "Caught exception initializing boost logging: ";
        err += e.what();
        // Since we cannot be sure of boost log state, output to cerr and cout.
        std::cerr << "ERROR: " << err << std::endl;
        std::cout << "ERROR: " << err << std::endl;
        LOG_ERROR(err);
      }
    }
  }

  // Indicate start of logging
  LOG_INFO("Log Start");
}

void 
Logger::disable() {
  boost::log::core::get()->set_logging_enabled(false);
}

void 
Logger::addDataFileLog(const std::string& logFileName) {
  // Create a text file sink
  boost::shared_ptr<boost::log::sinks::text_ostream_backend> backend(
    new boost::log::sinks::text_ostream_backend());
  backend->add_stream(boost::shared_ptr<std::ostream>(new std::ofstream(logFileName)));

  // Flush after each log record
  backend->auto_flush(true);
  
  // Create a sink for the backend
  typedef boost::log::sinks::synchronous_sink<boost::log::sinks::text_ostream_backend> sink_t;
  boost::shared_ptr<sink_t> sink(new sink_t(backend));

  // The log output formatter
  sink->set_formatter(
    boost::log::expressions::format("[%1%][%2%] %3%")
    % boost::log::expressions::attr<boost::posix_time::ptime>("TimeStamp")
    % boost::log::trivial::severity
    % boost::log::expressions::smessage
    );

  // Filter by severity and by DATALF channel
  sink->set_filter(
    boost::log::trivial::severity >= boost::log::trivial::info &&
    boost::log::expressions::attr<std::string>("Channel") == "DATALF");

  // Add it to the core
  boost::log::core::get()->add_sink(sink);
}
