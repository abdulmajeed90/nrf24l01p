#ifndef __NRF_H__
#define __NRF_H__
#include <stdint.h>
#include <stdbool.h>
typedef enum {
    NRF_PRIM_TX,
    NRF_PRIM_RX
}nrf_operation_mode_t;

typedef enum {
    NRF_POWER_DOWN,
    NRF_POWER_UP
}nrf_power_mode_t;

typedef enum {
    NRF_PIPE_0   = (1<<0),
    NRF_PIPE_1   = (1<<1),
    NRF_PIPE_2   = (1<<2),
    NRF_PIPE_3   = (1<<3),
    NRF_PIPE_4   = (1<<4),
    NRF_PIPE_5   = (1<<5),
    NRF_PIPE_ALL = (0x3F)
}nrf_pipe_t;
void nrf_init();
void nrf_clear_int();
void nrf_set_operation_mode(nrf_operation_mode_t op_mode);
void nrf_set_power_mode(nrf_power_mode_t pwr_mode);
void nrf_enable_pipe(nrf_pipe_t pipe, bool enable);
void nrf_enable_autoack(nrf_pipe_t pipe, bool enable);
void nrf_set_pipe_width(nrf_pipe_t pipe, int pw);
void nrf_set_address(uint8_t *addr);
void nrf_flush_tx();
void nrf_flush_rx();
void nrf_send(char *buf, int len);
void nrf_recv(char *buf, int len);
#endif//__NRF_H__

