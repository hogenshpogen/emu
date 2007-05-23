// CSCSupervisor.cc

#include "CSCSupervisor.h"

#include <sstream>
#include <set>
#include <cstdlib>  // strtol()

#include "xdaq/NamespaceURI.h"
#include "xoap/Method.h"
#include "xoap/MessageFactory.h"  // createMessage()
#include "xoap/SOAPPart.h"
#include "xoap/SOAPEnvelope.h"
#include "xoap/SOAPBody.h"
#include "xoap/SOAPSerializer.h"
#include "toolbox/task/WorkLoopFactory.h" // getWorkLoopFactory()

#include "cgicc/HTMLClasses.h"
#include "xgi/Utils.h"

using namespace std;
using namespace cgicc;

XDAQ_INSTANTIATOR_IMPL(CSCSupervisor);

static const string NS_XSI = "http://www.w3.org/2001/XMLSchema-instance";
static const unsigned int N_LOG_MESSAGES = 20;
static const string STATE_UNKNOWN = "unknown";

CSCSupervisor::CSCSupervisor(xdaq::ApplicationStub *stub)
		throw (xdaq::exception::Exception) :
		EmuApplication(stub),
		daq_mode_(""), trigger_config_(""), ttc_source_(""),
		wl_semaphore_(BSem::EMPTY), quit_calibration_(false),
		daq_descr_(NULL), tf_descr_(NULL), ttc_descr_(NULL),
		nevents_(-1),
		step_counter_(0),
		error_message_(""), keep_refresh_(false)
{
	run_type_ = "";
	run_number_ = 0;

	xdata::InfoSpace *i = getApplicationInfoSpace();
	i->fireItemAvailable("RunType", &run_type_);
	i->fireItemAvailable("RunNumber", &run_number_);

	i->fireItemAvailable("configKeys", &config_keys_);
	i->fireItemAvailable("pcKeys",     &pc_keys_);
	i->fireItemAvailable("pcConfigs",  &pc_configs_);
	i->fireItemAvailable("fcKeys",     &fc_keys_);
	i->fireItemAvailable("fcConfigs",  &fc_configs_);

	i->fireItemAvailable("DAQMode", &daq_mode_);
	i->fireItemAvailable("TriggerConfig", &trigger_config_);
	i->fireItemAvailable("TTCSource", &ttc_source_);

	i->fireItemAvailable("TTSCrate", &tts_crate_);
	i->fireItemAvailable("TTSSlot", &tts_slot_);
	i->fireItemAvailable("TTSBits", &tts_bits_);

	xgi::bind(this, &CSCSupervisor::webDefault,   "Default");
	xgi::bind(this, &CSCSupervisor::webConfigure, "Configure");
	xgi::bind(this, &CSCSupervisor::webEnable,    "Enable");
	xgi::bind(this, &CSCSupervisor::webDisable,   "Disable");
	xgi::bind(this, &CSCSupervisor::webHalt,      "Halt");
	xgi::bind(this, &CSCSupervisor::webReset,     "Reset");
	xgi::bind(this, &CSCSupervisor::webSetTTS,    "SetTTS");

	xoap::bind(this, &CSCSupervisor::onConfigure, "Configure", XDAQ_NS_URI);
	xoap::bind(this, &CSCSupervisor::onEnable,    "Enable",    XDAQ_NS_URI);
	xoap::bind(this, &CSCSupervisor::onDisable,   "Disable",   XDAQ_NS_URI);
	xoap::bind(this, &CSCSupervisor::onHalt,      "Halt",      XDAQ_NS_URI);
	xoap::bind(this, &CSCSupervisor::onReset,     "Reset",     XDAQ_NS_URI);
	xoap::bind(this, &CSCSupervisor::onSetTTS,    "SetTTS",    XDAQ_NS_URI);

	wl_ = toolbox::task::getWorkLoopFactory()->getWorkLoop("CSC SV", "waiting");
	wl_->activate();
	configure_signature_ = toolbox::task::bind(
			this, &CSCSupervisor::configureAction,  "configureAction");
	halt_signature_ = toolbox::task::bind(
			this, &CSCSupervisor::haltAction,       "haltAction");
	calibration_signature_ = toolbox::task::bind(
			this, &CSCSupervisor::calibrationAction, "calibrationAction");

	fsm_.addState('H', "Halted",     this, &CSCSupervisor::stateChanged);
	fsm_.addState('C', "Configured", this, &CSCSupervisor::stateChanged);
	fsm_.addState('E', "Enabled",    this, &CSCSupervisor::stateChanged);

	fsm_.addStateTransition(
			'H', 'C', "Configure", this, &CSCSupervisor::configureAction);
	fsm_.addStateTransition(
			'C', 'C', "Configure", this, &CSCSupervisor::configureAction);
	fsm_.addStateTransition(
			'C', 'E', "Enable",    this, &CSCSupervisor::enableAction);
	fsm_.addStateTransition(
			'E', 'C', "Disable",   this, &CSCSupervisor::disableAction);
	fsm_.addStateTransition(
			'C', 'H', "Halt",      this, &CSCSupervisor::haltAction);
	fsm_.addStateTransition(
			'E', 'H', "Halt",      this, &CSCSupervisor::haltAction);
	fsm_.addStateTransition(
			'H', 'H', "Halt",      this, &CSCSupervisor::haltAction);
	fsm_.addStateTransition(
			'E', 'E', "SetTTS",    this, &CSCSupervisor::setTTSAction);

	fsm_.setInitialState('H');
	fsm_.reset();

	state_ = fsm_.getStateName(fsm_.getCurrentState());

	state_table_.addApplication(this, "EmuFCrate");
	state_table_.addApplication(this, "EmuPeripheralCrateManager");
	state_table_.addApplication(this, "EmuPeripheralCrate");
	state_table_.addApplication(this, "EmuDAQManager");
	state_table_.addApplication(this, "LTCControl");

	last_log_.size(N_LOG_MESSAGES);

	LOG4CPLUS_INFO(getApplicationLogger(), "CSCSupervisor");
}

