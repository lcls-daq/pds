#include "LLNLv1.hh"
#include "Error.hh"
#include "Logger.hh"

static std::map<std::string, uint16_t> REG_NAMES = {
  {"FPGA_NUM", 0x000},
  {"FPGA_REV", 0x001},
  {"HS_TIMING_CTL", 0x010},
  {"HS_TIMING_DATA_ALO", 0x013},
  {"HS_TIMING_DATA_AHI", 0x014},
  {"HS_TIMING_DATA_BLO", 0x015},
  {"HS_TIMING_DATA_BHI", 0x016},
  {"SW_TRIGGER_CONTROL", 0x017},
  {"STAT_REG", 0x024},
  {"CTRL_REG", 0x025},
  {"POT_CTL", 0x026},
  {"POT_REG4_TO_1", 0x027},
  {"POT_REG8_TO_5", 0x028},
  {"POT_REG12_TO_9", 0x029},
  {"POT_REG13", 0x02A},
  {"LED_GP", 0x02B},
  {"SW_RESET", 0x02D},
  {"HST_SETTINGS", 0x02E},
  {"STAT_REG_SRC", 0x02F},
  {"STAT_REG2", 0x030},
  {"STAT_REG2_SRC", 0x031},
  {"ADC_BYTECOUNTER", 0x032},
  {"RBP_PIXEL_CNTR", 0x033},
  {"DIAG_MAX_CNT_0", 0x034},
  {"DIAG_MAX_CNT_1", 0x035},
  {"DIAG_CNTR_VAL_0", 0x036},
  {"DIAG_CNTR_VAL_1", 0x037},
  {"TRIGGER_CTL", 0x03A},
  {"SRAM_CTL", 0x03B},
  {"TIMER_CTL", 0x03C},
  {"TIMER_VALUE", 0x03D},
  {"HSTALLWEN_WAIT_TIME", 0x03F},
  {"FPA_ROW_INITIAL", 0x042},
  {"FPA_ROW_FINAL", 0x043},
  {"FPA_FRAME_INITIAL", 0x044},
  {"FPA_FRAME_FINAL", 0x045},
  {"FPA_DIVCLK_EN_ADDR", 0x046},
  {"FPA_OSCILLATOR_SEL_ADDR", 0x047},
  {"FRAME_ORDER_SEL", 0x04B},
  {"SENSOR_VOLT_STAT", 0x082},
  {"SENSOR_VOLT_CTL", 0x083},
  {"ADC_CTL", 0x090},
  {"ADC1_CONFIG_DATA", 0x091},
  {"ADC2_CONFIG_DATA", 0x092},
  {"ADC3_CONFIG_DATA", 0x093},
  {"ADC4_CONFIG_DATA", 0x094},
  {"ADC5_CONFIG_DATA", 0x095},
  {"ADC5_DATA_1", 0x096},
  {"ADC5_DATA_2", 0x097},
  {"ADC5_DATA_3", 0x098},
  {"ADC5_DATA_4", 0x099},
  {"ADC5_PPER", 0x09A},
  {"ADC_STANDBY", 0x09B},
  {"ADC_RESET", 0x09B},
  {"TEMP_SENSE_PPER", 0x0A0},
  {"TEMP_SENSE_DATA", 0x0A1},
};

