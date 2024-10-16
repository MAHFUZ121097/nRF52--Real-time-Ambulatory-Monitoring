/**
 * Copyright (c) 2014 - 2018, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 * Peripheral: SAADC
 * Compatibility: nRF52832 rev 2/nRF52840 rev 1, SDK 15.2.0
 * Softdevice used: S132 v6.1.0/S140 v6.1.0
 *
 * This SAADC example samples on 4 different input pins, and enables scan mode to do that. It is otherwise an
 * offsprint from the standard ble_app_uart example available in nRF5 SDK 15.2.0
 * Works together with softdevice S132 v6.1.0 on nRF52832 and S140 v6.1.0 on nRF52840
 * Transmits SAADC output to hardware UART and over BLE via Nordic UART Servive (NUS).
 * Info on NUS -> http://infocenter.nordicsemi.com/topic/com.nordic.infocenter.sdk5.v13.0.0/ble_sdk_app_nus_eval.html?cp=4_0_0_4_1_2_17
 * Info on hardware UART settings -> http://infocenter.nordicsemi.com/topic/com.nordic.infocenter.sdk5.v13.0.0/uart_example.html?cp=4_0_0_4_4_41
 */


#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "app_timer.h"
#include "app_error.h"
#include "ble_nus.h"
#include "app_uart.h"
#include "app_util_platform.h"
#include "bsp_btn_ble.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_drv_saadc.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_timer.h"
#include "nrf_drv_twi.h"
#include "mpu6050s.h"
#include "Si7021.h"
#include "MAX30100_3.h"
#include "math.h"

#if defined (UART_PRESENT)
#include "nrf_uart.h"
#endif
#if defined (UARTE_PRESENT)
#include "nrf_uarte.h"
#endif

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define APP_BLE_CONN_CFG_TAG            1                                           /**< A tag identifying the SoftDevice BLE configuration. */

#define DEVICE_NAME                     "EX_CPS_EIT1"                               /**< Name of device. Will be included in the advertising data. */
#define NUS_SERVICE_UUID_TYPE           BLE_UUID_TYPE_VENDOR_BEGIN                  /**< UUID type for the Nordic UART Service (vendor specific). */

#define APP_BLE_OBSERVER_PRIO           3                                           /**< Application's BLE observer priority. You shouldn't need to modify this value. */

#define APP_ADV_INTERVAL                64                                          /**< The advertising interval (in units of 0.625 ms. This value corresponds to 40 ms). */

#define APP_ADV_DURATION                18000                                       /**< The advertising duration (180 seconds) in units of 10 milliseconds. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(20, UNIT_1_25_MS)             /**< Minimum acceptable connection interval (20 ms), Connection interval uses 1.25 ms units. */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(50, UNIT_1_25_MS)             /**< Maximum acceptable connection interval (75 ms), Connection interval uses 1.25 ms units. */
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units. */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                       /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                      /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define UART_TX_BUF_SIZE                256                                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                256                                         /**< UART RX buffer size. */

#define SAADC_SAMPLES_IN_BUFFER         48
#define SAADC_SAMPLE_RATE               1                                         /**< SAADC sample rate in ms. */               

// si7021 initialization begins

//Initializing TWI1 instance for si7021
#define TWI_INSTANCE_ID1     1

#define SAMPLE_RATE 1000             // ECG sample rate: 1 kHz
#define BUFFER_SIZE 5000             // Buffer to store 5 seconds of ECG data (5000 samples)
#define ADC_BUFFER_SIZE 50           // Size of adc_buffer
#define THRESHOLD 512                // Threshold for R-peak detection (you can tune this)
#define MIN_RR_INTERVAL 250          // Minimum R-R interval in ms (240 bpm)
#define WINDOW_SIZE 150              // Moving window size for peak detection

