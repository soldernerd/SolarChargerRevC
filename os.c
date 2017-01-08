
#include <stdint.h>
#include <plib/pps.h>
#include <pic18f46j50.h>

#include "os.h"
#include "adc.h"
#include "i2c.h"
#include "rtcc.h"
#include "buck.h"
#include "ui.h"


 
//8ms for normal load, 1ms for short load
#define TIMER0_LOAD_HIGH_8MHZ 0xF8
#define TIMER0_LOAD_LOW_8MHZ 0x30
#define TIMER0_LOAD_SHORT_HIGH_8MHZ 0xFF
#define TIMER0_LOAD_SHORT_LOW_8MHZ 0x06

//8ms for normal load, 1ms for short load
#define TIMER0_LOAD_HIGH_48MHZ 0xD1
#define TIMER0_LOAD_LOW_48MHZ 0x20
#define TIMER0_LOAD_SHORT_HIGH_48MHZ 0xFA
#define TIMER0_LOAD_SHORT_LOW_48MHZ 0x24

//23ms in any case
//#define TIMER0_LOAD_HIGH_32KHZ 0xFF
//#define TIMER0_LOAD_LOW_32KHZ 0xFC
//#define TIMER0_LOAD_SHORT_HIGH_32KHZ 0xFF
//#define TIMER0_LOAD_SHORT_LOW_32KHZ 0xFC

//1second for normal load, 250ms for short load
#define TIMER0_LOAD_HIGH_32KHZ 0xFF
#define TIMER0_LOAD_LOW_32KHZ 0x80
#define TIMER0_LOAD_SHORT_HIGH_32KHZ 0xFF
#define TIMER0_LOAD_SHORT_LOW_32KHZ 0xE0


//void interrupt _isr(void)
void tmr_isr(void)
{
    //Timer 0
    if(INTCONbits.T0IF)
    {
        
        if(os.done) 
        {
            //8ms until overflow
            switch(os.clockFrequency)
            {
                case CPU_FREQUENCY_32kHz:
                    TMR0H = TIMER0_LOAD_HIGH_32KHZ;
                    TMR0L = TIMER0_LOAD_LOW_32KHZ;
                    break;
                case CPU_FREQUENCY_8MHz:
                    TMR0H = TIMER0_LOAD_HIGH_8MHZ;
                    TMR0L = TIMER0_LOAD_LOW_8MHZ;
                    break;
                case CPU_FREQUENCY_48MHz:
                    TMR0H = TIMER0_LOAD_HIGH_48MHZ;
                    TMR0L = TIMER0_LOAD_LOW_48MHZ;
                    break;
            }
            
            ++os.timeSlot;
//            if(os.timeSlot==NUMBER_OF_TIMESLOTS)
//            {
//                os.timeSlot = 0;
//            }
            os.done = 0;
        }
        else //Clock stretching
        {
            //1ms until overflow
            switch(os.clockFrequency)
            {
                case CPU_FREQUENCY_32kHz:
                    TMR0H = TIMER0_LOAD_SHORT_HIGH_32KHZ;
                    TMR0L = TIMER0_LOAD_SHORT_LOW_32KHZ;
                    break;
                case CPU_FREQUENCY_8MHz:
                    TMR0H = TIMER0_LOAD_SHORT_HIGH_8MHZ;
                    TMR0L = TIMER0_LOAD_SHORT_LOW_8MHZ;
                    break;
                case CPU_FREQUENCY_48MHz:
                    TMR0H = TIMER0_LOAD_SHORT_HIGH_48MHZ;
                    TMR0L = TIMER0_LOAD_SHORT_LOW_48MHZ;
                    break;
            }
        }
        INTCONbits.T0IF = 0;
    }
        
    //Push button
    if(INTCON3bits.INT1IF)
    {
        ++os.buttonCount;
        INTCON3bits.INT1IF = 0;
    }
    
    //Encoder
    if(INTCON3bits.INT2IF)
    {
        if(!ENCODER_B_BIT)
        {
            --os.encoderCount;
        }
        INTCON3bits.INT2IF = 0;
    }   
    if(INTCON3bits.INT3IF)
    {
        if(!ENCODER_A_BIT)
        {
            ++os.encoderCount;
        }
        INTCON3bits.INT3IF = 0;
    }
}


