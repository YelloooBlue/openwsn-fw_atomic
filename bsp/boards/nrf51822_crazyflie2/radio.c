/**
* brief Nordic nRF52840-specific definition of the "radio" bsp module. 
*
* Authors: Tamas Harczos (1, tamas.harczos@imms.de) and Adam Sedmak (2, adam.sedmak@gmail.com)
* Company: (1) Institut fuer Mikroelektronik- und Mechatronik-Systeme gemeinnuetzige GmbH (IMMS GmbH)
*          (2) Faculty of Electronics and Computing, Zagreb, Croatia
*    Date: June 2018
*
* Author: Lan HUANG (YelloooBlue@Outlook.com) Apr 2024
*/

#include "radio.h"
#include "nrf51.h"
#include "nrf51_bitfields.h"
#include "debugpins.h"
#include "leds.h"

//=========================== defines =========================================

#define RADIO_POWER_POWER_POS       0

#define STATE_DISABLED              0
#define STATE_RXRU                  1
#define STATE_RXIDLE                2
#define STATE_RX                    3
#define STATE_RXDISABLE             4
#define STATE_TXTU                  9
#define STATE_TXIDLE                10
#define STATE_TX                    11
#define STATE_TXDIABLE              12

/* For calculating frequency */
#define FREQUENCY_OFFSET  10
#define FREQUENCY_STEP    5

#define SFD_OCTET                 (0xA7)      ///< start of frame delimiter of IEEE 802.15.4
#define MAX_PACKET_SIZE           (127)       ///< maximal size of radio packet (one more byte at the beginning needed to store the length)
#define CRC_POLYNOMIAL            (0x11021)   ///< polynomial used for CRC calculation in 802.15.4 frames (x^16 + x^12 + x^5 + 1)

#define WAIT_FOR_RADIO_DISABLE    (0)         ///< whether the driver shall wait until the radio is disabled upon calling radio_rfOff()
#define WAIT_FOR_RADIO_ENABLE     (1)         ///< whether the driver shall wait until the radio is enabled upon calling radio_txEnable() or radio_rxEnable()

#define RADIO_CRCINIT_24BIT       0x555555
#define RADIO_CRCPOLY_24BIT       0x0000065B  /// ref: https://devzone.nordicsemi.com/f/nordic-q-a/44111/crc-register-values-for-a-24-bit-crc

#define MAX_PAYLOAD_LENGTH        (127)
#define INTERFRAM_SPACING         (150)       // in us

#define BLE_ACCESS_ADDR           0x8E89BED6  // the actual address is 0xD6, 0xBE, 0x89, 0x8E


// Pins if RFX2411N is used
#define RADIO_PA_RX_EN  20
#define RADIO_PA_ANT_SW 18
#define RADIO_PA_MODE   19
#define RADIO_PAEN_PIN 19
//#define RADIO_PATX_DIS_PIN 20


//=========================== variables =======================================

typedef struct {
   radio_capture_cbt   startFrame_cb;
   radio_capture_cbt   endFrame_cb;
   radio_state_t       state; 
   uint8_t             payload[1+MAX_PACKET_SIZE] __attribute__ ((aligned));
   int8_t              rssi_sample;
//  volatile bool event_ready;
} radio_vars_t;

static radio_vars_t radio_vars;

//=========================== prototypes ======================================

static uint8_t  ble_channel_to_frequency(uint8_t channel);

//=========================== public ==========================================

