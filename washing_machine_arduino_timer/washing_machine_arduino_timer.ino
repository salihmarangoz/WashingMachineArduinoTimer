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
SegmentDisplay seg_(6, 7, 8, 9, 3, 11, 10, 5);  // Look above or github page for pin connection
const int  relay_pin_             = 12;
const int  button_pin_            = 2;
const int relay_trigger_duration_ = 1500;       // in ms
const int hours_upper_limit_      = 10;          // limited to 9  hours (exclusive)

//////////// PROTOTYPES AND TYPEDEFS ////////////
void resetOnState();
void checkButtonOnState();
void checkTimeOnState();
void updateTimeOnState();
void resetTimeOnState();
void triggerMachineOnState();
void updateDisplayOnState();
unsigned int secondsToHours(unsigned long long sec);

typedef enum
{
    EVENT_OK = 0,
    EVENT_BUTTON_SHORT_PRESSED,
    EVENT_BUTTON_LONG_PRESSED,
    EVENT_TIME_OUT,
}Event;
String event_name_[] = {"EVENT_OK", "EVENT_BUTTON_SHORT_PRESSED", "EVENT_BUTTON_LONG_PRESSED", "EVENT_TIME_OUT"};

//////////// GLOBAL VARIABLES ////////////
Event current_event_ = EVENT_OK;
State state_reset_(NULL, resetOnState, NULL);
State state_check_button_(NULL, checkButtonOnState, NULL);
State state_check_time_(NULL, checkTimeOnState, NULL);
State state_update_time_(NULL, updateTimeOnState, NULL);
State state_trigger_machine_(NULL, triggerMachineOnState, NULL);
State state_update_display_(NULL, updateDisplayOnState, NULL);
Fsm fsm_(&state_reset_);
Chrono chrono_(Chrono::SECONDS);
unsigned long long timeout_seconds_;


//////////// SETUP FUNCTION ////////////
void setup() {
    Serial.begin(9600);
    pinMode(relay_pin_, OUTPUT);
    digitalWrite(relay_pin_, HIGH);
    
    fsm_.add_transition(&state_reset_, &state_check_button_, EVENT_OK, NULL);
    fsm_.add_transition(&state_check_button_, &state_check_time_, EVENT_OK, NULL);
    fsm_.add_transition(&state_check_button_, &state_update_time_, EVENT_BUTTON_SHORT_PRESSED, NULL);
    fsm_.add_transition(&state_update_time_, &state_check_time_, EVENT_OK, NULL);
    fsm_.add_transition(&state_check_button_, &state_reset_, EVENT_BUTTON_LONG_PRESSED, NULL);
    fsm_.add_transition(&state_check_time_, &state_update_display_, EVENT_OK, NULL);
    fsm_.add_transition(&state_update_display_, &state_check_button_, EVENT_OK, NULL);
    fsm_.add_transition(&state_check_time_, &state_trigger_machine_, EVENT_TIME_OUT, NULL);
    fsm_.add_transition(&state_trigger_machine_, &state_reset_, EVENT_OK, NULL);

#ifdef DEBUG
    //SERIAL_LOG("Running seven segment display test");
    //seg_.testDisplay();
#endif
}

//////////// MAIN LOOP FUNCTION ////////////
void loop() {
    SERIAL_LOG("Current Event: " + String(event_name_[current_event_]));
    fsm_.trigger(current_event_);
    fsm_.run_machine();
    SERIAL_LOG("====================");
}

//////////// ON-STATE CALLBACK FUNCTIONS ////////////
void resetOnState()
{
    SERIAL_LOG("Entered resetOnState");
    
    chrono_.restart();
    chrono_.stop();
    timeout_seconds_ = 0;
    
    current_event_ = EVENT_OK;
}

void checkButtonOnState()
{
    SERIAL_LOG("Entered checkButtonOnState");

    // TODO: READ BUTTON HERE !
}

void checkTimeOnState()
{
    SERIAL_LOG("Entered checkTimeOnState");

    SERIAL_LOG("Timeout Seconds: " + String((int)timeout_seconds_));  // String() constructor does not accept unsigned long long ? :(
    SERIAL_LOG("Chrono Elapsed: " + String(chrono_.elapsed()));
    SERIAL_LOG("Chrono isRunning: " + String(chrono_.isRunning()));
    
    if (chrono_.hasPassed(timeout_seconds_) && timeout_seconds_ != 0)
    {
        current_event_ = EVENT_TIME_OUT;
        return;
    }
    
    current_event_ = EVENT_OK;
}

void updateTimeOnState()
{
    SERIAL_LOG("Entered modifyTimeOnState");

    // Add one hour to timeout
    unsigned int timeout_hours = timeout_seconds_ / 3600;
    timeout_hours = (timeout_hours+1) % (hours_upper_limit_);
    timeout_seconds_ = timeout_hours * 3600;
    SERIAL_LOG("Timeout seconds changed to: " + String((int)timeout_seconds_));

    // Reset chrono
    chrono_.restart();
    chrono_.stop();
    SERIAL_LOG("Chrono reset");

    if (timeout_seconds_ != 0)
    {
        chrono_.start();
        SERIAL_LOG("Chrono started");
    }
    
    current_event_ = EVENT_OK;
}

void triggerMachineOnState()
{
    SERIAL_LOG("Entered triggerMachineOnState");
    
    SERIAL_LOG("Relay switched on");
    digitalWrite(relay_pin_, LOW);
    
    delay(relay_trigger_duration_);

    SERIAL_LOG("Relay switched off");
    digitalWrite(relay_pin_, HIGH);

    current_event_ = EVENT_OK;
}

void updateDisplayOnState()
{
    SERIAL_LOG("Entered updateDisplayOnState");
    
    unsigned int hours = timeout_seconds_ / 3600;
    seg_.displayHex(hours, false);
    SERIAL_LOG("Displaying digit: " + String(hours));

    current_event_ = EVENT_OK;
}

