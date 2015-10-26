/*
PBMonitor: Monitor a Pellet Burner
Copyright (C) 2015  Carsten Agerskov

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <SPI.h>
#include <MySensor.h>  
#include <Bounce2.h>

#include "FilterStatus.h"

#define LED_PIN 13

#define ALARM_PIN 2

#define THRESHOLD_STANDBY 400
#define THRESHOLD_LOW_POWER 400
#define THRESHOLD_HIGH_POWER 400
#define TIME_THRESHOLD 10000
#define BLINK_INTERVAL 500
#define STAND_BY 0
#define START 1
#define LOW_POWER 2
#define HIGH_POWER 3
#define ALARM_UPDATE_INTERVAL 30*1000

int state = STAND_BY;
int lastState = HIGH_POWER;
int alarmValue = 0;
int lastAlarmValue = 0;

int node_id = 50; 

#define CHILD_ID_STATE 1
#define CHILD_ID_ALARM 2

unsigned long lastAlarm = 0;

MySensor gw;
MyMessage msgStatus(CHILD_ID_STATE, V_VAR1);
MyMessage msgAlarm(CHILD_ID_ALARM, V_TRIPPED);
Bounce debouncerAlarm = Bounce(); 

FilterStatus highPower = FilterStatus(THRESHOLD_HIGH_POWER, 0, A0, TIME_THRESHOLD);
FilterStatus lowPower = FilterStatus(THRESHOLD_LOW_POWER, 0, A1, TIME_THRESHOLD);
FilterStatus standBy = FilterStatus(THRESHOLD_STANDBY, BLINK_INTERVAL, A2, TIME_THRESHOLD);

void setup() {
	Serial.begin(115000);
	pinMode(LED_PIN, OUTPUT);
	pinMode(ALARM_PIN, INPUT_PULLUP);
	digitalWrite(A0, HIGH);
	digitalWrite(A1, HIGH);
	digitalWrite(A2, HIGH);
	Serial.println("Burner monitor restarting");
	standBy.checkStatus();
	lowPower.checkStatus();
	highPower.checkStatus();
	standBy.getStatus();
	lowPower.getStatus();
	highPower.getStatus();
	delay(TIME_THRESHOLD+100);

    gw.begin(NULL,node_id);

    // Send the Sketch Version Information to the Gateway
    gw.sendSketchInfo("Heater", "1.0");

    // Register all sensors to gw (they will be created as child devices)
    gw.present(CHILD_ID_STATE, S_CUSTOM);
    gw.present(CHILD_ID_ALARM, S_CUSTOM);
    
    debouncerAlarm.attach(ALARM_PIN);
    debouncerAlarm.interval(50);

	Serial.println("Burner monitor running");
}

void loop() {
	standBy.checkStatus();
	lowPower.checkStatus();
	highPower.checkStatus();

    debouncerAlarm.update();
	alarmValue =  debouncerAlarm.read();

	if( alarmValue != lastAlarmValue ) {
		lastAlarmValue = alarmValue;
		lastAlarm = millis();
		gw.send(msgAlarm.set(alarmValue==HIGH ? 1 : 0));
	}

    if( alarmValue && (millis() - lastAlarm) >= ALARM_UPDATE_INTERVAL ) {
  		lastAlarm = millis();
		gw.send(msgAlarm.set(alarmValue==HIGH ? 1 : 0));
	}
	

    if( standBy.getStatus() == LIGHT_ON && lowPower.getStatus()==LIGHT_OFF )	 {
		state = STAND_BY;
    }
    if( standBy.getStatus() == LIGHT_BLINK && lowPower.getStatus()==LIGHT_OFF )	 {
		state = START;
    }
    if( lowPower.getStatus()==LIGHT_ON && highPower.getStatus()==LIGHT_OFF )	 {
 		state = LOW_POWER;
    }
    if( highPower.getStatus()==LIGHT_ON )	 {
		state = HIGH_POWER;
    }

	if( state != lastState ) {
		gw.send(msgStatus.set(state));
		gw.send(msgAlarm.set(alarmValue==HIGH ? 1 : 0));
		lastState = state;
	}    
}