static std::map<std::string, Pds::NsCam::SubRegister> SUBREGNAMES = {
  {"HST_MODE", {"HS_TIMING_CTL", 0, 1, true}},
  {"SW_TRIG_START", {"SW_TRIGGER_CONTROL", 0, 1, true}},
  {"LED_EN", {"CTRL_REG", 1, 1, true}},
  {"COLQUENCHEN", {"CTRL_REG", 2, 1, true}},
  {"POWERSAVE", {"CTRL_REG", 3, 1, true}},
  {"POT1", {"POT_REG4_TO_1", 0, 8, true, 0.0, 3.3}},
  {"POT2", {"POT_REG4_TO_1", 8, 8, true, 0.0, 3.3}},
  {"POT3", {"POT_REG4_TO_1", 16, 8, true, 0.0, 3.3}},
  {"POT4", {"POT_REG4_TO_1", 24, 8, true, 0.0, 3.3}},
  {"POT5", {"POT_REG8_TO_5", 0, 8, true, 0.0, 3.3}},
  {"POT6", {"POT_REG8_TO_5", 8, 8, true, 0.0, 3.3}},
  {"POT7", {"POT_REG8_TO_5", 16, 8, true, 0.0, 3.3}},
  {"POT8", {"POT_REG8_TO_5", 24, 8, true, 0.0, 3.3}},
  {"POT9", {"POT_REG12_TO_9", 0, 8, true, 0.0, 3.3}},
  {"POT10", {"POT_REG12_TO_9", 8, 8, true, 0.0, 3.3}},
  {"POT11", {"POT_REG12_TO_9", 16, 8, true, 0.0, 3.3}},
  {"POT12", {"POT_REG12_TO_9", 24, 8, true, 0.0, 3.3}},
  {"POT13", {"POT_REG13", 0, 8, true, 0.0, 3.96}},
  {"LED1", {"LED_GP", 0, 1, true}},
  {"LED2", {"LED_GP", 1, 1, true}},
  {"LED3", {"LED_GP", 2, 1, true}},
  {"LED4", {"LED_GP", 3, 1, true}},
  {"LED5", {"LED_GP", 4, 1, true}},
  {"LED6", {"LED_GP", 5, 1, true}},
  {"LED7", {"LED_GP", 6, 1, true}},
  {"LED8", {"LED_GP", 7, 1, true}},
  {"RESET", {"SW_RESET", 0, 1, true}},
  {"HST_SW_CTL_EN", {"HST_SETTINGS", 0, 1, true}},
  {"SW_HSTALLWEN", {"HST_SETTINGS", 1, 1, true}},
  {"MAXERR_FIT", {"DIAG_MAX_CNT_0", 16, 16, true}},
  {"MAXERR_SRT", {"DIAG_MAX_CNT_0", 0, 8, true}},
  {"MAXERR_UTTR", {"DIAG_MAX_CNT_1", 16, 16, true}},
  {"MAXERR_URTR", {"DIAG_MAX_CNT_1", 0, 16, true}},
  {"HW_TRIG_EN", {"TRIGGER_CTL", 0, 1, true}},
  {"SW_TRIG_EN", {"TRIGGER_CTL", 2, 1, true}},
  {"READ_SRAM", {"SRAM_CTL", 0, 1, true}},
  {"RESET_TIMER", {"TIMER_CTL", 0, 1, true}},
  {"OSC_SELECT", {"FPA_OSCILLATOR_SEL_ADDR", 0, 2, true}},
  {"ADC5_VREF", {"ADC5_CONFIG_DATA", 0, 10, true}},
  {"ADC5_VREF3", {"ADC5_CONFIG_DATA", 13, 1, true}},
  {"ADC5_INT", {"ADC5_CONFIG_DATA", 15, 1, true}},
  {"ADC5_MULT", {"ADC5_CONFIG_DATA", 19, 6, true}},
  {"PPER", {"ADC5_PPER", 0, 8, true}},
  {"SRAM_READY", {"STAT_REG", 0, 1, false}},
  {"STAT_COARSE", {"STAT_REG", 1, 1, false}},
  {"STAT_FINE", {"STAT_REG", 2, 1, false}},
  {"STAT_SENSREADIP", {"STAT_REG", 5, 1, false}},
  {"STAT_SENSREADDONE", {"STAT_REG", 6, 1, false}},
  {"STAT_SRAMREADSTART", {"STAT_REG", 7, 1, false}},
  {"STAT_SRAMREADDONE", {"STAT_REG", 8, 1, false}},
  {"STAT_HSTCONFIGSTART", {"STAT_REG", 9, 1, false}},
  {"STAT_ADCSCONFIGURED", {"STAT_REG", 10, 1, false}},
  {"STAT_POTSCONFIGURED", {"STAT_REG", 11, 1, false}},
  {"STAT_TIMERCOUNTERRESET", {"STAT_REG", 13, 1, false}},
  {"STAT_ARMED", {"STAT_REG", 14, 1, false}},
  {"STAT_TEMP", {"STAT_REG", 17, 11, false}},
  {"STAT_PRESS", {"STAT_REG", 28, 4, false}},
  {"FPA_IF_TO", {"STAT_REG2", 0, 1, false}},
  {"SRAM_RO_TO", {"STAT_REG2", 1, 1, false}},
  {"PIXELRD_TOUT_ERR", {"STAT_REG2", 2, 1, false}},
  {"UART_TX_TO_RST", {"STAT_REG2", 3, 1, false}},
  {"UART_RX_TO_RST", {"STAT_REG2", 4, 1, false}},
  {"SENSOR_POSN", {"SENSOR_VOLT_STAT", 0, 1, false}},
  {"SENSOR_NEGP", {"SENSOR_VOLT_STAT", 1, 1, false}},
  {"ICARUS_DET", {"SENSOR_VOLT_STAT", 2, 1, false}},
  {"DAEDALUS_DET", {"SENSOR_VOLT_STAT", 3, 1, false}},
  {"HORUS_DET", {"SENSOR_VOLT_STAT", 4, 1, false}},
  {"SENSOR_POWER", {"SENSOR_VOLT_STAT", 5, 1, false}},
  {"FIT_COUNT", {"DIAG_CNTR_VAL_0", 16, 16, false}},
  {"SRT_COUNT", {"DIAG_CNTR_VAL_0", 0, 8, false}},
  {"UTTR_COUNT", {"DIAG_CNTR_VAL_1", 16, 16, false}},
  {"URTR_COUNT", {"DIAG_CNTR_VAL_1", 0, 16, false}},
  {"MON_CH2", {"ADC5_DATA_1", 0, 16, false}},
  {"MON_CH3", {"ADC5_DATA_1", 16, 16, false}},
  {"MON_CH4", {"ADC5_DATA_2", 0, 16, false}},
  {"MON_CH5", {"ADC5_DATA_2", 16, 16, false}},
  {"MON_CH6", {"ADC5_DATA_3", 0, 16, false}},
  {"MON_CH7", {"ADC5_DATA_3", 16, 16, false}},
  {"MON_CH8", {"ADC5_DATA_4", 0, 16, false}},
  {"MON_VRST", {"ADC5_DATA_4", 16, 16, false}},
};

