#ifndef _MY_BUTTON_H_
#define _MY_BUTTON_H_

/* Cách sử dụng

 Khai báo chân + pin ID + biến kiểu dữ liệu Button  // uint8_t buttonPinOK = 3;
                                                    // #define BUTTON1_ID  1
                                                    //  Button buttonOK;
 Init trong hàm setup                               // button_init(&buttonOK,buttonPinOK,BUTTON1_ID);
 Trong Main                                         //  handle_button(&buttonOK);
*/


typedef void (*button_pressshort_callback)(int);
button_pressshort_callback pt_short = NULL;
typedef void (*button_presslong_callback)(int);
button_presslong_callback pt_long = NULL;
typedef void (*button_pressaccel_callback)(int);
button_pressaccel_callback pt_accel = NULL;
typedef enum {
  BUTTON_READ,
  BUTTON_WAIT_DEBOUND,
  BUTTON_WAIT_RELEASE_AND_CHECK_LONG_PRESS,
  BUTTON_WAIT_RELEASE,
}Button_State;

#define TIME_DEBOUND_BUTTON	20
#define TIME_SHORT_PRESS	1000
#define TIME_LONG_PRESS	2000
#define TIME_ACCEL_MIN	100
#define TIME_ACCEL_MAX	1000
#define TIME_ACCEL_DELTA	200

#define BUTTON_ACTIVE_LOW	0
#define BUTTON_ACTIVE_HIGH	1

typedef struct {
  uint16_t gpio_pin;
  uint8_t current_status;
  uint8_t last_status;
  uint32_t time_debounce;
  uint32_t t_long_press;
  Button_State button_state;
  uint8_t button_active;
	uint8_t button_id;
	int16_t t_accel_call;
	uint32_t t_accel_press;
  uint8_t check_long_first;
}Button;

// Hàm xử lí có nhấn nhanh
void button_press_short_callback(uint8_t button_id);
// Hàm timeout nếu nhấn nút giữ lâu
void button_pressing_timeout_callback(uint8_t button_id);

// Hàm timeout nếu nhấn nút giữ lâu
void button_pressing_accel_callback(uint8_t button_id);

// hàm xử lí nút nhấn, gọi hàm này trong main
void handle_button(Button *btn) {
  btn->current_status = digitalRead(btn->gpio_pin);
  if(btn->button_active == BUTTON_ACTIVE_HIGH)
	{
		 btn->current_status = !btn->current_status;
	}
	switch(btn->button_state)
	{
		case BUTTON_READ:
		{
			if((btn->current_status == 0 && btn->last_status == 1) )
			{
						btn->time_debounce = millis();	
						btn->button_state = BUTTON_WAIT_DEBOUND;
            btn->check_long_first = 0;
			}
		}
		break;
		case BUTTON_WAIT_DEBOUND:
		{
			if(millis() - btn->time_debounce>= TIME_DEBOUND_BUTTON)
			{
				if(btn->current_status ==0 && btn->last_status ==1)//nhan xuong
				{
					// button_pressing_callback(btn->button_id);
					btn->t_long_press = millis();
					btn->last_status = 0;
					btn->t_accel_press = millis();
					btn->t_accel_call = TIME_ACCEL_MAX;
					btn->button_state = BUTTON_WAIT_RELEASE_AND_CHECK_LONG_PRESS;
				}
				else if(btn->current_status ==1 && btn->last_status ==0)//nha ra
				{
					btn->t_long_press = millis() - btn->t_long_press;
					if(btn->t_long_press <= TIME_SHORT_PRESS)
					{
						pt_short(btn->button_id);
					}
					// button_release_callback(btn->button_id);
					btn->last_status = 1;
					btn->button_state = BUTTON_READ;
				}
				else //khong dung
				{
					btn->last_status = 1;
					btn->button_state = BUTTON_READ;
				}
			}
		}
		break;
		case BUTTON_WAIT_RELEASE_AND_CHECK_LONG_PRESS:
		{
				if(btn->current_status == 1 && btn->last_status == 0)
				{
					btn->button_state = BUTTON_WAIT_DEBOUND;
					btn->time_debounce = millis();	
				}
				else if(millis() - btn->t_long_press >= TIME_LONG_PRESS)
				{
					pt_long(btn->button_id);
          btn->check_long_first = 1;
					btn->button_state = BUTTON_WAIT_RELEASE;
				}		
				else if(millis() -  btn->t_accel_press >= btn->t_accel_call)
				{
					btn->t_accel_call -=TIME_ACCEL_DELTA;
					if(btn->t_accel_call <= TIME_ACCEL_MIN)
					{
						btn->t_accel_call = TIME_ACCEL_MIN;
					}
          if(btn->check_long_first == 1)
            pt_accel(btn->button_id);
					btn->t_accel_press = millis();
				}
		}
		break;
		case BUTTON_WAIT_RELEASE:
		{
			if(btn->current_status == 1 && btn->last_status == 0)
			{
				btn->button_state = BUTTON_WAIT_DEBOUND;
				btn->time_debounce = millis();	
			}
			else if(millis() -  btn->t_accel_press >= btn->t_accel_call)
			{
				btn->t_accel_call -=TIME_ACCEL_DELTA;
				if(btn->t_accel_call <= TIME_ACCEL_MIN)
				{
					btn->t_accel_call = TIME_ACCEL_MIN;
				}
        if(btn->check_long_first == 1)
				  pt_accel(btn->button_id);
				btn->t_accel_press = millis();
			}
		}
		break;
		default:
			
			break;
	}
}


void button_init(Button *btn, uint16_t pin,uint8_t button_active,uint8_t button_id)
{
	btn->gpio_pin = pin;
	btn->button_active = button_active;
	btn->button_state = BUTTON_READ;
	btn->button_id = button_id;
	btn->current_status = 1;
	btn->last_status = 1;
  btn->check_long_first = 0;
}


void button_pressshort_set_callback(void *cb) {
    pt_short = (button_pressshort_callback)cb;
}
void button_presslong_set_callback(void *cb) {
    pt_long = (button_presslong_callback)cb;
}
void button_pressaccel_set_callback(void *cb) {
	  pt_accel = (button_pressaccel_callback)cb;
}
#endif