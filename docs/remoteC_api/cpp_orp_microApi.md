# Purpose of this document - currently PHASE 1

*PHASE 1*  
To define a C API which enables easy integration of the Octave system into basic microcontroller projects

*PHASE 2*   
In addition describe how the C API wraps the ORP (Octave Resource Protocol) wrt to the remote microcontroller



# Goals
* Code written in as generic as possible C  
* Targets microcontrollers only
* Doesn't require an OS
* The API should be simple to use  
* The final code should be portable to other microcontrollers
* limited set of host microcontroller software dependencies

# Description
End users will include the code in their project and use the API to 
* Send data to Octave
* Receive unsolicited data from Octave 

The host microcontroller will provide certain interfaces for example
 non-blocking UART interfaces - full info TBD

# Limitations
It is proposed at this stage to implement a subset of the ORP
 protocol supporting only 
* Octave inputs 
* Octave outputs

# Terms
Remote - this is the microcontroller
Octave Edge - this the WP7702 or whatever Octave device is in the field
Octave Cloud - this is Sierra's cloud compute sytem
Octave_RemoteMicro_api - the api that is defined in this document
Octave_RemoteMicro_code - the code which implements the api and the functionality


---


# C API design philosophy

KEY / VALUE pairs will be implemented as strings

Values and Keys will be truncated at a size defined by C storage. The
 user will need to handle any errors caused by truncation  


## Octave output data

Data received from Octave is restricted to individual KEY / VALUE pairs. This
 removes the need to decode incoming data in a difficult to parse
 in C format like JSON.
 
My thinking is that if a consistent KEY format is used it will be easy to post
 process the data in the Octave cloud or the edge.
 
*Example output VALUE*  
Note that strings have been used to represent all types.

Numbers - expect a float
"101"  
"23.27"    

Strings
"powerOff"    

Bool  
"true"  
"false"  


*Example output KEY*  
A suggestion is that a meaningful KEY name is used which describes both the
 function and the datatype
 
A suggestion is to embed the datatype in the KEY path   
str  
num    
bool  


A remote KEY example
<dd>adc1/num</dd>  


Results in an Octave path   
    /remote/adc1/num/value
 
## Octave inputs
Data sent to Octave is encoded in simple JSON strings as it's easy to encode
 in C using prebuilt functions like snprintf(). By using
 [JSON](https://www.json.org/json-en.html) the value encoding used in JSON
 can be used as JSON data is supported by the Octave edge system.
 
For example the following JSON encoded data could be sent to the Octave edge  

{“temperature”:123.2,“humidity”:70,“pressure”:9997,“powerOn”:true}


## Octave interaction - remote initiated
The basic Octave ORP cycle is
1. Remote sends a message to Octave Edge
2. Octave edge responds with an ack / nack message

It is proposed to handle this cycle by implementing the following in the Octave_RemoteMicro_code

* The users remote application calls a Octave_RemoteMicro_Api function which sends
  an ORP message to the Octave Edge device to the serial port and then returns
* The Octave system then processes the ORP message
* After the Octave system has finished processing the ORP message it sends 
  a response message to the Octave_RemoteMicro_code code via the serial port
* The Octave_RemoteMicro_code decodes the response message
* The Octave_RemoteMicro_code calls the appropriate users callback* function which
  was registered during initialisation of the Octave_RemoteMicro_Api

## Octave interaction - Octave initiated



---

# C API

// Low level platform dependencies
/// serial port from Octave to Micro

The target micro provides these functions to the micro Octave remote C code 
serial_rxByte(uint8_t data);
serial_rxByte(uint8_t data),

// The micro Octave remote C API
octave_init(
    &serial_txByte(uint8_t),
    &serial_rxByte(uint8_t data),
    const uint8_t *HDLC_frameBuffer,
    uint16_t HDLC_frameBuffer_length), 
    uint16_t HDLC_maxFrameLength,      // maybe hide this? 
    
)

// Outputs are output from Octave
octaveResponse  octave_registerOutput(char * key, outputCallbackHandler);

*prototype of callback*
void outputCallbackHandler (char * key, char * value);

octaveResponse  octave_registerInput(char * key, char * value);


--- 
# Notes
Is this blocking or none blocking
What about failures?
 