static std::map<std::string, uint32_t> DEFAULTS = {
};

static std::map<std::string, std::string> ICARUS_ALIASES = {
  {"COL_BOT_IBIAS_IN", "POT1"},
  {"HST_A_PDELAY", "POT2"},
  {"HST_B_NDELAY", "POT3"},
  {"HST_RO_IBIAS", "POT4"},
  {"HST_OSC_VREF_IN", "POT5"},
  {"HST_B_PDELAY", "POT6"},
  {"HST_OSC_CTL", "POT7"},
  {"HST_A_NDELAY", "POT8"},
  {"COL_TOP_IBIAS_IN", "POT9"},
  {"HST_OSC_R_BIAS", "POT10"},
  {"VAB", "POT11"},
  {"HST_RO_NC_IBIAS", "POT12"},
  {"VRST", "POT13"},
  {"MON_HST_A_PDELAY", "MON_CH2"},
  {"MON_HST_B_NDELAY", "MON_CH3"},
  {"MON_HST_RO_IBIAS", "MON_CH4"},
  {"MON_HST_OSC_VREF_IN", "MON_CH5"},
  {"MON_HST_B_PDELAY", "MON_CH6"},
  {"MON_HST_OSC_CTL", "MON_CH7"},
  {"MON_HST_A_NDELAY", "MON_CH8"},
};

static std::map<std::string, std::string> DAEDALUS_ALIASES = {
  {"HST_OSC_CTL", "POT4"},
  {"HST_RO_NC_IBIAS", "POT5"},
  {"HST_OSC_VREF_IN", "POT6"},
  {"VAB", "POT11"},
  {"MON_TSENSEOUT", "MON_CH2"},
  {"MON_BGREF", "MON_CH3"},
  {"MON_HST_OSC_CTL", "MON_CH4"},
  {"MON_HST_RO_NC_IBIAS", "MON_CH5"},
  {"MON_HST_OSC_VREF_IN", "MON_CH6"},
  {"MON_COL_TST_IN", "MON_CH7"},
  {"MON_HST_OSC_PBIAS_PAD", "MON_CH8"},
};

static std::map<std::string, std::string> ICARUS_MONITORS = {
  {"POT2", "MON_CH2"},
  {"POT3", "MON_CH3"},
  {"POT4", "MON_CH4"},
  {"POT5", "MON_CH5"},
  {"POT6", "MON_CH6"},
  {"POT7", "MON_CH7"},
  {"POT8", "MON_CH8"},
  {"POT13", "MON_VRST"},
};

static std::map<std::string, std::string> DAEDALUS_MONITORS = {
  {"POT4", "MON_CH4"},
  {"POT5", "MON_CH5"},
  {"POT6", "MON_CH6"},
  {"POT13", "MON_VRST"},
};

