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
 * @brief       Driver for reading ISO 11784 & 11785 FDX-B Animal Tags
 *
 * @author      Jonas Radtke <jonas.radtke@haw-hamburg.de> <jonas@radtke.dk>
 *
 * @}
 */
 
/*
* 
*  10000000000 	  11bit Header
*  1 11110000 	
*  1 ->00000011 	  38 bit, 12 digits, ID.
*  1 00000000 	  eg. 000000001008 (decimal).
*  1 00000000 	
*  1 11->000000   10 bit (3 digit) Country code.
*  1 11111001 	  eg. 999 (decimal).
*  1 -------1 	  1 bit data block status flag.
*  1 1------- 	  1 bit animal application indicator.
*  1 11010110 	  16 bit checksum.
*  1 01011101 	
*  1 01010110 	  24 bits of extra data if present.
*  1 00110100 	 
*  1 00010010 	
*
*/
 
#include "mlx90109.h"
#include "mlx90109_params.h"

#include <xdc/runtime/Timestamp.h>
#include "../uart_helper.h"
#include <assert.h>


Float   factor;  /* Clock ratio cpu/timestamp */
uint32_t time_on;
uint32_t time_off;

int32_t get_timefactor()
{
	xdc_runtime_Types_FreqHz freq1;   /* Timestamp frequency */
	xdc_runtime_Types_FreqHz freq2;   /* BIOS frequency */
	Timestamp_getFreq(&freq1);
	BIOS_getCpuFreq(&freq2);
	factor = (Float)freq2.lo / freq1.lo;
	return 0;
}

int16_t mlx90109_init(mlx90109_t *dev, const mlx90109_params_t *params)
{
	get_timefactor();
	/* write config params to device descriptor */
	memcpy(&dev->p, params, sizeof(mlx90109_params_t));
	
	//Init Modul Pin if used
	GPIO_write(dev->p.modu, 0); //turns off CW field
	//disable clk interrupt
	GPIO_disableInt(nbox_lf_clk);

	//Init data Rate Pin if used
	if (dev->p.dataSelect){
		if (MLX90109_PARAM_SPEED == 2000){
			GPIO_write(dev->p.dataSelect, 1);
		}
		else if (MLX90109_PARAM_SPEED == 4000){
			GPIO_write(dev->p.dataSelect, 0);
		}
	}
	//Init Mode Pin if used
	if (dev->p.mode){
		if (MLX90109_PARAM_CODE == 1){
			GPIO_write(dev->p.mode, 1);
		}
		else if (MLX90109_PARAM_CODE == 2){
			GPIO_write(dev->p.mode, 0);
		}
	
	}
	return MLX90109_OK;
}

void mlx90109_activate_reader(mlx90109_t *dev)
{
	time_on = Timestamp_get32();
	GPIO_write(dev->p.modu, 1); //turns ON CW field
	// enable clock interrupt
	GPIO_enableInt(nbox_lf_clk);
}

#define MLX_OUTPUT_BUF_LEN 20
uint8_t outbuffer[MLX_OUTPUT_BUF_LEN];

void mlx90109_disable_reader(mlx90109_t *dev)
{
	GPIO_write(dev->p.modu, 0); //turns OFF CW field
	//disable clk interrupt
	GPIO_disableInt(nbox_lf_clk);

	//send out tag ID:
	int i=0;
	for(i=0;i<10;i++)
	{
		uint8_t hexbuf;
		ui2a(dev->tagId[i], 16, 1, &hexbuf);
		uart_serial_putc(&debug_uart,hexbuf);
	}
	uart_serial_putc(&debug_uart,',');

	//send out readout difference:
	int deltat = (time_off - time_on);//(uint32_t)(factor*(float)(time_off - time_on)/8.0);
	int strlen = ui2a(deltat, 10, 1, outbuffer);
	uart_serial_write(&debug_uart, outbuffer, strlen);
	uart_serial_putc(&debug_uart,'\n');
}

#define UINT8_BIT_SIZE 8U
#define RIGHTMOST_BIT_SET(value) ((value) & 0x0001U)

uint16_t ucrc16_calc_le(const uint8_t *buf, size_t len, uint16_t poly,
                        uint16_t seed)
{
    assert(buf != NULL);
    unsigned int c,i;
    for (c = 0; c < len; c++, buf++) {
        seed ^= (*buf);
        for (i = 0; i < UINT8_BIT_SIZE; i++) {
            seed = RIGHTMOST_BIT_SET(seed) ? ((seed >> 1) ^ poly) : (seed >> 1);
        }
    }
    return seed;
}

