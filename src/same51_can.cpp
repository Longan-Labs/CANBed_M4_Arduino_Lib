/**
 * @brief      The main class for SAME51 Canbus.
 * @details    Lots of extra SAMC21 remanants
 */

#include "same51_can.h"
#include "Arduino.h"

SAME51_CAN *same51_can_use_object[2];

struct CanQuantaValues {
    uint32_t speed;
    uint8_t prescale;
    uint8_t before_sp;
    uint8_t after_sp;
    uint8_t sync_jump;

    uint8_t prescale_fd;
    uint8_t before_sp_fd;
    uint8_t after_sp_fd;
    uint8_t sync_jump_fd;
};

static constexpr CanQuantaValues precalcQuantValues[] = {
    {CAN_5KBPS, 80, 188, 62, 3},
    {CAN_10KBPS, 40, 188, 62, 3},
    {CAN_20KBPS, 20, 188, 62, 3},
    {CAN_31K25BPS, 10, 240, 80, 3},
    {CAN_33K3BPS, 10, 225, 75, 3},
    {CAN_40KBPS, 10, 188, 62, 3},
    {CAN_50KBPS, 8, 188, 62, 3},
    {CAN_80KBPS, 5, 188, 62, 3},
    {CAN_100KBPS, 4, 188, 62, 3},
    {CAN_125KBPS, 4, 150, 50, 3},
    {CAN_200KBPS, 2, 188, 62, 3},
    {CAN_250KBPS, 2, 150, 50, 3},
    {CAN_500KBPS, 1, 150, 50, 3},
    {CAN_1000KBPS, 1, 75, 25, 3},
    {CAN_125K_500K, 4, 150, 50, 3, 5, 30, 10, 3},
    {CAN_250K_500K, 2, 150, 50, 3, 5, 30, 10, 3},
    {CAN_250K_1M, 2, 150, 50, 3, 4, 19, 6, 3},
    {CAN_250K_2M, 2, 150, 50, 3, 2, 19, 6, 3},
    {CAN_250K_4M, 2, 150, 50, 3, 1, 19, 6, 3},
    {CAN_500K_1M, 1, 150, 50, 3, 4, 19, 6, 3},
    {CAN_500K_2M, 1, 150, 50, 3, 2, 19, 6, 3},
    {CAN_500K_4M, 1, 150, 50, 3, 1, 19, 6, 3},
    {CAN_1000K_4M, 1, 75, 25, 3, 1, 19, 6, 3},
    {0}
};

static constexpr CanQuantaValues defaultQuantValues = {
    speed: 0,
    prescale: 0,
    before_sp: 10 + 2,
    after_sp: 3 + 1,
    sync_jump: 3 + 1,
    prescale_fd: 0,
    before_sp_fd: 3,
    after_sp_fd: 1,
    sync_jump_fd: 3 + 1,
};

static inline const CanQuantaValues& findQuantValues(uint32_t speed)
{
    for(int idx = 0; precalcQuantValues[idx].speed != 0; idx++)
    {
        if (precalcQuantValues[idx].speed == speed)
        {
            return precalcQuantValues[idx];
        }
    }
    return defaultQuantValues;
}

/**
* @brief Starts Canbus communication using the supplied paramaters
*
* This function returns OK or with errors
*
* @return uint8_t Result
*/