xoap::MessageReference CSCSupervisor::onConfigure(xoap::MessageReference message)
		throw (xoap::exception::Exception)
{
	run_number_ = 0;
	nevents_ = -1;

	submit(configure_signature_);

	return createReply(message);
}

xoap::MessageReference CSCSupervisor::onEnable(xoap::MessageReference message)
		throw (xoap::exception::Exception)
{
	fireEvent("Enable");

	return createReply(message);
}

xoap::MessageReference CSCSupervisor::onDisable(xoap::MessageReference message)
		throw (xoap::exception::Exception)
{
	fireEvent("Disable");

	return createReply(message);
}

xoap::MessageReference CSCSupervisor::onHalt(xoap::MessageReference message)
		throw (xoap::exception::Exception)
{
	quit_calibration_ = true;

	submit(halt_signature_);

	return createReply(message);
}

xoap::MessageReference CSCSupervisor::onReset(xoap::MessageReference message)
		throw (xoap::exception::Exception)
{
	resetAction();

	return onHalt(message);
}

xoap::MessageReference CSCSupervisor::onSetTTS(xoap::MessageReference message)
		throw (xoap::exception::Exception)
{
	fireEvent("SetTTS");

	return createReply(message);
}

void CSCSupervisor::webDefault(xgi::Input *in, xgi::Output *out)
		throw (xgi::exception::Exception)
{
	if (keep_refresh_) {
		HTTPResponseHeader &header = out->getHTTPResponseHeader();
		header.addHeader("Refresh", "2");
	}

	// Header
	*out << HTMLDoctype(HTMLDoctype::eStrict) << endl;
	*out << html() << endl;

	*out << head() << endl;
	*out << title("CSCSupervisor") << endl;
	*out << cgicc::link().set("rel", "stylesheet")
			.set("href", "/daq/lib/linux/x86/cscsv.css")
			.set("type", "text/css") << endl;
	*out << head() << endl;

	// Body
	*out << body() << endl;

	// Error message, if exists.
	if (!error_message_.empty()) {
		*out  << p() << span().set("style", "color: red;")
				<< error_message_ << span() << p() << endl;
		error_message_ = "";
	}

	// Config listbox
	*out << form().set("action",
			"/" + getApplicationDescriptor()->getURN() + "/Configure") << endl;

	int n_keys = config_keys_.size();

	*out << "Run Type: " << endl;
	*out << cgicc::select().set("name", "runtype") << endl;

	int selected_index = keyToIndex(run_type_.toString());

	for (int i = 0; i < n_keys; ++i) {
		if (i == selected_index) {
			*out << option()
					.set("value", (string)config_keys_[i])
					.set("selected", "");
		} else {
			*out << option()
					.set("value", (string)config_keys_[i]);
		}
		*out << (string)config_keys_[i] << option() << endl;
	}

	*out << cgicc::select() << br() << endl;
	
	*out << "Run Number: " << endl;
	*out << input().set("type", "text")
			.set("name", "runnumber")
			.set("value", run_number_.toString())
			.set("size", "40") << br() << endl;

	*out << "Max # of Events: " << endl;
	*out << input().set("type", "text")
			.set("name", "nevents")
			.set("value", toString(nevents_))
			.set("size", "40") << br() << endl;

	*out << input().set("type", "submit")
			.set("name", "command")
			.set("value", "Configure") << endl;
	*out << form() << endl;

	// Buttons
	*out << form().set("action",
			"/" + getApplicationDescriptor()->getURN() + "/Enable") << endl;
	*out << input().set("type", "submit")
			.set("name", "command")
			.set("value", "Enable") << endl;
	*out << form() << endl;

	*out << form().set("action",
			"/" + getApplicationDescriptor()->getURN() + "/Disable") << endl;
	*out << input().set("type", "submit")
			.set("name", "command")
			.set("value", "Disable") << endl;
	*out << form() << endl;

	*out << form().set("action",
			"/" + getApplicationDescriptor()->getURN() + "/Halt") << endl;
	*out << input().set("type", "submit")
			.set("name", "command")
			.set("value", "Halt") << endl;
	*out << form() << endl;

	*out << form().set("action",
			"/" + getApplicationDescriptor()->getURN() + "/Reset") << endl;
	*out << input().set("type", "submit")
			.set("name", "command")
			.set("value", "Reset") << endl;
	*out << form() << endl;

	// Configuration parameters
	refreshConfigParameters();
	*out << "Mode of DAQManager: " << daq_mode_.toString() << br() << endl;
	*out << "TF configuration: " << trigger_config_.toString() << br() << endl;
	*out << "TTCci inputs(Clock:Orbit:Trig:BGo): " << ttc_source_.toString() << br() << endl;

	*out << "Local DAQ state: " << getLocalDAQState() << br() << endl;

	// Application states
	*out << hr() << endl;
	state_table_.webOutput(out, (string)state_);

	// TTS operation
	*out << hr() << endl;
	*out << form().set("action",
			"/" + getApplicationDescriptor()->getURN() + "/SetTTS") << endl;

	*out << "Crate #: " << endl;
	*out << cgicc::select().set("name", "tts_crate") << endl;

	const char n[] = "1234";
	string str = "";
	for (int i = 0; i < 4; ++i) {
		if (n[i] == (tts_crate_.toString())[0]) {
			*out << option().set("value", str + n[i]).set("selected", "");
		} else {
			*out << option().set("value", str + n[i]);
		}
		*out << n[i] << option() << endl;
	}

	*out << cgicc::select() << br() << endl;
	
	*out << "Slot # (4-13): " << endl;
	*out << input().set("type", "text")
			.set("name", "tts_slot")
			.set("value", tts_slot_)
			.set("size", "10") << br() << endl;

	*out << "TTS value: (0-15)" << endl;
	*out << input().set("type", "text")
			.set("name", "tts_bits")
			.set("value", tts_bits_)
			.set("size", "10") << br() << endl;

	*out << input().set("type", "submit")
			.set("name", "command")
			.set("value", "SetTTS") << endl;
	*out << form() << endl;

	*out << "Step counter: " << step_counter_ << endl;

	// Message logs
	*out << hr() << endl;
	last_log_.webOutput(out);

	*out << body() << html() << endl;
}