static std::map<Pds::NsCam::SensorType, std::map<std::string, uint16_t>> SENSOR_REG_NAMES = {
  {Pds::NsCam::SensorType::ICARUS,
    {
      {"VRESET_WAIT_TIME", 0x03E},
      {"ICARUS_VER_SEL", 0x041},
      {"VRESET_HIGH_VALUE", 0x04A},
      {"MISC_SENSOR_CTL", 0x04C},
      {"MANUAL_SHUTTERS_MODE", 0x050},
      {"W0_INTEGRATION", 0x051},
      {"W0_INTERFRAME", 0x052},
      {"W1_INTEGRATION", 0x053},
      {"W1_INTERFRAME", 0x054},
      {"W2_INTEGRATION", 0x055},
      {"W2_INTERFRAME", 0x056},
      {"W3_INTEGRATION", 0x057},
      {"W0_INTEGRATION_B", 0x058},
      {"W0_INTERFRAME_B", 0x059},
      {"W1_INTEGRATION_B", 0x05A},
      {"W1_INTERFRAME_B", 0x05B},
      {"W2_INTEGRATION_B", 0x05C},
      {"W2_INTERFRAME_B", 0x05D},
      {"W3_INTEGRATION_B", 0x05E},
      {"TIME_ROW_DCD", 0x05F},                     
    }
  },
  {Pds::NsCam::SensorType::ICARUS2,
    {
      {"VRESET_WAIT_TIME", 0x03E},
      {"ICARUS_VER_SEL", 0x041},
      {"MISC_SENSOR_CTL", 0x04C},
      {"MANUAL_SHUTTERS_MODE", 0x050},
      {"W0_INTEGRATION", 0x051},
      {"W0_INTERFRAME", 0x052},
      {"W1_INTEGRATION", 0x053},
      {"W1_INTERFRAME", 0x054},
      {"W2_INTEGRATION", 0x055},
      {"W2_INTERFRAME", 0x056},
      {"W3_INTEGRATION", 0x057},
      {"W0_INTEGRATION_B", 0x058},
      {"W0_INTERFRAME_B", 0x059},
      {"W1_INTEGRATION_B", 0x05A},
      {"W1_INTERFRAME_B", 0x05B},
      {"W2_INTEGRATION_B", 0x05C},
      {"W2_INTERFRAME_B", 0x05D},
      {"W3_INTEGRATION_B", 0x05E},
      {"TIME_ROW_DCD", 0x05F},
    }
  },
  {Pds::NsCam::SensorType::DAEDALUS,
    {
      {"HST_READBACK_A_LO", 0x018},
      {"HST_READBACK_A_HI", 0x019},
      {"HST_READBACK_B_LO", 0x01A},
      {"HST_READBACK_B_HI", 0x01B},
      {"HSTALLWEN_WAIT_TIME", 0x03F},
      {"VRESET_HIGH_VALUE", 0x04A},
      {"FRAME_ORDER_SEL", 0x04B},
      {"EXT_PHI_CLK_SH0_ON", 0x050},
      {"EXT_PHI_CLK_SH0_OFF", 0x051},
      {"EXT_PHI_CLK_SH1_ON", 0x052},
      {"EXT_PHI_CLK_SH1_OFF", 0x053},
      {"EXT_PHI_CLK_SH2_ON", 0x054},
      {"HST_TRIGGER_DELAY_DATA_LO", 0x120},
      {"HST_TRIGGER_DELAY_DATA_HI", 0x121},
      {"HST_PHI_DELAY_DATA", 0x122},
      {"HST_EXT_CLK_HALF_PER", 0x129},
      {"HST_COUNT_TRIG", 0x130},
      {"HST_DELAY_EN", 0x131},
      {"RSL_HFW_MODE_EN", 0x133},
      {"RSL_ZDT_MODE_B_EN", 0x135},
      {"RSL_ZDT_MODE_A_EN", 0x136},
      {"BGTRIMA", 0x137},
      {"BGTRIMB", 0x138},
      {"COLUMN_TEST_EN", 0x139},
      {"RSL_CONFIG_DATA_B0", 0x140},
      {"RSL_CONFIG_DATA_B1", 0x141},
      {"RSL_CONFIG_DATA_B2", 0x142},
      {"RSL_CONFIG_DATA_B3", 0x143},
      {"RSL_CONFIG_DATA_B4", 0x144},
      {"RSL_CONFIG_DATA_B5", 0x145},
      {"RSL_CONFIG_DATA_B6", 0x146},
      {"RSL_CONFIG_DATA_B7", 0x147},
      {"RSL_CONFIG_DATA_B8", 0x148},
      {"RSL_CONFIG_DATA_B9", 0x149},
      {"RSL_CONFIG_DATA_B10", 0x14A},
      {"RSL_CONFIG_DATA_B11", 0x14B},
      {"RSL_CONFIG_DATA_B12", 0x14C},
      {"RSL_CONFIG_DATA_B13", 0x14D},
      {"RSL_CONFIG_DATA_B14", 0x14E},
      {"RSL_CONFIG_DATA_B15", 0x14F},
      {"RSL_CONFIG_DATA_B16", 0x150},
      {"RSL_CONFIG_DATA_B17", 0x151},
      {"RSL_CONFIG_DATA_B18", 0x152},
      {"RSL_CONFIG_DATA_B19", 0x153},
      {"RSL_CONFIG_DATA_B20", 0x154},
      {"RSL_CONFIG_DATA_B21", 0x155},
      {"RSL_CONFIG_DATA_B22", 0x156},
      {"RSL_CONFIG_DATA_B23", 0x157},
      {"RSL_CONFIG_DATA_B24", 0x158},
      {"RSL_CONFIG_DATA_B25", 0x159},
      {"RSL_CONFIG_DATA_B26", 0x15A},
      {"RSL_CONFIG_DATA_B27", 0x15B},
      {"RSL_CONFIG_DATA_B28", 0x15C},
      {"RSL_CONFIG_DATA_B29", 0x15D},
      {"RSL_CONFIG_DATA_B30", 0x15E},
      {"RSL_CONFIG_DATA_B31", 0x15F},
      {"RSL_CONFIG_DATA_A0", 0x160},
      {"RSL_CONFIG_DATA_A1", 0x161},
      {"RSL_CONFIG_DATA_A2", 0x162},
      {"RSL_CONFIG_DATA_A3", 0x163},
      {"RSL_CONFIG_DATA_A4", 0x164},
      {"RSL_CONFIG_DATA_A5", 0x165},
      {"RSL_CONFIG_DATA_A6", 0x166},
      {"RSL_CONFIG_DATA_A7", 0x167},
      {"RSL_CONFIG_DATA_A8", 0x168},
      {"RSL_CONFIG_DATA_A9", 0x169},
      {"RSL_CONFIG_DATA_A10", 0x16A},
      {"RSL_CONFIG_DATA_A11", 0x16B},
      {"RSL_CONFIG_DATA_A12", 0x16C},
      {"RSL_CONFIG_DATA_A13", 0x16D},
      {"RSL_CONFIG_DATA_A14", 0x16E},
      {"RSL_CONFIG_DATA_A15", 0x16F},
      {"RSL_CONFIG_DATA_A16", 0x170},
      {"RSL_CONFIG_DATA_A17", 0x171},
      {"RSL_CONFIG_DATA_A18", 0x172},
      {"RSL_CONFIG_DATA_A19", 0x173},
      {"RSL_CONFIG_DATA_A20", 0x174},
      {"RSL_CONFIG_DATA_A21", 0x175},
      {"RSL_CONFIG_DATA_A22", 0x176},
      {"RSL_CONFIG_DATA_A23", 0x177},
      {"RSL_CONFIG_DATA_A24", 0x178},
      {"RSL_CONFIG_DATA_A25", 0x179},
      {"RSL_CONFIG_DATA_A26", 0x17A},
      {"RSL_CONFIG_DATA_A27", 0x17B},
      {"RSL_CONFIG_DATA_A28", 0x17C},
      {"RSL_CONFIG_DATA_A29", 0x17D},
      {"RSL_CONFIG_DATA_A30", 0x17E},
      {"RSL_CONFIG_DATA_A31", 0x17F},
    }
  },
};