uint8_t SAME51_CAN::begin(uint8_t idmodeset, uint32_t speedset, enum mcan_can_mode canmode)
{
    uint8_t ret;

    flg_fd = (canmode == MCAN_MODE_EXT_LEN_CONST_RATE);
    
    //Serial.print("flg_fd = ");
    //Serial.println(flg_fd);

    rx_ded_buffer_data = false;
    _canid = ID_CAN0;
    _cantx = 22;
    _canrx = 23;
    _group = 0;
    _idmode = idmodeset;
    
    if (_canid == ID_CAN0) {
        same51_can_use_object[0] = this;
    }
    
    
    if ((_canid != ID_CAN0)) {
        return CAN_FAIL;  // Don't know what this is
    }
    const CanQuantaValues& quantValues = findQuantValues(speedset);
    const struct mcan_config mcan_cfg = {
        id : _canid,
        regs : CAN0,
        msg_ram :
        mcan_msg_ram,

        array_size_filt_std :
        RAM_ARRAY_SIZE_FILT_STD,
        array_size_filt_ext :
        RAM_ARRAY_SIZE_FILT_EXT,
        fifo_size_rx0 :
        RAM_FIFO_SIZE_RX0,
        fifo_size_rx1 : 0,
        array_size_rx :
        RAM_ARRAY_SIZE_RX,
        fifo_size_tx_evt : 0,
        array_size_tx :
        RAM_ARRAY_SIZE_TX,
        fifo_size_tx :
        RAM_FIFO_SIZE_TX,

        buf_size_rx_fifo0 : 64,
        buf_size_rx_fifo1 : 0,
        buf_size_rx : 64,
        buf_size_tx : 64,

        /*
        using values from AT6493 (SAMC21 app note); the plus values are to add on what the MCAN driver subtracts back off
        */
        bit_rate : speedset,
        quanta_prescale: quantValues.prescale,
        quanta_before_sp : quantValues.before_sp + 2,   //10 + 2,
        quanta_after_sp : quantValues.after_sp + 1,     //3 + 1,

        /*
        AT6493 (SAMC21 app note) 'fast' values were unhelpfully the same as normal speed; these are for double (1MBit)
                the maximum peripheral clock of 48MHz on the SAMC21 does restrict us from very high rates
        */
        bit_rate_fd : speedset,
        quanta_prescale_fd: quantValues.prescale_fd,
        quanta_before_sp_fd : quantValues.before_sp_fd + 2,     //3,
        quanta_after_sp_fd : quantValues.after_sp_fd + 1,       //1,

        quanta_sync_jump : quantValues.sync_jump + 1,           //3 + 1,
        quanta_sync_jump_fd : quantValues.sync_jump_fd + 1,     //3 + 1,
    };
    PORT->Group[_group].DIRSET.reg = (1 << _cantx);
    PORT->Group[_group].DIRCLR.reg = (1 << _canrx);
    PORT->Group[_group].PINCFG[_cantx].reg = PORT_PINCFG_INEN | PORT_PINCFG_PMUXEN;
    PORT->Group[_group].PINCFG[_canrx].reg = PORT_PINCFG_INEN | PORT_PINCFG_PMUXEN;
    PORT->Group[_group].PMUX[_cantx / 2].reg = PORT_PMUX_PMUXE(8 /* CAN0 I */) | PORT_PMUX_PMUXO(8 /* CAN0 I */); /* have to write odd and even at once */ //SAME51G19A appears to use 8
    switch (mcan_cfg.id) {
        case ID_CAN0:
            GCLK->PCHCTRL[CAN0_GCLK_ID].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK1; // SAMC21 was GCLK0/48MHz, on SAME51 (as conf.) GCLK1 is 48MHz
            MCLK->AHBMASK.reg |= MCLK_AHBMASK_CAN0;
            //NVIC_EnableIRQ(CAN0_IRQn);
            break;
        default:
            break;
    }
    if (mcan_configure_msg_ram(&mcan_cfg, &mcan_msg_ram_size)) {
        //// Serial.println("RAM configuration succeeded");
    } else {
        return CAN_FAIL;
    }
    ret = mcan_initialize(&mcan, &mcan_cfg);
    if (ret == 0) {
        //// Serial.println("CAN initialized");
    } else {
        //// Serial.print("CAN init failed, code ");
        //// Serial.println(ret);
        return CAN_FAIL;
    }
    mcan_set_tx_queue_mode(&mcan);
    if (_mode == MCP_LOOPBACK) {
        mcan_loopback_on(&mcan);
    } else {
        mcan_loopback_off(&mcan);
    }
    mcan_set_mode(&mcan, canmode);
    mcan_enable(&mcan);
    // Enable chip standby
    //pinMode(_cs, OUTPUT);
    //digitalWrite(_cs, LOW);
    //mcan_enable_rx_array_flag(&mcan, 0);
    // MCP_ANY means filters don't matter
    if (_idmode == MCP_ANY) {
        init_Mask(FILTER_0, (CAN_EXT_MSG_ID | MSG_ID_ALLOW_ALL_MASK));
        init_Mask(FILTER_1, (CAN_STD_MSG_ID | MSG_ID_ALLOW_ALL_MASK));
    }
    if (mcan_is_enabled(&mcan)) {
        // Serial.println("MCAN is enabled!");
        return CAN_OK;
    }
    // Serial.println("Something went wrong!!!!!!!!");
    return CAN_FAIL;
};