void CSCSupervisor::webConfigure(xgi::Input *in, xgi::Output *out)
		throw (xgi::exception::Exception)
{
	string value;

	value = getCGIParameter(in, "runtype");
	if (value.empty()) { error_message_ += "Please select run type.\n"; }
	run_type_ = value;

	value = getCGIParameter(in, "runnumber");
	if (value.empty()) { error_message_ += "Please set run number.\n"; }
	run_number_ = strtol(value.c_str(), NULL, 0);

	value = getCGIParameter(in, "nevents");
	if (value.empty()) { error_message_ += "Please set max # of events.\n"; }
	nevents_ = strtol(value.c_str(), NULL, 0);

	if (error_message_.empty()) {
		submit(configure_signature_);
	}

	webRedirect(in, out);
}

void CSCSupervisor::webEnable(xgi::Input *in, xgi::Output *out)
		throw (xgi::exception::Exception)
{
	fireEvent("Enable");

	webRedirect(in, out);
}

void CSCSupervisor::webDisable(xgi::Input *in, xgi::Output *out)
		throw (xgi::exception::Exception)
{
	fireEvent("Disable");

	webRedirect(in, out);
}

void CSCSupervisor::webHalt(xgi::Input *in, xgi::Output *out)
		throw (xgi::exception::Exception)
{
	quit_calibration_ = true;

	submit(halt_signature_);

	webRedirect(in, out);
}

void CSCSupervisor::webReset(xgi::Input *in, xgi::Output *out)
		throw (xgi::exception::Exception)
{
	resetAction();

	webHalt(in, out);
}

void CSCSupervisor::webSetTTS(xgi::Input *in, xgi::Output *out)
		throw (xgi::exception::Exception)
{
	tts_crate_ = getCGIParameter(in, "tts_crate");
	tts_slot_  = getCGIParameter(in, "tts_slot");
	tts_bits_  = getCGIParameter(in, "tts_bits");

	if (tts_crate_ == "") { error_message_ += "Please select TTS crate.\n"; }
	if (tts_slot_  == "") { error_message_ += "Please set TTS slot.\n"; }
	if (tts_bits_  == "") { error_message_ += "Please set TTS bits.\n"; }

	if (error_message_.empty()) {
		fireEvent("SetTTS");
	}

	webRedirect(in, out);
}

void CSCSupervisor::webRedirect(xgi::Input *in, xgi::Output *out)
		throw (xgi::exception::Exception)
{
	string url = in->getenv("PATH_TRANSLATED");

	HTTPResponseHeader &header = out->getHTTPResponseHeader();

	header.getStatusCode(303);
	header.getReasonPhrase("See Other");
	header.addHeader("Location",
			url.substr(0, url.find("/" + in->getenv("PATH_INFO"))));
}

bool CSCSupervisor::configureAction(toolbox::task::WorkLoop *wl)
{
	fireEvent("Configure");

	return false;
}

bool CSCSupervisor::haltAction(toolbox::task::WorkLoop *wl)
{
	fireEvent("Halt");

	return false;
}

bool CSCSupervisor::calibrationAction(toolbox::task::WorkLoop *wl)
{
	string command;
	unsigned int loop;

	if (run_type_ == "Calib_CFEB_Gain") {
		command = "EnableCalCFEBGain";
		loop = 160;
	} else if (run_type_ == "Calib_CFEB_Time") {
		command = "EnableCalCFEBTime";
		loop = 320;
	} else if (run_type_ == "Calib_CFEB_Time") {
		command = "EnableCalCFEBPed";
		loop = 1;
	} else {
		return false;
	}

	std::map<string, string> start_attr, stop_attr;
	
	start_attr.insert(std::map<string, string>::value_type("Param", "Start"));
	stop_attr.insert(std::map<string, string>::value_type("Param", "Stop"));

	sendCommandWithAttr("Cyclic", stop_attr, "LTCControl");

	for (step_counter_ = 0; step_counter_ < loop; ++step_counter_) {
		if (quit_calibration_) { break; }
		LOG4CPLUS_DEBUG(getApplicationLogger(),
				"calibrationAction: " << step_counter_);

		sendCommand(command, "EmuPeripheralCrateManager");
		sendCommandWithAttr("Cyclic", start_attr, "LTCControl");
		sleep(10U);
		sendCommandWithAttr("Cyclic", stop_attr, "LTCControl");
	}

	quit_calibration_ = false;
	keep_refresh_ = false;

	return false;
}

