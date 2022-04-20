#include <ADS1119.h>

// --------------------------------------------------------------------

ADS1119 ads = ADS1119 ( I2C_NUM_0, 0x42 ) ;

struct ADS1119Configuration adsCfg ;

// --------------------------------------------------------------------

extern "C"
{
    void app_main ( void )
    {
        adsCfg.mux 			    = ADS1119MuxConfiguration::positiveAIN0negativeAGND ;
        adsCfg.gain 		  	= ADS1119Configuration::Gain::one ;
        adsCfg.dataRate 	  	= ADS1119Configuration::DataRate::sps20 ;
        adsCfg.conversionMode   = ADS1119Configuration::ConversionMode::continuous ;
        adsCfg.voltageReference = ADS1119Configuration::VoltageReferenceSource::internal ;

        ads.init () ;
        ads.reset() ;
	    
        printf ( "setup done\n" ) ;

        while (1) 
        {
            printf ( "\n--- Measurement of voltage connected to AIN0 ---\n") ;
		    printf ( "Measured voltage: %f V\n", ads.readVoltage ( adsCfg ) ) ;
			
            vTaskDelay ( pdMS_TO_TICKS (5000) ) ;
        }     
    }
}
