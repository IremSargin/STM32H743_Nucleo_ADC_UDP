#include "main.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include <string.h>
#include <stdio.h>

#define UDP_SERVER_PORT 7
#define UDP_CLIENT_PORT 7


static int menu_sent = 0;

uint32_t data= 0;
void udp_echoserver_receive_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
void send_menu(struct udp_pcb *pcb, const ip_addr_t *addr, u16_t port);
void controls(int selection);
void send_adcvalue(uint32_t adc_value);

static struct udp_pcb *upcb;

void udp_echoserver_init(void)
{
    err_t err;

    upcb = udp_new();

    if (upcb)
    {
        err = udp_bind(upcb, IP_ADDR_ANY, UDP_SERVER_PORT);

        if(err == ERR_OK)
        {
            udp_recv(upcb, udp_echoserver_receive_callback, NULL);
        }
        else
        {
            udp_remove(upcb);
            upcb = NULL;
        }
    }
}

void send_menu(struct udp_pcb *pcb, const ip_addr_t *addr, u16_t port)
{
    const char menu[] =
        "*********************************\n"
        "What do you want to do?\n"
        "1 - Red LED\n"
        "2 - Blue LED\n"
        "3 - Both LEDs\n"
	    "4 - Start ADC measurement\n"
		"5 - Stop ADC measurement\n"
    	"6 - Turn of all LEDs\n"
        "*********************************\n";

    struct pbuf *menu_buf = pbuf_alloc(PBUF_TRANSPORT, strlen(menu), PBUF_RAM);
    if (menu_buf != NULL) {
        memcpy(menu_buf->payload, menu, strlen(menu));
        udp_sendto(pcb, menu_buf, addr, port);
        pbuf_free(menu_buf);
    }
}

void controls(int selection)
{
    switch (selection) {
        case 1:
            HAL_GPIO_WritePin(led1_GPIO_Port, led1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(led2_GPIO_Port, led2_Pin, GPIO_PIN_RESET);
            break;
        case 2:
            HAL_GPIO_WritePin(led1_GPIO_Port, led1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(led2_GPIO_Port, led2_Pin, GPIO_PIN_SET);
            break;
        case 3:
            HAL_GPIO_WritePin(led1_GPIO_Port, led1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(led2_GPIO_Port, led2_Pin, GPIO_PIN_SET);
            break;
        case 4:
           HAL_TIM_Base_Start_IT(&htim3);
            break;
        case 5:
           HAL_TIM_Base_Stop_IT(&htim3);
           break;
        case 6:
        	 HAL_GPIO_WritePin(led1_GPIO_Port, led1_Pin, GPIO_PIN_RESET);
        	 HAL_GPIO_WritePin(led2_GPIO_Port, led2_Pin, GPIO_PIN_RESET);
            break;

    }
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6) {
        HAL_IncTick();
    }

    if (htim->Instance == TIM3) {
        HAL_GPIO_TogglePin(led3_GPIO_Port, led3_Pin);
        HAL_ADC_Start(&hadc1);
        data = HAL_ADC_GetValue(&hadc1);
        HAL_Delay(0.1);
        HAL_ADC_Stop(&hadc1);
        send_adcvalue(data);
    }
}

void send_adcvalue(uint32_t adc_value)
{
    if (upcb == NULL) return;

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "ADC Value: %lu\n", adc_value);

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, strlen(buffer), PBUF_RAM);

    if (p != NULL) {
        memcpy(p->payload, buffer, strlen(buffer));


        udp_sendto(upcb, p, &upcb->remote_ip, upcb->remote_port);

        pbuf_free(p);
    }
}

void udp_echoserver_receive_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{

    ip_addr_copy(upcb->remote_ip, *addr);
    upcb->remote_port = port;

    if (!menu_sent) {
        send_menu(pcb, addr, port);
        menu_sent = 1;
    }
    else if (p != NULL && p->len > 0) {
        char response = ((char*)p->payload)[0];
        int selection = response - '0';
        controls(selection);
    }

    pbuf_free(p);
}
