#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"

//#region TC74_Variants
#define TC74_A0	0x48
#define TC74_A1	0x49
#define TC74_A2	0x4A
#define TC74_A3	0x4B
#define TC74_A4	0x4C
#define TC74_A5	0x4D 
#define TC74_A6	0x4E
#define TC74_A7	0x4F
//#endregion

//#region TC74_Pins
#define SCL GPIO_NUM_22
#define SDA GPIO_NUM_21

// #define SCL 22
// #define SDA 21
//#endregion

//#region TC74_Registers
#define TC74_TEMP_REG 0x00
#define TC74_CFG_REG  0x01
//#endregion

//#region TC74_Power_States
#define PWRSAVE 0x80
#define NOPWRSAVE 0x00
//#endregion


//#region I2C_DEFS
#define I2C_MASTER_FREQ_HZ    100000     /*!< I2C master clock frequency */
#define WRITE_BIT  I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT   I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN   0x1     /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS  0x0     /*!< I2C master will not check ack from slave */
#define ACK_VAL    0x0         /*!< I2C ack value */
#define NACK_VAL   0x1         /*!< I2C nack value */
#define I2C_MASTER_TX_BUF_DISABLE   0   /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0   /*!< I2C master do not need buffer */
//#endregion


typedef struct TC74_STRUCT {
   int I2C_PORT_NUM;
   int I2C_ADDRESS;
} TC74;

void TC74_init(int port_num, int variant);
float read_TC74();
esp_err_t  i2c_init(void);
void select_temperature_register(i2c_cmd_handle_t cmd);
void select_config_register(i2c_cmd_handle_t cmd);
void disable_standby(void);
float extract_value_from_buffer(int temp);