/*
 * load_cell.h
 *
 *  Created on: 03 Mar 2018
 *      Author: raffael
 */

#ifndef FW_LOAD_CELL_H_
#define FW_LOAD_CELL_H_

//#define USE_HX
#define USE_ADS

#include <xdc/cfg/global.h> //needed for semaphore
#include <ti/sysbios/knl/Semaphore.h>

// Semaphore_Handle semLoadCellDRDY;

void load_cell_Task();


void load_cell_isr();

#endif /* FW_LOAD_CELL_H_ */