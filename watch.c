#include <8051.h>

__xdata unsigned char *CSDS = (__xdata unsigned char *) 0xFF30;
__xdata unsigned char *CSDB = (__xdata unsigned char *) 0xFF38;
__bit __at (0x96) SEG_OFF;

const unsigned char digits[10] =  {0b00111111,0b00000110, 0b01011011,0b01001111, 0b01100110, 0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01101111};
unsigned char time[7] = {0, 0, 0, 0, 0, 0, 0};
unsigned char keyb_state = 0;
unsigned char ss, mm, gg;
unsigned char led_bit;
unsigned char led_idx;
unsigned char selected_seg = 0;
__bit conf_mode = 0;
int counter;
__bit hide = 0;

void t0_int(void) __interrupt(1) {
	TH0 = 232;
	F0 = 1;
}

void set_time() {
	time[0] = ss % 10;
    time[1] = ss / 10;
    time[2] = mm % 10;
    time[3] = mm / 10;
    time[4] = gg % 10;
    time[5] = gg / 10;
}

void inc_hours(){
	gg++;

	if(gg == 24) {
		gg = 0;
	}
}

void inc_min(){
	mm++;

	if(mm == 60) {
		mm = 0;
		if(!conf_mode){
			inc_hours();
 	    }
	}
}

void inc_sec(){
	ss++;

	if(ss == 60) {
        ss = 0;
		if(!conf_mode){
			inc_min();
 	    }
	}
}

void dec_time(char segment) {
     switch(segment) {
		case 's':
			if(ss == 0) {
	    		ss = 59;
			} else {
	    		ss--;
	    	}
 	    	break;
     	case 'm':
			if(mm == 0) {
	     		mm = 59;
 	    	} else {
	    		mm--;
	    	}
 	    	break;
     	case 'h':
			if(gg == 0) {
	     		gg = 23;
 	    	} else {
	    		gg--;
	    	}
 	    	break;
     	default:
 	    	break;
     }
}

void clock_configuration(unsigned char action) {
	switch(selected_seg) {
		case 0:
			if(action == 1) {
		    	inc_sec();
	     	} else if (!action) {
		     	dec_time('s');
            }
	     	break;
		case 1:
			if(action == 1) {
                inc_min();
			} else if (!action) {
				dec_time('m');
            }
	     	break;
		case 2:
			if(action == 1) {
		     	inc_hours();
			} else if (!action) {
				dec_time('h');
            }
	     	break;
   	    default:
			break;
	}
}

void process_keyboard() {
	if(keyb_state == 1) {
		conf_mode = 0;
		selected_seg = 0;
		counter = 1200;
		hide = 0;
 	} else if(keyb_state == 2) {
		conf_mode = 1;
 	}

 	if(conf_mode == 1) {
		if(keyb_state == 4) {
			if(selected_seg < 1){
				selected_seg = 2;
			} else {
				selected_seg--;
			}
		} else if(keyb_state == 32) {
			selected_seg++;
			if(selected_seg > 2){
				selected_seg = 0;
			}
		} else if(keyb_state == 8) {
            clock_configuration(1);
            set_time();
		} else if(keyb_state == 16) {
			clock_configuration(0);
			set_time();
		}
	}
}


//bool __bit
void main() {
    __bit is_pressed = 0;
	unsigned char key_pressed;

    ss = 22;
    mm = 55;
    gg = 23;
    set_time();

    TMOD = 0b01110000;		//blokujemy TIMER1, TIMER0 w trybie 0
    ET0 = 1;			//zgoda na przerwania od TIMER0
    EA = 1;       			//globalna zgoda na przerwanie
    TH0 = 232;			//zeby pierwsze przerwanie nastapilo po 1/1200 sek (poczatkowa liczba: mniej krokow wiecej przerwan na sek)
	TR0 = 1; 			//zgoda na zliczanie przez TIMER0

    counter = 1200;
    led_idx = 0;
    led_bit = 0b00000001;

    for(;;) {
    	if(!F0)
        	continue;
    	F0 = 0;

    	SEG_OFF = 1;
        *CSDS = led_bit;
        if(!conf_mode || led_idx / 2 == selected_seg) {
        	*CSDB = digits[time[led_idx]];
		} else {
	    	if(hide) {
	     		*CSDB = 0;
     	    } else {
	     		*CSDB = digits[time[led_idx]];
 	    	}
		}
        SEG_OFF = 0;

 		key_pressed = P3_5;

		if(!key_pressed && is_pressed && keyb_state == led_bit) {
			is_pressed = 0;
			keyb_state = 0;
		} else if(key_pressed && !is_pressed && keyb_state != led_bit ) {
			keyb_state = led_bit;
			is_pressed = 1;
			process_keyboard();
		}

        led_idx++;
        led_bit *= 2;
        counter--;
        if(!counter) {
        	P1_7 = !P1_7;

            if(conf_mode) {
	    		hide = !hide;
    	    } else {
              	inc_sec();
                set_time();
    	    }
            
            counter = 1200;
        }



        if(led_idx == 6) {
            led_idx = 0;
            led_bit = 0b00000001;
        }
    }
}