void radio_init(void) {

   // clear internal variables
   memset(&radio_vars, 0, sizeof(radio_vars));

   // set radio configuration parameters
   NRF_RADIO->TXPOWER   = (RADIO_TXPOWER_TXPOWER_Pos4dBm << RADIO_TXPOWER_TXPOWER_Pos);

   // =====BLE================================================================================================================
   // BRIEF: Since nRF51 does not support ieee802.15.4, radio is implemented using the physical layer of BLE
   
   NRF_RADIO->PCNF0 =   (((1UL) << RADIO_PCNF0_S0LEN_Pos) & RADIO_PCNF0_S0LEN_Msk) | 
                        (((0UL) << RADIO_PCNF0_S1LEN_Pos) & RADIO_PCNF0_S1LEN_Msk) |
                        (((8UL) << RADIO_PCNF0_LFLEN_Pos) & RADIO_PCNF0_LFLEN_Msk);

   NRF_RADIO->PCNF1 =   (((RADIO_PCNF1_ENDIAN_Little)    << RADIO_PCNF1_ENDIAN_Pos)  & RADIO_PCNF1_ENDIAN_Msk)  |
                        (((3UL)                          << RADIO_PCNF1_BALEN_Pos)   & RADIO_PCNF1_BALEN_Msk)   |
                        (((0UL)                          << RADIO_PCNF1_STATLEN_Pos) & RADIO_PCNF1_STATLEN_Msk) |
                        ((((uint32_t)MAX_PAYLOAD_LENGTH) << RADIO_PCNF1_MAXLEN_Pos)  & RADIO_PCNF1_MAXLEN_Msk)  |
                        ((RADIO_PCNF1_WHITEEN_Enabled    << RADIO_PCNF1_WHITEEN_Pos) & RADIO_PCNF1_WHITEEN_Msk);
   
   NRF_RADIO->CRCPOLY      = RADIO_CRCPOLY_24BIT;
   NRF_RADIO->CRCCNF       = (
                               ((RADIO_CRCCNF_SKIPADDR_Skip) << RADIO_CRCCNF_SKIPADDR_Pos) & RADIO_CRCCNF_SKIPADDR_Msk) |
                               (((RADIO_CRCCNF_LEN_Three)    << RADIO_CRCCNF_LEN_Pos)      & RADIO_CRCCNF_LEN_Msk
                             );
   NRF_RADIO->CRCINIT      = RADIO_CRCINIT_24BIT;

   NRF_RADIO->TXADDRESS    = 0;
   NRF_RADIO->RXADDRESSES  = 1;

   NRF_RADIO->MODE         = ((RADIO_MODE_MODE_Ble_1Mbit) << RADIO_MODE_MODE_Pos) & RADIO_MODE_MODE_Msk;
   NRF_RADIO->TIFS         = INTERFRAM_SPACING;
   NRF_RADIO->PREFIX0      = ((BLE_ACCESS_ADDR & 0xff000000) >> 24);
   NRF_RADIO->BASE0        = ((BLE_ACCESS_ADDR & 0x00ffffff) << 8 );

   // =====BLE_END============================================================================================================

   // =====RFX2411N===========================================================================================================
   /** 
    * The following operations will be done in the powermanager(pm.c) too.
    * But sometimes, the radio will be used without the pm.c file, so it is necessary to enable the PA here.
    */

   // RFX2411N pins
   NRF_GPIO->DIRSET = 1<<RADIO_PA_RX_EN;
   NRF_GPIO->DIRSET = 1<<RADIO_PA_MODE;
   NRF_GPIO->DIRSET = 1<<RADIO_PA_ANT_SW;

   // Enable PA
   NRF_GPIO->OUTSET = 1<<RADIO_PA_RX_EN;

   // Select antenna
   NRF_GPIO->OUTSET = 1<<RADIO_PA_ANT_SW;    //chip antenna A
   //NRF_GPIO->OUTCLR = 1<<RADIO_PA_ANT_SW;  //extra antenna B

   // Selecte mode
   NRF_GPIO->OUTCLR = 1<<RADIO_PA_MODE;    //Disable BYPASS mode
   //NRF_GPIO->OUTSET = 1<<RADIO_PA_MODE;  //Enable BYPASS mode

   // =====RFX2411N_END=======================================================================================================

   // ===============

   // set payload pointer
   NRF_RADIO->PACKETPTR = (uint32_t)(radio_vars.payload);

   // set up interrupts
   // disable radio interrupt
   NVIC_DisableIRQ(RADIO_IRQn);

   NRF_RADIO->INTENSET = (RADIO_INTENSET_ADDRESS_Enabled << RADIO_INTENSET_ADDRESS_Pos) |
                         (RADIO_INTENSET_END_Enabled        << RADIO_INTENSET_END_Pos); 

   NVIC_SetPriority(RADIO_IRQn, RADIO_PRIORITY);

   NVIC_ClearPendingIRQ(RADIO_IRQn);
   NVIC_EnableIRQ(RADIO_IRQn);
}