uint16_t ir_data, red_data;
// Variables
volatile float ecgBuffer[BUFFER_SIZE];     // Buffer to store 5 seconds of ECG data
volatile int ecgBufferIndex = 0;           // Current index in the ECG buffer
volatile bool bufferFull = false;          // Flag to check if the ECG buffer is full
uint16_t adc_buffer[ADC_BUFFER_SIZE];      // ADC buffer to hold 50 samples at a time
// Filters and thresholds (parameters need tuning)
float lowPassFilter(float x);
float highPassFilter(float x);
float derivative(float x);
float movingWindowIntegrator(float x);


// A flag to indicate the transfer state
static volatile bool m_xfer_done = false;

// Create a Handle for the twi communication
static const nrf_drv_twi_t my_twi_0 = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID1);

/* Buffer to read sensor data. */
static uint8_t m_sample[2];

/* Indicates mode of operation on TWI. */
static uint8_t twi_read_mode;

/* stores the sensor readings. */
static uint16_t humRaw;
static uint16_t tempRaw;
static uint16_t humCent;
static uint16_t tempCent;
static uint16_t tempF;
// si7021 initialization ends


BLE_NUS_DEF(m_nus, NRF_SDH_BLE_TOTAL_LINK_COUNT);                                   /**< BLE NUS service instance. */
NRF_BLE_GATT_DEF(m_gatt);                                                           /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                                                             /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising);                                                 /**< Advertising module instance. */

static uint16_t   m_conn_handle          = BLE_CONN_HANDLE_INVALID;                 /**< Handle of the current connection. */
static uint16_t   m_ble_nus_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;            /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
static ble_uuid_t m_adv_uuids[]          =                                          /**< Universally unique service identifier. */
{
    {BLE_UUID_NUS_SERVICE, NUS_SERVICE_UUID_TYPE}
};

volatile uint8_t state = 1;

static const nrf_drv_timer_t   m_timer = NRF_DRV_TIMER_INSTANCE(3);
static nrf_saadc_value_t       m_buffer_pool[2][SAADC_SAMPLES_IN_BUFFER];
static nrf_ppi_channel_t       m_ppi_channel;
static uint32_t                m_adc_evt_counter;
int16_t AccValue[3];
//uint16_t adc_value [SAADC_SAMPLES_IN_BUFFER];
uint16_t length = 116;
uint16_t adc_buffer[50];
uint8_t printf_buffer[116];
uint8_t data_ready;
//,adc_value2,adc_value3,adc_value4,adc_value5,adc_value6;

/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyse
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for initializing the timer module.
 */
static void timers_init(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for the GAP initialization.
 *
 * @details This function will set up all the necessary GAP (Generic Access Profile) parameters of
 *          the device. It also sets the permissions and appearance.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *) DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for handling the data from the Nordic UART Service.
 *
 * @details This function will process the data received from the Nordic UART BLE Service and send
 *          it to the UART module.
 *
 * @param[in] p_evt       Nordic UART Service event.
 */
/**@snippet [Handling the data received over BLE] */
static void nus_data_handler(ble_nus_evt_t * p_evt)
{

    if (p_evt->type == BLE_NUS_EVT_RX_DATA)
    {
        uint32_t err_code;

        NRF_LOG_DEBUG("Received data from BLE NUS. Writing data on UART.");
        NRF_LOG_HEXDUMP_DEBUG(p_evt->params.rx_data.p_data, p_evt->params.rx_data.length);

        for (uint32_t i = 0; i < p_evt->params.rx_data.length; i++)
        {
            do
            {
                err_code = app_uart_put(p_evt->params.rx_data.p_data[i]);
                if ((err_code != NRF_SUCCESS) && (err_code != NRF_ERROR_BUSY))
                {
                    NRF_LOG_ERROR("Failed receiving NUS message. Error 0x%x. ", err_code);
                    APP_ERROR_CHECK(err_code);
                }
            } while (err_code == NRF_ERROR_BUSY);
        }
        if (p_evt->params.rx_data.p_data[p_evt->params.rx_data.length - 1] == '\r')
        {
            while (app_uart_put('\n') == NRF_ERROR_BUSY);
        }
    }

}
/**@snippet [Handling the data received over BLE] */


