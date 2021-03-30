/*  OPDRACHT     : Eindopdracht EDRSD studiejaar 2020-2021
 *  OPDRACHTGEVER: Bart Snijder
 *  AUTEUR       : Mark Huizenga
 *  STUDENTNUMMER: 1155088
 *  GEWIJZIGD    : 30 maart 2021
 *  Variant C.
 */
 
#include "mbed.h"

enum STATES{STOP, VOORUIT, ACHTERUIT, PWM};                     //init 
enum EVENTS {NO_EVENT, START_STOP_PUSHED, MODE_PUSHED};

STATES state;
EVENTS event;

AnalogIn potDutyCycle(PA_1);
AnalogIn cny70(PA_3);
InterruptIn startStop(PB_6, PullDown);
InterruptIn mode(PB_7, PullDown);

PwmOut motor(PB_4);
DigitalOut TPA_5(PF_0);
DigitalOut TPA_6(PF_1);

Timer timer;

Serial pc(USBTX, USBRX);

int dutyCycle;
int newDutyCycle;

void setClockwise(){
    TPA_5 = 0;
    TPA_6 = 1;
}

void setCounterClockwise(){
    TPA_5 = 1;
    TPA_6 = 0;
}

void setStop(){
    TPA_5 = 0;
    TPA_6 = 0;
}

void stateMachine(EVENTS inputEvent){
    switch(state){
        case VOORUIT:
            setClockwise();
            timer.stop();
            timer.reset();
            if (inputEvent == START_STOP_PUSHED){
                state = STOP;
                event = NO_EVENT;
                pc.printf("De motor staat nu stil.\n");
            } else if (inputEvent == MODE_PUSHED){
                state = PWM;
                event = NO_EVENT;
                pc.printf("PWM instellingen geopend.\n");
                pc.printf("PWM duty cycle = %d%%\n", dutyCycle);
            } else if (cny70.read() < 0.2f){
                setStop();
                wait(0.5);
                state = ACHTERUIT;
                pc.printf("De motor draait nu tegen de klok in.\n");
            } else {
                state = VOORUIT;
            }
        break;
        case ACHTERUIT:
            setCounterClockwise();
            timer.start();
            if (inputEvent == START_STOP_PUSHED){
                state = STOP;
                event = NO_EVENT;
                pc.printf("De motor staat nu stil.\n");
            } else if (inputEvent == MODE_PUSHED){
                state = PWM;
                event = NO_EVENT;
                pc.printf("PWM instellingen geopend.\n");
                pc.printf("PWM duty cycle = %d%%\n", dutyCycle);
            } else if (timer > 4){
                setStop();
                wait(0.5);
                state = VOORUIT;
                event = NO_EVENT;
                pc.printf("De motor draait nu kloksgewijs.\n");
            } else {
                state = ACHTERUIT;
            }
        break;
        case STOP:
            setStop();
            timer.stop();
            timer.reset();
            if (inputEvent == START_STOP_PUSHED){
                state = VOORUIT;
                event = NO_EVENT;
                pc.printf("De motor draait nu kloksgewijs.\n");
            } else if (inputEvent == MODE_PUSHED){
                state = PWM;
                event = NO_EVENT;
                pc.printf("PWM instellingen geopend.\n");
                pc.printf("PWM duty cycle = %d%%\n", dutyCycle);
            } else {
                state = STOP;
            }
        break;
        case PWM:
            setStop();
            timer.stop();
            timer.reset();
            if (inputEvent == START_STOP_PUSHED){
                state = STOP;
                event = NO_EVENT;
                pc.printf("PWM instellingen gesloten.\n");
                pc.printf("De motor staat nu stil.\n");
            } else if (inputEvent == MODE_PUSHED){
                state = VOORUIT;
                event = NO_EVENT;
                motor.write(potDutyCycle.read());
                pc.printf("PWM instellingen gesloten.\n");
                pc.printf("De motor draait nu kloksgewijs.\n");
            } else {
                newDutyCycle = potDutyCycle.read()*100;
                if (newDutyCycle != dutyCycle){
                    dutyCycle = newDutyCycle;
                    pc.printf("PWM duty cycle = %d%%\n", dutyCycle);
                }
                state = PWM;
            }
        break;
    }
}

void startStopButtonPushed(){
    event = START_STOP_PUSHED;
}

void modeButtonPushed(){
    event = MODE_PUSHED;
}

int main() {
    motor.period(0.05);
    motor.write(potDutyCycle.read());
    
    dutyCycle = potDutyCycle.read()*100;
    
    startStop.rise(startStopButtonPushed);
    mode.rise(modeButtonPushed);

    pc.printf("De motor staat nu stil.\n");
    
    while (true) {
        stateMachine(event);
        wait_ms(10);
    }
}
