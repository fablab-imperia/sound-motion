#include <FatReader.h>
#include <SdReader.h>
#include <avr/pgmspace.h>
#include "WaveUtil.h"
#include "WaveHC.h"
#include "WaveShieldIniFile.h"
#include <NewPing.h>

#define TRIGGER_PIN  A4  // Arduino pin tied to trigger pin on ping sensor.
#define ECHO_PIN     A5  // Arduino pin tied to echo pin on ping sensor.
#define MAX_DISTANCE 300
#define MAX_TRIGGER_DISTANCE 120
#define MIN_TRIGGER_DISTANCE 20
#define BUFFER_SIZE 128


FatReader root;   // This holds the information for the filesystem on the card
FatReader f;      // This holds the information for the file we're play

char filename[20];

WaveHC wave;      // This is the only wave (audio) object, since we will only play one at a time
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

// this handy function will return the number of bytes currently free in RAM, great for debugging!   
int freeRam(void)
{
  extern int  __bss_end; 
  extern int  *__brkval; 
  int free_memory; 
  if((int)__brkval == 0) {
    free_memory = ((int)&free_memory) - ((int)&__bss_end); 
  }
  else {
    free_memory = ((int)&free_memory) - ((int)__brkval); 
  }
  return free_memory; 
} 

void sdErrorCheck(SdReader& card)
{
  if (!card.errorCode()) return;
  putstring("\n\rSD I/O error: ");
  Serial.print(card.errorCode(), HEX);
  putstring(", ");
  Serial.println(card.errorData(), HEX);
  while(1);
}

void setup() 
{
  // set up serial port
  Serial.begin(9600);
  putstring_nl("WaveHC with 6 buttons");
  
   putstring("Free RAM: ");       // This can help with debugging, running out of RAM is bad
  Serial.println(freeRam());      // if this is under 150 bytes it may spell trouble!
  
  // Set the output pins for the DAC control. This pins are defined in the library
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
 
  // pin13 LED
  pinMode(13, OUTPUT);

  SdReader card;    // This object holds the information for the card
  FatVolume vol;    // This holds the information for the partition on the card
 
  if (!card.init(true)) { //play with 4 MHz spi if 8MHz isn't working for you
  //if (!card.init()) {         //play with 8 MHz spi (default faster!)  
    putstring_nl("Card init. failed!");  // Something went wrong, lets print out why
    sdErrorCheck(card);
    while(1);                            // then 'halt' - do nothing!
  }
  
  // enable optimize read - some cards may timeout. Disable if you're having problems
  card.partialBlockRead(true);
 
// Now we will look for a FAT partition!
  uint8_t part;
  for (part = 0; part < 5; part++) {     // we have up to 5 slots to look in
    if (vol.init(card, part)) 
      break;                             // we found one, lets bail
  }
  if (part == 5) {                       // if we ended up not finding one  :(
    putstring_nl("No valid FAT partition!");
    sdErrorCheck(card);      // Something went wrong, lets print out why
    while(1);                            // then 'halt' - do nothing!
  }
  
  // Lets tell the user about what we found
  putstring("Using partition ");
  Serial.print(part, DEC);
  putstring(", type is FAT");
  Serial.println(vol.fatType(),DEC);     // FAT16 or FAT32?
  
  // Try to open the root directory
  if (!root.openRoot(vol)) {
    putstring_nl("Can't open root dir!"); // Something went wrong,
    while(1);                             // then 'halt' - do nothing!
  }

  WaveShieldIniFile ini(f);
  ini.open(root,"CONF.INI");

  char buffer[BUFFER_SIZE];
  
  // Check the file is valid. This can be used to warn if any lines
  // are longer than the buffer.
  if (!ini.validate(buffer, BUFFER_SIZE)) 
  {
    Serial.print("ini file ");
    Serial.print(" not valid ");

    // Cannot do anything else
    while (1);
  }
  
  // Fetch a value from a key which is present
  if (ini.getValue("Stellaria", "FileWav", buffer, BUFFER_SIZE)) {
    Serial.print("section 'Stellaria' has an entry 'FileWav' with value ");
    strncpy(filename,buffer,20);
    Serial.println(filename);
  }
  else {
    Serial.print("Could not read 'FileWav' from section 'Stellaria'");
    while (1);
  }
  
  // Whew! We got past the tough parts.
  putstring_nl("Ready to play!");
}


// Plays a full file from beginning to end with no pause.
void playcomplete(char *name) {
  // call our helper to find and play this name
  playfile(name);
  while (wave.isplaying) {
  // do nothing while its playing
  }
  // now its done playing
}

void playfile(char *name) {
  // see if the wave object is currently doing something
  if (wave.isplaying) {// already playing something, so stop it!
    wave.stop(); // stop it
  }
  // look in the root directory and open the file
  if (!f.open(root, name)) {
    putstring("Couldn't open file "); Serial.print(name); return;
  }
  // OK read the file and turn it into a wave object
  if (!wave.create(f)) {
    putstring_nl("Not a valid WAV"); return;
  }
  
  // ok time to play! start playback
  wave.play();
}


void loop() {


  long echoTime = sonar.ping_median();
  unsigned int distanceCm = sonar.convert_cm(echoTime); 
  
  Serial.print("PING: ");
  Serial.println(distanceCm);

  //if we are inside the range for trigger
  if (distanceCm > MIN_TRIGGER_DISTANCE && distanceCm < MAX_TRIGGER_DISTANCE)
  {
      playcomplete(filename);
  }

  delay(200);
}


