#ifndef TMBTester_h
#define TMBTester_h

#include<iostream>
#include<fstream>
#include<stdio.h>
#include <string>
#include "TMB.h"
#include "CCB.h"
#include "RAT.h"
#include "TMB_JTAG_constants.h"

class TMBTester {
 public:
  // not responsible for deleting pointers
  TMBTester();
  //
  virtual ~TMBTester();
  //
  inline void RedirectOutput(std::ostream * Output) { MyOutput_ = Output ; }
  inline void setTMB(TMB * tmb) {tmb_ = tmb;}
  inline void setCCB(CCB * ccb) {ccb_ = ccb;}
  inline void setRAT(RAT * rat) {rat_ = rat;}
  //
  inline int GetResultTestBootRegister() { return ResultTestBootRegister_ ; }
  inline int GetResultTestVMEfpgaDataRegister() { return ResultTestVMEfpgaDataRegister_ ; }
  inline int GetResultTestFirmwareDate() { return ResultTestFirmwareDate_ ; }
  inline int GetResultTestFirmwareType() { return ResultTestFirmwareType_ ; }
  inline int GetResultTestFirmwareVersion() { return ResultTestFirmwareVersion_ ; }
  inline int GetResultTestFirmwareRevCode() { return ResultTestFirmwareRevCode_ ; }
  inline int GetResultTestMezzId() { return ResultTestMezzId_ ; }
  inline int GetResultTestPromId() { return ResultTestPromId_ ; }
  inline int GetResultTestPROMPath() { return ResultTestPROMPath_ ; }
  inline int GetResultTestDSN() { return ResultTestDSN_ ; }
  inline int GetResultTestADC() { return ResultTestADC_ ; }
  inline int GetResultTest3d3444() { return ResultTest3d3444_ ; }
  inline int GetResultTestALCTtxrx() { return ResultTestALCTtxrx_ ; }
  inline int GetResultTestRATtemper() { return ResultTestRATtemper_ ; }
  inline int GetResultTestRATidCodes() { return ResultTestRATidCodes_ ; }
  inline int GetResultTestRATuserCodes() { return ResultTestRATuserCodes_ ; }
  //
  void reset();
  //
  bool runAllTests();
  bool testFirmwareDate();
  bool testFirmwareType();
  bool testFirmwareVersion();
  bool testFirmwareSlot();
  bool testFirmwareRevCode();
  bool testBootRegister();
  bool testHardReset();
  bool testVMEfpgaDataRegister();
  bool testJTAGchain();    // check user && boot
  bool testJTAGchain(int); // user=0, boot=1
  bool testMezzId();
  bool testPROMid();
  bool testPROMpath();
  bool testDSN(); // check TMB && mezzanine && RAT
  bool testDSN(int); // TMB=0, mezzanine=1, RAT=2
  bool testADC();
  bool test3d3444();
  bool testALCTtxrx();      
  bool testRATtemper();      
  bool testRATidCodes();
  bool testRATuserCodes();
  //
  bool compareValues(std::string, int, int, bool);
  bool compareValues(std::string, float, float, float);
  void messageOK(std::string,bool);
  //
  // Maybe should be in CrateTiming.cpp?
  void RatTmbDelayScan();
  void RpcRatDelayScan(int);
  //
  //Not yet working but should eventually be in TMB.cc...
  float tmb_temp(int, int);
  //
  void bit_to_array(int,int *,const int);
  //
 protected:
  //
 private: 
  std::ostream * MyOutput_ ;
  TMB * tmb_;
  CCB * ccb_;
  RAT * rat_;
  //
  int ResultTestBootRegister_ ;
  int ResultTestVMEfpgaDataRegister_ ;
  int ResultTestFirmwareDate_;
  int ResultTestFirmwareType_;
  int ResultTestFirmwareVersion_;
  int ResultTestFirmwareSlot_;
  int ResultTestFirmwareRevCode_;
  int ResultTestMezzId_;
  int ResultTestPromId_;
  int ResultTestPROMPath_;
  int ResultTestDSN_;
  int ResultTestADC_;
  int ResultTest3d3444_;
  int ResultTestALCTtxrx_;
  int ResultTestRATtemper_;
  int ResultTestRATidCodes_;
  int ResultTestRATuserCodes_;
  //
  //functions needed by above tests:
  int dowCRC(std::bitset<64>);
};

#endif

