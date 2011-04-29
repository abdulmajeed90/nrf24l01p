#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include "nrf24l01.h"
/*NRF commands*/
#define R_REGISTER          (0x00)      /*read register*/
#define W_REGISTER          (0x20)      /*write register*/
#define R_RX_PAYLOAD        (0x61)      /*read RX payload*/
#define W_TX_PAYLOAD        (0xA0)      /*write TX payload*/
#define FLUSH_TX            (0xE1)      /*flush TX FIFO*/
#define FLUSH_RX            (0xE2)      /*flush RX FIFO*/
/*NRF registers*/
#define CONFIG              (0x00)      /*configuration*/
#define EN_AA               (0x01)      /*enable automatic ACL*/
#define EN_RXADDR           (0x02)      /*enable RX addresses*/
#define SETUP_AW            (0x03)      /*setup address width*/
#define RETR                (0x04)      /*setup retransmission*/
#define RF_CH               (0x05)      /*RF channel*/
#define RF_SETUP            (0x06)      /*RF setup*/
#define STATUS              (0x07)      /*setup retransmission*/
#define OBSERVE_TX          (0x08)      /*TX observe*/
#define RPD                 (0x09)      /*received power detector*/
#define RX_ADDR_P0          (0x0A)      /*receive address data pipe 0*/
#define RX_ADDR_P1          (0x0B)      /*receive address data pipe 1*/
#define RX_ADDR_P2          (0x0C)      /*receive address data pipe 2*/
#define RX_ADDR_P3          (0x0D)      /*receive address data pipe 3*/
#define RX_ADDR_P4          (0x0E)      /*receive address data pipe 4*/
#define RX_ADDR_P5          (0x0F)      /*receive address data pipe 5*/
#define TX_ADDR             (0x10)      /*transmit address*/
#define RX_PW_P0            (0x11)      /*pipe 0 payload length*/
#define RX_PW_P1            (0x12)      /*pipe 1 payload length*/
#define RX_PW_P2            (0x13)      /*pipe 2 payload length*/
#define RX_PW_P3            (0x14)      /*pipe 3 payload length*/
#define RX_PW_P4            (0x15)      /*pipe 4 payload length*/ 
#define RX_PW_P5            (0x16)      /*pipe 5 payload length*/
#define FIFO_STATUS         (0x17)      /*FIFO status*/
#define DYNPD               (0x1C)      /*enable dynamic payload length*/
/*CONFIG register bits*/
#define PRIM_RX             (1<<0)
#define PWR_UP              (1<<1)
#define CRCO                (1<<2)
#define EN_CRC              (1<<3)
#define MASK_MAX_RT         (1<<4)
#define MASK_TX_DS          (1<<5)
#define MASK_RX_DR          (1<<6)
/*STATUS register bits*/
#define ST_TX_FULL          (1<<0)
#define MAX_RT              (1<<4)
#define TX_DS               (1<<5)
#define RX_DR               (1<<6)
/*FIFO_STATUS register bits*/
#define RX_EMPTY            (1<<0)
#define RX_FULL             (1<<1)
#define TX_EMPTY            (1<<4)
#define TX_FULL             (1<<5)
#define TX_REUSE            (1<<6)
/*timing definitions*/
#define POWONRST            (100/*ms*/) /*power on reset*/
#define TPD2STBY            (5/*ms*/)   /*powerdown to standby*/
/*avr SPI*/
#define DDR_SPI             DDRB
#define DD_CE               DDB0
#define DD_SSEL             DDB2
#define DD_MOSI             DDB3
#define DD_MISO             DDB4
#define DD_SCLK             DDB5
/*GPIO macros*/
#define CSN_LO()            PORTB &= ~(1<<DD_SSEL)
#define CSN_HI()            PORTB |=  (1<<DD_SSEL)
#define CE_LO()             PORTB &= ~(1<<DD_CE)
#define CE_HI()             PORTB |=  (1<<DD_CE)
uint8_t spi_read_byte()
{
    SPDR = 0xFF;
    while (!(SPSR & (1<<SPIF)));
    return SPDR;
}

uint8_t spi_write_byte(uint8_t w)
{
    SPDR = w;
    while (!(SPSR & (1<<SPIF)));
    return SPDR;
}

uint8_t nrf_read_reg(uint8_t reg)
{
    CSN_LO();
    spi_write_byte(R_REGISTER|reg);
    reg = spi_read_byte(); 
    CSN_HI();
    return reg;
}

void nrf_write_reg(uint8_t reg, uint8_t val)
{
    CSN_LO();
    spi_write_byte(W_REGISTER|reg);
    spi_write_byte(val);
    CSN_HI();
}

void nrf_write_multibyte_reg(uint8_t reg, uint8_t *buf, uint8_t len)
{
    CSN_LO();
    spi_write_byte(W_REGISTER|reg);
    while (len) {
        spi_write_byte(buf[len--]);
    }
    CSN_HI();
}

