/*
 * Copyright (C) 2007-2010 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Apple Computer, Inc. All rights reserved.
 *
 * This document is the property of Apple Inc.
 * It is considered confidential and proprietary.
 *
 * This document may not be reproduced or transmitted in any form,
 * in whole or in part, without the express written permission of
 * Apple Inc.
 */
#ifndef __DRIVERS_POWER_H
#define __DRIVERS_POWER_H

#include <sys/types.h>

__BEGIN_DECLS

#define kPowerBootFlagCold       (0)
#define kPowerBootFlagColdButton (1)
#define kPowerBootFlagWarm       (2)
#define kPowerBootFlagResumeLost (3)

int power_init(void);
int power_late_init(void);
int power_backlight_enable(u_int32_t backlight_level);
int power_get_boot_flag(void);

void power_suspend(void);
void power_shutdown(void); // tell the pmu to pull the plug
int power_set_soc_voltage(unsigned mv, int override);
int power_set_cpu_voltage(unsigned mv, int override);
int power_set_ram_voltage(unsigned mv, int override);

#define POWER_RAIL_CPU       0
#define POWER_RAIL_VDD_FIXED 1
#define POWER_RAIL_SOC		 2
#define POWER_RAIL_CPU_RAM   3
#define POWER_RAIL_GPU       4
#define POWER_RAIL_GPU_RAM   5	
// not a rail, might move this to target
#define POWER_RAIL_COUNT     6

// @rail one of POWER_RAIL_* (replace power_get_buck_value(), backward compatible)
int power_get_rail_value(int rail, unsigned mv, u_int32_t *buffer);
// @rail one of POWER_RAIL_* (backward compatible with power_convert_dwi_to_mv())
int power_convert_dwi_to_mv(int rail, u_int32_t dwival);
// deprecated, use power_get_rail_value() ( buck is not the buck# )
int power_get_buck_value(int buck, unsigned mv, u_int32_t *buffer);

int power_set_display_voltage_offset(u_int32_t voltage_offset, bool fixed_boost);

void power_set_usb_state(bool configured, bool suspended);
void power_set_usb_enabled(bool enabed);
int power_set_gpio(u_int32_t gpio, u_int32_t direction, u_int32_t value);
int power_set_ldo(u_int32_t ldo, u_int32_t voltage);
int power_enable_ldo(u_int32_t ldo, bool enable);
bool power_get_gpio(u_int32_t gpio);
void power_gpio_configure(u_int32_t gpio, u_int32_t config);

bool power_needs_precharge(void);
bool power_needs_thermal_trap(void);
bool power_do_chargetrap(void);
void power_cancel_buttonwait(void);
bool power_is_suspended(void);
void power_will_resume(void);
bool power_has_usb(void);
int power_read_dock_id(unsigned *id);
bool power_get_diags_dock(void);
u_int32_t power_get_boot_battery_level(void);
u_int32_t power_get_battery_level(void);
void power_get_usb_brick_id(uint32_t *id, size_t count);
bool power_has_batterypack(void);
bool power_enable_charging(bool inflow, bool charging);
uint32_t power_get_available_charge_current(void);

int power_get_nvram(u_int8_t key, u_int8_t *data);
int power_set_nvram(u_int8_t key, u_int8_t data);
void power_clr_events(int wake);


#define kPowerNVRAMiBootStateName			"iBootState"
#define kPowerNVRAMiBootStateKey			(0)

#define kPowerNVRAMiBootStatePrecharge			(1 << 5)    // can be or'd with modes below
#define kPowerNVRAMiBootStateThermalTrap		(1 << 0)
#define kPowerNVRAMiBootStateModeMask			(0xd0)	    // bits 4, 6 and 7 (keep backwards compatible)
#define kPowerNVRAMiBootStateModeNormalBoot		(0x00)
#define kPowerNVRAMiBootStateModeResumed		(0x40)
#define kPowerNVRAMiBootStateModeSuspended		(0x80)
#define kPowerNVRAMiBootStateModeButtonwaitWithGraphics	(0x10)
#define kPowerNVRAMiBootStateModeButtonwaitNoGraphics	(0x90)

#define kPowerNVRAMiBootDebugName			"iBootDebug"
#define kPowerNVRAMiBootDebugKey			(1)
#define kPowerNVRAMiBootDebugIAPSerial			(1 << 0)
#define kPowerNVRAMiBootDebugAltSerial			(1 << 1)
#define kPowerNVRAMiBootDebugJtag			(1 << 2)
#define kPowerNVRAMiBootDebugEarlyTracing		(1 << 3)
#define kPowerNVRAMiBootDebugBatteryTrap		(1 << 4)
#define kPowerNVRAMiBootDebugWDTWake			(1 << 7)

#define kPowerNVRAMiBootStageName			"iBootStage"
#define kPowerNVRAMiBootStageKey			(2)
#define kPowerNVRAMiBootStageOff			(0x00)
#define kPowerNVRAMiBootStageLLBStart			(0x10)
#define kPowerNVRAMiBootStageLLBEnd			(0x1F)
#define kPowerNVRAMiBootStageiBootStart			(0x20)
#define kPowerNVRAMiBootStageiBootEnd			(0x2F)
#define kPowerNVRAMDiagsStageDiagsBootComplete		(0xD0)
#define kPowerNVRAMiBootStagePanicSave			(0xE0)
#define kPowerNVRAMiBootStagePanicReboot		(0xE1)
#define kPowerNVRAMiBootStagePrechargeReboot		(0xE2)
#define kPowerNVRAMiBootStageBooted			(0xFF)