uint8_t SAME51_CAN::begin_fd(uint8_t idmodeset, uint32_t speedset, enum mcan_can_mode canmode)
{
    uint8_t ret;
    
    flg_fd = (canmode == MCAN_MODE_EXT_LEN_CONST_RATE);
    
    unsigned long speed1[14] = {0, 125000, 250000, 250000, 250000, 250000,
                                   250000, 250000, 250000, 500000, 500000,
                                   500000, 500000, 1000000,
                               };
        
    unsigned long speed2[14] = {0, 500000, 500000, 750000, 1000000, 1500000,
                                   2000000, 3000000, 4000000, 1000000, 2000000,
                                   3000000, 4000000, 4000000,
                               };
                                   
    unsigned long speedset1 = speed1[speedset];
    unsigned long speedset2 = speed2[speedset];
        

    rx_ded_buffer_data = false;
    _canid = ID_CAN0;
    _cantx = 22;
    _canrx = 23;
    _group = 0;
    _idmode = idmodeset;
    
    if (_canid == ID_CAN0) {
        same51_can_use_object[0] = this;
    }
    
    
    if ((_canid != ID_CAN0)) {
        return CAN_FAIL;  // Don't know what this is
    }
    const CanQuantaValues& quantValues = findQuantValues(speedset);
    const struct mcan_config mcan_cfg = {

        id : _canid,
        regs : CAN0,
        msg_ram :
        mcan_msg_ram,

        array_size_filt_std :
        RAM_ARRAY_SIZE_FILT_STD,
        array_size_filt_ext :
        RAM_ARRAY_SIZE_FILT_EXT,
        fifo_size_rx0 :
        RAM_FIFO_SIZE_RX0,
        fifo_size_rx1 : 0,
        array_size_rx :
        RAM_ARRAY_SIZE_RX,
        fifo_size_tx_evt : 0,
        array_size_tx :
        RAM_ARRAY_SIZE_TX,
        fifo_size_tx :
        RAM_FIFO_SIZE_TX,

        buf_size_rx_fifo0 : 64,
        buf_size_rx_fifo1 : 0,
        buf_size_rx : 64,
        buf_size_tx : 64,

        /*
        using values from AT6493 (SAMC21 app note); the plus values are to add on what the MCAN driver subtracts back off
        */
        

        bit_rate : speedset1,
        quanta_prescale: quantValues.prescale,
        quanta_before_sp : quantValues.before_sp + 2,   //10 + 2,
        quanta_after_sp : quantValues.after_sp + 1,     //3 + 1,

        /*
        AT6493 (SAMC21 app note) 'fast' values were unhelpfully the same as normal speed; these are for double (1MBit)
                the maximum peripheral clock of 48MHz on the SAMC21 does restrict us from very high rates
        */
        bit_rate_fd : speedset2,
        quanta_prescale_fd: quantValues.prescale_fd,
        quanta_before_sp_fd : quantValues.before_sp_fd + 2,     //3,
        quanta_after_sp_fd : quantValues.after_sp_fd + 1,       //1,

        quanta_sync_jump : quantValues.sync_jump + 1,           //3 + 1,
        quanta_sync_jump_fd : quantValues.sync_jump_fd + 1,     //3 + 1,
    };
    PORT->Group[_group].DIRSET.reg = (1 << _cantx);
    PORT->Group[_group].DIRCLR.reg = (1 << _canrx);
    PORT->Group[_group].PINCFG[_cantx].reg = PORT_PINCFG_INEN | PORT_PINCFG_PMUXEN;
    PORT->Group[_group].PINCFG[_canrx].reg = PORT_PINCFG_INEN | PORT_PINCFG_PMUXEN;
    PORT->Group[_group].PMUX[_cantx / 2].reg = PORT_PMUX_PMUXE(8 /* CAN0 I */) | PORT_PMUX_PMUXO(8 /* CAN0 I */); /* have to write odd and even at once */ //SAME51G19A appears to use 8
    switch (mcan_cfg.id) {
        case ID_CAN0:
            GCLK->PCHCTRL[CAN0_GCLK_ID].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK1; // SAMC21 was GCLK0/48MHz, on SAME51 (as conf.) GCLK1 is 48MHz
            MCLK->AHBMASK.reg |= MCLK_AHBMASK_CAN0;
            //NVIC_EnableIRQ(CAN0_IRQn);
            break;
        default:
            break;
    }
    if (mcan_configure_msg_ram(&mcan_cfg, &mcan_msg_ram_size)) {
        // Serial.println("RAM configuration succeeded");
    } else {
        return CAN_FAIL;
    }
    ret = mcan_initialize(&mcan, &mcan_cfg);
    if (ret == 0) {
        // Serial.println("CAN initialized");
    } else {
        // Serial.print("CAN init failed, code ");
        // Serial.println(ret);
        return CAN_FAIL;
    }
    mcan_set_tx_queue_mode(&mcan);
    if (_mode == MCP_LOOPBACK) {
        mcan_loopback_on(&mcan);
    } else {
        mcan_loopback_off(&mcan);
    }
    mcan_set_mode(&mcan, canmode);
    mcan_enable(&mcan);
    // Enable chip standby
    //pinMode(_cs, OUTPUT);
    //digitalWrite(_cs, LOW);
    //mcan_enable_rx_array_flag(&mcan, 0);
    // MCP_ANY means filters don't matter
    if (_idmode == MCP_ANY) {
        init_Mask(FILTER_0, (CAN_EXT_MSG_ID | MSG_ID_ALLOW_ALL_MASK));
        init_Mask(FILTER_1, (CAN_STD_MSG_ID | MSG_ID_ALLOW_ALL_MASK));
    }
    if (mcan_is_enabled(&mcan)) {
        // Serial.println("MCAN is enabled!");
        return CAN_OK;
    }
    // Serial.println("Something went wrong!!!!!!!!");
    return CAN_FAIL;
}


