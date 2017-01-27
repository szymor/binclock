#include <windows.h>
#include <stdio.h>
#include <malloc.h>

typedef struct
{
	int x, y;
} SPoint;

HDC bg;
HDC red;
HDC green;
HDC screen;
HDC dib;
unsigned char* dibptr;
unsigned char* redptr;
unsigned char* greenptr;
char display[4][4];
char toggle_sec;
SPoint dispoint[4][4];
SPoint secmarker;

LRESULT CALLBACK MainWindowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
int reset();
int get_time();
int set_column( int column, unsigned char digit );
COLORREF add_pixels( COLORREF one, COLORREF two );
int glow( int x, int y, HDC mask );
int glow_fast( int x, int y, unsigned char* ptr );

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd )
{
	MSG msg;
	WNDCLASSEX wcx;
	HWND hMainWindow;
	DWORD style, exstyle;
	RECT rect;
	HBITMAP bg_bitmap;
	HBITMAP red_bitmap;
	HBITMAP green_bitmap;
	HBITMAP screen_bitmap;
	HBITMAP dib_bitmap;
	HBITMAP old;
	BITMAPINFO bmi;
	HRGN hRegion;
	POINT pnt;
	int i, j;
	
	memset( display, 0, 16 );
	
	dispoint[0][0].x = 52 - 76;
	dispoint[0][0].y = 40 - 76;
	dispoint[1][0].x = 110 - 76;
	dispoint[1][0].y = 40 - 76;
	dispoint[2][0].x = 212 - 76;
	dispoint[2][0].y = 40 - 76;
	dispoint[3][0].x = 269 - 76;
	dispoint[3][0].y = 40 - 76;
	dispoint[0][1].x = 52 - 76;
	dispoint[0][1].y = 92 - 76;
	dispoint[1][1].x = 110 - 76;
	dispoint[1][1].y = 92 - 76;
	dispoint[2][1].x = 212 - 76;
	dispoint[2][1].y = 92 - 76;
	dispoint[3][1].x = 269 - 76;
	dispoint[3][1].y = 92 - 76;
	dispoint[0][2].x = 52 - 76;
	dispoint[0][2].y = 142 - 76;
	dispoint[1][2].x = 110 - 76;
	dispoint[1][2].y = 142 - 76;
	dispoint[2][2].x = 212 - 76;
	dispoint[2][2].y = 142 - 76;
	dispoint[3][2].x = 269 - 76;
	dispoint[3][2].y = 142 - 76;
	dispoint[0][3].x = 52 - 76;
	dispoint[0][3].y = 194 - 76;
	dispoint[1][3].x = 110 - 76;
	dispoint[1][3].y = 194 - 76;
	dispoint[2][3].x = 212 - 76;
	dispoint[2][3].y = 194 - 76;
	dispoint[3][3].x = 269 - 76;
	dispoint[3][3].y = 194 - 76;
	secmarker.x = 162 - 76;
	secmarker.y = 117 - 76;
	
	bg_bitmap = (HBITMAP) LoadImage( 0, "front.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE );
	bg = CreateCompatibleDC( NULL );
	old = (HBITMAP) SelectObject( bg, bg_bitmap );
	DeleteObject( old );
	
	red_bitmap = (HBITMAP) LoadImage( 0, "red.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE );
	red = CreateCompatibleDC( NULL );
	old = (HBITMAP) SelectObject( red, red_bitmap );
	DeleteObject( old );
	redptr = (unsigned char*) malloc( 152*152*3 );
	for( i = 0; i < 152; ++i )
		for( j = 0; j < 152; ++j )
		{
			COLORREF pix;
			pix = GetPixel( red, i, j );
			redptr[3*(152*j+i)] = GetBValue( pix );
			redptr[3*(152*j+i)+1] = GetGValue( pix );
			redptr[3*(152*j+i)+2] = GetRValue( pix );
		}

	green_bitmap = (HBITMAP) LoadImage( 0, "green.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE );
	green = CreateCompatibleDC( NULL );
	old = (HBITMAP) SelectObject( green, green_bitmap );
	DeleteObject( old );
	greenptr = (unsigned char*) malloc( 152*152*3 );
	for( i = 0; i < 152; ++i )
		for( j = 0; j < 152; ++j )
		{
			COLORREF pix;
			pix = GetPixel( green, i, j );
			greenptr[3*(152*j+i)] = GetBValue( pix );
			greenptr[3*(152*j+i)+1] = GetGValue( pix );
			greenptr[3*(152*j+i)+2] = GetRValue( pix );
		}
	
	screen_bitmap = (HBITMAP) LoadImage( 0, "front.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE );
	screen = CreateCompatibleDC( NULL );
	old = (HBITMAP) SelectObject( screen, screen_bitmap );
	DeleteObject( old );
	
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
	bmi.bmiHeader.biWidth = 320; 
	bmi.bmiHeader.biHeight = -240; 
	bmi.bmiHeader.biPlanes = 1; 
	bmi.bmiHeader.biBitCount = 24; 
	bmi.bmiHeader.biCompression = BI_RGB; 
	bmi.bmiHeader.biSizeImage = 0; 
	bmi.bmiHeader.biXPelsPerMeter = 300; 
	bmi.bmiHeader.biYPelsPerMeter = 300; 
	bmi.bmiHeader.biClrUsed = 0; 
	bmi.bmiHeader.biClrImportant = 0;
	dib = CreateCompatibleDC( NULL );
	dib_bitmap = CreateDIBSection( dib, &bmi, DIB_RGB_COLORS, (void**) &dibptr, NULL, 0 );
	old = (HBITMAP) SelectObject( dib, dib_bitmap );
	DeleteObject( old );
	reset();
	
	memset( &wcx, 0, sizeof(wcx) );
	wcx.cbSize = sizeof(wcx);
	wcx.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
	wcx.lpfnWndProc = MainWindowProc;
	wcx.hInstance = hInstance;
	wcx.hIcon = LoadIcon( NULL, IDI_APPLICATION );
	wcx.hCursor = LoadCursor( NULL, IDC_ARROW );
	wcx.hbrBackground = GetStockObject( WHITE_BRUSH );
	wcx.lpszClassName = "BinClockMainWindowClass";
	RegisterClassEx( &wcx );
	
	style = WS_OVERLAPPED | WS_VISIBLE | WS_CAPTION;
	exstyle = WS_EX_OVERLAPPEDWINDOW;
	rect.left = 320;
	rect.top = 240;
	rect.right = 320 + 320;
	rect.bottom = 240 + 240;
	AdjustWindowRectEx( &rect, style, FALSE, exstyle );
	
	hMainWindow = CreateWindowEx( exstyle,
								"BinClockMainWindowClass",
								"Binary Clock",
								style,
								CW_USEDEFAULT,
								CW_USEDEFAULT,
								rect.right - rect.left,
								rect.bottom - rect.top,
								NULL,
								NULL,
								hInstance,
								NULL );

	GetWindowRect( hMainWindow, &rect );
	pnt.x = 0;
	pnt.y = 0;
	ClientToScreen( hMainWindow, &pnt );
	rect.top = pnt.y - rect.top;
	rect.left = pnt.x - rect.left;
	rect.bottom = rect.top + 240;
	rect.right = rect.left + 320;
	hRegion = CreateRectRgn( rect.left, rect.top, rect.right, rect.bottom );
	SetWindowRgn( hMainWindow, hRegion, TRUE );
	
	SetTimer( hMainWindow, 10, 1000, NULL );
	
	while( GetMessage( &msg, NULL, 0, 0 ) )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}
	
	KillTimer( hMainWindow, 10 );
	DeleteDC( bg );
	DeleteDC( red );
	DeleteDC( green );
	DeleteDC( screen );
	DeleteDC( dib );
	free( redptr );
	
	return msg.wParam;
}

LRESULT CALLBACK MainWindowProc( HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam )
{
	HDC hdc;
	PAINTSTRUCT ps;
	LRESULT lresult;
	int i, j;

	switch( umsg )
	{
		case WM_NCHITTEST:
		{
			lresult = DefWindowProc( hwnd, umsg, wparam, lparam );
			if( lresult == HTCLIENT )
				return HTCAPTION;
			else return lresult;
		}
		case WM_TIMER:
		{
			reset();
			get_time();
			for( i = 0; i < 4; ++i )
				for( j = 0; j < 4; ++j )
				{
					if( display[i][j] )
						glow_fast( dispoint[i][j].x, dispoint[i][j].y, redptr );
				}
			if( toggle_sec )
				glow_fast( secmarker.x, secmarker.y, greenptr );
			GdiFlush();
			InvalidateRect( hwnd, NULL, FALSE );
			UpdateWindow( hwnd );
			return 0;
		}
		case WM_PAINT:
		{
			hdc = BeginPaint( hwnd, &ps );
			BitBlt( hdc, 0, 0, 320, 240, dib, 0, 0, SRCCOPY );
			EndPaint( hwnd, &ps );
			return 0;
		}
		case WM_DESTROY:
		{
			PostQuitMessage( 0 );
			return 0;
		}
	}
	return DefWindowProc( hwnd, umsg, wparam, lparam );
}

int reset()
{
	BitBlt( dib, 0, 0, 320, 240, bg, 0, 0, SRCCOPY );
	return 0;
}

COLORREF add_pixels( COLORREF one, COLORREF two )
{
	int r, g, b;
	r = GetRValue( one ) + GetRValue( two );
	if( r > 255 )
		r = 255;
	g = GetGValue( one ) + GetGValue( two );
	if( g > 255 )
		g = 255;
	b = GetBValue( one ) + GetBValue( two );
	if( b > 255 )
		b = 255;
	return RGB( r, g, b );
}

int glow( int x, int y, HDC mask )
{
	int i, j;
	COLORREF dib_pix, mask_pix, new_pix;
	
	for( i = 0; i < 152; ++i )
		for( j = 0; j < 152; ++j )
		{
			if( ( x + i ) >= 320 )
				continue;
			if( ( y + j ) >= 240 )
				continue;
			if( ( x + i ) < 0 )
				continue;
			if( ( y + j ) < 0 )
				continue;
			mask_pix = GetPixel( mask, i, j );
			dib_pix = GetPixel( dib, x + i, y + j );
			new_pix = add_pixels( mask_pix, dib_pix );
			SetPixel( dib, x + i, y + j, new_pix );
		}
	return 0;
}

int glow_fast( int x, int y, unsigned char* ptr )
{
	int i, j, current_line, help, help2, help3;
	for( j = 0; j < 152; ++j )
		for( i = 0; i < 152; ++i )
		{
			if( ( x + i ) >= 320 )
				continue;
			if( ( y + j ) >= 240 )
				continue;
			if( ( x + i ) < 0 )
				continue;
			if( ( y + j ) < 0 )
				continue;
			help = ( ( j + y ) * 320 + i + x ) * 3;
			help2 = ( j * 152 + i ) * 3;
			
			help3 = dibptr[ help ];
			help3 += ptr[ help2 ];
			if( help3 > 255 )
				help3 = 255;
			dibptr[ help ] = help3;
				
			help3 = dibptr[ help + 1 ];
			help3 += ptr[ help2 + 1];
			if( help3 > 255 )
				help3 = 255;
			dibptr[ help + 1 ] = help3;

			help3 = dibptr[ help + 2 ];
			help3 += ptr[ help2 + 2 ];
			if( help3 > 255 )
				help3 = 255;
			dibptr[ help + 2 ] = help3;
		}
	return 0;
}

int get_time()
{
	SYSTEMTIME systime;
	
	GetLocalTime( &systime );
	set_column( 0, systime.wHour / 10 );
	set_column( 1, systime.wHour % 10 );
	set_column( 2, systime.wMinute / 10 );
	set_column( 3, systime.wMinute % 10 );
	toggle_sec = !toggle_sec;
}

int set_column( int column, unsigned char digit )
{
	display[column][0] = digit & 0x08;
	display[column][1] = digit & 0x04;
	display[column][2] = digit & 0x02;
	display[column][3] = digit & 0x01;
}
