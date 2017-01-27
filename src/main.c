// zegar

#include <8052.h>

#define SDA	P3_4 
#define SCL	P3_3 
#define LA	P3_0 
#define CLK	P3_1 
#define SDI	P3_2 
#define klawisz1	P1_3 
#define klawisz2	P1_4 
#define sekundnik	P1_2 
#define onewire	P1_0 
#define oko	P3_7 

typedef struct
{
	unsigned char x;
	unsigned char y;
} Point;

int mode;
/*
1 - godziny i minuty
2 - minuty i sekundy - powrot po minucie

3 i 4 - disabled

5 - temperatura - powrot po minucie
6 - animacja
7 - snake - jesli nikt nie wciska przyciskow przez minute, powrot
8 - ustawianie godziny
*/
volatile char hour, minute, second, centisecond, quarter;
// quarters of milisecond
volatile char nflag, hflag, mflag, sflag, cflag, qflag;
// software event flags
// nflag - nychthemeron flag
char sm;
// second marker
unsigned char ad_lo, ad_hi, co_lo, co_hi;
char irflag;
Point snake_segments[3]; // 0 - glowa
char snake_direction;
/*
	0 - gora
	1 - dol
	2 - lewo
	3 - prawo
*/

void init( void );
void send_byte( char );
void display_ms( void );
void display_hm( void );
void timer0( void ) __interrupt(1);
char bcd( char );
char nbc( char );
void toggle_sm( void );
char onewire_reset( void );
void onewire_writebit( char );
char onewire_readbit( void );
void onewire_writebyte( char );
char onewire_readbyte( void );
char onewire_status( void );
/*
0 - device not present
1 - device present, parasite-powered
2 - device present, external supply
*/
void display_temperature( void );
void wait( unsigned int );
void display_animation( void );
void i2c_start( void );
void i2c_stop( void );
char i2c_writebyte( char );
char i2c_readbyte( char );
void restore_time( void );
void save_time( void );
void ir_monitor( void );
char ir_pulse( void );
char ir_byte( void );
void ir_service( void );
void snake_move( void );
void snake_display( void );
char code_correct( char* );
void code_add_byte( char*, char );
char code_read_digit( void );

