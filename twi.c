#include "./twi.h"

byte twi_error;

/* 
 * custom TWI library functions derived from datasheet
 * Mostly enabling the correct bits in a register to enable the functions
 */

 /* initialise twi registers to i2c on */
void twi_init()
{
	bool pullup_en = false;
	
	DDRC  |= (1 << TW_SDA_PIN) | (1 << TW_SCL_PIN);
	if (pullup_en)
	{
		PORTC |= (1 << TW_SDA_PIN) | (1 << TW_SCL_PIN);
	}
	else
	{
		PORTC &= ~((1 << TW_SDA_PIN) | (1 << TW_SCL_PIN));
	}
	DDRC  &= ~((1 << TW_SDA_PIN) | (1 << TW_SCL_PIN));
	
	
	//TWSR = 0x01; 
	//TWBR = 0x01;
	/* Set bit rate register 72 and prescaler to 1 resulting in
	SCL_freq = 16MHz/(16 + 2*72*1) = 100KHz	*/
	//TWBR = 72;
	    TWSR0 = 0x00; // ??? = 1
    TWBR0 = 0x02; // ? 1MHz ????????? 50kHz-62kHz ??? (?????)
	TWCR0 |= (1<<TWEN);
}

/* Enable correct registers for twi transmission process to begin */
void twi_start()
{
    volatile uint8_t status = 0;
    TWCR0 = _BV(TWINT)|_BV(TWSTA)|_BV(TWEN);
    while (!(TWCR0 & _BV(TWINT)));
    status = TWSR0;
}

void twi_repeat_start()
{
    TWCR0 = _BV(TWINT)|_BV(TWSTA)|_BV(TWEN);
    while (!(TWCR0 & _BV(TWINT)));
}

/* Enable correct registers for twi transmission to stop */
void twi_stop()
{
	TWCR0 = _BV(TWINT)|_BV(TWSTO)|_BV(TWEN); 
}

/* Release control of i2c lines */
void twi_releaseBus()
{
    TWCR0 = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT);
}

/* slave device write: activate a slave device */
byte twi_MT_SLA_W(byte addr)
{
	uint8_t TWDR_to_write = (addr << 1) | TW_WRITE;
    TWDR0 = (addr << 1) | TW_WRITE;
    TWCR0 = _BV(TWINT)|_BV(TWEN);                       
    while (!(TWCR0 & _BV(TWINT)));
	
	uint8_t test = TWSR0;
    return TWSR0 & ~0x1;
}

/* slave device read: activate a slave device */
byte twi_MR_SLA_R(byte addr)
{
    TWDR0 = (addr << 1) | TW_READ;
    TWCR0 = _BV(TWINT)|_BV(TWEN);
    while (!(TWCR0 & _BV(TWINT)));
	return TWSR0;
}

/*  write to slave device as a master device */
byte twi_MT_write(byte data)
{
    TWDR0 = data;
    TWCR0 = _BV(TWINT)|_BV(TWEN);
    while (!(TWCR0 & _BV(TWINT)));
	return TWSR0;
}

/* read slave device expecting an acknoledgement */
byte twi_MR_read_ACK()
{
    TWCR0 = _BV(TWINT)|_BV(TWEN)|_BV(TWEA);
    while (!(TWCR0 & _BV(TWINT)));
    return TWDR0;
}

/* read slave device expecting no acknoledgement */
byte twi_MR_read_NACK()
{
    TWCR0 = _BV(TWINT)|_BV(TWEN);
    while (!(TWCR0 & _BV(TWINT)));
    return TWDR0;
}