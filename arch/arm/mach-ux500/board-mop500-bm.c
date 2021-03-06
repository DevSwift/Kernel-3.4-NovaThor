/*
 * Copyright (C) ST-Ericsson SA 2011
 *
 * License terms:  GNU General Public License (GPL), version 2
 *
 * U8500 board specific charger and battery initialization parameters.
 *
 * Author: Johan Palsson <johan.palsson@stericsson.com> for ST-Ericsson.
 * Author: Johan Gardsmark <johan.gardsmark@stericsson.com> for ST-Ericsson.
 *
 */

#include <linux/power_supply.h>
#include <linux/mfd/abx500.h>
#include <linux/mfd/abx500/ab8500-bm.h>
#include <linux/mfd/ab8500/pwmleds.h>
#include "board-mop500-bm.h"

/*
 * These are the defined batteries that uses a NTC and ID resistor placed
 * inside of the battery pack.
 * Note that the res_to_temp table must be strictly sorted by falling resistance
 * values to work.
 */
static struct abx500_v_to_cap cap_tbl_A[] = {
	{4171,	100},
	{4114,	 95},
	{4009,	 83},
	{3947,	 74},
	{3907,	 67},
	{3863,	 59},
	{3830,	 56},
	{3813,	 53},
	{3791,	 46},
	{3771,	 33},
	{3754,	 25},
	{3735,	 20},
	{3717,	 17},
	{3681,	 13},
	{3664,	  8},
	{3651,	  6},
	{3635,	  5},
	{3560,	  3},
	{3408,    1},
	{3247,	  0},
};
#ifdef CONFIG_AB8500_BATTERY_THERM_ON_BATCTRL
/*
 * These are the defined batteries that uses a NTC and ID resistor placed
 * inside of the battery pack.
 * Note that the res_to_temp table must be strictly sorted by falling resistance
 * values to work.
 */
/* SEMC Type 1 battery */
static struct res_to_temp temp_tbl_A[] = {
	{-20, 67400},
	{  0, 49200},
	{  5, 44200},
	{ 10, 39400},
	{ 15, 35000},
	{ 20, 31000},
	{ 25, 27400},
	{ 30, 24300},
	{ 35, 21700},
	{ 40, 19400},
	{ 45, 17500},
	{ 50, 15900},
	{ 55, 14600},
	{ 60, 13500},
	{ 65, 12500},
	{ 70, 11800},
	{100,  9200},
};
/* SEMC Type 2 battery */
static struct res_to_temp temp_tbl_B[] = {
	{-20, 180700},
	{  0, 160000},
	{  5, 152700},
	{ 10, 144900},
	{ 15, 136800},
	{ 20, 128700},
	{ 25, 121000},
	{ 30, 113800},
	{ 35, 107300},
	{ 40, 101500},
	{ 45,  96500},
	{ 50,  92200},
	{ 55,  88600},
	{ 60,  85600},
	{ 65,  83000},
	{ 70,  80900},
	{100,  73900},
};
static struct abx500_v_to_cap cap_tbl_B[] = {
	{4161,	100},
	{4124,	 98},
	{4044,	 90},
	{4003,	 85},
	{3966,	 80},
	{3933,	 75},
	{3888,	 67},
	{3849,	 60},
	{3813,	 55},
	{3787,	 47},
	{3772,	 30},
	{3751,	 25},
	{3718,	 20},
	{3681,	 16},
	{3660,	 14},
	{3589,	 10},
	{3546,	  7},
	{3495,	  4},
	{3404,	  2},
	{3250,	  0},
};
#endif
static struct abx500_v_to_cap cap_tbl[] = {
	{4186,	100},
	{4163,	 99},
	{4114,	 95},
	{4068,	 90},
	{3990,	 80},
	{3926,	 70},
	{3898,	 65},
	{3866,	 60},
	{3833,	 55},
	{3812,	 50},
	{3787,	 40},
	{3768,	 30},
	{3747,	 25},
	{3730,	 20},
	{3705,	 15},
	{3699,	 14},
	{3684,	 12},
	{3672,	  9},
	{3657,	  7},
	{3638,	  6},
	{3556,	  4},
	{3424,	  2},
	{3317,	  1},
	{3094,	  0},
};

/*
 * Note that the res_to_temp table must be strictly sorted by falling
 * resistance values to work.
 */
static struct abx500_res_to_temp temp_tbl[] = {
	{-5, 214834},
	{ 0, 162943},
	{ 5, 124820},
	{10,  96520},
	{15,  75306},
	{20,  59254},
	{25,  47000},
	{30,  37566},
	{35,  30245},
	{40,  24520},
	{45,  20010},
	{50,  16432},
	{55,  13576},
	{60,  11280},
	{65,   9425},
};