/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    uint32_t           err_code;
    ble_nus_init_t     nus_init;
    nrf_ble_qwr_init_t qwr_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    // Initialize NUS.
    memset(&nus_init, 0, sizeof(nus_init));

    nus_init.data_handler = nus_data_handler;

    err_code = ble_nus_init(&m_nus, &nus_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling an event from the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module
 *          which are passed to the application.
 *
 * @note All this function does is to disconnect. This could have been done by simply setting
 *       the disconnect_on_fail config parameter, but instead we use the event handler
 *       mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling errors from the Connection Parameters module.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    uint32_t err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    uint32_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            APP_ERROR_CHECK(err_code);
            break;
        case BLE_ADV_EVT_IDLE:
            sleep_mode_enter();
            break;
        default:
            break;
    }
}


/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    uint32_t err_code;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected");
            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected");
            // LED indication will be changed when advertising starts.
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            // Pairing not supported
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for the SoftDevice initialization.
 *
 * @details This function initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}


/**@brief Function for handling events from the GATT library. */
void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
    if ((m_conn_handle == p_evt->conn_handle) && (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED))
    {
        m_ble_nus_max_data_len = p_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
        NRF_LOG_INFO("Data len is set to 0x%X(%d)", m_ble_nus_max_data_len, m_ble_nus_max_data_len);
    }
    NRF_LOG_DEBUG("ATT MTU exchange completed. central 0x%x peripheral 0x%x",
                  p_gatt->att_mtu_desired_central,
                  p_gatt->att_mtu_desired_periph);
}


/**@brief Function for initializing the GATT library. */
void gatt_init(void)
{
    ret_code_t err_code;

    err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated by button press.
 */
void bsp_event_handler(bsp_event_t event)
{
    uint32_t err_code;
    switch (event)
    {
        case BSP_EVENT_SLEEP:
            sleep_mode_enter();
            break;

        case BSP_EVENT_DISCONNECT:
            err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break;

        case BSP_EVENT_WHITELIST_OFF:
            if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
            {
                err_code = ble_advertising_restart_without_whitelist(&m_advertising);
                if (err_code != NRF_ERROR_INVALID_STATE)
                {
                    APP_ERROR_CHECK(err_code);
                }
            }
            break;

        default:
            break;
    }
}


/**@brief   Function for handling app_uart events.
 *
 * @details This function will receive a single character from the app_uart module and append it to
 *          a string. The string will be be sent over BLE when the last character received was a
 *          'new line' '\n' (hex 0x0A) or if the string has reached the maximum data length.
 */
/**@snippet [Handling the data received over UART] */
void uart_event_handle(app_uart_evt_t * p_event)
{
    static uint8_t data_array[BLE_NUS_MAX_DATA_LEN];
    static uint8_t index = 0;
    uint32_t       err_code;

    switch (p_event->evt_type)
    {
        case APP_UART_DATA_READY:
            UNUSED_VARIABLE(app_uart_get(&data_array[index]));
            index++;

            if ((data_array[index - 1] == '\n') ||
                (data_array[index - 1] == '\r') ||
                (index >= m_ble_nus_max_data_len))
            {
                if (index > 1)
                {
                    NRF_LOG_DEBUG("Ready to send data over BLE NUS");
                    NRF_LOG_HEXDUMP_DEBUG(data_array, index);

                    do
                    {
                        uint16_t length = (uint16_t)index;
                        err_code = ble_nus_data_send(&m_nus, data_array, &length, m_conn_handle);
                        if ((err_code != NRF_ERROR_INVALID_STATE) &&
                            (err_code != NRF_ERROR_RESOURCES) &&
                            (err_code != NRF_ERROR_NOT_FOUND))
                        {
                            APP_ERROR_CHECK(err_code);
                        }
                    } while (err_code == NRF_ERROR_RESOURCES);
                }

                index = 0;
            }
            break;

        case APP_UART_COMMUNICATION_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_communication);
            break;

        case APP_UART_FIFO_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_code);
            break;

        default:
            break;
    }
}
/**@snippet [Handling the data received over UART] */