void radio_setStartFrameCb(radio_capture_cbt cb) {

   radio_vars.startFrame_cb  = cb;
}


void radio_setEndFrameCb(radio_capture_cbt cb) {

   radio_vars.endFrame_cb = cb;
}


void radio_reset(void) {

   // reset is implemented by power off and power radio
   NRF_RADIO->POWER = ((uint32_t)(0)) << RADIO_POWER_POWER_POS;
   NRF_RADIO->POWER = ((uint32_t)(1)) << RADIO_POWER_POWER_POS;

   radio_vars.state  = RADIOSTATE_STOPPED;
}


void radio_setFrequency(uint8_t frequency, radio_freq_t tx_or_rx) {

   // =====BLE================================================================================================================
   // BRIEF: Since nRF51 does not support ieee802.15.4, radio is implemented using the physical layer of BLE

   // OpenWSN calls this function with frequency between 11 and 26, But BLE uses 0-36 (37-39 are advertising channels)
   // MAYBE BETTER? , but not tested
   //frequency = (frequency - 11) * 2;  
   /*
      // Channels: IEEE802.15.4(11-26) => (0-15) => BLE(0,2,4...30)
      // Step: IEEE802.15.4(5) => BLE(2)*2 = 4 MHz
   */
   
   NRF_RADIO->FREQUENCY   = ble_channel_to_frequency(frequency);
   NRF_RADIO->DATAWHITEIV = frequency; 

   radio_vars.state  = RADIOSTATE_FREQUENCY_SET;

   // =====BLE_END============================================================================================================
}


int8_t radio_getFrequencyOffset(void){
 
   return 0; 
}


void radio_rfOn(void) {
   // power on radio
   NRF_RADIO->POWER = ((uint32_t)(1)) << 0;

   radio_vars.state = RADIOSTATE_STOPPED;
}


void radio_rfOff(void) {

   radio_vars.state  = RADIOSTATE_TURNING_OFF;

   NRF_RADIO->EVENTS_DISABLED = 0;

   // stop radio
   NRF_RADIO->TASKS_DISABLE = (uint32_t)(1);

   while(NRF_RADIO->EVENTS_DISABLED==0);

   leds_radio_off();
   debugpins_radio_clr();

   radio_vars.state  = RADIOSTATE_RFOFF;
}


void radio_loadPacket(uint8_t* packet, uint16_t len) {

   radio_vars.state  = RADIOSTATE_LOADING_PACKET;

   ///< note: 1st byte should be the payload size (for Nordic), and
   ///   the two last bytes are used by the MAC layer for CRC
   if ((len > 0) && (len <= MAX_PACKET_SIZE)) {
       radio_vars.payload[0]= len;
       memcpy(&radio_vars.payload[1], packet, len);
   }

   // (re)set payload pointer
   NRF_RADIO->PACKETPTR = (uint32_t)(radio_vars.payload);

   radio_vars.state  = RADIOSTATE_PACKET_LOADED;
}


void radio_txEnable(void) {

   radio_vars.state  = RADIOSTATE_ENABLING_TX;

   NRF_RADIO->EVENTS_READY = (uint32_t)0;

   NRF_RADIO->TASKS_TXEN = (uint32_t)1;
   while(NRF_RADIO->EVENTS_READY==0);

   // wiggle debug pin
   debugpins_radio_set();
   leds_radio_on();

   radio_vars.state  = RADIOSTATE_TX_ENABLED;
}