#ifdef CONFIG_AB8500_BATTERY_THERM_ON_BATCTRL
/*
 * Note that the batres_vs_temp table must be strictly sorted by falling
 * temperature values to work.
 */
static struct batres_vs_temp temp_to_batres_tbl[] = {
	{ 40, 120},
	{ 30, 135},
	{ 20, 165},
	{ 10, 230},
	{ 00, 325},
	{-10, 445},
	{-20, 595},
};
#else
/*
 * Note that the batres_vs_temp table must be strictly sorted by falling
 * temperature values to work.
 */
#ifdef CONFIG_AB8500_9100_LI_ION_BATTERY
#define BATRES 		180
#else
#define BATRES 		300
#endif
static struct batres_vs_temp temp_to_batres_tbl[] = {
	{ 60, BATRES},
	{ 30, BATRES},
	{ 20, BATRES},
	{ 10, BATRES},
	{ 00, BATRES},
	{-10, BATRES},
	{-20, BATRES},
};
#endif
static const struct abx500_battery_type bat_type[] = {
	[BATTERY_UNKNOWN] = {
		/* First element always represent the UNKNOWN battery */
		.name = POWER_SUPPLY_TECHNOLOGY_UNKNOWN,
		.resis_high = 0,
		.resis_low = 0,
		.battery_resistance = 300,
#ifdef CONFIG_AB8500_9100_LI_ION_BATTERY
		.charge_full_design = 2600,
#else
		.charge_full_design = 612,
#endif
		.nominal_voltage = 3700,
#ifdef CONFIG_AB8500_9100_LI_ION_BATTERY
		.termination_vol = 4150,
#else
		.termination_vol = 4050,
#endif
		.termination_curr = 200,
#ifdef CONFIG_AB8500_9100_LI_ION_BATTERY
		.recharge_vol = 4130,
		.normal_cur_lvl = 520,
		.normal_vol_lvl = 4200,
#else
		.recharge_vol = 3990,
		.normal_cur_lvl = 400,
		.normal_vol_lvl = 4100,
#endif
		.maint_a_cur_lvl = 400,
		.maint_a_vol_lvl = 4050,
		.maint_a_chg_timer_h = 60,
		.maint_b_cur_lvl = 400,
		.maint_b_vol_lvl = 4000,
		.maint_b_chg_timer_h = 200,
		.low_high_cur_lvl = 300,
		.low_high_vol_lvl = 4000,
		.n_temp_tbl_elements = ARRAY_SIZE(temp_tbl),
		.r_to_t_tbl = temp_tbl,
		.n_v_cap_tbl_elements = ARRAY_SIZE(cap_tbl),
		.v_to_cap_tbl = cap_tbl,
		.n_batres_tbl_elements = ARRAY_SIZE(temp_to_batres_tbl),
		.batres_tbl = temp_to_batres_tbl,
	},

#ifdef CONFIG_AB8500_BATTERY_THERM_ON_BATCTRL
	{
		.name = POWER_SUPPLY_TECHNOLOGY_LIPO,
		.resis_high = 53407,
		.resis_low = 12500,
		.battery_resistance = 300,
		.charge_full_design = 900,
		.nominal_voltage = 3600,
		.termination_vol = 4150,
		.termination_curr = 80,
		.recharge_vol = 4130,
		.normal_cur_lvl = 700,
		.normal_vol_lvl = 4200,
		.maint_a_cur_lvl = 600,
		.maint_a_vol_lvl = 4150,
		.maint_a_chg_timer_h = 60,
		.maint_b_cur_lvl = 600,
		.maint_b_vol_lvl = 4100,
		.maint_b_chg_timer_h = 200,
		.low_high_cur_lvl = 300,
		.low_high_vol_lvl = 4000,
		.n_temp_tbl_elements = ARRAY_SIZE(temp_tbl_A),
		.r_to_t_tbl = temp_tbl_A,
		.n_v_cap_tbl_elements = ARRAY_SIZE(cap_tbl_A),
		.v_to_cap_tbl = cap_tbl_A,
		.n_batres_tbl_elements = ARRAY_SIZE(temp_to_batres_tbl),
		.batres_tbl = temp_to_batres_tbl,

	},
	{
		.name = POWER_SUPPLY_TECHNOLOGY_LIPO,
		.resis_high = 165418,
		.resis_low = 82869,
		.battery_resistance = 300,
		.charge_full_design = 900,
		.nominal_voltage = 3600,
		.termination_vol = 4150,
		.termination_curr = 80,
		.recharge_vol = 4130,
		.normal_cur_lvl = 700,
		.normal_vol_lvl = 4200,
		.maint_a_cur_lvl = 600,
		.maint_a_vol_lvl = 4150,
		.maint_a_chg_timer_h = 60,
		.maint_b_cur_lvl = 600,
		.maint_b_vol_lvl = 4100,
		.maint_b_chg_timer_h = 200,
		.low_high_cur_lvl = 300,
		.low_high_vol_lvl = 4000,
		.n_temp_tbl_elements = ARRAY_SIZE(temp_tbl_B),
		.r_to_t_tbl = temp_tbl_B,
		.n_v_cap_tbl_elements = ARRAY_SIZE(cap_tbl_B),
		.v_to_cap_tbl = cap_tbl_B,
		.n_batres_tbl_elements = ARRAY_SIZE(temp_to_batres_tbl),
		.batres_tbl = temp_to_batres_tbl,
	},
#else
/*
 * These are the batteries that doesn't have an internal NTC resistor to measure
 * its temperature. The temperature in this case is measure with a NTC placed
 * near the battery but on the PCB.
 */
	{
		.name = POWER_SUPPLY_TECHNOLOGY_LIPO,
		.resis_high = 76000,
		.resis_low = 53000,
		.battery_resistance = 300,
		.charge_full_design = 900,
		.nominal_voltage = 3700,
		.termination_vol = 4150,
		.termination_curr = 100,
		.recharge_vol = 4130,
		.normal_cur_lvl = 700,
		.normal_vol_lvl = 4200,
		.maint_a_cur_lvl = 600,
		.maint_a_vol_lvl = 4150,
		.maint_a_chg_timer_h = 60,
		.maint_b_cur_lvl = 600,
		.maint_b_vol_lvl = 4100,
		.maint_b_chg_timer_h = 200,
		.low_high_cur_lvl = 300,
		.low_high_vol_lvl = 4000,
		.n_temp_tbl_elements = ARRAY_SIZE(temp_tbl),
		.r_to_t_tbl = temp_tbl,
		.n_v_cap_tbl_elements = ARRAY_SIZE(cap_tbl),
		.v_to_cap_tbl = cap_tbl,
		.n_batres_tbl_elements = ARRAY_SIZE(temp_to_batres_tbl),
		.batres_tbl = temp_to_batres_tbl,
	},
	{
		.name = POWER_SUPPLY_TECHNOLOGY_LION,
		.resis_high = 30000,
		.resis_low = 10000,
		.battery_resistance = 300,
		.charge_full_design = 950,
		.nominal_voltage = 3700,
		.termination_vol = 4150,
		.termination_curr = 100,
		.recharge_vol = 4130,
		.normal_cur_lvl = 700,
		.normal_vol_lvl = 4200,
		.maint_a_cur_lvl = 600,
		.maint_a_vol_lvl = 4150,
		.maint_a_chg_timer_h = 60,
		.maint_b_cur_lvl = 600,
		.maint_b_vol_lvl = 4100,
		.maint_b_chg_timer_h = 200,
		.low_high_cur_lvl = 300,
		.low_high_vol_lvl = 4000,
		.n_temp_tbl_elements = ARRAY_SIZE(temp_tbl),
		.r_to_t_tbl = temp_tbl,
		.n_v_cap_tbl_elements = ARRAY_SIZE(cap_tbl),
		.v_to_cap_tbl = cap_tbl,
		.n_batres_tbl_elements = ARRAY_SIZE(temp_to_batres_tbl),
		.batres_tbl = temp_to_batres_tbl,
	},
	{
		.name = POWER_SUPPLY_TECHNOLOGY_LION,
		.resis_high = 95000,
		.resis_low = 76001,
		.battery_resistance = 300,
		.charge_full_design = 950,
		.nominal_voltage = 3700,
		.termination_vol = 4150,
		.termination_curr = 100,
		.recharge_vol = 4130,
		.normal_cur_lvl = 700,
		.normal_vol_lvl = 4200,
		.maint_a_cur_lvl = 600,
		.maint_a_vol_lvl = 4150,
		.maint_a_chg_timer_h = 60,
		.maint_b_cur_lvl = 600,
		.maint_b_vol_lvl = 4100,
		.maint_b_chg_timer_h = 200,
		.low_high_cur_lvl = 300,
		.low_high_vol_lvl = 4000,
		.n_temp_tbl_elements = ARRAY_SIZE(temp_tbl),
		.r_to_t_tbl = temp_tbl,
		.n_v_cap_tbl_elements = ARRAY_SIZE(cap_tbl),
		.v_to_cap_tbl = cap_tbl,
		.n_batres_tbl_elements = ARRAY_SIZE(temp_to_batres_tbl),
		.batres_tbl = temp_to_batres_tbl,
	},
#endif
};

