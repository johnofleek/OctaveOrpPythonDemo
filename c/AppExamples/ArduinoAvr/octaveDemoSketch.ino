#include <crcccitt.h>
#include <debug.h>
#include <hdlc.h>
#include <orp.h>
#include <orp_input.h>
#include <orp_output.h>
#include <orp_protocol.h>

/* 
 *  For spec on ORP
 *  https://docs.octave.dev/references/edge/octave_resource_protocol/#orp-api
 */


// #include <avr/pgmspace.h>
// #include <math.h>


#include <SimpleTimer.h>  // polled timer slightly better than delay()

#define TIME_CALL_MS 1000
SimpleTimer firstTimer(TIME_CALL_MS);

SimpleTimer secondTimer((uint64_t)TIME_CALL_MS * (uint64_t)(20)); // measurement data orp rate


// ----------------------------------------------------------------------------
// called by HDLC layer to create a 100ms delay
// ----------------------------------------------------------------------------
// plaform specific sleep
void sleep100ms(void)
{
  delay(100); 
}



// static int tempPin = 1;        //the analog pin the TMP36's Vout (sense) pin is connected to

// ----------------------------------------------------------------------------
// Arduino serial port interface code 
// ----------------------------------------------------------------------------

/* Function to send out one 8bit character */
static void serial_txByte(uint8_t txByte)
{
    Serial2.print((char)txByte);
    Serial.print((char) txByte);
}

/*
SerialEvent occurs whenever a new data comes in the hardware serial port 2
RX.  
This routine is run between each time loop() runs, so using delay inside 
loop can delay response.  Multiple bytes of data may be available.

Damn I hadn't realised that this won't run if stuck in loop()
*/
void serialEvent2() 
{
    while (Serial2.available()) 
    {
        // get the new byte:
        int rxByte = Serial2.read();
          
        Serial.print((char) rxByte);
        
        // Pass all incoming data to hdlc char receiver
        // when a valid hdlc frame has been decoded an app callback function
        //  is called
        orp_protocol_processHdlcRx(rxByte);
    }
}

// ----------------------------------------------------------------------------
// orp protocol setup
// ----------------------------------------------------------------------------

static bool orp_requestAck  = false; // 

// process Octave request acks
static void app_requestResponse_cbh(const uint8_t *buffer, uint16_t bufferLength)
{
  // (void)buffer;
  (void)bufferLength;
  // TRACE(("\n\rjsonCbh <%c%c%c%c> len %d\n\r", buffer[0],buffer[1],buffer[2],buffer[3], bufferLength));
  Serial.println();
  Serial.print((char)buffer[0]); 
  Serial.print(" R(");
  Serial.print(buffer[1] - 64);  // response code
  Serial.print(") "); 
  Serial.print((char)buffer[2]); 
  Serial.print((char)buffer[3]);  
  Serial.println(" ack cb");
  
  orp_requestAck = true; // fix this later
}

// process Octave notifications
static void app_notification_cbh(const uint8_t *buffer, uint16_t bufferLength)
{
  // TRACE(("\n\rNotification Cbh <%c%c%c%c> len %d\n\r\n\r", buffer[0],buffer[1],buffer[2],buffer[3], bufferLength));
  Serial.println();
  Serial.print((char)buffer[0]); 
  Serial.print(" R(");
  Serial.print(buffer[1] - 64);  // response code
  Serial.print(") "); 
  Serial.print((char)buffer[2]); 
  Serial.print((char)buffer[3]);  
  Serial.println(" Notify cb");
  
  uint16_t ctr;

  // decode()
  for (ctr = 0; ctr < bufferLength; ctr++)
  {
    if((ctr==3) || (buffer[ctr] == ','))
    {
      ctr++;
      switch(buffer[ctr])
      {
        case 'P':
          Serial.print(("Path "));
          break;
        case 'T':
          Serial.print(("Timestamp "));
          break;
        case 'D':
          Serial.print(("Data "));
          break;
        default:
          Serial.print(("Unknown "));
          break;

      }
      ctr++;
      for(; ctr < bufferLength ; ctr++ )
      {
        // TRACE(("%c",buffer[ctr]));
        Serial.print(( char) buffer[ctr]);
        if(( (char) buffer[ctr +1]) == ',')
        {
          Serial.println("");
          break;
        }
      }
    }
  }
  Serial.println("");
}


