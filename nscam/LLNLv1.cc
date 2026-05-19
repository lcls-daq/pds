#include "LLNLv1.hh"
#include "Error.hh"

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
  {"POT1", {"POT_REG4_TO_1", 0, 8, true}},
  {"POT2", {"POT_REG4_TO_1", 8, 8, true}},
  {"POT3", {"POT_REG4_TO_1", 16, 8, true}},
  {"POT4", {"POT_REG4_TO_1", 24, 8, true}},
  {"POT5", {"POT_REG8_TO_5", 0, 8, true}},
  {"POT6", {"POT_REG8_TO_5", 8, 8, true}},
  {"POT7", {"POT_REG8_TO_5", 16, 8, true}},
  {"POT8", {"POT_REG8_TO_5", 24, 8, true}},
  {"POT9", {"POT_REG12_TO_9", 0, 8, true}},
  {"POT10", {"POT_REG12_TO_9", 8, 8, true}},
  {"POT11", {"POT_REG12_TO_9", 16, 8, true}},
  {"POT12", {"POT_REG12_TO_9", 24, 8, true}},
  {"POT13", {"POT_REG13", 0, 8, true}},
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

using namespace Pds::NsCam;

LLNLv1::LLNLv1(SensorType stype, std::shared_ptr<Comm> comm) :
  Board(BoardType::LLNL_V1, stype, comm, REG_NAMES, SUBREGNAMES, DEFAULTS)
{}

void LLNLv1::initBoard()
{
  clearStatus();
  configADCs();

  // initialize vref
  double vrefmax = getSubRegister("ADC5_VREF3") ? 3.0 : 2.5;
  vref_ = vrefmax * getSubRegister("ADC5_VREF") / 1024.;

  // set the adc multiplier
  uint32_t maskbits = 0x35; // aka 0b110101
  uint32_t multmask = getSubRegister("ADC5_MULT") & maskbits;
  if (multmask == maskbits) {
    adc5_mult_ = 2;
  } else if (multmask == 0) {
    adc5_mult_ = 4;
  } else {
    throw BoardError("inconsistent mode settings on ADC5");
  }

  setSubRegister("LED_EN", 1);
}

void LLNLv1::clearStatus()
{
  getRegister("STAT_REG_SRC");
  getRegister("STAT_REG2_SRC");
}

void LLNLv1::configADCs()
{
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