int main()
{
	char little_timer = 0;
	char entry_code[4] = { 0, 0, 0, 0 };
	unsigned char hour_aux = 0, minute_aux = 0;
	
	init();

	mode = 1;
	
	while(1)
	{
		if( nflag )
		{
			// nychthemeron adjustment
			save_time();
			nflag = 0;
		}
		
		switch( mode )
		{
			case 1:
			{
				if( hflag )
				{
					display_animation();
					display_animation();
					display_animation();
					hflag = 0;
				}
				if( sflag )
				{
					display_hm();
					toggle_sm();
					sflag = 0;
				}
				ir_service();
			} break;
			
			case 2:
			{
				if( sflag )
				{
					display_ms();
					//toggle_sm();
					sflag = 0;

					++little_timer;
					if( little_timer == 60 )
					{
						little_timer = 0;
						mode = 1;
					}
				}
				ir_service();
			} break;
/*
			case 3:
			{
				if( hflag )
				{
					display_animation();
					hflag = 0;
				}
				if( sflag )
				{
					display_hm();
					toggle_sm();
					sflag = 0;
					++little_timer;
					if( little_timer == 30 )
					{
						little_timer = 0;
						mode = 4;
					}
				}
				ir_service();
			} break;
			
			case 4:
			{
				if( hflag )
				{
					display_animation();
					hflag = 0;
				}
				if( sflag )
				{
					display_ms();
					sekundnik = 0;
					sflag = 0;
					++little_timer;
					if( little_timer == 30 )
					{
						little_timer = 0;
						mode = 3;
					}
				}
				ir_service();
			} break;
*/
			case 5:
			{
				display_temperature();	// one second lag
				++little_timer;
				if( little_timer == 60 )
				{
					little_timer = 0;
					mode = 1;
				}
			} break;
			
			case 6:
			{
				int i;
				for( i = 0; i < 3; ++i )
					display_animation();
				mode = 1;
			} break;

			case 7:
			{
				if( sflag )
				{
					snake_move();
					snake_display();
					sflag = 0;

					++little_timer;
					if( little_timer == 60 )
					{
						little_timer = 0;
						mode = 1;
					}
				}
				ir_monitor();
				if( irflag )
				{
					switch( co_lo )
					{
						case 0x10:
						// zero
						{
							mode = 1;
							little_timer = 0;
						} break;

						case 0x06:
						// right
						{
							if( snake_direction != 2 )
								snake_direction = 3;
							little_timer = 0;
						} break;

						case 0x07:
						// left
						{
							if( snake_direction != 3 )
								snake_direction = 2;
							little_timer = 0;
						} break;

						case 0x40:
						// up
						{
							if( snake_direction != 1 )
								snake_direction = 0;
							little_timer = 0;
						} break;

						case 0x41:
						// down
						{
							if( snake_direction != 0 )
								snake_direction = 1;
							little_timer = 0;
						} break;
						
					}
					irflag = 0;
				}
			} break;

			case 8:
			{
				LA = 0;
				send_byte( 0x00 );
				send_byte( 0x00 );
				LA = 1;

				while( !code_correct( entry_code ) )
				{
					ir_monitor();
					if( irflag )
					{
						code_add_byte( entry_code, co_lo );
						little_timer = 0;
						irflag = 0;
					}
					if( sflag )
					{
						++little_timer;
						if( little_timer == 60 )
						{
							little_timer = 0;
							mode = 1;
							break;
						}
						sflag = 0;
					}
				}

				if( code_correct( entry_code ) )
				{
					entry_code[0] = 0;
					entry_code[1] = 0;
					entry_code[2] = 0;
					entry_code[3] = 0;
				
					LA = 0;
					send_byte( 0xFF );
					send_byte( 0xFF );
					LA = 1;
					wait(1000);

					LA = 0;
					send_byte( 0x00 );
					send_byte( 0x10 );
					LA = 1;
					hour_aux = code_read_digit() * 10;

					LA = 0;
					send_byte( 0x00 );
					send_byte( 0x01 );
					LA = 1;
					hour_aux += code_read_digit();

					LA = 0;
					send_byte( 0x10 );
					send_byte( 0x00 );
					LA = 1;
					minute_aux = code_read_digit() * 10;

					LA = 0;
					send_byte( 0x01 );
					send_byte( 0x00 );
					LA = 1;
					minute_aux += code_read_digit();

					quarter = 0;
					centisecond = 0;
					second = 0;
					minute = minute_aux;
					hour = hour_aux;

					save_time();
					mode = 1;
				}
			} break;
		}
	}
}

void timer0() __interrupt(1)
{
	++quarter;
	qflag = 1;
	if( quarter == 40 )
	{
		quarter = 0;
		++centisecond;
		cflag = 1;
	}
	if( centisecond == 100 )
	{
		centisecond = 0;
		++second;
		sflag = 1;
	}
	if( second == 60 )
	{
		second = 0;
		++minute;
		mflag = 1;
	}
	if( minute == 60 )
	{
		minute = 0;
		++hour;
		hflag = 1;
	}
	if( hour == 24 )
	{
		hour = 0;
		nflag = 1;
	}
	TF0 = 0;
}

void init()
{
	CLK = 0;
	LA = 1;
	SCL = 0;
	SDA = 1;
	klawisz1 = 1;
	klawisz2 = 1;
	oko = 1;
	onewire = 1;

	nflag = 0;
	hflag = 0;
	mflag = 0;
	sflag = 0;
	cflag = 0;
	qflag = 0;
	irflag = 0;

	snake_direction = 3;
	snake_segments[0].x = 2;
	snake_segments[0].y = 0;
	snake_segments[1].x = 1;
	snake_segments[1].y = 0;
	snake_segments[2].x = 0;
	snake_segments[2].y = 0;

	TMOD = 0x02;
	TH0 = 0x06;
	EA = 1;
	ET0 = 1;
	TR0 = 1;

	restore_time();
}

void send_byte( char data )
{
	char temp;
	for( temp = 0x01; temp; temp <<= 1 )
	{
		CLK = 0;
		if( data & temp )
			SDI = 1;
		else
			SDI = 0;
		CLK = 1;
	}
	CLK = 0;
}