static std::map<Pds::NsCam::SensorType, std::map<std::string, Pds::NsCam::SubRegister>> SENSOR_SUBREG_NAMES = {
  {Pds::NsCam::SensorType::ICARUS,
    {
      {"MANSHUT_MODE", {"MANUAL_SHUTTERS_MODE", 0, 1, true}},
      {"REVREAD", {"CTRL_REG", 4, 1, true}},
      {"PDBIAS_LOW", {"CTRL_REG", 6, 1, true}},
      {"ROWDCD_CTL", {"CTRL_REG", 7, 1, true}},
      {"ACCUMULATION_CTL", {"MISC_SENSOR_CTL", 0, 1, true}},
      {"HST_TST_ANRST_EN", {"MISC_SENSOR_CTL", 1, 1, true}},
      {"HST_TST_BNRST_EN", {"MISC_SENSOR_CTL", 2, 1, true}},
      {"HST_TST_ANRST_IN", {"MISC_SENSOR_CTL", 3, 1, true}},
      {"HST_TST_BNRST_IN", {"MISC_SENSOR_CTL", 4, 1, true}},
      {"HST_PXL_RST_EN", {"MISC_SENSOR_CTL", 5, 1, true}},
      {"HST_CONT_MODE", {"MISC_SENSOR_CTL", 6, 1, true}},
      {"COL_DCD_EN", {"MISC_SENSOR_CTL", 7, 1, true}},
      {"COL_READOUT_EN", {"MISC_SENSOR_CTL", 8, 1, true}},
      {"READOFF_DELAY_EN", {"TRIGGER_CTL", 4, 1, true}},
      {"STAT_W3TOPAEDGE1", {"STAT_REG", 3, 1, false}},
      {"STAT_W3TOPBEDGE1", {"STAT_REG", 4, 1, false}},
      {"STAT_HST_ALL_W_EN_DETECTED", {"STAT_REG", 12, 1, false}},
      {"PDBIAS_UNREADY", {"STAT_REG2", 5, 1, false}},
      {"VRESET_HIGH", {"VRESET_HIGH_VALUE", 0, 8, true}},
    }
  },
  {Pds::NsCam::SensorType::ICARUS2,
    {
      {"MANSHUT_MODE", {"MANUAL_SHUTTERS_MODE", 0, 1, true}},
      {"REVREAD", {"CTRL_REG", 4, 1, true}},
      {"PDBIAS_LOW", {"CTRL_REG", 6, 1, true}},
      {"ROWDCD_CTL", {"CTRL_REG", 7, 1, true}},
      {"ACCUMULATION_CTL", {"MISC_SENSOR_CTL", 0, 1, true}},
      {"HST_TST_ANRST_EN", {"MISC_SENSOR_CTL", 1, 1, true}},
      {"HST_TST_BNRST_EN", {"MISC_SENSOR_CTL", 2, 1, true}},
      {"HST_TST_ANRST_IN", {"MISC_SENSOR_CTL", 3, 1, true}},
      {"HST_TST_BNRST_IN", {"MISC_SENSOR_CTL", 4, 1, true}},
      {"HST_PXL_RST_EN", {"MISC_SENSOR_CTL", 5, 1, true}},
      {"HST_CONT_MODE", {"MISC_SENSOR_CTL", 6, 1, true}},
      {"COL_DCD_EN", {"MISC_SENSOR_CTL", 7, 1, true}},
      {"COL_READOUT_EN", {"MISC_SENSOR_CTL", 8, 1, true}},
      {"STAT_W3TOPAEDGE1", {"STAT_REG", 3, 1, false}},
      {"STAT_W3TOPBEDGE1", {"STAT_REG", 4, 1, false}},
      {"STAT_HST_ALL_W_EN_DETECTED", {"STAT_REG", 12, 1, false}},
      {"PDBIAS_UNREADY", {"STAT_REG2", 5, 1, false}},
    }
  },
  {Pds::NsCam::SensorType::DAEDALUS,
    {
      {"HST_MODE", {"HS_TIMING_CTL", 0, 1, true}},
      {"SLOWREADOFF_1", {"CTRL_REG", 5, 1, true}},
      {"MANSHUT_MODE", {"CTRL_REG", 8, 1, true}},
      {"INTERLACING_EN", {"CTRL_REG", 9, 1, true}},
      {"HFW", {"RSL_HFW_MODE_EN", 0, 1, true}},
      {"ZDT_A", {"RSL_ZDT_MODE_A_EN", 0, 1, true}},
      {"ZDT_B", {"RSL_ZDT_MODE_B_EN", 0, 1, true}},
      {"HST_DEL_EN", {"HST_DELAY_EN", 0, 1, true}},
      {"PHI_DELAY_A", {"HST_PHI_DELAY_DATA", 0, 10, true}},
      {"PHI_DELAY_B", {"HST_PHI_DELAY_DATA", 20, 10, true}},
      {"VRESET_HIGH", {"VRESET_HIGH_VALUE", 0, 16, true}},
      {"STAT_SH0RISEUR", {"STAT_REG", 3, 1, false}},
      {"STAT_SH0FALLUR", {"STAT_REG", 4, 1, false}},
      {"STAT_RSLNALLWENA", {"STAT_REG", 12, 1, false}},
      {"STAT_RSLNALLWENB", {"STAT_REG", 15, 1, false}},
      //{"STAT_CONFIGHSTDONE", {"STAT_REG", 16, 1, false}},
    }
  },
};

