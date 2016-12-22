/* 
 * File:   mpols_control_01.c
 * Author: JhowelCuevas
 *
 * Created on February 25, 2016, 6:26 PM
 */

#define FCY 4000000UL
#include <stdio.h>
#include <stdlib.h>
#include <libpic30.h>
#include <string.h>
#include <math.h>
#include "i2c_test.h"

unsigned short j, rxi = 0, txi = 0;
int x, t1, t2;
unsigned short tx_data;
unsigned char rxbuffer[2], old_rxbuffer[2];
long long decimal;

void Init(){
    
    
    //I2C configuration.
    I2C1ADD = 0xBE;                     //BE need to figure out why actual I2C becomes 0x7C;
    I2C1BRG = 39;                       //BRG of 39 for 4MHz Fcy. Not really needed for Slave Mode
    I2C1CONbits.I2CEN = 0;              // Disable I2C Mode
    I2C1CONbits.SCLREL = 1;             // Releases SCL1 clock
    I2C1CONbits.DISSLW = 1;             // Disable slew rate control
    IFS1bits.MI2C1IF = 0;               // Clear Interrupt
    I2C1CONbits.I2CEN = 1;              // Enable I2C Mode
    j = I2C1RCV;                        // read buffer to clear buffer full
    I2C1CONbits.ACKDT = 0;              //Dont Care. Slave still acknowledges w/o this setting.
    IEC1bits.SI2C1IE = 1;               //Slave I2C1 Event Interrupt Enable bit, very impt.
    
    //This was added 11-2 to force OC1 pin to be HIGH during initialization and so will drive the SSR OFF.
    OC1CON = 0x0000;                        //Turn Off Output Compare 1 Module
    T2CONbits.TCKPS = 2;                    //select 1:64 pre scale value (for FOSC=8MHz; 16bit Timer2, Max period is 1s)
    OC1R = 65535;                           //Set OC1R to max range.                 
    OC1CON = 0x0002;                        //Set to One Shot default HIGH and Driven Low at compare.
    //T2CONbits.TON = 1;                    //No need to enable the timer otherwise it will always be low.
    
    
    
    //AD1PCFGbits.PCFG2 = 1;            //Dont care. I2C still works whether to set this or not.
    //AD1PCFGbits.PCFG3 = 1;            //Dont care. I2C still works whether to set this or not.
    //TRISBbits.TRISB8 = 1;             //Dont care. Interrupt still triggers even if not setting this to 1.
    //TRISBbits.TRISB9 = 1;             //Dont care. Interrupt still triggers even if not setting this to 1.
    //LATBbits.LATB0 = 1;
    //LATBbits.LATB1 = 1;
    //TRISBbits.TRISB0 = 0;
    //TRISBbits.TRISB1 = 0;
    //ODCBbits.ODB0 = 0;  
    //ODCBbits.ODB1 = 0;
}

void pwmInit()
{
    OC1CON = 0x0000;                        //Turn Off Output Compare 1 Module
    T2CONbits.TCKPS = 2;                    //select 1:64 pre scale value (for FOSC=8MHz; 16bit Timer2, Max period is 1s)
    OC1RS = (float) (rxbuffer[1]*62.50);    //rxbuffer[1] is the off time but since we drive a FET then this will become ON time.
    OC1R = OC1RS;                           //Need to set an initial value on OC1R since this was configured initially during startup with 0xFFFF value.
    PR2 = (float) ((rxbuffer[0]+rxbuffer[1])*62.50)-1;          //Initialize PR2 with new value. Based on PWM PERIOD = (PR2+1)*TCY*(TIMER PRE-SCALE)
                                                                //PR2 is a 16bit register can have max of (2^16+1)*(1/4000000)*64                    
    OC1CON = 0x0006;                    //Load new compare mode to OC1CON as simple PWM.
//    IPC1bits.T2IP = 1;                  //Setup Output Compare 1 interrupt for
//    IFS0bits.T2IF = 0;                  //Clear Output Compare 1 interrupt flag
//    IEC0bits.T2IE = 1;                  //Enable Output Compare 1 interrupts
    T2CONbits.TON = 1;                  //Start Timer2 with assumed settings

}