uint8_t SAME51_CAN::init_Mask(uint8_t num, uint8_t ext, uint32_t ulData)
{
    uint32_t id;
    if (ext) {
        id = 0x1fffffff;
        ulData &= id;
        id |= CAN_EXT_MSG_ID;
        if (ext >= mcan.cfg.array_size_filt_ext) {
            return MCP2515_FAIL;
        }
    } else {
        id = 0x7ff;
        ulData &= id;
        id |= CAN_STD_MSG_ID;
        if (ext >= mcan.cfg.array_size_filt_std) {
            return MCP2515_FAIL;
        }
    }
    mcan_filter_id_mask(&mcan, 0, num, id, ulData);
    return MCP2515_OK;
};              // Initilize Mask(s)

uint8_t SAME51_CAN::init_Mask(uint8_t num, uint32_t ulData)
{
    return init_Mask(num, ((ulData & CAN_EXT_MSG_ID) == CAN_EXT_MSG_ID), ulData);
};                          // Initilize Mask(s)

uint8_t SAME51_CAN::init_Filt(uint8_t num, uint8_t ext, uint32_t ulData)
{
    uint32_t mask;
    if (ext) {
        mask = 0x1fffffff;
        if (ext >= mcan.cfg.array_size_filt_ext) {
            return MCP2515_FAIL;
        }
    } else {
        mask = 0x7ff;
        if (ext >= mcan.cfg.array_size_filt_std) {
            return MCP2515_FAIL;
        }
    }
    ulData &= mask;
    mcan_filter_id_mask(&mcan, 0, num, ulData, mask);
    return MCP2515_OK;
};              // Initilize Filter(s)

uint8_t SAME51_CAN::init_Filt(uint8_t num, uint32_t ulData)
{
    return init_Filt(num, ((ulData & CAN_EXT_MSG_ID) == CAN_EXT_MSG_ID), ulData);
}; // Initilize Filter(s)