void display_ms( void )
{
	LA = 0;
	send_byte( bcd(second) );
	send_byte( bcd(minute) );
	LA = 1;
}

void display_hm( void )
{
	LA = 0;
	send_byte( bcd(minute) );
	send_byte( bcd(hour) );
	LA = 1;
}

char bcd( char data )
{
	return ( data / 10 << 4 ) | ( data % 10 );
}

char nbc( char data )
{
	return ( (data & 0xF0) >> 4 ) * 10 + (data & 0x0F);
}

void toggle_sm( void )
{
	sm = !sm;
	sekundnik = sm;
}

char onewire_reset( void )
{
	char result;
	
	while( TL0 < 0xF0 );
	onewire = 0;
	
	// begin
	qflag = 0;
	while( !qflag );
	while( TL0 < 0xF0 );
	// and end of 250us period
	
	// and second one
	qflag = 0;
	while( !qflag );
	while( TL0 < 0xF0 );

	qflag = 0;
	while( !qflag );
	while( TL0 < 0x80 );
	onewire = 1;

	while( TL0 < 0xC0 );
	result = onewire;

	// recovery time
	qflag = 0;
	while( !qflag );
	while( TL0 < 0xF0 );

	qflag = 0;
	while( !qflag );
	while( TL0 < 0xF0 );
	
	return result;
}

void onewire_writebit( char data )
{
	qflag = 0;
	while( !qflag );
	
	if( data )
	{
		while( TL0 < 0x80 );
		onewire = 0;
		while( TL0 < 0x86 );
		onewire = 1;
	}
	else
	{
		while( TL0 < 0x80 );
		onewire = 0;
		while( TL0 < 0xBD );
		onewire = 1;
	}

	// recovery time
	qflag = 0;
	while( !qflag );
	while( TL0 < 0xF0 );

	qflag = 0;
	while( !qflag );
	while( TL0 < 0xF0 );
}

char onewire_readbit()
{
	char result;

	qflag = 0;
	while( !qflag );
	
	while( TL0 < 0x80 );
	onewire = 0;
	onewire = 1;
	while( TL0 < 0x85 );
	result = onewire;

	// recovery time
	qflag = 0;
	while( !qflag );
	while( TL0 < 0xF0 );

	qflag = 0;
	while( !qflag );
	while( TL0 < 0xF0 );
	
	return result;
}

void onewire_writebyte( char data )
{
	char temp;
	for( temp = 0x01; temp; temp <<= 1 )
		onewire_writebit( data & temp );
}

char onewire_readbyte()
{
	unsigned char i, result;
	result = 0;
	for( i = 0; i < 8; ++i )
	{
		//result |= onewire_readbit();
		//result <<= 1;
		result >>= 1;
		if( onewire_readbit() )
			result |= 0x80;
	}
	return result;
}

void display_temperature()
{
	unsigned char templo, temphi, tempdeclo, tempdechi, tempbcdlo, tempbcdhi;
	unsigned int temp;

	onewire_reset();
	onewire_writebyte( 0xCC );
	onewire_writebyte( 0x44 );
	wait(1000);	// try to improve it later
	
	onewire_reset();
	onewire_writebyte( 0xCC );
	onewire_writebyte( 0xBE );
	templo = onewire_readbyte();
	temphi = onewire_readbyte();
	onewire_reset();

	tempdechi = (temphi << 4) | (templo >> 4);
	tempdeclo = templo & 0x0F;
	temp = (tempdeclo * 100) >> 4;

	tempbcdhi = bcd( tempdechi );
	tempbcdlo = bcd( temp & 0x00FF );

	LA = 0;
	send_byte( tempbcdlo );
	send_byte( tempbcdhi );
	LA = 1;
}

void wait( unsigned int msec )
{
	msec <<= 2;
	while( msec )
	{
		qflag = 0;
		while( !qflag );
		--msec;
	}
}

char onewire_status()
{
	if( onewire_reset() )
		return 0;
	onewire_writebyte( 0xCC );
	onewire_writebyte( 0xB4 );
	return onewire_readbit() ? 2 : 1;
}