int16_t mlx90109_format(mlx90109_t *dev, tagdata *tag)
{
	uint8_t i=0;
	uint8_t k=0;
	uint16_t crc = 0;
	
	dev->counter = 0;
	dev->counter_header = 0;
	
	// Data for Checksum
	for (k=0; k<=7; k++)
	{
		tag->checksumData[k] = 0;
		for (i=(k*9); i<=7+(k*9); i++)
		{
			tag->checksumData[k] |= (dev->data[i] << (i-(k*9)));
		}
	}

	// Checksum format
	tag->checksumArr[0] = 0; tag->checksumArr[1] = 0;
	for (i=72; i<=79; i++)
	{
		tag->checksumArr[0] |= (dev->data[i] << (i-72));
	}
	
	for (i=81; i<=88; i++)
	{
		tag->checksumArr[1] |= (dev->data[i] << (i-81));
	}
	
	tag->checksum16 = 0;
	tag->checksum16 = tag->checksum16 | tag->checksumArr[0] | (uint16_t)tag->checksumArr[1] << 8;
	// Checksum calculaton
	crc = ucrc16_calc_le(&tag->checksumData[0], sizeof(tag->checksumData), 0x8408 , 0x0000);
	
	if ((tag->checksum16 != crc))
	{
		tag->tagId = 0;
		return MLX90109_CRC_NOT_OK;
	}
	
	// Tag ID format
	tag->tagId = 0;
	tag->tagId = tag->tagId | (uint64_t)tag->checksumData[0] | (uint64_t)tag->checksumData[1] << 8 | (uint64_t)tag->checksumData[2] << 16 | (uint64_t)tag->checksumData[3] << 24 | (uint64_t)(tag->checksumData[4] & 0x3F) << 32 ;
	
	// Tag Countrycode format
	tag->countryCode = 0;
	tag->countryCode = tag->countryCode | ((uint16_t)tag->checksumData[5] << 2) | ((tag->checksumData[4] & 0xC0) >> 6);
	
	// Tag Data Block used and Tag for Animal Identification
	tag->dataBlock = 0;
	tag->dataBlock = tag->checksumData[6] & 0x01;
	tag->animalTag = 0;
	tag->animalTag = (tag->checksumData[7] & 0x80) >> 7;
	
	// Datablock for additional data and individual application
	for (i=90; i<=97; i++)
	{
		tag->dataB[0] |= (dev->data[i] << (i-90));
	}
	
	for (i=99; i<=106; i++)
	{
		tag->dataB[1] |= (dev->data[i] << (i-99));
	}
	
	for (i=108; i<=115; i++)
	{
		tag->dataB[2] |= (dev->data[i] << (i-108));
	}
	
	return MLX90109_OK;
}


int16_t em4100_read(mlx90109_t *dev)
{
#ifdef EM4100
	if((dev->counter_header==9))
	{
		// Detect "1"
		if((GPIO_read(dev->p.data) > 0))
		{
			dev->data[dev->counter]=1;
		}
		// Detect "0"
		if ((!(GPIO_read(dev->p.data))))
		{
			dev->data[dev->counter]=0;
		}

		if(dev->nibble_counter==4 && dev->id_counter<10)
		{
			//crc
			dev->nibble_counter = 0;
			dev->tagId[dev->id_counter] = dev->data[dev->counter-1] + //
											((dev->data[dev->counter-2])<<1) +//
											((dev->data[dev->counter-3])<<2) +//
											((dev->data[dev->counter-4])<<3);
			uint8_t crc = dev->data[dev->counter-1] +//
								dev->data[dev->counter-2] +//
								dev->data[dev->counter-3] +//
								dev->data[dev->counter-4];


			if((crc&0x01) == (dev->data[dev->counter]&0x01))
			{
				// CRC OK!
				dev->nibble_counter = 0;
				dev->id_counter++;
			}
			else
			{
				// start over
				dev->counter_header=0;
				dev->counter = 0;
				dev->nibble_counter = 0;
				return MLX90109_CRC_NOT_OK;
			}

		}
		else
		{
			dev->nibble_counter++;
		}


		dev->counter++;
	}
	else if(dev->counter_header<9)
	{
		// Detect Header (111111111 	 9bit Header (following a stop-0)
		if (!(GPIO_read(dev->p.data)))
		{// 0's
			dev->counter_header=0;
		}

		if (GPIO_read(dev->p.data) > 0)
		{//1's
			dev->counter_header++;
		}
		dev->counter = 0;
		dev->nibble_counter = 0;
		dev->id_counter = 0;
	}
	// Data complete after 64 bit incl header
	if (dev->counter > 63-9 && (dev->counter_header==9))
	{
		time_off = Timestamp_get32();

		dev->counter = 0;
		dev->counter_header = 0;
		return MLX90109_DATA_OK;
	}
	else
	{
		return MLX90109_OK;
	}
#else //FDX-B protocol
	// Detect "1"
	if((GPIO_read(dev->p.data) > 0)&&(dev->counter_header==11))
	{
		dev->data[dev->counter]=1;
		dev->counter++;
	}
	// Detect "0"
	if ((!(GPIO_read(dev->p.data)))&&(dev->counter_header==11))
	{
		dev->data[dev->counter]=0;
		dev->counter++;
	}

	// Detect Header (10000000000 	  11bit Header)
	if ((!(GPIO_read(dev->p.data)))&&(dev->counter_header<10))
	{// 0's
		dev->counter_header++;
		dev->counter = 0;
	}

	if ((GPIO_read(dev->p.data) > 0)&&(dev->counter_header<10))
	{//1's
		dev->counter_header=0;
		dev->counter = 0;
	}

	if ((GPIO_read(dev->p.data) > 0) && (dev->counter_header == 10))
	{
		dev->counter_header++;
	}

	// Data complete after 127 bit
	if ( dev->counter > 127)
	{
		dev->counter = 0;
		dev->counter_header = 0;
		return MLX90109_DATA_OK;
	}
	else
	{
		return MLX90109_OK;
	}
#endif
}