void CSCSupervisor::configureAction(toolbox::Event::Reference evt) 
		throw (toolbox::fsm::exception::Exception)
{
	LOG4CPLUS_DEBUG(getApplicationLogger(), evt->type() << "(begin)");
	LOG4CPLUS_DEBUG(getApplicationLogger(), "runtype: " << run_type_.toString()
			<< " runnumber: " << run_number_ << " nevents: " << nevents_);

	try {
		if (state_table_.getState("EmuDAQManager", 0) == "Configured") {
			sendCommand("Halt", "EmuDAQManager");
		}
		if (state_table_.getState("LTCControl", 0) == "Ready") {
			sendCommand("Halt", "LTCControl");
		}
		string str = trim(getCrateConfig("PC", run_type_.toString()));
		if (!str.empty()) {
			setParameter(
					"EmuPeripheralCrate", "xmlFileName", "xsd:string", str);
		}

		setParameter("EmuDAQManager", "maxNumberOfEvents", "xsd:integer",
				toString(nevents_));
		setParameter("EmuDAQManager", "runType", "xsd:string",
				run_type_.toString());

		sendCommand("Configure", "EmuFCrate");
		if (!isCalibrationMode()) {
			sendCommand("Configure", "EmuPeripheralCrate");
		} else {
			sendCommand("ConfigCalCFEB", "EmuPeripheralCrateManager");
		}
		sendCommand("Configure", "EmuDAQManager");
		sendCommand("Configure", "LTCControl");

		refreshConfigParameters();

	} catch (xoap::exception::Exception e) {
		LOG4CPLUS_ERROR(getApplicationLogger(),
				"Exception in " << evt->type() << ": " << e.what());
		XCEPT_RETHROW(toolbox::fsm::exception::Exception,
				"SOAP fault was returned", e);
	} catch (xdaq::exception::Exception e) {
		LOG4CPLUS_ERROR(getApplicationLogger(),
				"Exception in " << evt->type() << ": " << e.what());
		XCEPT_RETHROW(toolbox::fsm::exception::Exception,
				"Failed to send a command", e);
	}

	LOG4CPLUS_DEBUG(getApplicationLogger(), evt->type() << "(end)");
}

void CSCSupervisor::enableAction(toolbox::Event::Reference evt) 
		throw (toolbox::fsm::exception::Exception)
{
	LOG4CPLUS_DEBUG(getApplicationLogger(), evt->type() << "(begin)");
	LOG4CPLUS_DEBUG(getApplicationLogger(), "runtype: " << run_type_.toString()
			<< " runnumber: " << run_number_ << " nevents: " << nevents_);

	try {
		if (state_table_.getState("EmuDAQManager", 0) == "Halted") {
			setParameter("EmuDAQManager", "maxNumberOfEvents", "xsd:integer",
					toString(nevents_));
			sendCommand("Configure", "EmuDAQManager");
		}
		setParameter("EmuDAQManager", "runNumber", "xsd:unsignedLong",
				run_number_.toString());
		sendCommand("Enable", "EmuFCrate");
		if (!isCalibrationMode()) {
			sendCommand("Enable", "EmuPeripheralCrate");
		}
		sendCommand("Enable", "EmuDAQManager");
		sendCommand("Enable", "LTCControl");

		refreshConfigParameters();

	} catch (xoap::exception::Exception e) {
		XCEPT_RETHROW(toolbox::fsm::exception::Exception,
				"SOAP fault was returned", e);
	} catch (xdaq::exception::Exception e) {
		XCEPT_RETHROW(toolbox::fsm::exception::Exception,
				"Failed to send a command", e);
	}

	if (isCalibrationMode()) {
		submit(calibration_signature_);
	}

	LOG4CPLUS_DEBUG(getApplicationLogger(), evt->type() << "(end)");
}

void CSCSupervisor::disableAction(toolbox::Event::Reference evt) 
		throw (toolbox::fsm::exception::Exception)
{
	LOG4CPLUS_DEBUG(getApplicationLogger(), evt->type() << "(begin)");

	try {
		sendCommand("Halt", "LTCControl");
		sendCommand("Halt", "EmuDAQManager");
		sendCommand("Disable", "EmuFCrate");
		if (!isCalibrationMode()) {
			sendCommand("Disable", "EmuPeripheralCrate");
		} else {
			sendCommand("Disable", "EmuPeripheralCrateManager");
		}
		sendCommand("Configure", "LTCControl");
	} catch (xoap::exception::Exception e) {
		XCEPT_RETHROW(toolbox::fsm::exception::Exception,
				"SOAP fault was returned", e);
	} catch (xdaq::exception::Exception e) {
		XCEPT_RETHROW(toolbox::fsm::exception::Exception,
				"Failed to send a command", e);
	}

	LOG4CPLUS_DEBUG(getApplicationLogger(), evt->type() << "(end)");
}

void CSCSupervisor::haltAction(toolbox::Event::Reference evt) 
		throw (toolbox::fsm::exception::Exception)
{
	LOG4CPLUS_DEBUG(getApplicationLogger(), evt->type() << "(begin)");

	try {
		sendCommand("Halt", "EmuFCrate");
		sendCommand("Halt", "EmuPeripheralCrateManager");
		sendCommand("Halt", "EmuPeripheralCrate");
		sendCommand("Halt", "EmuDAQManager");
		sendCommand("Halt", "LTCControl");
	} catch (xoap::exception::Exception e) {
		XCEPT_RETHROW(toolbox::fsm::exception::Exception,
				"SOAP fault was returned", e);
	} catch (xdaq::exception::Exception e) {
		XCEPT_RETHROW(toolbox::fsm::exception::Exception,
				"Failed to send a command", e);
	}

	LOG4CPLUS_DEBUG(getApplicationLogger(), evt->type() << "(end)");
}

void CSCSupervisor::resetAction() throw (toolbox::fsm::exception::Exception)
{
	LOG4CPLUS_DEBUG(getApplicationLogger(), "reset(begin)");

	fsm_.reset();
	state_ = fsm_.getStateName(fsm_.getCurrentState());

	LOG4CPLUS_DEBUG(getApplicationLogger(), "reset(end)");
}