void display_animation()
{
	char frames[2*16] = { 0x00, 0x00,
			0x10, 0x00,
			0x30, 0x00,
			0x70, 0x00,
			0xF0, 0x00,
			0xF8, 0x00,
			0xF8, 0x80,
			0xF8, 0x88,
			0xF8, 0x8C,
			0xF8, 0x8E,
			0xF8, 0x8F,
			0xF8, 0x9F,
			0xF9, 0x9F,
			0xFB, 0x9F,
			0xFF, 0x9F,
			0xFF, 0xDF };
	int i;
	for( i = 0; i < 32; i += 2 )
	{
		LA = 0;
		send_byte( frames[i+1] );
		send_byte( frames[i] );
		LA = 1;
		wait(100);
	}
	for( i = 0; i < 32; i += 2 )
	{
		LA = 0;
		send_byte( ~frames[i+1] );
		send_byte( ~frames[i] );
		LA = 1;
		wait(100);
	}
}

void i2c_start( void )
{
	SDA = 1;
	wait(1);
	SCL = 1;
	wait(1);
	SDA = 0;
	wait(1);
	SCL = 0;
	wait(1);
}

void i2c_stop( void )
{
	SDA = 0;
	wait(1);
	SCL = 1;
	wait(1);
	SDA = 1;
	wait(1);
	SCL = 0;
	wait(1);
}

char i2c_writebyte( char data )
{
	unsigned char temp, ack;
	for( temp = 0x80; temp; temp >>= 1 )
	{
		SCL = 0;
		wait(1);
		if( data & temp )
			SDA = 1;
		else
			SDA = 0;
		wait(1);
		SCL = 1;
		wait(1);
	}
	SCL = 0;
	wait(1);
	SCL = 1;
	wait(1);
	ack = !SDA;
	SCL = 0;
	wait(1);
	SDA = 1;
	wait(1);
	return ack;
}

char i2c_readbyte( char ack )
{
	unsigned char temp, result;
	result = 0;
	for( temp = 0; temp < 8; ++temp )
	{
		SCL = 1;
		wait(1);
		result <<= 1;
		result |= SDA;
		wait(1);
		SCL = 0;
		wait(1);
	}
	SDA = !ack;
	wait(1);
	SCL = 1;
	wait(1);
	SCL = 0;
	wait(1);
	SDA = 1;
	wait(1);
	return result;
}

void restore_time()
{
	char second_bcd, minute_bcd, hour_bcd;

	i2c_start();
	i2c_writebyte( 0xD0 );
	i2c_writebyte( 0x00 );
	i2c_start();
	i2c_writebyte( 0xD1 );
	second_bcd = i2c_readbyte(1);
	minute_bcd = i2c_readbyte(1);
	hour_bcd = i2c_readbyte(0);
	i2c_stop();

	TR0 = 0;
	TL0 = TH0;
	hour = nbc( hour_bcd & 0x3F );
	minute = nbc( minute_bcd );
	second = nbc( second_bcd & 0x7F );
	centisecond = 0;
	quarter = 0;
	TR0 = 1;
}

void save_time()
{
	char second_bcd, minute_bcd, hour_bcd;

	qflag = 0;
	while( !qflag );
	second_bcd = second;
	minute_bcd = minute;
	hour_bcd = hour;
	/*
	second_bcd = 0;
	minute_bcd = 5;
	hour_bcd = 18;
	*/
	second_bcd = bcd( second_bcd );
	minute_bcd = bcd( minute_bcd );
	hour_bcd = bcd( hour_bcd );

	i2c_start();
	i2c_writebyte( 0xD0 );
	i2c_writebyte( 0x00 );
	i2c_writebyte( second_bcd );
	i2c_writebyte( minute_bcd );
	i2c_writebyte( hour_bcd );
	i2c_stop();
}

void ir_monitor( void )
{
	int i;
	int pulse_length = 0;

	// 2000 - 500*250us == 0.5s - okno monitoringu oka
	for( i = 0; i < 2000; ++i )
	{
		qflag = 0;
		while( !qflag );
		if( oko )
			pulse_length = 0;
		else
			++pulse_length;
		if( pulse_length > 30 )
		{
			pulse_length = 0;
			while( !oko );
			while( oko )
			{
				qflag = 0;
				while( !qflag );
				if( oko )
					++pulse_length;
			}
			if( pulse_length > 15 )
			{
				// toggle_sm();
				ad_lo = ir_byte();
				ad_hi = ir_byte();
				co_lo = ir_byte();
				co_hi = ir_byte();
				irflag = 1;
			}
		}
	}
}

