// CDKey.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "kdw\dialog.h"
#include "kdw\entry.h"
#include "kdw\HBox.h"
#include "kdw\Label.h"
#include "kdw\kdWidgetsLib.h"
#include "XMath\XMathLib.h"
#include "Serialization\SerializationLib.h"
#include "windows.h"
#include "CDKey.h"
#include <deque>

#pragma comment(lib, "verifier.lib")
__declspec(dllimport) bool VerifyCDKey(const char* String); 
bool verifyCDKeyInternal(const char * String);

static const char* keyPath = "SOFTWARE\\StrategyFirst\\Perimeter 2";

CDKeyChecker::CDKeyChecker()
{
	for(int i = 0; i < 13; i++)
		permutations_[i] = i;
	RandomGenerator gen(1283);
	random_shuffle(permutations_, permutations_ + 13, gen);
	for(int i = 0; i < 13; i++)
		permutationsInv_[permutations_[i]] = i;
}

bool CDKeyChecker::check()
{
	if(readRegistry())
		return true;

	kdw::Dialog dialog((HWND)0);
	dialog.setTitle("Product Registration");
	dialog.add(new kdw::Label("Please enter a valid CD-key:"));
	kdw::Entry* entry = new kdw::Entry;
	dialog.add(entry);
	dialog.showModal();

	writeRegistry(entry->text());

	if(readRegistry())
		return true;

	kdw::Dialog dialog1((HWND)0);
	dialog1.setTitle("Product Registration");
	kdw::Label* label1 = new kdw::Label("Incorrect CD-key");
	label1->setAlignment(kdw::ALIGN_CENTER, kdw::ALIGN_MIDDLE);
	dialog1.add(label1);
	dialog1.addButton("Retry", 1);
	dialog1.addButton("Exit", 0);
	int ret = dialog1.showModal();
	if(ret)
		return check();

	return false;
}

bool CDKeyChecker::readRegistry()
{
	HKEY hKey;
	LONG lRet = RegOpenKeyEx(HKEY_CURRENT_USER, keyPath, 0, KEY_QUERY_VALUE, &hKey);
	if(lRet == ERROR_SUCCESS){
		const int BUFSIZE = 20;
		char keyString[BUFSIZE];
		DWORD keyStringSize = BUFSIZE;
		lRet = RegQueryValueEx(hKey, "Key", NULL, NULL, (LPBYTE)keyString, &keyStringSize);
		RegCloseKey(hKey);
		if(lRet == ERROR_SUCCESS && keyStringSize < BUFSIZE && VerifyCDKey(decode(keyString)) && verifyCDKeyInternal(decode(keyString)))
			return true;
	}
	return false;
}

bool CDKeyChecker::writeRegistry(const char* key)
{
	HKEY hKey; 
	DWORD dwDisp; 

	if(RegCreateKeyEx(HKEY_CURRENT_USER, keyPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisp)) 
		return false;

	if(RegSetValueEx(hKey, "Key", 0, REG_SZ, (LPBYTE)encode(key), (DWORD)strlen(key) + 1)) 
		return false;

	RegCloseKey(hKey); 
	return true;
}

const char* CDKeyChecker::encode(const char* key)
{
	static char buffer[14];
	buffer[13] = 0;
	for(int i = 0; i < 13; i++)
		buffer[i] = key[permutations_[i]];
	return buffer;
}

const char* CDKeyChecker::decode(const char* key)
{
	static char buffer[14];
	buffer[13] = 0;
	for(int i = 0; i < 13; i++)
		buffer[i] = key[permutationsInv_[i]];
	return buffer;
}



static unsigned long HashString( const char * string );
static void AlphaNumeric( char * String, UINT Number );
static char AlphaConvert( UINT Number );
static UINT SuperRand( void );
static UINT AlphaValue( char * String );
static UINT DeAlphaConvert( char Character );

// Calculates a hash value for the string passed in.
//
// See http://www.ddj.com/articles/1996/9604/9604b/9604b.htm
// for a good article explaining hashing. Note that this
// algorithm may still collide. The proper way to handle that
// would be to keep a linked list of all the hash entries that
// collide then search through the list to get when doing the 
// look up.
//
/*--- ElfHash ---------------------------------------------------
*  The published hash algorithm used in the UNIX ELF format
*  for object files. Accepts a pointer to a string to be hashed
*  and returns an unsigned long.
*-------------------------------------------------------------*/

unsigned long HashString( const char * string )

{
	unsigned long hash;
	unsigned long g;  

	if ( string == NULL ) { return 0; }

	hash = 0;

	while ( *string != '\0' )

	{
		hash = ( hash << 4 ) + *string;

		string++;

		// NOTE: This is supposed to be an
		// assignment. It's not a bug!

		if ( (g = hash & 0xF0000000) ) { hash ^= g >> 24; }

		hash &= ~g;
	}

	return (unsigned long)hash;
}