//void __attribute__ ((__interrupt__)) _T1Interrupt(void)
//{
//    IFS0bits.T2IF = 0;
//}

void _ISR __attribute__((no_auto_psv)) _SI2C1Interrupt(void)
{
    if (IFS1bits.SI2C1IF == 1)
    {
        IFS1bits.SI2C1IF = 0;                           //clears the interrupt.
        
        //transmits data to master
        if(I2C1STATbits.R_NOT_W == 1) 
        {
            I2C1TRN = rxbuffer[txi];                    //get data to send
            I2C1CONbits.SCLREL = 1;                     //releases SCL 
            j = I2C1TRN;                                //thats it.
            txi++;
            if (txi == 2) txi=0;
            return;
        }
            
                    
        if(I2C1STATbits.RBF == 0) 
        {
            I2C1CONbits.SCLREL = 1;         //releases SCL 
            j = I2C1RCV;                    //nothing in buffer so exit.
            return;
        }
                       
        if(I2C1STATbits.D_NOT_A == 1) 
        {
            rxbuffer[rxi] = I2C1RCV;
            rxi++;
            if (rxi == 2) rxi = 0;
            j = I2C1RCV;                                                //thats it.
            return;
        }
        
    }
            j = I2C1RCV;                    //read buffer to clear flag
            
}

void main() {
    
    Init();
        
    while(1)
    {
        while((old_rxbuffer[0]!=rxbuffer[0])||(old_rxbuffer[1]!=rxbuffer[1]))
        {
            do
            {
                __delay_ms(300);
                old_rxbuffer[0] = rxbuffer[0];
                old_rxbuffer[1] = rxbuffer[1];
                
            }
            while(I2C1STATbits.RBF!=0);
            
            pwmInit();
        } 
    }
}   
    /*
    register int t1 = 0, t2 = 0;
    
    Init();
    pwmInit();
    
    
    while (1){
        
        if ((int) rxbuffer[2]==3) 
        {
            t1 = (float)rxbuffer[0];
            t2 = (float)rxbuffer[1];  
           
            //__delay_ms(1000);  
        LATBbits.LATB0 = 1;
        __delay_ms(t1);
         LATBbits.LATB0 = 0;
        __delay_ms(t2);
        }
                
    } */   

   
    
    
     