uint8_t SAME51_CAN::setMode(uint8_t opMode)
{
    if ((opMode == MCP_LOOPBACK) && (_mode != MCP_LOOPBACK)) {
        _mode = opMode;
        mcan_disable(&mcan);
        mcan_reconfigure(&mcan);
        mcan_loopback_on(&mcan);
        mcan_enable(&mcan);
    } else if ((opMode == MCP_NORMAL) && (_mode != MCP_NORMAL)) {
        _mode = opMode;
        mcan_disable(&mcan);
        mcan_reconfigure(&mcan);
        mcan_loopback_off(&mcan);
        mcan_enable(&mcan);
    } else {
        return MCP2515_FAIL;
    }
    return MCP2515_OK;
};                                        // Set operational mode

uint8_t SAME51_CAN::sendMsgBuf(uint32_t id, uint8_t ext, uint8_t len, uint8_t *buf)
{
    uint8_t ret = 0xff;
    if (!mcan_is_enabled(&mcan)) {
        return CAN_CTRLERROR;
    }
    if (ext) {
        id |= CAN_EXT_MSG_ID;
    }
    ret = mcan_enqueue_outgoing_msg(0, &mcan, id, len, buf);
    if (ret != 0xFF) {
        return CAN_OK;
    }
    return CAN_FAILTX;
};      // Send message to transmit buffer

uint8_t SAME51_CAN::sendMsgBuf(uint32_t id, uint8_t len, uint8_t *buf)
{
    return sendMsgBuf(id, 1, len, buf);
};                 // Send message to transmit buffer

uint8_t SAME51_CAN::sendFdMsgBuf(uint32_t id, uint8_t ext, uint8_t len, uint8_t *buf)
{
    uint8_t ret = 0xff;
    if (!mcan_is_enabled(&mcan)) {
        return CAN_CTRLERROR;
    }
    if (ext) {
        id |= CAN_EXT_MSG_ID;
    }
    ret = mcan_enqueue_outgoing_msg(1, &mcan, id, len, buf);
    if (ret != 0xFF) {
        return CAN_OK;
    }
    return CAN_FAILTX;
};      // Send message to transmit buffer

uint8_t SAME51_CAN::sendFdMsgBuf(uint32_t id, uint8_t len, uint8_t *buf)
{
    return sendFdMsgBuf(id, 1, len, buf);
};        

uint8_t SAME51_CAN::readMsgBuf(uint32_t *id, uint8_t *ext, uint8_t *len, uint8_t *buf)
{
    struct mcan_msg_info msg;
    msg.data = buf;
    msg.data_len = 64;
    uint8_t fifo_entries;
    if (mcan_is_tx_complete(&mcan)) {
        mcan_clear_tx_flag(&mcan);
    }
    fifo_entries = mcan_dequeue_received_msg(&mcan, 0, &msg);
    if (fifo_entries > 0) {
        *id = mcan_get_id(msg.id);
        *len = msg.data_len;
        *ext = (msg.id & CAN_EXT_MSG_ID) == CAN_EXT_MSG_ID;
        return CAN_OK;
    }
    return CAN_NOMSG;
};   // Read message from receive buffer

uint8_t SAME51_CAN::readMsgBuf(uint32_t *id, uint8_t *len, uint8_t *buf)
{
    uint8_t ext;
    return readMsgBuf(id, &ext, len, buf);
};               // Read message from receive buffer

uint8_t SAME51_CAN::checkReceive(void)
{
    return (uint8_t)mcan_rx_fifo_data(&mcan, 0);
};                                           // Check for received data

uint8_t SAME51_CAN::checkError(void)
{
    return 0;
};                                             // Check for errors

uint8_t SAME51_CAN::getError(void)
{
    return 0;
};                                               // Check for errors

uint8_t SAME51_CAN::errorCountRX(void)
{
    return 0;
};                                           // Get error count

uint8_t SAME51_CAN::errorCountTX(void)
{
    return 0;
};                                           // Get error count

uint8_t SAME51_CAN::enOneShotTX(void)
{
    return 0;
};                                            // Enable one-shot transmission

uint8_t SAME51_CAN::disOneShotTX(void)
{
    return 0;
};                                           // Disable one-shot transmission

/*
void CAN0_Handler(void)
{
    // Serial.println("A");
    if (mcan_rx_array_data(&(use_object->mcan))) {
        mcan_clear_rx_array_flag(&(use_object->mcan));
        use_object->rx_ded_buffer_data = true;
        // Serial.println("Got a Packet 0!");
    }
}

*/


