/*
 * HTTP client interface.
 * client.h
 *
 *  Created on: 13 lut 2021
 *      Author: andrzej
 */

#ifndef MAIN_CLIENT_H_
#define MAIN_CLIENT_H_

#include <stdint.h>

typedef void (*reponse_handler)(const char * key, const char * value);

/**
 * Open connection to server.
 */
int client_open(void);
/**
 * Close connection with server.
 */
int client_close(void);
/**
 * Send data to server.
 * @param request complete HTTP request data
 * @param length length of request string
 */
int client_request(const char * request, int length);
/**
 * Get response from server - debug version.
 * This function simply prints to console all contents
 * of received data.
 */
int client_response(void);
/**
 * Get response and handle commands from server.
 * Checks HTTP status and looks for special commands
 * in data returned from server.
 * @param handler callback for handling key+value commands
 * from server
 */
int client_response_iterate(reponse_handler handler);

#endif /* MAIN_CLIENT_H_ */