void system_delay_ms(uint8_t ms)
{
    uint8_t cntr;
    switch(os.clockFrequency)
    {
        case CPU_FREQUENCY_32kHz:
            for(cntr=0; cntr<ms; ++cntr)
            {
                __delay_us(4);
            }
            break;
        case CPU_FREQUENCY_8MHz:
            for(cntr=0; cntr<ms; ++cntr)
            {
                __delay_ms(1);
            }
            break;
        case CPU_FREQUENCY_48MHz:
            for(cntr=0; cntr<ms; ++cntr)
            {
                __delay_ms(6);
            }
            break;
        default:
            for(cntr=0; cntr<ms; ++cntr)
            {
                __delay_ms(6);
            }
            break;      
    }
}


static void _system_encoder_init(void)
{
    PPSUnLock();
    PPSInput(PPS_INT1, PPS_RP0); //Pushbutton
    PPSInput(PPS_INT3, PPS_RP9); //Encoder A
    PPSInput(PPS_INT2, PPS_RP10); //Encoder B
    PPSLock();
    
    INTCON2bits.INTEDG1 = 0; //0=falling
    INTCON3bits.INT1IF = 0;
    INTCON3bits.INT1IE = 1;
    
    INTCON2bits.INTEDG2 = 1; //1=rising
    INTCON3bits.INT2IF = 0;
    //INTCON3bits.INT2IE = 1;
    
    INTCON2bits.INTEDG3 = 1; //1=rising
    INTCON3bits.INT3IF = 0;
    //INTCON3bits.INT3IE = 1;
    
    INTCONbits.GIE = 1;
    
    os.encoderCount = 0;
    os.buttonCount = 0;
}

void system_encoder_disable(void)
{
    //Disable Interrupts  
    INTCON3bits.INT2IE = 0;
    INTCON3bits.INT3IE = 0;
}

void system_encoder_enable(void)
{
    //Clear interrupt flags
    INTCON3bits.INT2IF = 0;
    INTCON3bits.INT3IF = 0; 
    
    //Reset encoder count
    os.encoderCount = 0;
    
    //Enable Interrupts  
    INTCON3bits.INT2IE = 1;
    INTCON3bits.INT3IE = 1;
}

/*

static void _system_timer0_init(void)
{
    //Clock source = Fosc/4
    T0CONbits.T0CS = 0;
    //Operate in 16bit mode
    T0CONbits.T08BIT = 0;
    //Prescaler=8
    T0CONbits.T0PS2 = 0;
    T0CONbits.T0PS1 = 1;
    T0CONbits.T0PS0 = 0;
    //Use prescaler
    T0CONbits.PSA = 0;
    //8ms until overflow
    switch(os.clockFrequency)
    {
        case CPU_FREQUENCY_8MHz:
            TMR0H = TIMER0_LOAD_HIGH_8MHZ;
            TMR0L = TIMER0_LOAD_LOW_8MHZ;
            break;
        case CPU_FREQUENCY_48MHz:
            TMR0H = TIMER0_LOAD_HIGH_48MHZ;
            TMR0L = TIMER0_LOAD_LOW_48MHZ;
            break;
    }
    //Turn timer0 on
    T0CONbits.TMR0ON = 1;
            
    //Enable timer0 interrupts
    INTCONbits.TMR0IF = 0;
    INTCONbits.TMR0IE = 1;
    INTCONbits.GIE = 1;
    
    //Initialize timeSlot
    os.timeSlot = 0;
}
*/

