/*
  Open Source Computed Tomography (CT) Scanner 
  Demonstration code -- Early release alpha state

  This code is intended to demonstrate the basic functionality of the Open Source CT scanner, and 
  verify the basic functionality of the motion and detction system. 
  
  This code is intended for pairing with:
  1) An Arduino Uno R3
  2) A shield containing at least 3 (but preferably 4) Pololu stepper drivers
  3) A shield containing an SD card socket for data storage (the data is also mirrored over the serial console)
  4) A Radiation Watch Type 5 high energy particle detector modified with the external high-sensitivity comparator board
  5) The Open Source CT 4-axis motion system
  
  This code is in an prototype alpha state, and is intended for demonstration purposes only.  
  Portions of this file are from the Radiation Watch Type 5 Arduino example code.  All 
  other portions are released under the GPL V3 license. 
  
  TODO: 
  - Add 4 axis control as soon as the official 4-axis stepper shield is complete 
    (currently my prototype shield only has room for 3 stepper controllers, so either the table or rotational axis is disabled)
  - Add end-stop support for homing all axes
  - Add accelerometer for absolute angle sensing for rotational axis
  
 */
 
#include <SPI.h>
#include <SD.h>
const int SD_CS  = 10;    // Chip select pin of SD card
 
int TYPE5_DETECTOR = 2;   // High-sensitivity signal pin of detector from external comparator

// Stepper controller pinouts
int STEPPER_SLEEP = A0;   // Enable/disable pin for stepper controller
int STEP1   = 5;
int DIR1    = 4;
int STEP2   = 6;
int DIR2    = 7;
int STEP3   = 8;
int DIR3    = 9;

// Motion Parameters
int CHANNEL_SOURCE   = 3;
int CHANNEL_DETECTOR = 2;
int CHANNEL_TABLE    = 1;
int CHANNEL_ROTATION = 4;

int travel_max_srcdet   = 2500;
int travel_max_table    = 5000;

int SOURCE_UP     = 1;
int SOURCE_DOWN   = 0;
int DETECTOR_UP   = 1;
int DETECTOR_DOWN = 0;
int TABLE_IN      = 0;
int TABLE_OUT     = 1;

// Location information
int stepper_locations[4];

// scan information
int start_linear = 0;
int end_linear = 60;
int res_linear = 5;

int start_table = 0;
int end_table = 120;
int res_table = 5;

int integrationTime = 30;      // integration time, in seconds
int detector_offset = 2;       // detector offset, in millimeters 


// STEPPER FUNCTIONS
void stepper_enable(int mode) {
  // mode = 1 enables the stepper drivers, mode = 0 disables them
  if (mode == 1) {
    // enable
    digitalWrite(STEPPER_SLEEP, HIGH);    
  } else {
    digitalWrite(STEPPER_SLEEP, LOW);
  }
}


void stepper_move(int channel, int dir, int num_steps) {
  // Step motor 'channel' one step in direction 'dir'
  int i=0;
  int step_delay = 2000;    // in microseconds
  
  if (channel == CHANNEL_TABLE) {
    // Setup direction
    if (dir == 0) {
          digitalWrite(DIR1, LOW);    
    } else {
          digitalWrite(DIR1, HIGH);    
    }
    
    for (i=0; i<num_steps; i++) {
      // Step one step
      digitalWrite(STEP1, LOW);    
      digitalWrite(STEP1, HIGH);
      delayMicroseconds(step_delay); 
      digitalWrite(STEP1, LOW);   
      delayMicroseconds(step_delay);       
    }
    
  } else if (channel == CHANNEL_DETECTOR) {
    // Setup direction
    if (dir == 0) {
          digitalWrite(DIR2, LOW);    
    } else {
          digitalWrite(DIR2, HIGH);    
    }
    
    for (i=0; i<num_steps; i++) {
      // Step one step
      digitalWrite(STEP2, LOW);    
      digitalWrite(STEP2, HIGH);
      delayMicroseconds(step_delay); 
      digitalWrite(STEP2, LOW);
      delayMicroseconds(step_delay);     
    }
    
  } else if (channel == CHANNEL_SOURCE) {
    // Setup direction
    if (dir == 0) {
          digitalWrite(DIR3, LOW);    
    } else {
          digitalWrite(DIR3, HIGH);    
    }
    
    for (i=0; i<num_steps; i++) {
      // Step one step
      digitalWrite(STEP3, LOW);    
      digitalWrite(STEP3, HIGH);
      delayMicroseconds(step_delay); 
      digitalWrite(STEP3, LOW);    
      delayMicroseconds(step_delay);       
    }
  }      
  
}


void stepper_move_absolute(int channel, int distance) {
  int travel_per_mm_linear = 20;
  int travel_per_mm_table = 600;
  int delta = distance - stepper_locations[channel];
  
  if (channel == CHANNEL_TABLE) {
    delta = delta * travel_per_mm_table;
    if (delta < 0) {
      stepper_move(channel, TABLE_OUT, -delta);
    } else { 
      stepper_move(channel, TABLE_IN, delta);
    }        
  } else if (channel == CHANNEL_DETECTOR) {
    delta = delta * travel_per_mm_linear;
    if (delta < 0) {
      stepper_move(channel, DETECTOR_DOWN, -delta);
    } else { 
      stepper_move(channel, DETECTOR_UP, delta);
    }
  } else if (channel == CHANNEL_SOURCE) {
    delta = delta * travel_per_mm_linear;
    if (delta < 0) {
      stepper_move(channel, SOURCE_DOWN, -delta);
    } else { 
      stepper_move(channel, SOURCE_UP, delta);
    }    
  }
  stepper_locations[channel] = distance;
    
}