void AlphaNumeric( char * String, UINT Number )

{
	deque < int > Deck;

	while( Number != 0 )

	{
		UINT Remainder = Number % 36;

		Deck.push_front( Remainder );

		Number /= 36;
	}

	string NumberString;

	for( int Index = 0; Index < Deck.size(); Index ++ )

		NumberString += AlphaConvert( Deck[ Index ] );


	lstrcpy( String, NumberString.c_str() );	

}

char AlphaConvert( UINT Number )
{
	switch( Number )

	{
	case(  0 ):	return( '0' );
	case(  1 ):	return( 'P' );
	case(  2 ):	return( '9' );
	case(  3 ):	return( 'O' );
	case(  4 ):	return( '8' );
	case(  5 ):	return( 'I' );
	case(  6 ):	return( '7' );
	case(  7 ):	return( 'U' );
	case(  8 ):	return( '6' );
	case(  9 ):	return( 'Y' );
	case( 10 ): return( '5' );
	case( 11 ): return( 'T' );
	case( 12 ): return( '4' );
	case( 13 ): return( 'R' );
	case( 14 ): return( '3' );
	case( 15 ): return( 'E' );
	case( 16 ): return( '2' );
	case( 17 ): return( 'W' );
	case( 18 ): return( '1' );
	case( 19 ): return( 'Q' );
	case( 20 ): return( 'L' );
	case( 21 ): return( 'K' );
	case( 22 ): return( 'J' );
	case( 23 ): return( 'H' );
	case( 24 ): return( 'G' );
	case( 25 ): return( 'F' );
	case( 26 ): return( 'D' );
	case( 27 ): return( 'S' );
	case( 28 ): return( 'M' );
	case( 29 ): return( 'N' );
	case( 30 ): return( 'B' );
	case( 31 ): return( 'V' );
	case( 32 ): return( 'C' );
	case( 33 ): return( 'X' );
	case( 34 ): return( 'A' );
	case( 35 ): return( 'Z' );

	default: return( '_' );
	}
}


UINT AlphaValue( char * String )
{
	UINT Length = lstrlen( String );

	UINT Number = 0;

	for( int Index = 0; Index < Length; Index ++ )

	{
		Number *= 36;
		Number += DeAlphaConvert( String[ Index ] );
	}

	return( Number );
}


UINT DeAlphaConvert( char Character )
{
	switch( Character )
	{
	case ( '0' ): return( 0 );
	case ( 'P' ): return( 1 );
	case ( '9' ): return( 2 );
	case ( 'O' ): return( 3 );
	case ( '8' ): return( 4);
	case ( 'I' ): return( 5);
	case ( '7' ): return( 6 );
	case ( 'U' ): return( 7 );
	case ( '6' ): return( 8 );
	case ( 'Y' ): return( 9 );
	case ( '5' ): return( 10 );
	case ( 'T' ): return( 11 );
	case ( '4' ): return( 12 );
	case ( 'R' ): return( 13 );
	case ( '3' ): return( 14 );
	case ( 'E' ): return( 15 );
	case ( '2' ): return( 16);
	case ( 'W' ): return( 17 );
	case ( '1' ): return( 18 );
	case ( 'Q' ): return( 19 );
	case ( 'L' ): return( 20 );
	case ( 'K' ): return( 21 );
	case ( 'J' ): return( 22 );
	case ( 'H' ): return( 23 );
	case ( 'G' ): return( 24 );
	case ( 'F' ): return( 25 );
	case ( 'D' ): return( 26 );
	case ( 'S' ): return( 27 );
	case ( 'M' ): return( 28 );
	case ( 'N' ): return( 29 );
	case ( 'B' ): return( 30 );
	case ( 'V' ): return( 31 );
	case ( 'C' ): return( 32 );
	case ( 'X' ): return( 33 );
	case ( 'A' ): return( 34 );
	case ( 'Z' ): return( 35 );
	default: return( 99 );
	}

}

bool verifyCDKeyInternal(const char * String)
{
	//	split string into 2

	char String1[ 256 ];
	char String2[ 256 ];

	UINT Length = lstrlen( String );

	lstrcpyn( String1, String, 10 );
	lstrcpy( String2, & String[ Length - 4 ] );

	UINT HashNumber = HashString( String1 );

	char HashString[ 256 ];

	AlphaNumeric( HashString, HashNumber );

	Length = lstrlen( HashString );

	if( lstrcmp( & HashString[ Length - 4 ], String2 ) == 0 ) return( true );

	return( false );
}