void CSCSupervisor::setTTSAction(toolbox::Event::Reference evt) 
		throw (toolbox::fsm::exception::Exception)
{
	LOG4CPLUS_DEBUG(getApplicationLogger(), evt->type() << "(begin)");

	const string fed_app = "EmuFCrate";

	try {
		setParameter(fed_app, "ttsCrate", "xsd:unsignedInt", tts_crate_);
		setParameter(fed_app, "ttsSlot",  "xsd:unsignedInt", tts_slot_);
		setParameter(fed_app, "ttsBits",  "xsd:unsignedInt", tts_bits_);

		int instance = (tts_crate_ == "1") ? 0 : 1;
		sendCommand("SetTTSBits", fed_app, instance);
	} catch (xoap::exception::Exception e) {
		XCEPT_RETHROW(toolbox::fsm::exception::Exception,
				"SOAP fault was returned", e);
	} catch (xdaq::exception::Exception e) {
		XCEPT_RETHROW(toolbox::fsm::exception::Exception,
				"Failed to send a command", e);
	}

	LOG4CPLUS_DEBUG(getApplicationLogger(), evt->type() << "(end)");
}

void CSCSupervisor::submit(toolbox::task::ActionSignature *signature)
{
	wl_->submit(signature);
	keep_refresh_ = true;
}

void CSCSupervisor::stateChanged(toolbox::fsm::FiniteStateMachine &fsm)
        throw (toolbox::fsm::exception::Exception)
{
	keep_refresh_ = false;

    EmuApplication::stateChanged(fsm);
}

void CSCSupervisor::sendCommand(string command, string klass)
		throw (xoap::exception::Exception, xdaq::exception::Exception)
{
	// Exceptions:
	// xoap exceptions are thrown by analyzeReply() for SOAP faults.
	// xdaq exceptions are thrown by postSOAP() for socket level errors.

	// find applications
	std::set<xdaq::ApplicationDescriptor *> apps;
	try {
		apps = getApplicationContext()->getDefaultZone()
				->getApplicationDescriptors(klass);
	} catch (xdaq::exception::ApplicationDescriptorNotFound e) {
		return; // Do nothing if the target doesn't exist
	}

	if (klass == "EmuDAQManager" && !isDAQManagerControlled(command)) {
		return;  // Do nothing if EmuDAQManager is not under control.
	}

	// prepare a SOAP message
	xoap::MessageReference message = createCommandSOAP(command);
	xoap::MessageReference reply;

	// send the message one-by-one
	std::set<xdaq::ApplicationDescriptor *>::iterator i = apps.begin();
	for (; i != apps.end(); ++i) {
		// postSOAP() may throw an exception when failed.
		reply = getApplicationContext()->postSOAP(message, *i);

		analyzeReply(message, reply, *i);
	}
}

void CSCSupervisor::sendCommand(string command, string klass, int instance)
		throw (xoap::exception::Exception, xdaq::exception::Exception)
{
	// Exceptions:
	// xoap exceptions are thrown by analyzeReply() for SOAP faults.
	// xdaq exceptions are thrown by postSOAP() for socket level errors.

	// find applications
	xdaq::ApplicationDescriptor *app;
	try {
		app = getApplicationContext()->getDefaultZone()
				->getApplicationDescriptor(klass, instance);
	} catch (xdaq::exception::ApplicationDescriptorNotFound e) {
		return; // Do nothing if the target doesn't exist
	}

	if (klass == "EmuDAQManager" && !isDAQManagerControlled(command)) {
		return;  // Do nothing if EmuDAQManager is not under control.
	}

	// prepare a SOAP message
	xoap::MessageReference message = createCommandSOAP(command);
	xoap::MessageReference reply;

	// send the message
	// postSOAP() may throw an exception when failed.
	reply = getApplicationContext()->postSOAP(message, app);

	analyzeReply(message, reply, app);
}

void CSCSupervisor::sendCommandWithAttr(
		string command, std::map<string, string> attr, string klass)
		throw (xoap::exception::Exception, xdaq::exception::Exception)
{
	// Exceptions:
	// xoap exceptions are thrown by analyzeReply() for SOAP faults.
	// xdaq exceptions are thrown by postSOAP() for socket level errors.

	// find applications
	std::set<xdaq::ApplicationDescriptor *> apps;
	try {
		apps = getApplicationContext()->getDefaultZone()
				->getApplicationDescriptors(klass);
	} catch (xdaq::exception::ApplicationDescriptorNotFound e) {
		return; // Do nothing if the target doesn't exist
	}

	if (klass == "EmuDAQManager" && !isDAQManagerControlled(command)) {
		return;  // Do nothing if EmuDAQManager is not under control.
	}

	// prepare a SOAP message
	xoap::MessageReference message = createCommandSOAPWithAttr(command, attr);
	xoap::MessageReference reply;

	// send the message one-by-one
	std::set<xdaq::ApplicationDescriptor *>::iterator i = apps.begin();
	for (; i != apps.end(); ++i) {
		// postSOAP() may throw an exception when failed.
		reply = getApplicationContext()->postSOAP(message, *i);

		analyzeReply(message, reply, *i);
	}
}

xoap::MessageReference CSCSupervisor::createCommandSOAP(string command)
{
	xoap::MessageReference message = xoap::createMessage();
	xoap::SOAPEnvelope envelope = message->getSOAPPart().getEnvelope();
	xoap::SOAPName name = envelope.createName(command, "xdaq", XDAQ_NS_URI);
	envelope.getBody().addBodyElement(name);

	return message;
}

xoap::MessageReference CSCSupervisor::createCommandSOAPWithAttr(
		string command, std::map<string, string> attr)
{
	xoap::MessageReference message = xoap::createMessage();
	xoap::SOAPEnvelope envelope = message->getSOAPPart().getEnvelope();
	xoap::SOAPName name = envelope.createName(command, "xdaq", XDAQ_NS_URI);
	xoap::SOAPElement element = envelope.getBody().addBodyElement(name);

	std::map<string, string>::iterator i;
	for (i = attr.begin(); i != attr.end(); ++i) {
		xoap::SOAPName p = envelope.createName((*i).first, "xdaq", XDAQ_NS_URI);
		element.addAttribute(p, (*i).second);
	}

	return message;
}

