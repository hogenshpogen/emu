//
// EmuDisplayClientV.cc

#include "emu/dqm/cscdisplay/version.h"

#include "toolbox/version.h"
#include "xoap/version.h"
#include "xdaq/version.h"

GETPACKAGEINFO(emudqmcscdisplay)

void emudqmcscdisplay::checkPackageDependencies()
throw (config::PackageInfo::VersionException)
{
  CHECKDEPENDENCY(toolbox)
  CHECKDEPENDENCY(xoap)
  CHECKDEPENDENCY(xdaq)
}

std::set<std::string, std::less<std::string> > xdaq::getPackageDependencies()
{
  std::set<std::string, std::less<std::string> > dependencies;

  ADDDEPENDENCY(dependencies, toolbox);
  ADDDEPENDENCY(dependencies, xoap);
  ADDDEPENDENCY(dependencies, xdaq);

  return dependencies;
}