static std::map<Pds::NsCam::SensorType, std::map<std::string, uint32_t>> SENSOR_DEFAULTS = {
  {Pds::NsCam::SensorType::ICARUS,
    {
      {"ICARUS_VER_SEL", 0x00000001},
      {"FPA_FRAME_INITIAL", 0x00000001},
      {"FPA_FRAME_FINAL", 0x00000002},
      {"FPA_ROW_INITIAL", 0x00000000},
      {"FPA_ROW_FINAL", 0x000003FF},
      {"VRESET_WAIT_TIME", 0x000927C0},
      {"HS_TIMING_DATA_BHI", 0x00000000},
      {"HS_TIMING_DATA_BLO", 0x00006666},
      {"HS_TIMING_DATA_AHI", 0x00000000},
      {"HS_TIMING_DATA_ALO", 0x00006666},
      {"VRESET_HIGH_VALUE", 0x000000D5},
    }
  },
  {Pds::NsCam::SensorType::ICARUS2,
    {
      {"ICARUS_VER_SEL", 0x00000000},
      {"FPA_FRAME_INITIAL", 0x00000000},
      {"FPA_FRAME_FINAL", 0x00000003},
      {"FPA_ROW_INITIAL", 0x00000000},
      {"FPA_ROW_FINAL", 0x000003FF},
      {"HS_TIMING_DATA_BHI", 0x00000000},
      {"HS_TIMING_DATA_BLO", 0x00006666},
      {"HS_TIMING_DATA_AHI", 0x00000000},
      {"HS_TIMING_DATA_ALO", 0x00006666},
    }
  },
  {Pds::NsCam::SensorType::DAEDALUS,
    {
      {"FPA_FRAME_INITIAL", 0x00000000},
      {"FPA_FRAME_FINAL", 0x00000002},
      {"FPA_ROW_INITIAL", 0x00000000},
      {"FPA_ROW_FINAL", 0x000003FF},
      {"HS_TIMING_DATA_ALO", 0x00006666}, // 0db6 = 2-1; 6666 = 2-2
      {"HS_TIMING_DATA_AHI", 0x00000000},
      {"HS_TIMING_DATA_BLO", 0x00006666},
      {"HS_TIMING_DATA_BHI", 0x00000000},
      {"FRAME_ORDER_SEL", 0x00000000},
      {"RSL_HFW_MODE_EN", 0x00000000},
      {"RSL_ZDT_MODE_B_EN", 0x00000000},
      {"RSL_ZDT_MODE_A_EN", 0x00000000},
      {"RSL_CONFIG_DATA_B0", 0x00000000},
      {"RSL_CONFIG_DATA_B1", 0x00000000},
      {"RSL_CONFIG_DATA_B2", 0x00000000},
      {"RSL_CONFIG_DATA_B3", 0x00000000},
      {"RSL_CONFIG_DATA_B4", 0x00000000},
      {"RSL_CONFIG_DATA_B5", 0x00000000},
      {"RSL_CONFIG_DATA_B6", 0x00000000},
      {"RSL_CONFIG_DATA_B7", 0x00000000},
      {"RSL_CONFIG_DATA_B8", 0x00000000},
      {"RSL_CONFIG_DATA_B9", 0x00000000},
      {"RSL_CONFIG_DATA_B10", 0x00000000},
      {"RSL_CONFIG_DATA_B11", 0x00000000},
      {"RSL_CONFIG_DATA_B12", 0x00000000},
      {"RSL_CONFIG_DATA_B13", 0x00000000},
      {"RSL_CONFIG_DATA_B14", 0x00000000},
      {"RSL_CONFIG_DATA_B15", 0x00000000},
      {"RSL_CONFIG_DATA_B16", 0x00000000},
      {"RSL_CONFIG_DATA_B17", 0x00000000},
      {"RSL_CONFIG_DATA_B18", 0x00000000},
      {"RSL_CONFIG_DATA_B19", 0x00000000},
      {"RSL_CONFIG_DATA_B20", 0x00000000},
      {"RSL_CONFIG_DATA_B21", 0x00000000},
      {"RSL_CONFIG_DATA_B22", 0x00000000},
      {"RSL_CONFIG_DATA_B23", 0x00000000},
      {"RSL_CONFIG_DATA_B24", 0x00000000},
      {"RSL_CONFIG_DATA_B25", 0x00000000},
      {"RSL_CONFIG_DATA_B26", 0x00000000},
      {"RSL_CONFIG_DATA_B27", 0x00000000},
      {"RSL_CONFIG_DATA_B28", 0x00000000},
      {"RSL_CONFIG_DATA_B29", 0x00000000},
      {"RSL_CONFIG_DATA_B30", 0x00000000},
      {"RSL_CONFIG_DATA_B31", 0x00000000},
      {"RSL_CONFIG_DATA_A0", 0x00000000},
      {"RSL_CONFIG_DATA_A1", 0x00000000},
      {"RSL_CONFIG_DATA_A2", 0x00000000},
      {"RSL_CONFIG_DATA_A3", 0x00000000},
      {"RSL_CONFIG_DATA_A4", 0x00000000},
      {"RSL_CONFIG_DATA_A5", 0x00000000},
      {"RSL_CONFIG_DATA_A6", 0x00000000},
      {"RSL_CONFIG_DATA_A7", 0x00000000},
      {"RSL_CONFIG_DATA_A8", 0x00000000},
      {"RSL_CONFIG_DATA_A9", 0x00000000},
      {"RSL_CONFIG_DATA_A10", 0x00000000},
      {"RSL_CONFIG_DATA_A11", 0x00000000},
      {"RSL_CONFIG_DATA_A12", 0x00000000},
      {"RSL_CONFIG_DATA_A13", 0x00000000},
      {"RSL_CONFIG_DATA_A14", 0x00000000},
      {"RSL_CONFIG_DATA_A15", 0x00000000},
      {"RSL_CONFIG_DATA_A16", 0x00000000},
      {"RSL_CONFIG_DATA_A17", 0x00000000},
      {"RSL_CONFIG_DATA_A18", 0x00000000},
      {"RSL_CONFIG_DATA_A19", 0x00000000},
      {"RSL_CONFIG_DATA_A20", 0x00000000},
      {"RSL_CONFIG_DATA_A21", 0x00000000},
      {"RSL_CONFIG_DATA_A22", 0x00000000},
      {"RSL_CONFIG_DATA_A23", 0x00000000},
      {"RSL_CONFIG_DATA_A24", 0x00000000},
      {"RSL_CONFIG_DATA_A25", 0x00000000},
      {"RSL_CONFIG_DATA_A26", 0x00000000},
      {"RSL_CONFIG_DATA_A27", 0x00000000},
      {"RSL_CONFIG_DATA_A28", 0x00000000},
      {"RSL_CONFIG_DATA_A29", 0x00000000},
      {"RSL_CONFIG_DATA_A30", 0x00000000},
      {"RSL_CONFIG_DATA_A31", 0x00000000},
      {"HST_TRIGGER_DELAY_DATA_LO", 0x00000000},
      {"HST_TRIGGER_DELAY_DATA_HI", 0x00000000},
      {"HST_PHI_DELAY_DATA", 0x00000000},
      {"SLOWREADOFF_0", 0x00000000},
      {"SLOWREADOFF_1", 0x00000000},
    }
  },
};