static char *ab8500_charger_supplied_to[] = {
	"ab8500_chargalg",
	"ab8500_fg",
	"ab8500_btemp",
};

static char *ab8500_btemp_supplied_to[] = {
	"ab8500_chargalg",
	"ab8500_fg",
};

static char *ab8500_fg_supplied_to[] = {
	"ab8500_chargalg",
	"ab8500_usb",
};

static char *ab8500_chargalg_supplied_to[] = {
	"ab8500_fg",
};

struct abx500_charger_platform_data ab8500_charger_plat_data = {
	.supplied_to = ab8500_charger_supplied_to,
	.num_supplicants = ARRAY_SIZE(ab8500_charger_supplied_to),
	.autopower_cfg		= false,
};

struct abx500_btemp_platform_data ab8500_btemp_plat_data = {
	.supplied_to = ab8500_btemp_supplied_to,
	.num_supplicants = ARRAY_SIZE(ab8500_btemp_supplied_to),
};

struct abx500_fg_platform_data ab8500_fg_plat_data = {
	.supplied_to = ab8500_fg_supplied_to,
	.num_supplicants = ARRAY_SIZE(ab8500_fg_supplied_to),
};

struct abx500_chargalg_platform_data ab8500_chargalg_plat_data = {
	.supplied_to = ab8500_chargalg_supplied_to,
	.num_supplicants = ARRAY_SIZE(ab8500_chargalg_supplied_to),
};