/**@brief  Function for initializing the UART module.
 */
/**@snippet [UART Initialization] */
static void uart_init(void)
{
    uint32_t                     err_code;
    app_uart_comm_params_t const comm_params =
    {
        .rx_pin_no    = RX_PIN_NUMBER,
        .tx_pin_no    = TX_PIN_NUMBER,
        .rts_pin_no   = RTS_PIN_NUMBER,
        .cts_pin_no   = CTS_PIN_NUMBER,
        .flow_control = APP_UART_FLOW_CONTROL_DISABLED,
        .use_parity   = false,
#if defined (UART_PRESENT)
        .baud_rate    = NRF_UART_BAUDRATE_115200
#else
        .baud_rate    = NRF_UARTE_BAUDRATE_115200
#endif
    };

    APP_UART_FIFO_INIT(&comm_params,
                       UART_RX_BUF_SIZE,
                       UART_TX_BUF_SIZE,
                       uart_event_handle,
                       APP_IRQ_PRIORITY_LOWEST,
                       err_code);
    APP_ERROR_CHECK(err_code);
}
/**@snippet [UART Initialization] */


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    uint32_t               err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance = false;
    init.advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;

    init.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.srdata.uuids_complete.p_uuids  = m_adv_uuids;

    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;
    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}


/**@brief Function for initializing buttons and leds.
 *
 * @param[out] p_erase_bonds  Will be true if the clear bonding button was pressed to wake the application up.
 */
static void buttons_leds_init(bool * p_erase_bonds)
{
    bsp_event_t startup_event;

    uint32_t err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, bsp_event_handler);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_btn_ble_init(NULL, &startup_event);
    APP_ERROR_CHECK(err_code);

    *p_erase_bonds = (startup_event == BSP_EVENT_CLEAR_BONDING_DATA);
}


/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{
    UNUSED_RETURN_VALUE(NRF_LOG_PROCESS());
    nrf_pwr_mgmt_run();
}


/**@brief Function for starting advertising.
 */
static void advertising_start(void)
{
    uint32_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);
}


void timer_handler(nrf_timer_event_t event_type, void* p_context)
{

}


void saadc_sampling_event_init(void)
{
    ret_code_t err_code;
    err_code = nrf_drv_ppi_init();
    APP_ERROR_CHECK(err_code);
    
    nrf_drv_timer_config_t timer_config = NRF_DRV_TIMER_DEFAULT_CONFIG;
    timer_config.frequency = NRF_TIMER_FREQ_31250Hz;
    err_code = nrf_drv_timer_init(&m_timer, &timer_config, timer_handler);
    APP_ERROR_CHECK(err_code);

    /* setup m_timer for compare event */
    uint32_t ticks = nrf_drv_timer_ms_to_ticks(&m_timer,SAADC_SAMPLE_RATE);
    nrf_drv_timer_extended_compare(&m_timer, NRF_TIMER_CC_CHANNEL0, ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);
    nrf_drv_timer_enable(&m_timer);

    uint32_t timer_compare_event_addr = nrf_drv_timer_compare_event_address_get(&m_timer, NRF_TIMER_CC_CHANNEL0);
    uint32_t saadc_sample_event_addr = nrf_drv_saadc_sample_task_get();

    /* setup ppi channel so that timer compare event is triggering sample task in SAADC */
    err_code = nrf_drv_ppi_channel_alloc(&m_ppi_channel);
    APP_ERROR_CHECK(err_code);
    
    err_code = nrf_drv_ppi_channel_assign(m_ppi_channel, timer_compare_event_addr, saadc_sample_event_addr);
    APP_ERROR_CHECK(err_code);
}


