var TESTS_MAP = {
	"bindings": [
	{"testID": "ALL_DDU_IN_READOUT", "testPlot": "EMU_Test01_DDUs_in_Readout", "testScope": "EMU"},
	{"testID": "ALL_DDU_EVENTS_COUNTER", "testPlot": "EMU_Test01_DDUs_in_Readout", "testScope": "EMU"},
	{"testID": "ALL_HOT_DDU_IN_READOUT", "testPlot": "EMU_Test01_DDUs_in_Readout", "testScope": "EMU"},
	{"testID": "ALL_LOW_DDU_IN_READOUT", "testPlot": "EMU_Test01_DDUs_in_Readout", "testScope": "EMU"},
	{"testID": "ALL_NO_DDU_IN_READOUT", "testPlot": "EMU_Test01_DDUs_in_Readout", "testScope": "EMU"},
	{"testID": "ALL_DDU_WITH_TRAILER_ERRORS", "testPlot": "EMU_Test03_DDU_Reported_Errors", "testScope": "EMU"},
	{"testID": "ALL_DDU_WITH_FORMAT_ERRORS", "testPlot": "EMU_Test04_DDU_Format_Errors", "testScope": "EMU"},
	{"testID": "ALL_DDU_WITH_LIVE_INPUTS", "testPlot": "EMU_Test05_DDU_Inputs_Status", "testScope": "EMU"},
	{"testID": "ALL_DDU_WITH_DATA", "testPlot": "EMU_Test05_DDU_Inputs_Status", "testScope": "EMU"},
	{"testID": "ALL_DDU_INPUT_IN_ERROR_STATE", "testPlot": "EMU_Test06_DDU_Inputs_in_Error_State", "testScope": "EMU"},
	{"testID": "ALL_DDU_INPUT_IN_WARNING_STATE", "testPlot": "EMU_Test06_DDU_Inputs_in_Error_State", "testScope": "EMU"},
	{"testID": "ALL_HOT_CHAMBERS", "testPlot": "EMU_Test08_Reporting_Unpacked_CSCs", "testScope": "EMU"},
	{"testID": "ALL_LOW_EFF_CHAMBERS", "testPlot": "EMU_Test08_Reporting_Unpacked_CSCs", "testScope": "EMU"},
	{"testID": "ALL_CHAMBERS_WITHOUT_DATA", "testPlot": "EMU_Test08_Reporting_Unpacked_CSCs", "testScope": "EMU"},
	{"testID": "ALL_CHAMBERS_WITH_FORMAT_ERRORS", "testPlot": "EMU_Test10_CSC_Errors_Warnings_Fract", "testScope": "EMU"},
	{"testID": "ALL_CHAMBERS_WITH_INPUT_FIFO_FULL", "testPlot": "EMU_Test10_CSC_Errors_Warnings_Fract", "testScope": "EMU"},
	{"testID": "ALL_CHAMBERS_WITH_INPUT_TIMEOUT", "testPlot": "EMU_Test10_CSC_Errors_Warnings_Fract", "testScope": "EMU"},
	{"testID": "ALL_CHAMBERS_WITHOUT_ALCT", "testPlot": "EMU_Test11_CSC_wo_Data_Blocks", "testScope": "EMU"},
	{"testID": "ALL_CHAMBERS_WITHOUT_CLCT", "testPlot": "EMU_Test11_CSC_wo_Data_Blocks", "testScope": "EMU"},
	{"testID": "ALL_CHAMBERS_WITHOUT_CFEB", "testPlot": "EMU_Test11_CSC_wo_Data_Blocks", "testScope": "EMU"},
	{"testID": "ALL_CHAMBERS_WITH_L1A_OUT_OF_SYNC", "testPlot": "EMU_Test11_CSC_wo_Data_Blocks", "testScope": "EMU"},
	{"testID": "ALL_CHAMBERS_WITH_BWORDS", "testPlot": "EMU_Test11_CSC_wo_Data_Blocks", "testScope": "EMU"},
	{"testID": "ALL_CHAMBERS_ALCT_TIMING", "testPlot": "EMU_Test14_ALCT_Timing", "testScope": "EMU"},
	{"testID": "ALL_CHAMBERS_CLCT_TIMING", "testPlot": "EMU_Test15_CLCT_Timing", "testScope": "EMU"},

	{"testID": "DDU_WITH_TRAILER_ERRORS", "testPlot": "DDU_Error_Status_from_DDU_Trailer", "testScope": "DDU"},
	{"testID": "DDU_WITH_FORMAT_ERRORS", "testPlot": "EMU_Test04_DDU_Format_Errors", "testScope": "EMU"},
	{"testID": "DDU_NO_INPUT_DATA", "testPlot": "DDU_Connected_and_Active_Inputs", "testScope": "DDU"},
	{"testID": "DDU_INPUT_IN_ERROR_STATE", "testPlot": "DDU_State_of_CSCs", "testScope": "DDU"},
	{"testID": "DDU_INPUT_IN_WARNING_STATE", "testPlot": "DDU_State_of_CSCs", "testScope": "DDU"},

        {"testID": "CSC_NO_DATA", "testPlot": "EMU_Test08_Reporting_Unpacked_CSCs", "testScope": "EMU"},
	{"testID": "CSC_HOT_CHAMBER", "testPlot": "EMU_Test08_Reporting_Unpacked_CSCs", "testScope": "EMU"},
	{"testID": "CSC_LOW_EFF_CHAMBER", "testPlot": "EMU_Test08_Reporting_Unpacked_CSCs", "testScope": "EMU"},
	{"testID": "CSC_WITH_FORMAT_ERRORS", "testPlot": "DMB/DMB_Test01_Binary_Examiner_Report", "testScope": "CSC"},
	{"testID": "CSC_WITH_INPUT_FIFO_FULL", "testPlot": "DMB/DMB_Test01_Binary_Examiner_Report", "testScope": "CSC"},
	{"testID": "CSC_WITH_BWORDS", "testPlot": "DMB/DMB_Test01_Binary_Examiner_Report", "testScope": "CSC"},
	{"testID": "CSC_WITH_INPUT_TIMEOUT", "testPlot": "DMB/DMB_Test01_Binary_Examiner_Report", "testScope": "CSC"},
	{"testID": "CSC_WITHOUT_ALCT", "testPlot": "DMB/DMB_Test02_ALCT_TMB_CFEB_Data_Blocks", "testScope": "CSC"},
	{"testID": "CSC_WITHOUT_CLCT", "testPlot": "DMB/DMB_Test02_ALCT_TMB_CFEB_Data_Blocks", "testScope": "CSC"},
	{"testID": "CSC_WITHOUT_CFEB", "testPlot": "DMB/DMB_Test02_ALCT_TMB_CFEB_Data_Blocks", "testScope": "CSC"},
	{"testID": "CSC_WITH_L1A_OUT_OF_SYNC", "testPlot": "SYNC/SYNC_Test01_L1A_Differences", "testScope": "CSC"},
	{"testID": "CSC_WITH_LOW_CFEB_DAV_EFF", "testPlot": "DMB/DMB_Test03_Individual_CFEB_Data_Blocks", "testScope": "CSC"},
	{"testID": "CSC_CFEB_NO_SCA_DATA", "testPlot": "CFEB/CFEB_SCA_Active_Strips_Occupancy", "testScope": "CSC"},
	{"testID": "CSC_CFEB_SCA_LOW_EFF", "testPlot": "CFEB/CFEB_SCA_Active_Strips_Occupancy", "testScope": "CSC"},
	{"testID": "CSC_CFEB_SCA_NOISY", "testPlot": "CFEB/CFEB_SCA_Active_Strips_Occupancy", "testScope": "CSC"},
	{"testID": "CSC_CFEB_SCA_NOISY_CHANNEL", "testPlot": "CFEB/CFEB_SCA_Active_Strips_Occupancy", "testScope": "CSC"},
	{"testID": "CSC_CFEB_SCA_DEAD_CHANNEL", "testPlot": "CFEB/CFEB_SCA_Active_Strips_Occupancy", "testScope": "CSC"},
	{"testID": "CSC_CFEB_NO_COMPARATORS_DATA", "testPlot": "TMB/TMB-CLCT_Cathode_Comparator_Hit_Occupancy_per_Half_Strip", "testScope": "CSC"},
	{"testID": "CSC_CFEB_COMPARATORS_LOW_EFF", "testPlot": "TMB/TMB-CLCT_Cathode_Comparator_Hit_Occupancy_per_Half_Strip", "testScope": "CSC"},
	{"testID": "CSC_CFEB_COMPARATORS_NOISY", "testPlot": "TMB/TMB-CLCT_Cathode_Comparator_Hit_Occupancy_per_Half_Strip", "testScope": "CSC"},
	{"testID": "CSC_CFEB_COMPARATORS_NOISY_CHANNEL", "testPlot": "TMB/TMB-CLCT_Cathode_Comparator_Hit_Occupancy_per_Half_Strip", "testScope": "CSC"},
	{"testID": "CSC_ALCT_NO_ANODE_DATA", "testPlot": "ALCT/ALCT_Anode_Hit_Occupancy_per_Wire_Group", "testScope": "CSC"},
	{"testID": "CSC_ALCT_AFEB_NOISY", "testPlot": "ALCT/ALCT_Anode_Hit_Occupancy_per_Wire_Group", "testScope": "CSC"},
	{"testID": "CSC_NO_HV_SEGMENT", "testPlot": "ALCT/ALCT_Anode_Hit_Occupancy_per_Wire_Group", "testScope": "CSC"},
        {"testID": "CSC_ALCT_TIMING", "testPlot": "ALCT/ALCT_ALCT0_BXN_and_ALCT_L1A_BXN_Synchronization", "testScope": "CSC"},
	{"testID": "CSC_CLCT_TIMING", "testPlot": "TMB/TMB-CLCT_CLCT0_BXN_and_TMB_L1A_BXN_Synchronization", "testScope": "CSC"},
	{"testID": "CSC_LOWERED_HV_SEGMENT", "testPlot": "CFEB/CFEB_SCA_Active_Strips_Occupancy","testScope": "CSC"}

	]
} 
