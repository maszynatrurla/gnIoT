/*
 * High level code controlling passing of data between gniot and server.
 * service.h
 *
 *  Created on: 14 lut 2021
 *      Author: andrzej
 */

#ifndef MAIN_SERVICE_H_
#define MAIN_SERVICE_H_

#include <stdint.h>

#define NO_MEASUREMENT  0xFFFFFFFFUL

void service_send(int connection_status, uint32_t measurement);

#endif /* MAIN_SERVICE_H_ */