void saadc_sampling_event_enable(void)
{
    ret_code_t err_code = nrf_drv_ppi_channel_enable(m_ppi_channel);
    APP_ERROR_CHECK(err_code);
}


void saadc_callback(nrf_drv_saadc_evt_t const * p_event)
{
    ret_code_t err_code;
    //uint16_t adc_value[SAADC_SAMPLES_IN_BUFFER];
    //uint8_t printf_buffer[SAADC_SAMPLES_IN_BUFFER*2];
        if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
    {
        
               
               err_code =  nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, SAADC_SAMPLES_IN_BUFFER);
               APP_ERROR_CHECK(err_code);
                //for (int j=0;j<SAADC_SAMPLES_IN_BUFFER;j++){
                //adc_value[j] = p_event->data.done.p_buffer[j];
                //printf_buffer[j] = p_event->data.done.p_buffer[j];
                //printf_buffer[j+1] = p_event->data.done.p_buffer[j/2] >> 8;
            }
            
        
        for (int i=0;i<SAADC_SAMPLES_IN_BUFFER;i++)
        {
            adc_buffer[i] = p_event->data.done.p_buffer[i];
            printf_buffer[i*2] = p_event->data.done.p_buffer[i];
            printf_buffer[i*2 + 1] = p_event->data.done.p_buffer[i] >> 8;


        }
        //nrf_gpio_pin_clear(5);
        
        
         data_ready=1;

m_adc_evt_counter++;
            
        /*
        else{
            printf_buffer[192] = 0x00;
            printf_buffer[193] = 0x00;
            printf_buffer[194] = 0x00;
            printf_buffer[195] = 0x00;
            printf_buffer[196] = 0x00;
            printf_buffer[197] = 0x00;
            printf_buffer[198] = 0x00;
            printf_buffer[199] = 0x00;

        }
	*/
        
    }


void saadc_init(void)
{
    ret_code_t err_code;
	
    nrf_drv_saadc_config_t saadc_config = NRF_DRV_SAADC_DEFAULT_CONFIG;
    saadc_config.resolution = NRF_SAADC_RESOLUTION_10BIT;
	
    nrf_saadc_channel_config_t channel_0_config =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN2);
    channel_0_config.gain = NRF_SAADC_GAIN1_4;
    channel_0_config.reference = NRF_SAADC_REFERENCE_VDD4;
	/*
    nrf_saadc_channel_config_t channel_1_config =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN5);
    channel_1_config.gain = NRF_SAADC_GAIN1_4;
    channel_1_config.reference = NRF_SAADC_REFERENCE_VDD4;
	
    nrf_saadc_channel_config_t channel_2_config =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN6);
    channel_2_config.gain = NRF_SAADC_GAIN1_4;
    channel_2_config.reference = NRF_SAADC_REFERENCE_VDD4;
	
    nrf_saadc_channel_config_t channel_3_config =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN7);
    channel_3_config.gain = NRF_SAADC_GAIN1_4;
    channel_3_config.reference = NRF_SAADC_REFERENCE_VDD4;				
	*/
    err_code = nrf_drv_saadc_init(&saadc_config, saadc_callback);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_channel_init(0, &channel_0_config);
    APP_ERROR_CHECK(err_code);
    /*
    err_code = nrf_drv_saadc_channel_init(1, &channel_1_config);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_saadc_channel_init(2, &channel_2_config);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_saadc_channel_init(3, &channel_3_config);
    APP_ERROR_CHECK(err_code);	
    */
    err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[0],SAADC_SAMPLES_IN_BUFFER);
    APP_ERROR_CHECK(err_code);   
    err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[1],SAADC_SAMPLES_IN_BUFFER);
    APP_ERROR_CHECK(err_code);
}