#if PRODUCT_LLB
#define kPowerNVRAMiBootStageProductStart		kPowerNVRAMiBootStageLLBStart
#define kPowerNVRAMiBootStageProductEnd			kPowerNVRAMiBootStageLLBEnd
#endif

#if PRODUCT_IBOOT || PRODUCT_IBEC
#define kPowerNVRAMiBootStageProductStart		kPowerNVRAMiBootStageiBootStart
#define kPowerNVRAMiBootStageProductEnd			kPowerNVRAMiBootStageiBootEnd
#endif

#if PRODUCT_IBSS
#define kPowerNVRAMiBootStageProductStart		kPowerNVRAMiBootStageLLBStart
#define kPowerNVRAMiBootStageProductEnd			kPowerNVRAMiBootStageiBootEnd
#endif

#define kPowerNVRAMiBootErrorCountName			"iBootErrorCount"
#define kPowerNVRAMiBootErrorCountKey			(3)
#define kPowerNVRAMiBootErrorCountMask			(0x0F)
#define kPowerNVRAMiBootErrorPanicShift 		(4)
#define kPowerNVRAMiBootErrorBootShift  		(0)

#define kPowerNVRAMiBootErrorStageName			"iBootErrorStage"
#define kPowerNVRAMiBootErrorStageKey			(4)

#define kPowerNVRAMiBootMemCalCAOffset0Key		(5)	// CA offset need 2 bytes
#define kPowerNVRAMiBootMemCalCAOffset1Key		(6)
#define kPowerNVRAMiBootMemCalCAOffset2Key		(7)
#define kPowerNVRAMiBootMemCalCAOffset3Key		(8)

#define kPowerNVRAMiBootMemCalCAOffset0Name		"iBootMemCalCAOffset0"
#define kPowerNVRAMiBootMemCalCAOffset1Name		"iBootMemCalCAOffset1"
#define kPowerNVRAMiBootMemCalCAOffset2Name		"iBootMemCalCAOffset2"
#define kPowerNVRAMiBootMemCalCAOffset3Name		"iBootMemCalCAOffset3"

#define kPowerNVRAMiBootBootFlags0Key			(9)
#define kPowerNVRAMiBootBootFlags1Key			(10)

#define kPowerNVRAMiBootBootFlags0Name			"iBootBootFlags0"
#define kPowerNVRAMiBootBootFlags1Name			"iBootBootFlags1"

#define kPowerNVRAMiBootEnterDFUName			"iBootEnterDFU"
#define kPowerNVRAMiBootEnterDFUKey			(11)
#define kPowerNVRAMiBootEnterDFURequest			(0xA0)
#define kPowerNVRAMiBootEnterDFUOff			(0x00)

#define kPowerNVRAMPropertyCount			(12)

struct power_charge_limits
{
    uint16_t	upperVoltageLimit;	/* mV */
    int16_t	lowerTempLimit;	/* cC */
    int16_t	upperTempLimit;	/* cC */
    uint16_t	currentSetting;	/* charger-dependent */
};

bool power_load_memory_calibration(void *settings, uint32_t settingsSize);
bool power_store_memory_calibration(void *settings, uint32_t settingsSize);

void pmu_early_init(void);
void pmu_setup(void);
void pmu_late_init(void);
void pmu_will_resume(void);
void pmu_shutdown(void);
void pmu_suspend(void);
void pmu_set_backlight_enable(uint32_t backlight_level);
u_int32_t pmu_read_brick_id_level(void);
int pmu_read_system_temperature(int idx, int *centiCelsiusTemperature);
void pmu_check_events(bool *powersupply_change_event,
		     bool *button_event,
		     bool *other_wake_event);
int pmu_uvwarn_config(int dev, uint32_t thresholdMV);

bool power_load_voltage_knobs(void *settings, uint32_t settingsSize);
bool power_store_voltage_knobs(void *settings, uint32_t settingsSize);

utime_t power_get_calendar_time(void);


// Dark boot API's

void power_clear_dark_boot_flag (void); // Clears the dark boot flag from NVRAM. Call on every startup AFTER nvram is writable
void power_disable_dark_boot (void);  // Disables dark boot by lighting up display to previous backlight command
void power_dark_boot_checkpoint (void); // Checks for button presses and disables dark boot if needed.
bool power_is_dark_boot (void); // Returns true if we are doing a dark boot.

// smartport API
//

#define SMARTPORT_EXT_PWR_IN_SEL 1
#define SMARTPORT_ACC_PWR_IN_SEL 0

int smartport_get_data(int dev, uint16_t reg, uint8_t *byte); // low level, you don't need this
int smartport_get_pwr_in_sel(int dev, uint8_t *sel); // power switch state


__END_DECLS

#endif /* __DRIVERS_POWER_H */
