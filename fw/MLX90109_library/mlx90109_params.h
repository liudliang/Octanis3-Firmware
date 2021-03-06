/*
 * Copyright (C) 2017 Jonas Radtke <jonas.radtke@haw-hamburg.de> <jonas@radtke.dk>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

 /**
 * @ingroup     drivers_MLX90109
 * @{
 *
 * @file
 * @brief       Default parameters for MLX90109 driver
 *
 * @author      Jonas Radtke <jonas.radtke@haw-hamburg.de> <jonas@radtke.dk>
 */

#ifndef MLX90109_PARAMS_H
#define MLX90109_PARAMS_H

#include "../../Board.h"
#include "mlx90109.h"

//#define EM4100 // comment if decoding FDX tags

#ifndef EM4100
	#define FDX
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Set default configuration parameters for the MLX90109 driver
 * @{
 */
#ifndef MLX90109_PARAM_CLOCK_PIN
#define MLX90109_PARAM_CLOCK_PIN       nbox_lf_clk // (GPIO_PIN(PORT_A, 15))
#endif
#ifndef MLX90109_PARAM_DATA_PIN
#define MLX90109_PARAM_DATA_PIN   	   nbox_lf_data//(GPIO_PIN(PORT_C, 14))
#endif
#ifndef MLX90109_PARAM_MODU_PIN
#define MLX90109_PARAM_MODU_PIN   nbox_lf_modul
#endif

#ifdef EM4100
// http://www.priority1design.com.au/em4100_protocol.html
	#ifndef MLX90109_PARAM_SPEED
	#define MLX90109_PARAM_SPEED      (2000)//2000 baud for EM4100 or 4000 baud for fdx
	#endif
	#ifndef MLX90109_PARAM_CODE
	#define MLX90109_PARAM_CODE       (1)//  1 = Manchester for EM4100, 2 = Biphase for FDX
	#endif
#else
// http://www.priority1design.com.au/fdx-b_animal_identification_protocol.html
	#ifndef MLX90109_PARAM_SPEED
	#define MLX90109_PARAM_SPEED      (4000)//2000 baud for EM4100 or 4000 baud for fdx
	#endif
	#ifndef MLX90109_PARAM_CODE
	#define MLX90109_PARAM_CODE       (2)//  1 = Manchester for EM4100, 2 = Biphase for FDX
	#endif
#endif


#define MLX_TAG_EM4100 		1
#define MLX_TAG_FDX		 	0
/** @} */

/**
 * @brief   MLX90109 configuration
 */
#ifndef MLX90109_PARAMS
#define MLX90109_PARAMS           { .clock			= MLX90109_PARAM_CLOCK_PIN ,\
									.data			= MLX90109_PARAM_DATA_PIN ,\
									.modu			= MLX90109_PARAM_MODU_PIN , \
									.tag_select     = MLX_TAG_FDX}
#endif

static const mlx90109_params_t mlx90109_params[] ={
	MLX90109_PARAMS
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* MLX90109_PARAMS_H */
/** @} */
 