char ir_pulse( void )
{
	int pulse_length = 0;
	while( !oko );
	while( oko )
	{
		qflag = 0;
		while( !qflag );
		if( oko )
		++pulse_length;
	}
	return pulse_length;
}

char ir_byte( void )
{
	unsigned char result, i;
	result = 0;
	for( i = 0; i < 8; ++i )
	{
		result >>= 1;
		if( ir_pulse() > 4 )
			result |= 0x80;
	}
	return result;
}

void ir_service( void )
{
	ir_monitor();
	if( irflag )
	{
		switch( co_lo )
		{
			case 0x10:
			{
			} break;
			case 0x11:
			{
				mode = 1;
			} break;
			case 0x12:
			{
				mode = 2;
			} break;
			case 0x13:
			{
				mode = 5;
			} break;
			case 0x14:
			{
				mode = 6;
			} break;
			case 0x15:
			{
				mode = 7;
			} break;
			case 0x16:
			{
				mode = 8;
			} break;
			case 0x17:
			{
			} break;
			case 0x18:
			{
			} break;
			case 0x19:
			{
			} break;
		}
		irflag = 0;
	}
}

void snake_move( void )
{
	snake_segments[2].x = snake_segments[1].x;
	snake_segments[1].x = snake_segments[0].x;
	snake_segments[2].y = snake_segments[1].y;
	snake_segments[1].y = snake_segments[0].y;
	
	switch( snake_direction )
	{
		case 0:
		{
			--snake_segments[0].y;
			if( snake_segments[0].y == 255 )
				snake_segments[0].y = 3;
		} break;
		case 1:
		{
			++snake_segments[0].y;
			if( snake_segments[0].y == 4 )
				snake_segments[0].y = 0;
		} break;
		case 2:
		{
			--snake_segments[0].x;
			if( snake_segments[0].x == 255 )
				snake_segments[0].x = 3;
		} break;
		case 3:
		{
			++snake_segments[0].x;
			if( snake_segments[0].x == 4 )
				snake_segments[0].x = 0;
		} break;
	}
}

void snake_display( void )
{
	char byte_hi = 0, byte_lo = 0, i, temp;

	for( i = 0; i < 3; ++i )
	{
		if( snake_segments[i].x % 2 )
			temp = 3;
		else
			temp = 7;
		temp -= snake_segments[i].y;
		if( snake_segments[i].x < 2 )
			byte_hi |= 1 << temp;
		else
			byte_lo |= 1 << temp;
	}
	LA = 0;
	send_byte( byte_lo );
	send_byte( byte_hi );
	LA = 1;
}

char code_correct( char* code )
{
	if( (code[0] == 0x12) &&
		(code[1] == 0x10) &&
		(code[2] == 0x11) &&
		(code[3] == 0x13) )
		return 1;
	else
		return 0;
	
}

void code_add_byte( char* code, char byte )
{
	code[0] = code[1];
	code[1] = code[2];
	code[2] = code[3];
	code[3] = byte;
}

char code_read_digit( void )
{
	char digit = 0x0F;

	while( digit == 0x0F )
	{
		ir_monitor();
		if( irflag )
		{
			switch( co_lo )
			{
				case 0x10:
				{
					digit = 0;
				} break;
				case 0x11:
				{
					digit = 1;
				} break;
				case 0x12:
				{
					digit = 2;
				} break;
				case 0x13:
				{
					digit = 3;
				} break;
				case 0x14:
				{
					digit = 4;
				} break;
				case 0x15:
				{
					digit = 5;
				} break;
				case 0x16:
				{
					digit = 6;
				} break;
				case 0x17:
				{
					digit = 7;
				} break;
				case 0x18:
				{
					digit = 8;
				} break;
				case 0x19:
				{
					digit = 9;
				} break;
			}
			irflag = 0;
		}
	}
	return digit;
}
