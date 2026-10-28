#include "stm32g4xx.h"

#define PASO     819    // para cada led 4095/5
#define OSCURO   1500   // arriba de eso esta tapado 


static volatile uint16_t muestra[2];   //  potenenciometroenciometro 0, fotoresistor 1
static uint8_t  indice;

void leds_config(void)
{
    RCC->AHB2ENR |= (1u << 1);
    
    GPIOB->MODER &= ~(3u << 0);    // PB0 salida
    GPIOB->MODER |=  (1u << 0);

    GPIOB->MODER &= ~(3u << 8);    // PB4 salida
    GPIOB->MODER |=  (1u << 8);

    GPIOB->MODER &= ~(3u << 10);   // PB5 salida
    GPIOB->MODER |=  (1u << 10);

    GPIOB->MODER &= ~(3u << 12);   // PB6 salida
    GPIOB->MODER |=  (1u << 12);

    GPIOB->ODR &= ~(1u << 0);      // PB0 apagado
    GPIOB->ODR &= ~(1u << 4);      // PB4 apagado
    GPIOB->ODR &= ~(1u << 5);      // PB5 apagado
    GPIOB->ODR &= ~(1u << 6);      // PB6 apagado
}


void pwm_config(void)
{
    RCC->AHB2ENR |= (1u << 0);
    RCC->APB2ENR |= (1u << 11);

    GPIOA->MODER  &= ~(3u << 16);
    GPIOA->MODER  |=  (2u << 16); // alterna 

    GPIOA->AFR[1] &= ~(0xFu << 0);// limpiar PA8 
    GPIOA->AFR[1] |=  (6u << 0); // periferico TIM1_CH1

    TIM1->PSC   = 31;   // 500kHz              
    TIM1->ARR   = 4095; // periodo 
    TIM1->CCR1  = 0; // brillo en 0 

    TIM1->CCMR1 = (6u << 4) | (1u << 3);  // modo PWM 1 con preload
    TIM1->CCER |= (1u << 0); // conecta el canal 1 al pin
    TIM1->BDTR |= (1u << 15);// MOE, deja salir la señal
    TIM1->CR1  |= (1u << 7); // preload del ARR
    TIM1->EGR   = (1u << 0);// carga los valores de una vez
    TIM1->CR1  |= (1u << 0);// arranca el contador
}

void adc_config(void)
{
    volatile int espera;

    RCC->AHB2ENR |= (1u << 0) | (1u << 13);

    GPIOA->MODER |= (3u << 0) | (3u << 2); // PA0 y PA1 como analogico 

    ADC2->CR &= ~(1u << 29);  // despierta  el ADC
    ADC2->CR |=  (1u << 28);  // enciende su fuente interna
    for (espera = 0; espera < 2000; espera++); // tiempo de estabilizarse

    ADC12_COMMON->CCR = (1u << 16);  // le da reloj al convertidor

    ADC2->CFGR  = (1u << 13) | (1u << 12); // modo continuo, nunca para
    ADC2->SQR1  = (1u << 0) | (1u << 6) | (2u << 12);// Lee e; PA0 y el PA1
    ADC2->SMPR1 = (7u << 3) | (7u << 6);  // se toma su tiempo en cada lectura
    ADC2->IER   = (1u << 2) | (1u << 3); // avisar cuando termine lectura 

    ADC2->CR |= (1u << 0); // prende el ADC
    while ((ADC2->ISR & (1u << 0)) == 0); // esperar a que se prenda 
    ADC2->ISR = (1u << 0); // borra el aviso de prendido 

    NVIC_EnableIRQ(ADC1_2_IRQn); // activar interrupciones del cpu 
    ADC2->CR |= (1u << 2); // arranca y no para 
}

void leds_write(uint16_t potenenciometro)
{
    GPIOB->ODR &= ~(1u << 0);
    GPIOB->ODR &= ~(1u << 4);
    GPIOB->ODR &= ~(1u << 5);
    GPIOB->ODR &= ~(1u << 6);

    if (potenenciometro >= PASO)       GPIOB->ODR |= (1u << 0);
    if (potenenciometro >= 2 * PASO)   GPIOB->ODR |= (1u << 4);
    if (potenenciometro >= 3 * PASO)   GPIOB->ODR |= (1u << 5);
    if (potenenciometro >= 4 * PASO)   GPIOB->ODR |= (1u << 6);
}


void ADC1_2_IRQHandler(void)
{
    if (ADC2->ISR & (1u << 2)) { // termino 1 lectura 
        muestra[indice] = (uint16_t)ADC2->DR; 
        indice ^= 1u; // cambia el indice 
    }

    if (ADC2->ISR & (1u << 3)) { // termino las 2 lecturas 
        ADC2->ISR = (1u << 3);
        indice = 0; // reinicia el indice
    }
}

int main(void)
{

    leds_config();
    pwm_config();
    adc_config();

    while (1) {
        
        uint16_t potenenciometro = muestra[0];
        uint16_t luz = muestra[1];

        leds_write(potenenciometro);

        if (luz > OSCURO)
            TIM1->CCR1 = potenenciometro; // no luz = prender
        else
            TIM1->CCR1 = 0; // Luz = apagar
    }
}