void CSCSupervisor::setParameter(
		string klass, string name, string type, string value)
{
	// find applications
	std::set<xdaq::ApplicationDescriptor *> apps;
	try {
		apps = getApplicationContext()->getDefaultZone()
				->getApplicationDescriptors(klass);
	} catch (xdaq::exception::ApplicationDescriptorNotFound e) {
		return; // Do nothing if the target doesn't exist
	}

	// prepare a SOAP message
	xoap::MessageReference message = createParameterSetSOAP(
			klass, name, type, value);
	xoap::MessageReference reply;

	// send the message one-by-one
	std::set<xdaq::ApplicationDescriptor *>::iterator i = apps.begin();
	for (; i != apps.end(); ++i) {
		reply = getApplicationContext()->postSOAP(message, *i);
		analyzeReply(message, reply, *i);
	}
}

xoap::MessageReference CSCSupervisor::createParameterSetSOAP(
		string klass, string name, string type, string value)
{
	xoap::MessageReference message = xoap::createMessage();
	xoap::SOAPEnvelope envelope = message->getSOAPPart().getEnvelope();
	envelope.addNamespaceDeclaration("xsi", NS_XSI);

	xoap::SOAPName command = envelope.createName(
			"ParameterSet", "xdaq", XDAQ_NS_URI);
	xoap::SOAPName properties = envelope.createName(
			"properties", klass, "urn:xdaq-application:" + klass);
	xoap::SOAPName parameter = envelope.createName(
			name, klass, "urn:xdaq-application:" + klass);
	xoap::SOAPName xsitype = envelope.createName("type", "xsi", NS_XSI);

	xoap::SOAPElement properties_e = envelope.getBody()
			.addBodyElement(command)
			.addChildElement(properties);
	properties_e.addAttribute(xsitype, "soapenc:Struct");

	xoap::SOAPElement parameter_e = properties_e.addChildElement(parameter);
	parameter_e.addAttribute(xsitype, type);
	parameter_e.addTextNode(value);

	return message;
}

xoap::MessageReference CSCSupervisor::createParameterGetSOAP(
		string klass, string name, string type)
{
	xoap::MessageReference message = xoap::createMessage();
	xoap::SOAPEnvelope envelope = message->getSOAPPart().getEnvelope();
	envelope.addNamespaceDeclaration("xsi", NS_XSI);

	xoap::SOAPName command = envelope.createName(
			"ParameterGet", "xdaq", XDAQ_NS_URI);
	xoap::SOAPName properties = envelope.createName(
			"properties", klass, "urn:xdaq-application:" + klass);
	xoap::SOAPName parameter = envelope.createName(
			name, klass, "urn:xdaq-application:" + klass);
	xoap::SOAPName xsitype = envelope.createName("type", "xsi", NS_XSI);

	xoap::SOAPElement properties_e = envelope.getBody()
			.addBodyElement(command)
			.addChildElement(properties);
	properties_e.addAttribute(xsitype, "soapenc:Struct");

	xoap::SOAPElement parameter_e = properties_e.addChildElement(parameter);
	parameter_e.addAttribute(xsitype, type);
	parameter_e.addTextNode("");

	return message;
}

xoap::MessageReference CSCSupervisor::createParameterGetSOAP2(
		string klass, int length, string names[], string types[])
{
	xoap::MessageReference message = xoap::createMessage();
	xoap::SOAPEnvelope envelope = message->getSOAPPart().getEnvelope();
	envelope.addNamespaceDeclaration("xsi", NS_XSI);

	xoap::SOAPName command = envelope.createName(
			"ParameterGet", "xdaq", XDAQ_NS_URI);
	xoap::SOAPName properties = envelope.createName(
			"properties", klass, "urn:xdaq-application:" + klass);
	xoap::SOAPName xsitype = envelope.createName("type", "xsi", NS_XSI);

	xoap::SOAPElement properties_e = envelope.getBody()
			.addBodyElement(command)
			.addChildElement(properties);
	properties_e.addAttribute(xsitype, "soapenc:Struct");

	for (int i = 0; i < length; ++i) {
		xoap::SOAPName parameter = envelope.createName(
				names[i], klass, "urn:xdaq-application:" + klass);
		xoap::SOAPElement parameter_e = properties_e.addChildElement(parameter);
		parameter_e.addAttribute(xsitype, types[i]);
		parameter_e.addTextNode("");
	}

	return message;
}

void CSCSupervisor::analyzeReply(
		xoap::MessageReference message, xoap::MessageReference reply,
		xdaq::ApplicationDescriptor *app)
{
	string message_str, reply_str;

	reply->writeTo(reply_str);
	ostringstream s;
	s << "Reply from "
			<< app->getClassName() << "(" << app->getInstance() << ")" << endl
			<< reply_str;
	last_log_.add(s.str());
	LOG4CPLUS_DEBUG(getApplicationLogger(), reply_str);

	xoap::SOAPBody body = reply->getSOAPPart().getEnvelope().getBody();

	// do nothing when no fault
	if (!body.hasFault()) { return; }

	ostringstream error;

	error << "SOAP message: " << endl;
	message->writeTo(message_str);
	error << message_str << endl;
	error << "Fault string: " << endl;
	error << reply_str << endl;

	LOG4CPLUS_ERROR(getApplicationLogger(), error.str());
	XCEPT_RAISE(xoap::exception::Exception, "SOAP fault: \n" + reply_str);

	return;
}

string CSCSupervisor::extractParameter(
		xoap::MessageReference message, string name)
{
	xoap::SOAPElement root = message->getSOAPPart()
			.getEnvelope().getBody().getChildElements(
			*(new xoap::SOAPName("ParameterGetResponse", "", "")))[0];
	xoap::SOAPElement properties = root.getChildElements(
			*(new xoap::SOAPName("properties", "", "")))[0];
	xoap::SOAPElement parameter = properties.getChildElements(
			*(new xoap::SOAPName(name, "", "")))[0];

	return parameter.getValue();
}

