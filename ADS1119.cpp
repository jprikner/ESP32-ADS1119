/**
*  ESP32 Library for Texas Instruments ADS1119 - 16-Bit 4ch Analog-to-Digital Converter
*  
*  @author Oktawian Chojnacki <oktawian@elowro.com>
*  https://www.elowro.com
*
*  @author Jakub Prikner <jakub.prikner@gmail.com>
*  https://www.prikner.net
*
*/

/**
 * The MIT License
 *
 * Copyright (c) 2020 Oktawian Chojnacki <oktawian@elowro.com>
 * 
 * Copyright (c) 2022 Jakub Prikner <jakub.prikner@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// ====================================================================

#include "ADS1119.h"

static const char *TAG = "ADS1119" ;

// ====================================================================

ADS1119::ADS1119 ( i2c_port_t port, uint8_t address ) : port (port), address (address)
{
    
}

// --------------------------------------------------------------------

void ADS1119::init ()
{
    ESP_LOGI ( TAG, "Init --- I2C port: %d, address: 0x%x", port, address ) ;
}

// --------------------------------------------------------------------

float ADS1119::readVoltage ( ADS1119Configuration config )
{
    uint16_t twoBytesRead = readTwoBytes(config);
    if (twoBytesRead > 0x7FFF) 
    {
        twoBytesRead = 0x0;
    }
    uint16_t value = twoBytesRead - offset;
    float gain = gainAsFloat(config);
    float referenceVoltage = referenceVoltageAsFloat(config);  
    float voltage = referenceVoltage * (float(value) / ADS1119_RANGE) * gain;

    return voltage;
}

// --------------------------------------------------------------------

float ADS1119::performOffsetCalibration ( ADS1119Configuration config )
{
    config.mux = ADS1119MuxConfiguration::positiveAIN2negativeAGND;

    float totalOffset = 0;
    for (int i = 0; i <= 100; i++) {
        totalOffset += referenceVoltageAsFloat(config) - readVoltage(config);
        vTaskDelay (10) ;
    }
    
    offset = totalOffset / 100.0;

    return offset;
}

// --------------------------------------------------------------------

float ADS1119::gainAsFloat ( ADS1119Configuration config )
{
    return uint8_t(config.gain) == uint8_t(0B0) ? 1.0 : 4.0;
}

// --------------------------------------------------------------------

float ADS1119::referenceVoltageAsFloat ( ADS1119Configuration config )
{
    return bool(config.voltageReference) ? config.externalReferenceVoltage : ADS1119_INTERNAL_REFERENCE_VOLTAGE;
}

// --------------------------------------------------------------------

bool ADS1119::reset ()
{
    // 8.5.3.2 RESET (0000 011x) / Page 25
    // http://www.ti.com/lit/ds/sbas925a/sbas925a.pdf
    return writeByte(0B00000110); // 0x06
}

// --------------------------------------------------------------------

bool ADS1119::powerDown ()
{
    // 8.5.3.4 POWERDOWN (0000 001x) / Page 25
    // http://www.ti.com/lit/ds/sbas925a/sbas925a.pdf
    return writeByte(0B00000010); // 0x02
}

// --------------------------------------------------------------------

uint16_t ADS1119::readTwoBytes ( ADS1119Configuration config )
{
    // 8.5.3.6 RREG (0010 0rxx) / Page 26
    // http://www.ti.com/lit/ds/sbas925a/sbas925a.pdf
    uint8_t registerValue = 0B01000000;
    uint8_t value = 0x0;

    unsigned long conversionTime;
    
    switch (config.dataRate) 
    {
        case ADS1119Configuration::DataRate::sps20: conversionTime = 1000.0/20.0; break;
        case ADS1119Configuration::DataRate::sps90: conversionTime = 1000.0/90.0; break;
        case ADS1119Configuration::DataRate::sps330: conversionTime = 1000.0/330.0; break;
        default: conversionTime = 1.0; break;
    }

    value |= (uint8_t(config.mux) << 5);                // XXX00000
    value |= (uint8_t(config.gain) << 4);               // 000X0000
    value |= (uint8_t(config.dataRate) << 2);           // 0000XX00
    value |= (uint8_t(config.conversionMode) << 1);     // 000000X0
    value |= (uint8_t(config.voltageReference) << 0);   // 0000000X
    commandStart();

    write(registerValue, value);
    vTaskDelay (conversionTime) ;
    commandReadData();

    return read();
}

// --------------------------------------------------------------------

bool ADS1119::commandReadData ()
{
    // 8.5.3.5 RDATA (0001 xxxx) / Page 26
    // http://www.ti.com/lit/ds/sbas925a/sbas925a.pdf
    return writeByte(0B00010000); // 0x10
}

// --------------------------------------------------------------------

bool ADS1119::commandStart ()
{
    // 8.5.3.3 START/SYNC (0000 100x) / Page 25
    // http://www.ti.com/lit/ds/sbas925a/sbas925a.pdf
    return writeByte(0B00001000); // 0x08
}

// --------------------------------------------------------------------

uint16_t ADS1119::read ()
{
	uint8_t conv[2] = {0} ;

    esp_err_t ret = i2c_manager_read ( port, address, I2C_NO_REG, conv, 2 ) ; // read conversion data
    
    if ( ret != ESP_OK )
	{
        ESP_LOGE ( TAG, "Error reading two bytes" ) ;
        return -1 ;
    }
    
    return ( conv[0] << 8 | conv[1] ) ;
}

// --------------------------------------------------------------------

bool ADS1119::write ( uint8_t registerValue, uint8_t value )
{
	esp_err_t ret = i2c_manager_write ( port, address, registerValue, &value, 1 ) ;

    return ret == ESP_OK ;
}

// --------------------------------------------------------------------

bool ADS1119::writeByte ( uint8_t value )
{
	esp_err_t ret = i2c_manager_write ( port, address, I2C_NO_REG, &value, 1 ) ;

    return ret == ESP_OK ;
}

// --------------------------------------------------------------------

uint8_t ADS1119::readRegister ( ADS1119RegisterToRead registerToRead )
{
    // 8.5.3.6 RREG (0010 0rxx) / Page 26
    // http://www.ti.com/lit/ds/sbas925a/sbas925a.pdf
 
    uint8_t *data = 0x00 ;
    uint8_t byteToWrite = 0B00100000 | ( uint8_t(registerToRead) << 2 ) ;
 
    esp_err_t ret = i2c_manager_read ( port, address, byteToWrite, data, 1 ) ;
    
    if ( ret != ESP_OK )
	{
        ESP_LOGE ( TAG, "Error reading register %d", uint8_t(registerToRead) ) ;
        return -1 ;
    }
    return *data ;
}