#define ORP_PROTOCOL_TX_BUFFER_LENGTH 128
static char orpProtocol_txBuffer[ORP_PROTOCOL_TX_BUFFER_LENGTH];

#define HDLC_RX_BUFFER_LENGTH 256
static uint8_t hdlc_rxBuffer[HDLC_RX_BUFFER_LENGTH];

static void init_orp_protocol(void)
{
  orp_protocol
  (
      orpProtocol_txBuffer,           // encoder input payload 
      ORP_PROTOCOL_TX_BUFFER_LENGTH,  // encoder input payload size

      serial_txByte,                  // encoder - bind TX a byte to UART

      hdlc_rxBuffer,                  // decoder - working buffer
      HDLC_RX_BUFFER_LENGTH,          // decoder - length of working buffer
      
      app_requestResponse_cbh,        // decoder request responses from Octave 
      app_notification_cbh            // decoder notifications from Octave
    
  );
  // note the serial_rxByte() needs handling by the app feeding it data from the serial port
}

// ----------------------------------------------------------------------------
// Standard arduino setup
// ----------------------------------------------------------------------------

void setup() { 
  // Initialize serial port to 9600 baud
  Serial2.begin(9600);
  Serial.begin(115200);
  Serial.println("Setup Start");
  
  pinMode(16, OUTPUT); // Serial port TX to output
  pinMode(17, INPUT);
  digitalWrite(17, LOW); // disable internal pullup

  // Need to work out what happens by default with the adc pins
 

  // To reduce noise - connect the 3V3 source to AREF - gives 0 to 3V3 span
  // analogReference(EXTERNAL);

  init_orp_protocol();
  
  Serial.println("setup done");
}




// ----------------------------------------------------------------------------
// core state machine loop
// ----------------------------------------------------------------------------

static char ipKeyString[] = "val/adcs";
static char opKeyString[] = "motorspd/num";

void loop() 
{
  static uint8_t state = 0;
  

  if (firstTimer.isReady())
  {
    firstTimer.reset();
    
    switch(state)
    {
      case 0:
        if(orp_requestAck)
        {
          orp_requestAck = false; 
          state = 1;           
        }
        else
        {
          Serial.println("Create a json input in the Octave edge");
          orp_protocol_wakeup( sleep100ms);     
          orp_input_registerJson(ipKeyString);        
        }

        break;
      case 1:
        // Create a numnber resource in the Octave edge
        if(orp_requestAck)
        {
          orp_requestAck = false; 
          state = 2;          
        } 
        else
        {
          Serial.println("Create a numnber resource in the Octave edge ");
          orp_protocol_wakeup( sleep100ms);  
          orp_output_create_num(opKeyString); orp_requestAck = false; 
        }
        break;
      case 2:
          // Activate a callback from Octave edge output to micro
          if(orp_requestAck)
          {
            orp_requestAck = false; 
            state = 3;          
          } 
          else
          {
            Serial.println("Activate a callback from Octave edge output to micro");
            orp_output_registerCallback_num(opKeyString);  orp_requestAck = false;
          }
          break;
      case 3:
        if(secondTimer.isReady()) // send data to server
        {
          secondTimer.reset();
          Serial.println("Send data to json input");
          orp_protocol_wakeup( sleep100ms);
          orp_input_sendJson(ipKeyString, (char *) "{\"adc0\":\"1012\"}");
        }
        break;
      default:
        state = 0;  
    }
  }
}
