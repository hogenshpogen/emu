#ifndef _Test_CFEB04_h_
#define _Test_CFEB04_h_


#include <iostream>
#include <iomanip>
#include <math.h>
#include <map>
#include <sys/stat.h>

#include "emu/dqm/calibration/Test_Generic.h"

#include <TROOT.h>
#include <TSystem.h>
#include <TProfile.h>
#include <TFile.h>

#define MAX_VALID_SLOPE 12.
#define MIN_VALID_SLOPE 4.

class Test_CFEB04: public Test_Generic
{
public:
  Test_CFEB04(std::string datafile);
  void analyze(const char * data, int32_t dataSize, uint32_t errorStat, int32_t nodeNumber = 0);
//	void finish();

protected:

  void initCSC(std::string cscID);
  void analyzeCSC(const CSCEventData& data);
  void finishCSC(std::string cscID);
  bool checkResults(std::string cscID);
  double fivePoleFitTime( int tmax,  double* adc, double t_peak);

  std::map<std::string, uint32_t> l1a_cntrs;
  std::map<std::string, test_step> test_steps;
  std::map<int, int> dduL1A;
  int currL1A;
  int startL1A;
  int dduID;
  int ltc_bug;
  std::map<int, ddu_stats> DDUstats;
  std::map<int, std::map<std::string, test_step> > htree;
  bool fSwitch;
  cscGainData gdata;
  cscTPeakData tpdata;	// Pulse Peaking Time 

};

#endif