void init_stepper_positioning() {
  stepper_enable (1);
  stepper_move (CHANNEL_SOURCE, SOURCE_DOWN, travel_max_srcdet);
  stepper_move (CHANNEL_DETECTOR, DETECTOR_DOWN, travel_max_srcdet);
  
  // Initialize absolute position information
  stepper_locations[0] = 0;
  stepper_locations[1] = 0;
  stepper_locations[2] = 0;
  stepper_locations[3] = 0;
}



void scan_source_detector() {
  // Scans the source and detector from the top to the bottom, then moves them back to the top
  
  // Step 1: Move the source and detector to the start position
  stepper_enable (1);
  stepper_move_absolute(CHANNEL_SOURCE, 0);
  stepper_move_absolute(CHANNEL_DETECTOR, 0);  
//  stepper_enable (0);
      
  int i=0;
  for (i=start_linear; i<end_linear; i+= res_linear) {
    
    int y_position = i; // in millimeters
        
    stepper_enable (1);    
    stepper_move_absolute (CHANNEL_SOURCE, y_position);
    stepper_move_absolute (CHANNEL_DETECTOR, y_position + detector_offset); 

    // wait briefly for vibrations to stop
    delay(500);

    // take measurement
    int measurement = read_type5(integrationTime);
    double cpm = (double)measurement * (60.0f / (double)integrationTime);
    
    String csv = "";
    csv += stepper_locations[CHANNEL_SOURCE];
    csv += ",";
    csv += stepper_locations[CHANNEL_TABLE];
    csv += ",";
    csv += measurement;
    csv += ",";
    csv += cpm;
    
    Serial.println(csv);
    SD_writeline(csv);
    
  }
  
  stepper_enable (0);
  //stepper_move(ch, dir, steps);
}


int read_type5(int maxSeconds) {
  // This code is adapted from the Radiation Watch Type 5 Arduino example
  
  int totalSec=0;     // Elapsed time of measurement [sec]
  int totalHour=0;    // Elapsed time of measurement [hour]

  //Time settings for CPM calcuaration
  int cpmTimeMSec=0;
  int cpmTimeSec=0;
  int cpmTimeMin=0;
  
  int startTime = millis();
  int index = 0;

  int signCount=0;  //Counter for Radiation Pulse

  int sON=0;//Lock flag for Radiation Pulse
  int nON=0;//Lock flag for Noise Puls  
  
  while (totalSec < maxSeconds) {
    
    // Raw data of Radiation Pulse: Not-detected -> High, Detected -> Low
    int sign = digitalRead(TYPE5_DETECTOR);
  
    //### Output from external comparator is inverted from normal Type 5 output (high is detected)
    if (sign == 1) {
     sign = 0;
    } else {
     sign = 1;
    }

    // Radiation Pulse normally keeps low for about 100[usec]
    if(sign==0 && sON==0) { // Deactivate Radiation Pulse counting for a while
      sON = 1;
      signCount++;
    } else if(sign==1 && sON==1){
      sON = 0;
    }

    index += 1;
    if (index == 10000) {
      int currTime = millis();
      cpmTimeMSec += abs(currTime - startTime);
      if(cpmTimeMSec >= 1000) {
        cpmTimeMSec -= 1000;
        //Add measurement time to calcurate cpm readings (max=20min.)
        if( cpmTimeSec >= 20*60 ) {
          cpmTimeSec = 20*60;
        } else {
          cpmTimeSec++;
        }
        
        //Total measurement time
        totalSec++;
      }
    }

  }
  
  return signCount;
}

void SD_writeline (String text) {
  File fp = SD.open("data.txt", FILE_WRITE);
  fp.println(text);
  fp.flush();
  fp.close(); 
}

// the setup routine runs once when you press reset:
void setup() {           
  delay(1000);      // Give a moment to open the serial console
  
  // Open the serial port
  Serial.begin(9600);
  Serial.println ("Initializing...");
  
  // sleep pin for stepper drivers
  pinMode(STEPPER_SLEEP, OUTPUT);     
  
  // step/direction control for stepper drivers
  pinMode(DIR1, OUTPUT);     
  pinMode(STEP1, OUTPUT);     
  pinMode(DIR2, OUTPUT);     
  pinMode(STEP2, OUTPUT);     
  pinMode(DIR3, OUTPUT);     
  pinMode(STEP3, OUTPUT);     
  
  // SD card
  Serial.print("Initializing SD card...");
  pinMode(SD_CS, OUTPUT);
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR: SD card failed, or not present");
    return;
  }
  Serial.println("SD card initialized...");
  SD_writeline("----------");
  
  
  // Enable the stepper drivers
  stepper_enable(1);  
  init_stepper_positioning();
  
  // Display prompt to user
  delay(2000);
  Serial.println ("Format: linear,table,measurement,cpm");
  Serial.println ("Scanning... ");
  
  int z = 0;
  for (z=start_table; z<end_table; z+=res_table) {
    scan_source_detector();
    stepper_enable (1);
    stepper_move_absolute(CHANNEL_TABLE, z);
    stepper_enable (0);
    delay(500);
  }
  
  stepper_move_absolute(CHANNEL_TABLE, 0);
  Serial.println ("");
  Serial.println ("Scan complete...");
  return;

}


void loop() {
  while(1) {
    // do nothing
  }
}