void radio_txNow(void) {

   NRF_RADIO->TASKS_START = (uint32_t)1;

   radio_vars.state = RADIOSTATE_TRANSMITTING;
}


void radio_rxEnable(void) {

   radio_vars.state = RADIOSTATE_ENABLING_RX;

   if (NRF_RADIO->STATE != STATE_RX) {

       // turn off radio first
       radio_rfOff();

       NRF_RADIO->EVENTS_READY = (uint32_t)0;

       NRF_RADIO->TASKS_RXEN  = (uint32_t)1;

       while(NRF_RADIO->EVENTS_READY==0);
   }
}


void radio_rxNow(void) {

   NRF_RADIO->TASKS_START = (uint32_t)1;

   debugpins_radio_set();
   leds_radio_on();

   radio_vars.state  = RADIOSTATE_LISTENING;
}


void radio_getReceivedFrame(uint8_t* pBufRead,
                           uint8_t* pLenRead,
                           uint8_t  maxBufLen,
                            int8_t* pRssi,
                           uint8_t* pLqi,
                              bool* pCrc)
{
   // check for length parameter; if too long, payload won't fit into memory
   uint8_t len;

   len = radio_vars.payload[0];

   if (len == 0) {
       return; 
   }

   if (len > MAX_PACKET_SIZE) { 
       len = MAX_PACKET_SIZE; 
   }

   if (len > maxBufLen) { 
       len = maxBufLen; 
   }

   // copy payload
   memcpy(pBufRead, &radio_vars.payload[1], len);

   // store other parameters
   *pLenRead = len;
   *pLqi = radio_vars.payload[radio_vars.payload[0]-1];

   *pRssi = (int8_t)(0-NRF_RADIO->RSSISAMPLE);

   *pCrc = (NRF_RADIO->CRCSTATUS == 1U);

}

//=========================== private =========================================

static uint8_t ble_channel_to_frequency(uint8_t channel) {

   uint8_t frequency;
   
   if (channel<=10) {

       frequency = 4+2*channel;
   } else {
       if (channel >=11 && channel <=36) {
           
           frequency = 28+2*(channel-11);
       } else {
           switch(channel){
               case 37:
                   frequency = 2;
               break;
               case 38:
                   frequency = 26;
               break;
               case 39:
                   frequency = 80;
               break;
               default:
                   // something goes wrong
                   frequency = 2;

           }
       }
   }

   return frequency;
}

//=========================== callbacks =======================================

kick_scheduler_t radio_isr(void){

   uint32_t time_stampe;

   time_stampe = NRF_RTC0->COUNTER;

   // start of frame (payload)
   if (NRF_RADIO->EVENTS_ADDRESS){                          //===nRF51===
   //if (NRF_RADIO->EVENTS_FRAMESTART){

       // start sampling rssi
       NRF_RADIO->TASKS_RSSISTART = (uint32_t)1;

       if (radio_vars.startFrame_cb!=NULL){
           radio_vars.startFrame_cb(time_stampe);
       }
       
       //NRF_RADIO->EVENTS_FRAMESTART = (uint32_t)0;
       NRF_RADIO->EVENTS_ADDRESS = (uint32_t)0;             //===nRF51===
       return KICK_SCHEDULER;
   }

   // END 
   if (NRF_RADIO->EVENTS_END) {

       if (radio_vars.endFrame_cb!=NULL){
           radio_vars.endFrame_cb(time_stampe);
       }
       
       NRF_RADIO->EVENTS_END = (uint32_t)0;
       return KICK_SCHEDULER;
   }

   return DO_NOT_KICK_SCHEDULER;
}

//=========================== interrupt handlers ==============================

void RADIO_IRQHandler(void) {

   debugpins_isr_set();

   radio_isr();

   debugpins_isr_clr();
}