/*
void _ISR _SI2C1Interrupt()
{
    if (IFS1bits.SI2C1IF == 1)
    {
        LATBbits.LATB0 = 0;         //testing if the interrupt is working
        __delay_ms(1000);
    }
    IFS1bits.SI2C1IF = 0;           //Clears the interrupt flag
}

//Resets the I2C bus to Idle
void reset_i2c_bus(void)
{
   int x = 0;

   //initiate stop bit
   I2C1CONbits.PEN = 1;

   //wait for hardware clear of stop bit
   while (I2C1CONbits.PEN)
   {
      __delay_us(1);
      x ++;
      if (x > 20) break;
   }
   I2C1CONbits.RCEN = 0;
   IFS1bits.MI2C1IF = 0; // Clear Interrupt
   I2C1STATbits.IWCOL = 0;
   I2C1STATbits.BCL = 0;
   __delay_us(10);
}


//function initiates I2C1 module to baud rate BRG
void i2c_init(int BRG)
{
   int temp;

   //I2CBRG = 78 for 8Mhz 
    
   I2C1BRG = BRG;
   I2C1CONbits.I2CEN = 0;	// Disable I2C Mode
   I2C1CONbits.SCLREL = 1;  // Releases SCL1 clock
   I2C1CONbits.DISSLW = 1;	// Disable slew rate control
   IFS1bits.MI2C1IF = 0;	// Clear Interrupt
   I2C1CONbits.I2CEN = 1;	// Enable I2C Mode
   temp = I2C1RCV;           // read buffer to clear buffer full
   reset_i2c_bus();         // set bus to idle
}


//function initiates a start condition on bus
void i2c_start(void)
{
   int x = 0;
   I2C1CONbits.ACKDT = 0;	//Reset any previous Ack
   __delay_us(10);
   I2C1CONbits.SEN = 1;     //Initiate Start condition
   Nop();

   //the hardware will automatically clear Start Bit
   //wait for automatic clear before proceding
   while (I2C1CONbits.SEN)
   {
      __delay_us(1);
      x++;
      if (x > 20)
      break;
   }
   __delay_us(2);
}


void i2c_restart(void)
{
   int x = 0;

   I2C1CONbits.RSEN = 1;	//Initiate restart condition
   Nop();
    
   //the hardware will automatically clear restart bit
   //wait for automatic clear before proceeding
   while (I2C1CONbits.RSEN)
   {
      __delay_us(1);
      x++;
      if (x > 20)	break;
   }
    
   __delay_us(2);
}


//basic I2C byte send
char send_i2c_byte(int data)
{
   int i;

   while (I2C1STATbits.TBF) { }
   IFS1bits.MI2C1IF = 0; // Clear Interrupt
   I2C1TRN = data; // load the outgoing data byte

   // wait for transmission
   for (i=0; i<500; i++)
   {
      if (!I2C1STATbits.TRSTAT) break;
      __delay_us(1);

      }
      if (i == 500) {
      return(1);
   }

   // Check for NO_ACK from slave, abort if not found
   if (I2C1STATbits.ACKSTAT == 1)
   {
      reset_i2c_bus();
      return(1);
   }
   
   __delay_us(2);
   return(0);
}


//function reads data, returns the read data, with ack
char i2c_read_ack(void)	//does not reset bus!!!
{
   int i = 0;
   char data = 0;

   //set I2C module to receive, for some reason causes spurious SCL
   I2C1CONbits.RCEN = 1;

   //if no response, break
   while (!I2C1STATbits.RBF)
   {
      i++;
      if (i > 2000) break;
   }

   //get data from I2CRCV register
   data = I2C1RCV;

   //set ACK to high
   I2C1CONbits.ACKEN = 1;

   //wait before exiting
   __delay_us(10);

   //return data
   return data;
}



void IdleI2C1(void)
{
    // Wait until I2C Bus is Inactive //
    while(I2C1CONbits.SEN || I2C1CONbits.PEN || I2C1CONbits.RCEN || I2C1CONbits.ACKEN || I2C1CONbits.RSEN || I2C1STATbits.TRSTAT);  
}

void main(void) {
    
    int a;
    I2C1ADD = 0xBE;
    i2c_init(39);                       //BRG of 39 for 4MHz Fcy
    IEC1bits.SI2C1IE = 1;               //Slave I2C1 Event Interrupt Enable bit
    AD1PCFGbits.PCFG2 = 1;
    AD1PCFGbits.PCFG3 = 1;
    LATBbits.LATB0 = 1;
    LATBbits.LATB1 = 1;
    TRISBbits.TRISB0 = 0;
    TRISBbits.TRISB1 = 0;
    ODCBbits.ODB0 = 0;  
    ODCBbits.ODB1 = 0;
    
    
    
    while (1){
        
         __delay_ms(1000);
        //I2C1CONbits.RCEN = 1;
        //__delay_ms(1000);
        //IdleI2C1();
        //if (I2C1STATbits.RBF) a = i2c_read_ack();
        //i2c_start();
        //send_i2c_byte(0xAB);
        //reset_i2c_bus();
        //IdleI2C1();
        
        
        //IdleI2C1();
        //a = i2c_read_ack();
        //IdleI2C1();
        
        
        if (a==0xAA) {
        LATBbits.LATB0 = 0;
        __delay_ms(1000);
         LATBbits.LATB0 = 1;
        __delay_ms(1000);
        }
        
        else LATBbits.LATB1 = 0;
    
    }
    
   
    
    
    
}

*/