using namespace Pds::NsCam;

LLNLv1::LLNLv1(SensorType stype, std::shared_ptr<Comm> comm) :
  Board(BoardType::LLNL_V1,
        stype,
        comm,
        REG_NAMES,
        SUBREGNAMES,
        DEFAULTS,
        stype == SensorType::DAEDALUS ? DAEDALUS_ALIASES : ICARUS_ALIASES,
        stype == SensorType::DAEDALUS ? DAEDALUS_MONITORS : ICARUS_MONITORS)
{
  // Add sensor specific register addresses
  auto reg_it = SENSOR_REG_NAMES.find(stype);
  if (reg_it != SENSOR_REG_NAMES.end()) {
    for (const auto& kv : reg_it->second) {
      addRegister(kv.first, kv.second);
    }
  }

  // Add sensor specific subregister addresses
  auto sub_it = SENSOR_SUBREG_NAMES.find(stype);
  if (sub_it != SENSOR_SUBREG_NAMES.end()) {
    for (const auto& kv : sub_it->second) {
      addSubRegister(kv.first, kv.second);
    }
  }

  // Add sensor specific defaults
  auto def_it = SENSOR_DEFAULTS.find(stype);
  if (def_it != SENSOR_DEFAULTS.end()) {
    for (const auto& kv : def_it->second) {
      addDefault(kv.first, kv.second);
    }
  }
}

void LLNLv1::softReboot()
{
  LOG_INFO(__func__);
  setSubRegister("RESET", 1);
}