void system_init(void)
{
    //Configure ports
    VCC_HIGH_TRIS = PIN_OUTPUT;
    PWR_GOOD_TRIS = PIN_INPUT;
    DISP_EN_TRIS = PIN_OUTPUT;
    USBCHARGER_EN_TRIS = PIN_OUTPUT;
    USBCHARGER_EN_PORT = 0;
    
    //SPI Pins
    SPI_MISO_TRIS = PIN_INPUT;
    SPI_MOSI_TRIS = PIN_OUTPUT;
    SPI_SCLK_TRIS = PIN_OUTPUT;
    SPI_SS_TRIS = PIN_OUTPUT;
    SPI_SS_PORT = 1;
    
    //Pins for temperature sensing
    VOLTAGE_REFERENCE_TRIS = PIN_INPUT;
    TEMPERATURE1_TRIS = PIN_INPUT;
    TEMPERATURE1_ANCON = PIN_ANALOG;
    TEMPERATURE2_TRIS = PIN_INPUT;
    TEMPERATURE2_ANCON = PIN_ANALOG;
    TEMPERATURE3_TRIS = PIN_INPUT;
    TEMPERATURE3_ANCON = PIN_ANALOG;
    
    TRISAbits.TRISA0 = 1; //Push button 
    ANCON0bits.PCFG0 = 1; //Pushb button as digital input
    TRISBbits.TRISB6 = 1; //Encoder A
    TRISBbits.TRISB7 = 1; //Encoder B
    //TRISBbits.TRISB0 = 0; //LCD Backlight, LCD reset
    //LATBbits.LATB0 = 0;
    
    /*
    
    //Temperature sensors
    TRISAbits.TRISA1 = 1; //On-board temperature sensor
    ANCON0bits.PCFG1 = 0;
    TRISAbits.TRISA2 = 1; //External temperature sensor 1
    ANCON0bits.PCFG2 = 0;
    TRISBbits.TRISB3 = 1; //External temperature sensor 2
    ANCON1bits.PCFG9 = 0;
    
    //Outputs
    TRISAbits.TRISA5 = 0; //output 1
    TRISCbits.TRISC2 = 0; //output 2

    */
    
    //Initialize variables
    os.clockFrequency = CPU_FREQUENCY_48MHz;
    os.boardVoltage = BOARD_VOLTAGE_LOW;
    os.buckFrequency = BUCK_OFF;
    os.buckDutyCycle = 0;
    os.buckLastStep = -1;
    os.display_mode = DISPLAY_MODE_OVERVIEW;
    
    //Just for now
    os.output_voltage = 13000;

    //Configure timer 1
    //Clear interrupt flag
    PIR1bits.TMR1IF = 0; 
    //Load timer (1024 cycles until overflow)
    TMR1H = 0xFC; 
    TMR1L = 0x00;
    //Turn timer on
    T1CONbits.TMR1ON = 1;
    //Enable secondary (32kHz) oscillator
    T1CONbits.T1OSCEN = 1; 
    //Wait for secondary oscillator to have stabilized
    while(!PIR1bits.TMR1IF){};
    //Turn timer off
    T1CONbits.TMR1ON = 0;
    
    //Initialize I2C
    i2c_init();
    //Initialize display
    ui_init();

    //Initialize internal ADC module
    adc_init();
  
    //Initialize Real Time Clock
    rtcc_init();
    
    /*
    //Set up timer0 for timeSlots
    _system_timer0_init();
    
    //Initialize user interface
    ui_init();
     * */
    
    //Set up encoder with interrupts
    _system_encoder_init();
    
    flash_init();

    //Buck init
    buck_init();

}