void CSCSupervisor::refreshConfigParameters()
{
	daq_mode_ = getDAQMode();
	trigger_config_ = getTFConfig();
	ttc_source_ = getTTCciSource();
}

string CSCSupervisor::getCGIParameter(xgi::Input *in, string name)
{
	cgicc::Cgicc cgi(in);
	string value;

	form_iterator i = cgi.getElement(name);
	if (i != cgi.getElements().end()) {
		value = (*i).getValue();
	}

	return value;
}

int CSCSupervisor::keyToIndex(const string key)
{
	int index = -1;

	for (unsigned int i = 0; i < config_keys_.size(); ++i) {
		if (config_keys_[i] == key) {
			index = i;
			break;
		}
	}

	return index;
}

string CSCSupervisor::getCrateConfig(const string type, const string key) const
{
	xdata::Vector<xdata::String> keys;
	xdata::Vector<xdata::String> values;

	if (type == "PC") {
		keys = pc_keys_;
		values = pc_configs_;
	} else if (type == "FC") {
		keys = fc_keys_;
		values = fc_configs_;
	} else {
		return "";
	}

	string result = "";
	for (unsigned int i = 0; i < keys.size(); ++i) {
		if (keys[i] == key) {
			result = values[i];
			break;
		}
	}

	return result;
}

bool CSCSupervisor::isCalibrationMode()
{
	return (run_type_.toString().substr(0, 5) == "Calib");
}

string CSCSupervisor::trim(string orig) const
{
	string s = orig;

	s.erase(0, s.find_first_not_of(" \t\n"));
	s.erase(s.find_last_not_of(" \t\n") + 1);

	return s;
}

string CSCSupervisor::toString(const long int i) const
{
	ostringstream s;
	s << i;

	return s.str();
}

string CSCSupervisor::getDAQMode()
{
	string result = "";

	if (daq_descr_ == NULL) {
		try {
			daq_descr_ = getApplicationContext()->getDefaultZone()
					->getApplicationDescriptor("EmuDAQManager", 0);
		} catch (xdaq::exception::ApplicationDescriptorNotFound e) {
			return result; // Do nothing if the target doesn't exist
		}
		daq_param_ = createParameterGetSOAP(
				"EmuDAQManager", "globalMode", "xsd:boolean");
		daq_configured_param_ = createParameterGetSOAP(
				"EmuDAQManager", "configuredInGlobalMode", "xsd:boolean");
		daq_state_param_ = createParameterGetSOAP(
				"EmuDAQManager", "daqState", "xsd:string");
	}

	xoap::MessageReference reply;
	try {
		reply = getApplicationContext()->postSOAP(daq_param_, daq_descr_);

		result = extractParameter(reply, "globalMode");
		result = (result == "true") ? "global" : "local";
	} catch (xdaq::exception::Exception e) {
		result = "Unknown";
	}

	return result;
}

string CSCSupervisor::getTFConfig()
{
	string result = "";

	if (tf_descr_ == NULL) {
		try {
			tf_descr_ = getApplicationContext()->getDefaultZone()
					->getApplicationDescriptor("TF_hyperDAQ", 0);
		} catch (xdaq::exception::ApplicationDescriptorNotFound e) {
			return result; // Do nothing if the target doesn't exist
		}
		tf_param_ = createParameterGetSOAP(
				"TF_hyperDAQ", "triggerMode", "xsd:string");
	}

	xoap::MessageReference reply;
	try {
		reply = getApplicationContext()->postSOAP(tf_param_, tf_descr_);

		result = extractParameter(reply, "triggerMode");
	} catch (xdaq::exception::Exception e) {
		result = "Unknown";
	}

	return result;
}

string CSCSupervisor::getTTCciSource()
{
	string result = "";

	if (ttc_descr_ == NULL) {
		try {
			ttc_descr_ = getApplicationContext()->getDefaultZone()
					->getApplicationDescriptor("TTCciControl", 0);
		} catch (xdaq::exception::ApplicationDescriptorNotFound e) {
			return result; // Do nothing if the target doesn't exist
		}

		string names[4], types[4];
		names[0] = "ClockSource";
		names[1] = "OrbitSource";
		names[2] = "TriggerSource";
		names[3] = "BGOSource";
		for (int i = 0; i < 4; ++i) {
			types[i] = "xsd:string";
		}

		ttc_param_ = createParameterGetSOAP2(
				"TTCciControl", 4, names, types);
	}

	xoap::MessageReference reply;
	try {
		reply = getApplicationContext()->postSOAP(ttc_param_, ttc_descr_);

		result = extractParameter(reply, "ClockSource");
		result += ":" + extractParameter(reply, "OrbitSource");
		result += ":" + extractParameter(reply, "TriggerSource");
		result += ":" + extractParameter(reply, "BGOSource");
	} catch (xdaq::exception::Exception e) {
		result = "Unknown";
	}

	return result;
}

bool CSCSupervisor::isDAQConfiguredInGlobal()
{
	string result = "";

	if (daq_descr_ != NULL) {
		xoap::MessageReference reply;
		try {
			reply = getApplicationContext()->postSOAP(
					daq_configured_param_, daq_descr_);

			result = extractParameter(reply, "configuredInGlobalMode");
		} catch (xdaq::exception::Exception e) {
			result = "Unknown";
		}
	}

	return result == "true";
}

string CSCSupervisor::getLocalDAQState()
{
	string result = "";

	if (daq_descr_ != NULL) {
		xoap::MessageReference reply;
		try {
			reply = getApplicationContext()->postSOAP(
					daq_state_param_, daq_descr_);

			result = extractParameter(reply, "daqState");
		} catch (xdaq::exception::Exception e) {
			result = "Unknown";
		}
	}

	return result;
}

