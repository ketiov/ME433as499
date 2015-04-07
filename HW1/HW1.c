#include<xc.h> // processor SFR definitions
#include<sys/attribs.h> // __ISR macro

// DEVCFGs here
#pragma config FPLLIDIV = DIV_4 // 8MHz input divided to 2MHz
#pragma config FPLLMUL = MUL_20 // Multiply 2MHz by 20 to desired 40MHz

#pragma config DEBUG = OFF          // Background Debugger disabled
#pragma config FPLLODIV = DIV_1     // PLL Output Divider: Divide by 1
#pragma config FWDTEN = OFF         // WD timer: OFF
#pragma config POSCMOD = HS         // Primary Oscillator Mode: High Speed xtal
#pragma config FNOSC = PRIPLL       // Oscillator Selection: Primary oscillator w/ PLL
#pragma config FPBDIV = DIV_1       // Peripheral Bus Clock: Divide by 1
#pragma config BWP = OFF            // Boot write protect: OFF
#pragma config ICESEL = ICS_PGx2    // ICE pins configured on PGx2, Boot write protect OFF.
#pragma config FSOSCEN = OFF        // Disable second osc to get pins back
//#pragma config FSRSSEL = PRIORITY_7 // Shadow Register Set for interrupt priority 7


//function prototypes
int readADC(void);



//ISRs








int main() {

	// startup
	__builtin_disable_interrupts();

	// set the CP0 CONFIG register to indicate that
	// kseg0 is cacheable (0x3) or uncacheable (0x2)
	// see Chapter 2 "CPU for Devices with M4K Core"
	// of the PIC32 reference manual
	__builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

	// no cache on this chip!

	// 0 data RAM access wait states
	BMXCONbits.BMXWSDRM = 0x0;

	// enable multi vector interrupts
	INTCONbits.MVEC = 0x1;

	// disable JTAG to be able to use TDI, TDO, TCK, TMS as digital
	DDPCONbits.JTAGEN = 0;







	// set up USER pin as input
	//B13
        ANSELBbits.ANSB13 = 0;
        TRISBbits.TRISB13 = 1;

	// set up LED1 pin as a digital output
	//B7
        TRISBbits.TRISB7 = 0;

	// set up LED2 as OC1 using Timer2 at 1kHz
	//B15
        T2CONbits.TCKPS = 1;     // Timer2 prescaler N=2
	T2CONbits.TGATE = 0;    //
	T2CONbits.TCS = 0;
	PR2 = 39999;              // period = (PR2+1) * N * 25 ns = 1 ms, 2 kHz
	TMR2 = 0;                // initial TMR3 count is 0
        OC1CONbits.OCTSEL = 0;
	OC1CONbits.OCM = 0b110;  // PWM mode without fault pin; other OC1CON bits are defaults
	OC1RS = 20000;             // duty cycle = OC1RS/(PR2+1) = 50%, 1kHz
	OC1R = 20000;              // initialize before turning OC1 on; afterward it is read-only
	T2CONbits.ON = 1;        // turn on Timer2
	OC1CONbits.ON = 1;       // turn on OC1

	// set up A0 as AN0
	ANSELAbits.ANSA0 = 1;
	AD1CON3bits.ADCS = 3;
	AD1CHSbits.CH0SA = 0;
	AD1CON1bits.ADON = 1;

        //set up timer 3 for isr
        T3CONbits.TCKPS = 0;     // Timer3 prescaler N=1
	T3CONbits.TGATE = 0;
	T3CONbits.TCS = 0;
	PR3 = 3999;              // period = (PR2+1) * N * 25 ns = 1 ms, 1 kHz
	TMR3 = 0;                // initial TMR3 count is 0
	__builtin_enable_interrupts();



        _CP0_SET_COUNT(0);
	while (1) {               // invert pin every 0.5s, set PWM duty cycle % to the pot voltage output

            //toggle LED at 2Hz
            if((_CP0_GET_COUNT())<10000000) {
                LATBbits.LATB7=0;
            }
            else if((_CP0_GET_COUNT())<20000000)
            {
                LATBbits.LATB7=1;
            }
            else
            {
                _CP0_SET_COUNT(0);
            }
            
            //while user is pressed, keep inverting the value on LED1
            while(!PORTBbits.RB13) LATBbits.LATB7 =  !PORTBbits.RB7;

            //set LED2 proportional to potentiometer voltage
            OC1RS=4000*readADC()/1024;
	}
}

int readADC(void) {
	int elapsed = 0;
	int finishtime = 0;
	int sampletime = 20;
	int a = 0;

	AD1CON1bits.SAMP = 1;
	elapsed = _CP0_GET_COUNT();
	finishtime = elapsed + sampletime;
	while (_CP0_GET_COUNT() < finishtime) {
	}
	AD1CON1bits.SAMP = 0;
	while (!AD1CON1bits.DONE) {
	}
	a = ADC1BUF0;
	return a;
}