//Event Handler for si7021
void twi_si7021_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
    uint32_t err_code;
        uint8_t reg[2];
        
    NRF_LOG_DEBUG("twi_handler %u\n\r",p_event->type);
    //Check the event to see what type of event occurred
    switch (p_event->type)
    {
        case NRF_DRV_TWI_EVT_DONE:
            NRF_LOG_DEBUG("twi_handler NRF_DRV_TWI_EVT_DONE %u\n\r",p_event->xfer_desc.type);
            if (p_event->xfer_desc.type == NRF_DRV_TWI_XFER_TX) {
                NRF_LOG_DEBUG("twi_handler TX\n\r");
                if (twi_read_mode==SI7021_REG_HUM_READ || twi_read_mode==SI7021_REG_TEMP_READ) {
                	err_code = nrf_drv_twi_rx(&my_twi_0, SI7021_ADDR, m_sample, sizeof(m_sample));
                	APP_ERROR_CHECK(err_code);
                }
            } else if (p_event->xfer_desc.type == NRF_DRV_TWI_XFER_RX) {
                NRF_LOG_DEBUG("twi_handler RX %d,%d\n\r",m_sample[0],m_sample[1]);
                if (twi_read_mode==SI7021_REG_HUM_READ) {
                	humRaw=(uint16_t)m_sample[0] << 8 |m_sample[1];
    				reg[0] = SI7021_REG_TEMP_READ;
    				twi_read_mode = reg[0];
    				NRF_LOG_DEBUG("nrf_drv_twi_tx %x %u\n\r",SI7021_ADDR,reg[0]);
    				err_code = nrf_drv_twi_tx(&my_twi_0, SI7021_ADDR, reg, 1, false);
    				APP_ERROR_CHECK(err_code);
                } else if (twi_read_mode==SI7021_REG_TEMP_READ) {
                	humCent = (uint16_t)((12500 * humRaw) >> 16) - 600;
                	tempRaw = (uint16_t)m_sample[0] << 8 |m_sample[1];
					tempCent = (uint16_t)((17572 * tempRaw) >> 16) - 4685;
                                        tempF = (uint16_t) ((tempCent*1.8/100) + 32);
					// printf("Relative Humidity %d.%d%% and Temp %d.%d°F        \r",humCent/100,humCent%100,tempF,tempF%100);
					NRF_LOG_DEBUG("\r\n");
                                        reg[0] = SI7021_REG_HUM_READ;
    				twi_read_mode = reg[0];
    				NRF_LOG_DEBUG("nrf_drv_twi_tx %x %u\n\r",SI7021_ADDR,reg[0]);
    				err_code = nrf_drv_twi_tx(&my_twi_0, SI7021_ADDR, reg, 1, false);
    				APP_ERROR_CHECK(err_code);
					//si7021_sensor_update(humRaw,tempRaw);
                }
            }
            m_xfer_done = true;
        
        default:
        // do nothing
        NRF_LOG_DEBUG("twi_handler default\n\r");
          break;
    }
    
}

//Initialize the TWI as Master device
void twi_master_7021_init(void)
{
    ret_code_t err_code;

    // Configure the settings for twi communication
    const nrf_drv_twi_config_t twi_config1 = {
       .scl                = TWI_SCL_M1,  //SCL Pin
       .sda                = TWI_SDA_M1,  //SDA Pin
       .frequency          = NRF_DRV_TWI_FREQ_100K, //Communication Speed
       .interrupt_priority = APP_IRQ_PRIORITY_LOW, //Interrupt Priority(Note: if using Bluetooth then select priority carefully)
       .clear_bus_init     = true //automatically clear bus
    };


    //A function to initialize the twi communication
    err_code = nrf_drv_twi_init(&my_twi_0, &twi_config1, twi_si7021_handler, NULL);
    APP_ERROR_CHECK(err_code);
    
    //Enable the TWI Communication..
    nrf_drv_twi_enable(&my_twi_0);
twi_read_mode = SI7021_REG_HUM_READ;
	NRF_LOG_DEBUG("nrf_drv_twi_tx %x %u\n\r",SI7021_ADDR,twi_read_mode);
	err_code = nrf_drv_twi_tx(&my_twi_0, SI7021_ADDR, &twi_read_mode, 1, false);
	APP_ERROR_CHECK(err_code);
    
}