void nrf_read_multibyte_reg(uint8_t reg, uint8_t *buf, uint8_t len)
{
    CSN_LO();
    spi_write_byte(R_REGISTER|reg);
    while (len--) {
       *buf++ = spi_read_byte();
    }
    CSN_HI();
}

void nrf_init()
{
    /*set MOSI, SCLK, SSEL, CE output*/
    DDR_SPI = (1<<DD_MOSI)|(1<<DD_SCLK)|(1<<DD_SSEL)|(1<<DD_CE);
    CE_LO();
    CSN_HI();
    /*enable SPI, Master*/
    SPCR = (1<<SPE)|(1<<MSTR);
    delay_ms(POWONRST);
    /*init nordic chip*/
    nrf_write_reg(CONFIG, 0);
    nrf_write_reg(RF_SETUP, 0x08); //TODO
    nrf_enable_pipe(NRF_PIPE_ALL, false);
    nrf_enable_autoack(NRF_PIPE_ALL, false);
    nrf_write_reg(CONFIG, (MASK_TX_DS|MASK_RX_DR|MASK_MAX_RT|PWR_UP));
    delay_ms(TPD2STBY);
    /*should now enter standby mode*/
}

void nrf_clear_int()
{
    uint8_t st;
    st = nrf_read_reg(STATUS);
    nrf_write_reg(STATUS, (st | 0x70));
}

void nrf_set_operation_mode(nrf_operation_mode_t op_mode)
{
    uint8_t config;
    config = nrf_read_reg(CONFIG);
    switch (op_mode) {
        case NRF_PRIM_TX:
            nrf_write_reg(CONFIG, (config & ~PRIM_RX));
        break;
        case NRF_PRIM_RX:
            nrf_write_reg(CONFIG, (config | PRIM_RX));
        break;        
    }
}

void nrf_set_power_mode(nrf_power_mode_t pwr_mode)
{
    uint8_t config;
    config = nrf_read_reg(CONFIG);
    switch (pwr_mode) {
        case NRF_POWER_DOWN:
            nrf_write_reg(CONFIG, (config & ~PWR_UP)); 
        break;
        case NRF_POWER_UP:
            nrf_write_reg(CONFIG, (config | PWR_UP));
        break;        
    }
    delay_ms(TPD2STBY);
}

void nrf_enable_pipe(nrf_pipe_t pipe, bool enable)
{
    uint8_t pipes;
    pipes = nrf_read_reg(EN_RXADDR);
    if (enable) {
        pipes |= pipe;
    } else {
        pipes &= ~pipe;
    }
    nrf_write_reg(EN_RXADDR, pipes);
}

void nrf_enable_autoack(nrf_pipe_t pipe, bool enable)
{
    uint8_t pipes;
    pipes = nrf_read_reg(EN_AA);
    if (enable) {
        pipes |= pipe;
    } else {
        pipes &= ~pipe;
    }
    nrf_write_reg(EN_AA, pipes);
}

void nrf_set_pipe_width(nrf_pipe_t pipe, int pw)
{
    uint8_t reg=0;
    switch (pipe) {
        case NRF_PIPE_0:
            reg = RX_PW_P0;
            break;
        case NRF_PIPE_1:
            reg = RX_PW_P1;
            break;
        case NRF_PIPE_2:
            reg = RX_PW_P2;
            break;
        case NRF_PIPE_3:
            reg = RX_PW_P3;
            break;
        case NRF_PIPE_4:
            reg = RX_PW_P4;
            break;
        case NRF_PIPE_5:
            reg = RX_PW_P5;
            break;
        case NRF_PIPE_ALL:/*TODO*/
            break;
    }
    nrf_write_reg(reg, pw);
}

void nrf_set_tx_address(uint8_t *addr)
{
    nrf_write_multibyte_reg(TX_ADDR, addr, 5);
}

void nrf_flush_tx()
{
    CSN_LO();
    spi_write_byte(FLUSH_TX);
    CSN_HI();
}

void nrf_flush_rx()
{
    CSN_LO();
    spi_write_byte(FLUSH_RX);
    CSN_HI();
}

void nrf_send(char *buf, int len)
{
    nrf_set_operation_mode(NRF_PRIM_TX);
    CSN_LO();
    spi_write_byte(W_TX_PAYLOAD); /*load TX FIFO*/
    while (len--) {        
        spi_write_byte(*buf++);
    }
    CSN_HI();
    /*TX mode*/
    CE_HI();
    delay_ms(1); //10us
    CE_LO();
    while (!(nrf_read_reg(STATUS) & TX_DS) && !(nrf_read_reg(STATUS) & MAX_RT));
    nrf_clear_int();
}

void nrf_recv(char *buf, int len)
{
    nrf_set_operation_mode(NRF_PRIM_RX);
    CE_HI(); 
    /*wait for packet*/
    while (!(nrf_read_reg(STATUS) & RX_DR)); 
    CE_LO();
    CSN_LO();
    spi_write_byte(R_RX_PAYLOAD);
    while (len--) {
        *buf++ = spi_read_byte();
    }
    CSN_HI();
    nrf_clear_int();
}
