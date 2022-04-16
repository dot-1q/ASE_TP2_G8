#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"

// Defintions

static const char *TAG = "TC74";

#define TC74_SLAVE_ADDR_A0   0x48
#define TC74_SLAVE_ADDR_A1   0x49
#define TC74_SLAVE_ADDR_A2   0x4A
#define TC74_SLAVE_ADDR_A3   0x4B
#define TC74_SLAVE_ADDR_A4   0x4C
#define TC74_SLAVE_ADDR_A5   0x4D  /*!< default slave address for TC74 sensor */
#define TC74_SLAVE_ADDR_A6   0x4E
#define TC74_SLAVE_ADDR_A6   0x4F

#define READ_TEMP_REGISTER          0x00
#define READ_WRITE_CONFIG_REGISTER  0x01
#define SET_NORM_OP_VALUE           0x00  /*!< sets the 7th bit of configuration register to normal mode */
#define SET_STANBY_VALUE            0x80  /*!< sets the 7th bit of configuration register to standby mode */

#define _I2C_NUMBER(num) I2C_NUM_##num
#define I2C_NUMBER(num) _I2C_NUMBER(num)

#define I2C_MASTER_SCL_IO GPIO_NUM_22               /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO GPIO_NUM_21               /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM    I2C_NUMBER(0)             /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 100000                   /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0                 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0                 /*!< I2C master doesn't need buffer */

#define TC74_SLAVE_ADDR  TC74_SLAVE_ADDR_A5         /*!< slave address for TC74 sensor */
#define WRITE_BIT I2C_MASTER_WRITE                  /*!< I2C master write */
#define READ_BIT  I2C_MASTER_READ                   /*!< I2C master read */
#define ACK_CHECK_EN 0x1                            /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0                           /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                                 /*!< I2C ack value */
#define NACK_VAL 0x1                                /*!< I2C nack value */

// Function declarations

esp_err_t i2c_master_init(void);
esp_err_t i2c_master_read_temp(i2c_port_t i2c_num, uint8_t *tmprt);
esp_err_t i2c_master_set_tc74_mode(i2c_port_t i2c_num,uint8_t mode);
esp_err_t i2c_master_read_tc74_config(i2c_port_t i2c_num, uint8_t *mode);