// Function to process ECG data and detect peaks
void detectPeaksAndCalculateHR() {
    int lastRPeakTime = 0;
    int peakCount = 0;
    int rPeakTimes[BUFFER_SIZE / MIN_RR_INTERVAL]; // To store times of detected R-peaks

    for (int i = WINDOW_SIZE; i < BUFFER_SIZE - WINDOW_SIZE; i++) {
        float windowMax = 0;
        for (int j = i - WINDOW_SIZE; j <= i + WINDOW_SIZE; j++) {
            if (ecgBuffer[j] > windowMax) {
                windowMax = ecgBuffer[j];
            }
        }

        // Detect a peak if it's above the threshold
        if (ecgBuffer[i] == windowMax && ecgBuffer[i] > THRESHOLD) {
            int currentTime = i; // Time in milliseconds based on buffer index

            // Ensure minimum R-R interval (no false peaks)
            if (currentTime - lastRPeakTime >= MIN_RR_INTERVAL) {
                rPeakTimes[peakCount] = currentTime;
                peakCount++;
                lastRPeakTime = currentTime;
            }
        }
    }

    // Calculate heart rate from the R-peaks detected
    if (peakCount > 1) {
        int totalRRInterval = 0;
        for (int i = 1; i < peakCount; i++) {
            totalRRInterval += (rPeakTimes[i] - rPeakTimes[i - 1]);
        }

        int averageRRInterval = totalRRInterval / (peakCount - 1);
        float heartRate = 60000.0 / averageRRInterval; // Convert R-R interval to BPM
        printf("Hear rate: %f\n", heartRate);

        // Serial.print("Heart Rate: ");
        // Serial.println(heartRate);
    } else {
        // Serial.println("No peaks detected or insufficient peaks to calculate heart rate.");
    }

    // Reset buffer and index after processing
    ecgBufferIndex = 0;
    bufferFull = false;
}

// Function to fill the ECG buffer with adc_buffer data
void fillECGBuffer() {
    for (int i = 0; i < ADC_BUFFER_SIZE; i++) {
        if (ecgBufferIndex < BUFFER_SIZE) {
            // Store the filtered adc_buffer sample into the ecgBuffer
            float filtered = lowPassFilter(adc_buffer[i]);
            filtered = highPassFilter(filtered);
            filtered = derivative(filtered);
            filtered = filtered * filtered;
            filtered = movingWindowIntegrator(filtered);

            ecgBuffer[ecgBufferIndex] = filtered;
            ecgBufferIndex++;
        }

        // Check if the ECG buffer is full (5000 samples)
        if (ecgBufferIndex >= BUFFER_SIZE) {
            bufferFull = true;

            // Process the ECG buffer to detect peaks and calculate heart rate
            detectPeaksAndCalculateHR();
            break; // Exit loop as the buffer is full
        }
    }
}

/**@brief Application main function.
 */