void LLNLv1::initBoard()
{
  LOG_INFO(__func__);
  clearStatus();
  configADCs();

  // initialize vref
  double vrefmax = getSubRegister("ADC5_VREF3") ? 3.0 : 2.5;
  vref_ = vrefmax * getSubRegister("ADC5_VREF") / 1024.;

  // adc is bipolar
  adc5_bipolar_ = true;

  // set the adc multiplier
  uint32_t maskbits = 0x35; // aka 0b110101
  uint32_t multmask = getSubRegister("ADC5_MULT") & maskbits;
  if (multmask == maskbits) {
    adc5_mult_ = 2;
  } else if (multmask == 0) {
    adc5_mult_ = 4;
  } else {
    LOG_EXCEPTION(DeviceError(name(), "inconsistent mode settings on ADC5"));
  }

  setSubRegister("LED_EN", 1);
}

void LLNLv1::configADCs()
{
  LOG_INFO(__func__);
  // pull all adcs out of reset
  setRegister("ADC_RESET", 0x00000000);
  // workaround for uncertain behavior after previous readoff
  setRegister("ADC1_CONFIG_DATA", 0xFFFFFFFF);
  setRegister("ADC2_CONFIG_DATA", 0xFFFFFFFF);
  setRegister("ADC3_CONFIG_DATA", 0xFFFFFFFF);
  setRegister("ADC4_CONFIG_DATA", 0xFFFFFFFF);
  setRegister("ADC_CTL", 0xFFFFFFFF);
  setRegister("ADC1_CONFIG_DATA", 0x81A801FF);
  setRegister("ADC2_CONFIG_DATA", 0x81A801FF);
  setRegister("ADC3_CONFIG_DATA", 0x81A801FF);
  setRegister("ADC4_CONFIG_DATA", 0x81A801FF);
  setRegister("ADC5_CONFIG_DATA", 0x81A883FF);
}

void LLNLv1::initPots()
{
  LOG_INFO(__func__);
  if (stype_ == SensorType::ICARUS || stype_ == SensorType::ICARUS2) {
    setPot("HST_A_PDELAY", 0);
    setPotV("HST_B_NDELAY", 3.3);
    setPotV("HST_RO_IBIAS", 2.5, true);
    setPotV("HST_OSC_VREF_IN", 2.9, true);
    setPot("HST_B_PDELAY", 0);
    setPotV("HST_OSC_CTL", 1.45, true);
    setPotV("HST_A_NDELAY", 3.3);
    setPotV("VAB", 0.5);
    setPotV("HST_RO_NC_IBIAS", 2.5);
    setPotV("VRST", 0.3, true);
  } else { // Daedalus
    setPotV("HST_OSC_CTL", 1.0, true);
    setPotV("HST_RO_NC_IBIAS", 1.0);
    setPotV("HST_OSC_VREF_IN", 1.0, true);
    setPotV("VAB", 0.5);
  }
}

void LLNLv1::initSensor()
{
  LOG_INFO(__func__);
  initDefaults();
  // ring w/caps=01, relax=00, ring w/o caps = 02
  setSubRegister("OSC_SELECT", 0x00);
  setRegister("FPA_DIVCLK_EN_ADDR", 0x00000001);
}

void LLNLv1::latchPots()
{
  LOG_DEBUG(__func__);
  // latches register settings for pot 1
  setRegister("POT_CTL", 0x00000003);
  // latches register settings for pot 2
  setRegister("POT_CTL", 0x00000005);
  // latches register settings for pot 3
  setRegister("POT_CTL", 0x00000007);
  // latches register settings for pot 4
  setRegister("POT_CTL", 0x00000009);
  // latches register settings for pot 5
  setRegister("POT_CTL", 0x0000000B);
  // latches register settings for pot 6
  setRegister("POT_CTL", 0x0000000D);
  // latches register settings for pot 7
  setRegister("POT_CTL", 0x0000000F);
  // latches register settings for pot 8
  setRegister("POT_CTL", 0x00000011);
  // latches register settings for pot 9
  setRegister("POT_CTL", 0x00000013);
  // latches register settings for pot 10
  setRegister("POT_CTL", 0x00000015);
  // latches register settings for pot 11
  setRegister("POT_CTL", 0x00000017);
  // latches register settings for pot 12
  setRegister("POT_CTL", 0x00000019);
  // latches register settings for pot 13
  setRegister("POT_CTL", 0x0000001B);
}

void LLNLv1::enableLED(bool status)
{
  LOG_DEBUG(__func__);
  setSubRegister("LED_EN", status ? 1 : 0);
}

void LLNLv1::setLED(uint32_t led, bool status)
{
  LOG_DEBUG(__func__);
  setSubRegister("LED" + std::to_string(led), status ? 1 : 0);
}

double LLNLv1::getTemp(TempType scale) const
{
  LOG_DEBUG(std::string(__func__) + " scale = " + toString(scale));
  uint32_t raw = getRegister("TEMP_SENSE_DATA");

  double temp = (0xfff & raw) / 16.0;
  if (scale == TempType::K) {
    temp += 273.15;
  } else if (scale == TempType::F) {
    temp *= 1.8;
    temp += 32;
  }

  return temp;
}