bool CSCSupervisor::isDAQManagerControlled(string command)
{
	if (getDAQMode() != "global") { return false; }

	if (command != "Configure" && !isDAQConfiguredInGlobal()) { return false; }

	return true;
}

void CSCSupervisor::StateTable::addApplication(CSCSupervisor *sv, string klass)
{
	sv_ = sv;

	// find applications
	std::set<xdaq::ApplicationDescriptor *> apps;
	try {
		apps = sv->getApplicationContext()->getDefaultZone()
				->getApplicationDescriptors(klass);
	} catch (xdaq::exception::ApplicationDescriptorNotFound e) {
		return; // Do nothing if the target doesn't exist
	}

	// add to the table
	std::set<xdaq::ApplicationDescriptor *>::iterator i = apps.begin();
	for (; i != apps.end(); ++i) {
		table_.push_back(
				pair<xdaq::ApplicationDescriptor *, string>(*i, "NULL"));
	}
}

void CSCSupervisor::StateTable::refresh()
{
	string klass = "";
	xoap::MessageReference message, reply;

	vector<pair<xdaq::ApplicationDescriptor *, string> >::iterator i =
			table_.begin();
	for (; i != table_.end(); ++i) {
		if (klass != i->first->getClassName()) {
			klass = i->first->getClassName();
			message = createStateSOAP(klass);
		}

		try {
			reply = sv_->getApplicationContext()->postSOAP(message, i->first);
			sv_->analyzeReply(message, reply, i->first);

			i->second = extractState(reply, klass);
		} catch (xdaq::exception::Exception e) {
			i->second = STATE_UNKNOWN;
		} catch (...) {
			LOG4CPLUS_DEBUG(sv_->getApplicationLogger(), "Exception with " << klass);
			i->second = STATE_UNKNOWN;
		}
	}
}

string CSCSupervisor::StateTable::getState(string klass, unsigned int instance)
{
	string state = "";

	vector<pair<xdaq::ApplicationDescriptor *, string> >::iterator i =
			table_.begin();
	for (; i != table_.end(); ++i) {
		if (klass == i->first->getClassName()
				&& instance == i->first->getInstance()) {
			state = i->second;
			break;
		}
	}

	return state;
}

void CSCSupervisor::StateTable::webOutput(xgi::Output *out, string sv_state)
		throw (xgi::exception::Exception)
{
	refresh();
	*out << table() << tbody() << endl;

	// My state
	*out << tr();
	*out << td() << "CSCSupervisor" << "(" << "0" << ")" << td();
	*out << td().set("class", sv_state) << sv_state << td();
	*out << tr() << endl;

	// Applications
	vector<pair<xdaq::ApplicationDescriptor *, string> >::iterator i =
			table_.begin();
	for (; i != table_.end(); ++i) {
		string klass = i->first->getClassName();
		int instance = i->first->getInstance();
		string state = i->second;

		*out << tr();
		*out << td() << klass << "(" << instance << ")" << td();
		*out << td().set("class", state) << state << td();
		*out << tr() << endl;
	}

	*out << tbody() << table() << endl;
}

xoap::MessageReference CSCSupervisor::StateTable::createStateSOAP(
		string klass)
{
	xoap::MessageReference message = xoap::createMessage();
	xoap::SOAPEnvelope envelope = message->getSOAPPart().getEnvelope();
	envelope.addNamespaceDeclaration("xsi", NS_XSI);

	xoap::SOAPName command = envelope.createName(
			"ParameterGet", "xdaq", XDAQ_NS_URI);
	xoap::SOAPName properties = envelope.createName(
			"properties", klass, "urn:xdaq-application:" + klass);
	xoap::SOAPName parameter = envelope.createName(
			"stateName", klass, "urn:xdaq-application:" + klass);
	xoap::SOAPName xsitype = envelope.createName("type", "xsi", NS_XSI);

	xoap::SOAPElement properties_e = envelope.getBody()
			.addBodyElement(command)
			.addChildElement(properties);
	properties_e.addAttribute(xsitype, "soapenc:Struct");

	xoap::SOAPElement parameter_e = properties_e.addChildElement(parameter);
	parameter_e.addAttribute(xsitype, "xsd:string");

	return message;
}

string CSCSupervisor::StateTable::extractState(xoap::MessageReference message, string klass)
{
	xoap::SOAPElement root = message->getSOAPPart()
			.getEnvelope().getBody().getChildElements(
			*(new xoap::SOAPName("ParameterGetResponse", "", "")))[0];
	xoap::SOAPElement properties = root.getChildElements(
			*(new xoap::SOAPName("properties", "", "")))[0];
	xoap::SOAPElement state = properties.getChildElements(
			*(new xoap::SOAPName("stateName", "", "")))[0];

	return state.getValue();
}

void CSCSupervisor::LastLog::size(unsigned int size)
{
	size_ = size;
}

unsigned int CSCSupervisor::LastLog::size() const
{
	return size_;
}

void CSCSupervisor::LastLog::add(string message)
{
	messages_.push_back(message);
	if (messages_.size() > size_) { messages_.pop_front(); }
}

void CSCSupervisor::LastLog::webOutput(xgi::Output *out)
		throw (xgi::exception::Exception)
{
	*out << "Last " << messages_.size() << " log messages:" << br() << endl;
	*out << textarea().set("cols", "120").set("rows", "20")
			.set("readonly").set("class", "log") << endl;

	deque<string>::iterator i = messages_.begin();
	for (; i != messages_.end(); ++i) {
		*out << *i << endl;
	}

	*out << textarea() << endl;
}

// End of file
// vim: set sw=4 ts=4:
