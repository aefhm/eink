/*****************************************************************************
* | File        :   EPD_12in48.c
* | Author      :   Waveshare team
* | Function    :   Electronic paper driver
* | Info     :
*----------------
* | This version:   V1.0
* | Date     :   2018-11-29
* | Info     :
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files(the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#include "EPD_13in3e.h"
#include "FrameConsole.h"

// Local change: bound BUSY waits and expose refresh status.
static bool epd_ok = true;
bool EPD_13IN3E_Ok(void) { return epd_ok; }


// const UBYTE spiCsPin[2] = {
// 		SPI_CS0, SPI_CS1
// };
const UBYTE PSR_V[2] = {
	0xDF, 0x69
};
const UBYTE PWR_V[6] = {
	0x0F, 0x00, 0x28, 0x2C, 0x28, 0x38
};
const UBYTE POF_V[1] = {
	0x00
};
const UBYTE DRF_V[1] = {
	0x00
};
const UBYTE CDI_V[1] = {
	0xF7
};
const UBYTE TCON_V[2] = {
	0x03, 0x03
};
const UBYTE TRES_V[4] = {
	0x04, 0xB0, 0x03, 0x20
};
const UBYTE CMD66_V[6] = {
	0x49, 0x55, 0x13, 0x5D, 0x05, 0x10
};
const UBYTE EN_BUF_V[1] = {
	0x07
};
const UBYTE CCSET_V[1] = {
	0x01
};
const UBYTE PWS_V[1] = {
	0x22
};
const UBYTE AN_TM_V[9] = {
	0xC0, 0x1C, 0x1C, 0xCC, 0xCC, 0xCC, 0x15, 0x15, 0x55
};


const UBYTE AGID_V[1] = {
	0x10
};

const UBYTE BTST_P_V[2] = {
	0xE8, 0x28
};
const UBYTE BOOST_VDDP_EN_V[1] = {
	0x01
};
const UBYTE BTST_N_V[2] = {
	0xE8, 0x28
};
const UBYTE BUCK_BOOST_VDDN_V[1] = {
	0x01
};
const UBYTE TFT_VCOM_POWER_V[1] = {
	0x02
};


static void EPD_13IN3E_CS_ALL(UBYTE Value)
{
    DEV_Digital_Write(EPD_CS_M_PIN, Value);
    DEV_Digital_Write(EPD_CS_S_PIN, Value);
}


static void EPD_13IN3E_SPI_Sand(UBYTE Cmd, const UBYTE *buf, UDOUBLE Len)
{
    DEV_SPI_WriteByte(Cmd);
    DEV_SPI_Write_nByte(buf,Len);
}


/******************************************************************************
function :	Software reset
parameter:
******************************************************************************/
static void EPD_13IN3E_Reset(void)
{
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(30);
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(30);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(30);
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(30);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(30);
}

/******************************************************************************
function :	Wait until the busy_pin goes LOW
parameter:
******************************************************************************/
static bool EPD_13IN3E_ReadBusyH(void)
{
    const uint32_t started = millis();
	while(!DEV_Digital_Read(EPD_BUSY_PIN)) {      //LOW: busy, HIGH: idle
        if (millis() - started >= 120000UL) {
            epd_ok = false;
            return false;
        }
        DEV_Delay_ms(10);
    }
	DEV_Delay_ms(20);
    return true;
}


/******************************************************************************
function :  Turn On Display
parameter:
******************************************************************************/
static void EPD_13IN3E_TurnOnDisplay(void)
{
    FrameLog.printf("Write PON \r\n");
    EPD_13IN3E_CS_ALL(0);
    DEV_SPI_WriteByte(0x04); // POWER_ON
    EPD_13IN3E_CS_ALL(1);
    if (!EPD_13IN3E_ReadBusyH()) return;

    FrameLog.printf("Write DRF \r\n");
    DEV_Delay_ms(50);
    EPD_13IN3E_CS_ALL(0);
    EPD_13IN3E_SPI_Sand(DRF, DRF_V, sizeof(DRF_V));
    EPD_13IN3E_CS_ALL(1);
    if (!EPD_13IN3E_ReadBusyH()) return;

    FrameLog.printf("Write POF \r\n");
    EPD_13IN3E_CS_ALL(0);
    EPD_13IN3E_SPI_Sand(POF, POF_V, sizeof(POF_V));
    EPD_13IN3E_CS_ALL(1);
    if (!EPD_13IN3E_ReadBusyH()) return;
    FrameLog.printf("Display Done!! \r\n");
}

/******************************************************************************
function :	Initialize the e-Paper register
parameter:
******************************************************************************/
void EPD_13IN3E_Init(void)
{
    epd_ok = true;
	EPD_13IN3E_Reset();
//    if (!EPD_13IN3E_ReadBusyH()) return;

    DEV_Digital_Write(EPD_CS_M_PIN, 0);
	EPD_13IN3E_SPI_Sand(AN_TM, AN_TM_V, sizeof(AN_TM_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
	EPD_13IN3E_SPI_Sand(CMD66, CMD66_V, sizeof(CMD66_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
	EPD_13IN3E_SPI_Sand(PSR, PSR_V, sizeof(PSR_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
	EPD_13IN3E_SPI_Sand(CDI, CDI_V, sizeof(CDI_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
	EPD_13IN3E_SPI_Sand(TCON, TCON_V, sizeof(TCON_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
	EPD_13IN3E_SPI_Sand(AGID, AGID_V, sizeof(AGID_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
	EPD_13IN3E_SPI_Sand(PWS, PWS_V, sizeof(PWS_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
	EPD_13IN3E_SPI_Sand(CCSET, CCSET_V, sizeof(CCSET_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
	EPD_13IN3E_SPI_Sand(TRES, TRES_V, sizeof(TRES_V));
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_M_PIN, 0);
	EPD_13IN3E_SPI_Sand(PWR_epd, PWR_V, sizeof(PWR_V));
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_M_PIN, 0);
	EPD_13IN3E_SPI_Sand(EN_BUF, EN_BUF_V, sizeof(EN_BUF_V));
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_M_PIN, 0);
	EPD_13IN3E_SPI_Sand(BTST_P, BTST_P_V, sizeof(BTST_P_V));
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_M_PIN, 0);
	EPD_13IN3E_SPI_Sand(BOOST_VDDP_EN, BOOST_VDDP_EN_V, sizeof(BOOST_VDDP_EN_V));
    EPD_13IN3E_CS_ALL(1);
	
    DEV_Digital_Write(EPD_CS_M_PIN, 0);
	EPD_13IN3E_SPI_Sand(BTST_N, BTST_N_V, sizeof(BTST_N_V));
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_M_PIN, 0);
	EPD_13IN3E_SPI_Sand(BUCK_BOOST_VDDN, BUCK_BOOST_VDDN_V, sizeof(BUCK_BOOST_VDDN_V));
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_M_PIN, 0);
	EPD_13IN3E_SPI_Sand(TFT_VCOM_POWER, TFT_VCOM_POWER_V, sizeof(TFT_VCOM_POWER_V));
    EPD_13IN3E_CS_ALL(1);
    
}


void EPD_13IN3E_Display(const UBYTE *Image)
{
    UDOUBLE Width, Width1, Height;
    Width = (EPD_13IN3E_WIDTH % 2 == 0)? (EPD_13IN3E_WIDTH / 2 ): (EPD_13IN3E_WIDTH / 2 + 1);
    Width1 = (Width % 2 == 0)? (Width / 2 ): (Width / 2 + 1);
    Height = EPD_13IN3E_HEIGHT;
    
    DEV_Digital_Write(EPD_CS_M_PIN, 0);
    DEV_SPI_WriteByte(0x10);
    for(UDOUBLE i=0; i<Height; i++ )
    {
        DEV_SPI_Write_nByte(Image + i*Width,Width1);
        DEV_Delay_ms(1);
    }
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_S_PIN, 0);
    DEV_SPI_WriteByte(0x10);
    for(UDOUBLE i=0; i<Height; i++ )
    {
        DEV_SPI_Write_nByte(Image + i*Width + Width1,Width1);
        DEV_Delay_ms(1);
    }
       
    EPD_13IN3E_CS_ALL(1);
    
    EPD_13IN3E_TurnOnDisplay();
}


/******************************************************************************
function :  Enter sleep mode
parameter:
******************************************************************************/
void EPD_13IN3E_Sleep(void)
{
    EPD_13IN3E_CS_ALL(0);
    DEV_SPI_WriteByte(0x07); // DEEP_SLEEP
    DEV_SPI_WriteByte(0XA5);
    EPD_13IN3E_CS_ALL(1);
}