int main(void)
{
    bool erase_bonds;
    //uint16_t length = SAADC_SAMPLES_IN_BUFFER*2;
    //uint8_t printf_buffer[SAADC_SAMPLES_IN_BUFFER*2];
    // Initialize.
    uint16_t temp_data, humd_data;
    uart_init();
    log_init();
    timers_init();
    buttons_leds_init(&erase_bonds);
    nrf_gpio_cfg_output(22);
    nrf_gpio_cfg_output(5);
    power_management_init();
    ble_stack_init();
    gap_params_init();
    gatt_init();
    services_init();
    advertising_init();
    conn_params_init();
//  twi_master_7021_init();
max_twi_init();
//  twi_master_7021_init();
twi_master_init();
    while (mpu6050_init()== false){
 NRF_LOG_INFO("MPU not started.");
}      

    saadc_sampling_event_init();
    
    saadc_sampling_event_enable();
    saadc_init();
    
advertising_start();
    // Start execution.
    printf("\r\nUART started.\r\n");
    NRF_LOG_INFO("Debug logging for UART over RTT started.");
    

    // Enter main loop.
    for (;;)
    {
        if (NRF_LOG_PROCESS() == false){
   

       if (data_ready==1){
        if (MPU6050_ReadAcc(&AccValue[0], &AccValue[1], &AccValue[2]) == true)
        {
            
            printf_buffer[96] = 0x00;
            printf_buffer[97] = AccValue[0];
            printf_buffer[98] = AccValue[0] >> 8;
            printf_buffer[99] = AccValue[1];
            printf_buffer[100] = AccValue[1] >> 8;
            printf_buffer[101] = AccValue[2];
            printf_buffer[102] = AccValue[2] >> 8;
            printf_buffer[103] = 0xFF;  
            
        //nrf_gpio_pin_set(5);

        }
        //nrf_delay_ms(10);

     
max30100_init();
        max30100_read_sensor_data(&ir_data, &red_data);
            printf_buffer[104] = 0x00;
            // printf_buffer[105] = humCent/100;
            // printf_buffer[106] = (humCent/100) >> 8;
            // printf_buffer[107] = tempF;
            // printf_buffer[108] = (tempF) >> 8;
            printf_buffer[105] = ir_data;
            printf_buffer[106] = (ir_data) >> 8;
            printf_buffer[107] = red_data;
            printf_buffer[108] = (red_data) >> 8;
            printf_buffer[109] = 0xFF;
       
        
        si7021_read_sensor_data(&temp_data, &humd_data);
        
       
            printf_buffer[110] = 0x00;
            printf_buffer[111] = humd_data;
            printf_buffer[112] = humd_data >> 8;
            printf_buffer[113] = temp_data;
            printf_buffer[114] = temp_data >> 8;
            printf_buffer[115] = 0xFF;
           ble_nus_data_send(&m_nus, printf_buffer, &length, m_conn_handle);
           data_ready=0;

        //    fillECGBuffer();

           
           
           //nrf_delay_ms(100);
       }
        }
        idle_state_handle();
    }
}



// Low-pass filter (sample implementation, adjust coefficients as needed)
float lowPassFilter(float x) {
    static float y[2] = {0, 0};
    static float x_old[13] = {0};

    y[0] = y[1];
    y[1] = 2 * y[0] - x_old[12] + (2 * x - 2 * x_old[6] + x_old[12]) / 32;

    for (int i = 12; i > 0; i--) {
        x_old[i] = x_old[i - 1];
    }
    x_old[0] = x;

    return y[1];
}

// High-pass filter (sample implementation, adjust coefficients as needed)
float highPassFilter(float x) {
    static float y[2] = {0, 0};
    static float x_old[33] = {0};

    y[0] = y[1];
    y[1] = y[0] - x / 32 + x_old[16] - x_old[32] + x_old[31] / 32;

    for (int i = 32; i > 0; i--) {
        x_old[i] = x_old[i - 1];
    }
    x_old[0] = x;

    return y[1];
}

// Derivative function (emphasizes high slope changes)
float derivative(float x) {
    static float x_old[5] = {0};

    float y = (2 * x + x_old[1] - x_old[3] - 2 * x_old[4]) / 8;

    for (int i = 4; i > 0; i--) {
        x_old[i] = x_old[i - 1];
    }
    x_old[0] = x;

    return y;
}

// Moving window integrator (averages the signal)
float movingWindowIntegrator(float x) {
    static float x_old[BUFFER_SIZE] = {0};
    float y = 0;

    for (int i = BUFFER_SIZE - 1; i > 0; i--) {
        x_old[i] = x_old[i - 1];
        y += x_old[i];
    }
    x_old[0] = x;
    y += x_old[0];

    return y / BUFFER_SIZE;
}

/**
 * @}
 */
