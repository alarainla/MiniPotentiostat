/**************************************************************************/
/*
 * This is firmware for a open-source portable potentiostat
 * Developed by Roman Landin, KTH and Alar Ainla, International Iberian Nanotechnology Laboratory INL
 * Hamedi Lab, KTH (Royal University of Technology), Stockholm, Sweden
 * November 2020
 * Version: 1.0
 * Communication: 57600 baud Serial
 * Commands:
 * A set DAC for potential (Default: 2048)
 * B set offset DAC (Default: 4095), normally not changed
 * C set base time in us (Default: 10000, 10ms), Min: 2500
 * D set potential step (odd step)
 * E set potential step (even step)
 * F number of steps in the scan (up to 10000)
 * G run the sequence
 * H halt the sequence
 * I ask the current measurement
 * J number of ADC values to integrate (Default: 16)
 * Each command requires numerical parameter after it, if its not really needed use just 0
 * Every command return confirmation: small corresponding letter and :DONE
 * I returns i:current value, G returns data: Step,Time(us),Potential,Current
 * All communication is ASCII text
 */
/**************************************************************************/
#include <Wire.h>
#include <stdlib.h>

//Main DAC for potential
#define DAC1 0x60
//Offset DAC
#define DAC2 0x61

//Global variables
unsigned int dac=2048;    //DAC potential (Default: middle --> 0V)
unsigned int offset=4095; //Offset potential (Default: max)
unsigned long timerBaseStep=10000; //Time base step in us (default: 10ms)
int IncOdd=1;             //Potential change off step
int IncEven=1;            //Potential change even step
int numberOfSteps=1;      //Number of steps in the scan
bool sRunning=false;      //If scan is running
int N=0;                  //Number of the data point in the scan
long current;             //Current value
unsigned int N_Cyc=16;    //Integration of ADC values (Default 16!)
unsigned long nextStep;   //Next step to make in us
unsigned long currTime;   //Current time in us
unsigned int dac_prev;    //Last dac value before update
char cmd;                 //Command received
long param;                //Command parameter
unsigned long begTime;    //Beginning time in us

//Setup the device
void setup(void) {
  Serial.begin(57600);
  Wire.begin();
  setDAC(DAC2,offset);
  setDAC(DAC1,dac);
  pinMode(A3,INPUT);  
}

//Main loop
void loop(void) {
  long tmp;
  //Check about timing
  if(sRunning)
  {
    currTime=micros();
    if(currTime>nextStep) //Time to do something
    {
      //Read current
      current=readADC();
      dac_prev=dac;
      //Set potential
      N=N+1;
      if(N>numberOfSteps) //Done
      {
        sRunning=false;
      }else //Set potentials
      {
        tmp=dac;
        if(N%2==0) //Even step
        {
           tmp=tmp+IncEven; //Make step    
        }else //Odd step
        {
           tmp=tmp+IncOdd; //Make step
        }
        if(tmp>4095) { tmp=4095; }else if(tmp<0){ tmp=0; }else; // Check that end value is in the range
        dac=(unsigned int)tmp; setDAC(DAC1, dac); //Set value   
      }
      //Transmit values
      Serial.print('>'); //Start symbol of data
      Serial.print(N-1,DEC);
      Serial.print(',');
      Serial.print(currTime-begTime,DEC);
      Serial.print(',');
      Serial.print(dac_prev,DEC);
      Serial.print(',');   
      Serial.println(current,DEC);         
      nextStep=currTime+timerBaseStep;
      if(!sRunning)
        Serial.println("g:DONE");
    }    
  }
  if(Serial.available()>0)
  {
    cmd=Serial.read();
    delay(1);
    if(Serial.available()>0)
    {
      param=Serial.parseInt();
      action(); //If correct format perform action
    }
  }

}

//Interprete command and perform action
bool action()
{
  switch(cmd)
  {
    case 'A': //Set main dac
              dac=(unsigned int)param;
              setDAC(DAC1, dac);
              Serial.println("a:DONE");
              return true; 
    case 'B': //set offset
              offset=(unsigned int)param;
              setDAC(DAC2, offset);
              Serial.println("b:DONE");
              return true;         
    case 'C': //Set base timer in microsecond min: 2500, max: N/A unsigned long
              timerBaseStep=(unsigned long)param;
              if(timerBaseStep<2500) timerBaseStep=2500;            
              Serial.println("c:DONE");
              return true;     
    case 'D': //Set scan rate for electrode reference channel to increase at odd steps
              if(param>32767){ IncOdd=32767; }else
              if(param<-32768){ IncOdd=-32768; }else
              { IncOdd=(int)param; }
              Serial.println("d:DONE");
              return true;      
    case 'E': //Set scan rate for reference electrode channel to increase at even steps
              if(param>32767){ IncEven=32767; }else
              if(param<-32768){ IncEven=-32768; }else
              { IncEven=(int)param; }
              Serial.println("e:DONE");  
              return true;     
    case 'F': //Set number of step to make during the scan
              if(param>10000){ numberOfSteps=10000; }else
              if(param<1){ numberOfSteps=1; }else
              { numberOfSteps=(int)param; }
              Serial.println("f:DONE");
              return true;    
    case 'G': //Run the sequence
              nextStep=micros();
              begTime=nextStep;
              sRunning=true;
              N=0;
              return true;             
    case 'H': //Halt the sequence
              sRunning=false;
              N=0;
              Serial.println("h:DONE");
              return true;  
    case 'I': //Ask ADC value
              current=readADC();
              Serial.print("i:DONE");
              Serial.println(current,DEC);              
              return true;      
    case 'J': //Integration time
              N_Cyc=(unsigned int)param;
              Serial.print("i:DONE");            
              return true;      
                               
    default:
      return false;     
  }
}

//Set DAC MCP4725
//Execution time about: 425us
void setDAC(char address, unsigned int value)
{
  Wire.beginTransmission(address);
  Wire.write(64);                     // cmd to update the DAC
  Wire.write(value >> 4);        // the 8 most significant bits...
  Wire.write((value & 15) << 4); // the 4 least significant bits...
  Wire.endTransmission();  
}

//Read Analog signal 16 bit
//Execution time about: 1792us (N_Cyc=16)
long readADC()
{
  long value=0;
  for(int i=0; i<N_Cyc; i++)
    value=value+analogRead(A3);
  return value;
}

// END OF CODE