/*
void system_set_cpu_frequency(clockFrequency_t newFrequency)
{
    if(os.clockFrequency==newFrequency)
    {
        return;
    }
    switch(newFrequency)
    {
        case CPU_FREQUENCY_32kHz:
            OSCCONbits.SCS1 = 0;
            OSCCONbits.SCS0 = 1;
            OSCTUNEbits.PLLEN = 0;
            os.clockFrequency = CPU_FREQUENCY_32kHz;
            break;
        case CPU_FREQUENCY_8MHz:
            OSCCONbits.SCS1 = 0;
            OSCCONbits.SCS0 = 0; 
            OSCTUNEbits.PLLEN = 0;
            os.clockFrequency = CPU_FREQUENCY_8MHz;
            break;
        case CPU_FREQUENCY_48MHz:
            OSCCONbits.SCS1 = 0;
            OSCCONbits.SCS0 = 0; 
            OSCTUNEbits.PLLEN = 1;
            os.clockFrequency = CPU_FREQUENCY_48MHz;
            break;
    }        
    i2c_set_frequency(i2c_get_frequency());
}

void system_power_save(void)
{
    //Lower CPU frequency and board voltage if buck and user interface are off
    if(buck_get_mode()==BUCK_STATUS_OFF) //also check for USB later
    {
        if(ui_get_status()==USER_INTERFACE_STATUS_OFF)
        {
            //User interface is off, we can enter low power mode
            i2c_expander_low(I2C_EXPANDER_BOARD_VOLTAGE);
            system_set_cpu_frequency(CPU_FREQUENCY_32kHz);
            TMR0H = TIMER0_LOAD_HIGH_32KHZ;
            TMR0L = TIMER0_LOAD_LOW_32KHZ;
            os.timeSlot = 0;
        }
        else
        {
            //User interface is on. We can only lower the CPU frequency to 8MHz
            system_set_cpu_frequency(CPU_FREQUENCY_8MHz);
        }
    }
}


uint8_t system_output_is_on(outputs_t output)
{
    if(os.outputs&output)
        return 1;
    else
        return 0;
}

void system_output_toggle(outputs_t output)
{
    if(system_output_is_on(output))
        system_output_off(output);
    else
        system_output_on(output);
}

void system_output_on(outputs_t output)
{
    os.outputs |= output;
    
    switch(output)
    {
        case OUTPUT_1:
            i2c_expander_high(I2C_EXPANDER_OUTPUTS_ENABLE);
            LATAbits.LA5 = 1;    
            break;
        case OUTPUT_2:
            i2c_expander_high(I2C_EXPANDER_OUTPUTS_ENABLE);
            LATCbits.LC2 = 1;
            break;
        case OUTPUT_3:
            i2c_expander_high(I2C_EXPANDER_OUTPUTS_ENABLE);
            i2c_expander_high(I2C_EXPANDER_OUTPUT_3);
            break;
        case OUTPUT_4:
            i2c_expander_high(I2C_EXPANDER_OUTPUTS_ENABLE);
            i2c_expander_high(I2C_EXPANDER_OUTPUT_4);
            break;
        case OUTPUT_USB:
            i2c_expander_high(I2C_EXPANDER_USB_CHARGER);
            break;
    }      
}

void system_output_off(outputs_t output)
{
    os.outputs &= (~output);
    if(!(os.outputs&(OUTPUT_1 | OUTPUT_2 | OUTPUT_3 | OUTPUT_4)))
    {
        i2c_expander_low(I2C_EXPANDER_OUTPUTS_ENABLE);
    }
        
    switch(output)
    {
        case OUTPUT_1:
            LATAbits.LA5 = 0;    
            break;
        case OUTPUT_2:
            LATCbits.LC2 = 0;
            break;
        case OUTPUT_3:
            i2c_expander_low(I2C_EXPANDER_OUTPUT_3);
            break;
        case OUTPUT_4:
            i2c_expander_low(I2C_EXPANDER_OUTPUT_4);
            break;
        case OUTPUT_USB:
            i2c_expander_low(I2C_EXPANDER_USB_CHARGER);
            break;
    }     
}

void system_calculate_input_voltage()
{
    float tmp = 3.00 * 0.25 * 1.009184;
    int16_t adc_sum = os.input_voltage_adc[0] + os.input_voltage_adc[1] + os.input_voltage_adc[2] + os.input_voltage_adc[3];
    tmp *= adc_sum;
    os.input_voltage = (int16_t) tmp;
}

void system_calculate_output_voltage()
{
    float tmp = 1.95 * 0.25 * 1.004857;
    int16_t adc_sum = os.output_voltage_adc[0] + os.output_voltage_adc[1] + os.output_voltage_adc[2] + os.output_voltage_adc[3];
    tmp *= adc_sum;
    os.output_voltage = (int16_t) tmp;
}

void system_calculate_input_current()
{
    float tmp = 0.500 * 0.25 * 0.958512;
    int16_t adc_sum = os.input_current_adc[0] + os.input_current_adc[1] + os.input_current_adc[2] + os.input_current_adc[3];
    adc_sum -= os.input_current_calibration;
    tmp *= adc_sum;
    os.input_current = (int16_t) tmp;
}

void system_calculate_output_current()
{
    float tmp = 0.500 * 0.25;
    int16_t adc_sum = os.output_current_adc[0] + os.output_current_adc[1] + os.output_current_adc[2] + os.output_current_adc[3];
    adc_sum -= os.output_current_calibration;
    tmp *= adc_sum;
    os.output_current = (int16_t) tmp;
}


*/