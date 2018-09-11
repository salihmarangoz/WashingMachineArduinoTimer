// BSD 3-Clause License
// Copyright (c) 2018, Salih Marangoz
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// 
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/**
  * Arduino Based Timer for Washing Machine
  * Salih MARANGOZ - 2018
  */

#include <SegmentDisplay.h> // https://github.com/dgduncan/SevenSegment
#include <Fsm.h>            // https://github.com/jonblack/arduino-fsm
#include <Chrono.h>         // https://github.com/SofaPirate/Chrono

#define DEBUG
#ifdef DEBUG
  #define SERIAL_LOG(M) Serial.println(M)
#else
  #define SERIAL_LOG(M)
#endif

//////////// CONSTANTS ////////////
//10, 9, CA, 7, 6
// ____________
//|            |
//|            |
//|            |
//|            |
//|            |
//|____________|
//1, 2, CA, 4, 5
SegmentDisplay seg(6, 7, 8, 9, 3, 11, 10, 5); // Look above or github page for pin connection
const int  relay_pin   =     12;
const int  button_pin  =     2;
const int relay_trigger_duration = 1500; // in ms

//////////// PROTOTYPES AND TYPEDEFS ////////////
void resetOnState();
void checkButtonOnState();
void checkTimeOnState();
void modifyTimeOnState();
void resetTimeOnState();
void triggerMachineOnState();
void updateDisplayOnState();

typedef enum
{
    EVENT_OK = 0,
    EVENT_BUTTON_SHORT_PRESSED,
    EVENT_BUTTON_LONG_PRESSED,
    EVENT_TIME_OUT,
    EVENT_TIME_PASSED_30_MIN
}Event;

String event_name[5] = {"EVENT_OK", "EVENT_BUTTON_SHORT_PRESSED", "EVENT_BUTTON_LONG_PRESSED", "EVENT_TIME_OUT", "EVENT_TIME_PASSED_30_MIN"};

//////////// GLOBAL VARIABLES ////////////
Event current_event = EVENT_OK;
State state_reset(NULL, resetOnState, NULL);
State state_check_button(NULL, checkButtonOnState, NULL);
State state_check_time(NULL, checkTimeOnState, NULL);
State state_modify_time(NULL, modifyTimeOnState, NULL);
State state_reset_time(NULL, resetTimeOnState, NULL);
State state_trigger_machine(NULL, triggerMachineOnState, NULL);
State state_update_display(NULL, updateDisplayOnState, NULL);
Fsm fsm(&state_reset);
Chrono chrono(Chrono::SECONDS);
unsigned long timeout_seconds;


//////////// SETUP FUNCTION ////////////
void setup() {
    Serial.begin(9600);
    pinMode(relay_pin, OUTPUT);
    digitalWrite(relay_pin, HIGH);
    
    fsm.add_transition(&state_reset, &state_check_button, EVENT_OK, NULL);
    fsm.add_transition(&state_check_button, &state_check_time, EVENT_OK, NULL);
    fsm.add_transition(&state_check_time, &state_check_button, EVENT_OK, NULL);
    fsm.add_transition(&state_check_button, &state_modify_time, EVENT_BUTTON_SHORT_PRESSED, NULL);
    fsm.add_transition(&state_modify_time, &state_check_time, EVENT_OK, NULL);
    fsm.add_transition(&state_check_button, &state_reset_time, EVENT_BUTTON_LONG_PRESSED, NULL);
    fsm.add_transition(&state_reset_time, &state_check_time, EVENT_OK, NULL);
    fsm.add_transition(&state_check_time, &state_update_display, EVENT_TIME_PASSED_30_MIN, NULL);
    fsm.add_transition(&state_update_display, &state_check_button, EVENT_OK, NULL);
    fsm.add_transition(&state_check_time, &state_trigger_machine, EVENT_TIME_OUT, NULL);
    fsm.add_transition(&state_trigger_machine, &state_reset, EVENT_OK, NULL);
}

//////////// MAIN LOOP FUNCTION ////////////
void loop() {
    SERIAL_LOG("Current Event: " + String(current_event));
    fsm.trigger(current_event);
    fsm.run_machine();
    SERIAL_LOG(String(chrono.elapsed()));
    SERIAL_LOG("====================");
}

//////////// ON-STATE CALLBACK FUNCTIONS ////////////
void resetOnState() // done
{
    SERIAL_LOG("Entered resetOnState");
#ifdef DEBUG
    SERIAL_LOG("Running testDisplay()");
    seg.testDisplay();
#endif
    seg.displayDecimalPoint();
    chrono.restart();
    chrono.stop();
    timeout_seconds = -1;
    current_event = EVENT_OK;
}

void checkButtonOnState()
{
    SERIAL_LOG("Entered checkButtonOnState");
}

void checkTimeOnState() // done
{
    SERIAL_LOG("Entered checkTimeOnState");
    if (chrono.hasPassed(timeout_seconds))
    {
        current_event = EVENT_TIME_OUT;
        return;
    }
    
    current_event = EVENT_OK;
}

void modifyTimeOnState()
{
    SERIAL_LOG("Entered modifyTimeOnState");
}

void resetTimeOnState() // done
{
    SERIAL_LOG("Entered resetTimeOnState");
    chrono.restart();
    chrono.stop();
    seg.displayDecimalPoint();
    current_event = EVENT_OK;
}

void triggerMachineOnState() // done
{
    SERIAL_LOG("Entered triggerMachineOnState");
    
    SERIAL_LOG("Relay switched on");
    digitalWrite(relay_pin, LOW);
    
    delay(relay_trigger_duration);

    SERIAL_LOG("Relay switched off");
    digitalWrite(relay_pin, HIGH);

    current_event = EVENT_OK;
}

void updateDisplayOnState()
{
    SERIAL_LOG("Entered updateDisplayOnState");
}