static struct ab8500_led_pwm leds_pwm_data[] = {
	[0] = {
		.pwm_id = 1,
		.blink_en = 1,
	},
	[1] = {
		.pwm_id = 2,
		.blink_en = 0,
	},
	[2] = {
		.pwm_id = 3,
		.blink_en = 0,
	},
};

struct ab8500_pwmled_platform_data ab8500_pwmled_plat_data = {
	.num_pwm = 3,
	.leds = leds_pwm_data,
};

static const struct abx500_bm_capacity_levels cap_levels = {
	.critical	= 2,
	.low		= 10,
	.normal		= 70,
	.high		= 95,
	.full		= 100,
};

static const struct abx500_fg_parameters fg = {
	.recovery_sleep_timer = 10,
	.recovery_total_time = 100,
	.init_timer = 1,
	.init_discard_time = 5,
	.init_total_time = 40,
	.high_curr_time = 60,
	.accu_charging = 30,
	.accu_high_curr = 30,
	.high_curr_threshold = 50,
	.lowbat_threshold = 3100,
	.battok_falling_th_sel0 = 2860,
	.battok_raising_th_sel1 = 2860,
	.user_cap_limit = 15,
	.maint_thres = 97,
};

static const struct abx500_maxim_parameters maxi_params = {
	.ena_maxi = true,
	.chg_curr = 910,
	.wait_cycles = 10,
	.charger_curr_step = 100,
};

static const struct abx500_bm_charger_parameters chg = {
	.usb_volt_max		= 5500,
	.usb_curr_max		= 1500,
	.ac_volt_max		= 7500,
	.ac_curr_max		= 1500,
};

struct abx500_bm_data ab8500_bm_data = {
	.temp_under		= 3,
	.temp_low		= 8,
	.temp_high		= 43,
	.temp_over		= 48,
	.main_safety_tmr_h	= 4,
	.temp_interval_chg	= 20,
	.temp_interval_nochg	= 120,
	.usb_safety_tmr_h	= 4,
	.bkup_bat_v		= BUP_VCH_SEL_2P6V,
	.bkup_bat_i		= BUP_ICH_SEL_150UA,
#ifdef CONFIG_AB8500_9100_LI_ION_BATTERY
	.no_maintenance		= true,
#else
	.no_maintenance		= false,
#endif
#ifdef CONFIG_AB8500_BATTERY_THERM_ON_BATCTRL
	.adc_therm		= ABx500_ADC_THERM_BATCTRL,
#else
	.adc_therm		= ABx500_ADC_THERM_BATTEMP,
#endif
#ifdef CONFIG_AB8500_9100_LI_ION_BATTERY
	.chg_unknown_bat	= true,
#else
	.chg_unknown_bat	= false,
#endif
	.enable_overshoot	= false,
	.fg_res			= 100,
	.cap_levels		= &cap_levels,
	.bat_type		= bat_type,
	.n_btypes		= ARRAY_SIZE(bat_type),
	.batt_id		= 0,
	.interval_charging	= 5,
	.interval_not_charging	= 120,
	.temp_hysteresis	= 3,
	.gnd_lift_resistance	= 34,
	.maxi			= &maxi_params,
	.chg_params		= &chg,
	.fg_params		= &fg,
};
