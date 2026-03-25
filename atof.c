
 /* fast_atof by Peter Miller 23/1/2020
    See below for license information.
    
    It contains the following functions:
    	double fast_atof(const char *s,bool * not_number);
		double fast_atof_nan(const char *s);// like fast_atof, but returns NAN if whole string is not a valid number
		double fast_strtod(const char *s,char ** endptr);
		float fast_strtof(const char *s,char **endptr); // if endptr != NULL returns 1st character that's not in the number
		long double fast_strtold(const char *s,char **endptr); // if endptr != NULL returns 1st character that's not in the number
		__float128 fast_strtof128(const char *s,char **endptr); 
    
    Assumes ASCII character encoding (or a superset of it like utf-8) i.e.
    	if d is '0'..'9' then assume 'd'-'0' gives 0..9
    	letter | 0x20 = lowercase letter ('A'=0x41, 'a'=0x61, etc).
		letters 'a' to 'z' are adjacent values (e.g. 'b'-'a'=1)
		and '.' and '0' to '9' are all < 'A'
    
	 The code should be robust to anything thrown at it (lots of leading of trailing zeros, extremely large numbers of digits, etc ).
	 As well as floating point numbers this also accepts NAN and INF (case does not matter).
	 21-10-2025 NAN's are now signed on output (previously the sign of a NAN was ignored).
	 21-10-2025 returns INFINITY on overflow (previously might return NAN in this case).
	 28-10-2025 supports float128 even if int128 is not available (uses u2_64.[ch] instead).
	 12-11-2025 fast_strtold() added, some optimisations to fast_strtof128, and code tidied (e.g. int128 version of f128 removed as u2_64 version was the same speed and some compilers don't have int128 but do have float128).
	 29-11-2025 the only times NAN/INFINITY can be returned are when input is "nan" or "inf", otherwise we clip at 0/"DBL_MAX". 
	 			This avoids issues with rounded up numbers very near "DBL_MAX" being returned as "inf".
	 			The argument being that if the input is given as a number the user expects it to be interpreted as a number (even if its clipped), and also that this matches converting very small numbers as zero.
	 18-1-2026 option (USE_RYU_FOR_POWERS10) to use Ryu to convert 10's mantissa/exponent to ieee double - which is faster (see "Timings" below)
	 21-1-2026- githib 1v0 release
	 28-1-2026  - strtof() optimised so it can use a single uint32_t (previously it could swap to a uint64_t which added complexity to the code) - this is faster and has be exhaustively tested using Ryo float->string (f2s.c)
	 			- this also optimised nan/inf processing (taken out of critical path as well for the commonest case of processing numbers).
	 			- same nan/inf processing used for doubles, LD and f128's
				- my_isdigit() used in place of isdigit() and my_tolower() used in place of tolower() - these are both faster [ see assumptions above]
	 			- compared Ryu s2f.c with fast_strtof() - Ryu was significantly slower (47.2ns/test vs 34.7ns/test for fast_strtof(), so no point in adding a Ryu option for fast_strtof().
	 			- USE_RYU_FORL_POWERS10 renamed USE_RYU_FOR_DBL_POWERS10 to make it clear Ryu is only used for doubles
				- strtof() rounds when given excess digits for %a format inputs
				- Note test program in ya_sprintf corrected for %a tests on floats (previously created %a string from double rather than from float).
				- optimisations from strtof() also done to strtod()
				- USE_LD option to use long doubles in strtod() removed (it was accidentally disabled in the 1v0 release) as it can give results that are 1 bit in error (e.g. 1E126 where (double)1E126L !=1E126 ).
				- there seemed to be no reason to keep double-double solution for strtod() other versions were faster so that was also deleted.
	 25/3/2026  - strtod() conversion updated so its now faster than Ryu (uses dconvert.c/h) 
				- NAN(n-char-sequenceopt) - from C99 (and better defined in C17) is now supported (but "n-char-sequenceopt" is just skipped in the input stream)
	 25/3/2026 - ngithub 1v1 release
	 
	 As the name suggests its also written to be fast (much faster than the built in strtod or atof library functions at least for mingw64/UCRT ).
	 Its round trip exact for doubles/long doubles or float128's  when used with ya_sprintf and the built in sprintf functions 
	 ie if you start with a value, convert it to a string (with enough digits in it) and then use one of these functions you get exactly (bit wise) the same number as you started with.  

 Timings (Winlibs gcc 15.2.0, w64, average from test program on i3 10100 ):
     MSVCRT strtod() 	 - 2786ns/test
   	 UCRT strtod()   	 - 527ns/test (5,3* faster)
	 fast_strtod() 1v0	 - 187ns/test (14.9* faster MSVCRT, *2.8 faster than UCRT)
	 fast_strtod+ryu 1v0 - 112ns/test (24.9* faster MSVCRT, *4.7 vs UCRT, *1.7 vs fast_strtod() 1v0)	
	 fast_strtod (1v1) 	 - 64ns/test  (43.5* faster MSVCRT, *8.2 vs UCRT, *2.9 vs fast_strtod() 1v0 , *1.8 vs fast_strtod+ryu 1v0)
	 
	 fast_strtof()	 - 34.7ns/test (using Ryu f2s.c for float->string conversion - using sprintf() for float->string conversion is significantly slower as the float gets promoted to double and is processed by sprintf as a double - using the UCRT sprintf this gives 306.2ns/test)

 Note this code leverages (all available at https://github.com/p-j-miller )
	ryu - from ya_sprintf
	double-double functions from double-double
	128 bit functionality using two uint64_t's from u2_64 
	power of 10 tables from power10
	my_printf for debugging from my_printf
	
 A test program for these functions is included in ya_sprintf - to compile this (from directory containing ya_sprintf):
 Under Windows using winlibs gcc 15.2.0 -
  C:\winlibs\winlibs-x86_64-posix-seh-gcc-15.2.0-mingw-w64ucrt-13.0.0-r2\mingw64\bin\gcc -Wall -m64 -fexcess-precision=standard -Ofast  -std=gnu99 -I. -I../ya-sprintf/ main.c ../atof-and-ftoa/atof.c ../double-double/double-double.c ../u2_64-128bits-with-two-u64/u2_64.c ../ya-sprintf/ya-dconvert.c ../my_printf/my_printf.c ../nan_type/nan_type.c ../ya-sprintf/ryu/d2fixed_ya_sprintf.c ../ya-sprintf/ryu/s2d_fast_atof.c ../hr_timer/hr_timer.c ../fma/fmaq.c -lquadmath -static -o test.exe
 Under Linux -
  gcc -m64 -Wall -Ofast -fexcess-precision=standard -I. -I../ya-sprintf/ -D_FORTIFY_SOURCE=1 main.c ../atof-and-ftoa/atof.c ../double-double/double-double.c ../u2_64-128bits-with-two-u64/u2_64.c ../ya-sprintf/ya-dconvert.c ../my_printf/my_printf.c ../nan_type/nan_type.c ../ya-sprintf/ryu/d2fixed_ya_sprintf.c ../ya-sprintf/ryu/s2d_fast_atof.c ../hr_timer/hr_timer.c ../fma/fmaq.c -lquadmath -lm -o test
 */   


/*----------------------------------------------------------------------------
 * MIT License:
 *
 * Copyright (c) 2020,2025,2026 Peter Miller
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *--------------------------------------------------------------------------*/
//#define DEBUG
//#define RYU_I_POWER10 /* if defined use Ryu derived code for I*power10 [ for doubles only] - this is slower now */
#if defined(DEBUG)
#define __USE_MINGW_ANSI_STDIO 1   /* if this is defined then writing to stdout > NUL is VERY slow (70 secs vs 4 secs) ! Note strtof() etc are still much slower than fast_atof() etc */
#include <stdio.h>
#endif
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h> /* for bool */
#include <stdint.h>  /* for int64_t etc */
#include <math.h>    /* for NAN, INFINITY */
#include "../double-double/double-double.h"
#include <inttypes.h> /* defines PRI64 etc */
#include <limits.h>
#include <float.h> /* for limits for float, double , long double */
#include "atof.h"
#include "../power10/table_bin_10.h" /* for double powers of 10 tables */
#ifdef RYU_I_POWER10
 #include "../ya-sprintf/ryu/s2d_fast_atof.h"
#else 
 #include "../ya-sprintf/ya-dconvert.h" /* new I*power10 from ya_sprintf */
#endif


#define AFormatSupport /* if defined then support numbers as generated by printf %a ie 0xh.hhhhp+/-d */
#if defined(__SIZEOF_FLOAT128__)   && !defined(__BORLANDC__)      /* Builder C++ Community version 12.1 patch 1 defines __SIZEOF_FLOAT128__ but __float128's cannot be used in sensible programs due to compiler bugs */
 #define ATOF128 /* if defined add support for reading __float128 's */
#endif

#define nos_elements_in(x) (sizeof(x)/(sizeof(x[0]))) /* number of elements in x , max index is 1 less than this as we index 0... */

static inline bool my_isdigit(char x) /* hopefully faster than isdigit() - but as importantly only returns true for 0..9 whereas isdigit could in theory change with the locale set */ 
	{return x>='0' && x<='9';
	}
static inline char my_tolower(char x) /* this only works on letters (but it will not map a non-letter onto a letter) and assumes ASCII coding [or a superset of it like utf8] */
	{return x|(uint8_t)0x20;
	}	

/* code below cannot be compiled with -Ofast as this makes the compiler break some C rules that we need (even use NAN etc) , so make sure of this here */
/* code below cannot be compiled with -Ofast as this makes the compiler break some C rules that we need, so make sure of this here */
/* we also need -msse2 and -mfpmath=sse to actually use the sse instructions for float and double maths */
/* there seems to be no way to duplicate "-fexcess-precision=standard" using a pragma - so that must be present on the command line - to pass all f128_to_a tests -fexcess-precision=standard MUST be present on the command line with -Ofast */
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)) || defined(__clang__)
 #pragma GCC push_options
 #pragma GCC optimize ("-O3") /* cannot use Ofast, normally -O3 is OK. Note macro expansion does not work here ! */
 // based on  https://jdebp.uk/FGA/predefined-macros-processor.html "__i386__" is set by GCC,Clang,Intel which is good enough as the outer #if limits us to gcc and clang
 #ifdef __i386__
   #pragma GCC target("sse2,fpmath=sse") /* -msse2 and -mfpmath=sse */
 #endif 
#endif

#ifdef __BORLANDC__
 #define ldexpl(x,i) ldexp((x),(i))    /* long double == double - this must be after #include <math.h> -> fixes bug in Builder C++ where using ldexpl() gives a linker "not found" error */
#endif

/* ieee floating point maths limits:
   double (64 bits) max 				1.797 e + 308
   					min 				2.225 e-308
   					min denormalised	4.94 e-324
   					sig digits			15-17
   	float (32 bit)	max					3.4e38
   					min					1.17e-38
   					min denormalised	1.4e-45
   					sig digits			6-9

Also note that 0x0fff ffff ffff ffff =  1,152,921,504,606,846,975  	, so 18 digits easily fit into a 64 bit unsigned (with 4 bits spare) - which is enough for a double mantissa.
2^64= 18,446,744,073,709,551,616 
0xffff ffff = 	4,294,967,295 so 9 digits fits with a 32 bit unsigned (with 2 bits spare) which is NOT enough for a float mantissa			

*/   					
double fast_atof(const char *s,bool * not_number);
double fast_atof_nan(const char *s);// like fast_atof, but returns NAN if whole string is not a valid number
double fast_strtod(const char *s,char ** endptr);
float fast_strtof(const char *s,char **endptr); // if endptr != NULL returns 1st character thats not in the number
   					
static const int maxExponent = 308;	/* Largest possible base 10 for a double exponent. (must match array below) */
static const int maxfExponent = 38;	/* Largest possible base 10 for a float exponent. (must match array below) */


static float const fltpowersOf10[] = /* always float - used for float input where the value is exact  */

                {
                    1e0f,   1e1f,   1e2f,   1e3f,   1e4f,   1e5f,   1e6f,   1e7f,   1e8f,    1e9f,
                    1e10f,  1e11f,  1e12f,  1e13f,  1e14f,  1e15f,  1e16f,  1e17f,  1e18f,  1e19f,
                    1e20f,  1e21f,  1e22f,  1e23f,  1e24f,  1e25f,  1e26f,  1e27f,  1e28f,  1e29f,
                    1e30f,  1e31f,  1e32f,  1e33f,  1e34f,  1e35f,  1e36f,  1e37f,  1e38f
                };
                
static const uint64_t u64powersOf10[]=
				{
					UINT64_C(1), 	// 10^ 0
					UINT64_C(10), 	// 10^1
					UINT64_C(100), 	// 10^2
					UINT64_C(1000),	// 10^3
					UINT64_C(10000),// 10^4
					UINT64_C(100000),// 5
					UINT64_C(1000000),// 6
					UINT64_C(10000000),// 7
					UINT64_C(100000000),// 8
					UINT64_C(1000000000),// 9   
					UINT64_C(10000000000),// 10 
					UINT64_C(100000000000),// 11  
					UINT64_C(1000000000000),// 12   
					UINT64_C(10000000000000),// 13
					UINT64_C(100000000000000),// 14
					UINT64_C(1000000000000000), // 15
					UINT64_C(10000000000000000), // 16 
					UINT64_C(100000000000000000), // 17 
					UINT64_C(1000000000000000000), // 18 
					UINT64_C(10000000000000000000)  // 19  2^64=1.8446744073709551616e+19 so 10^19 should fit into a uint64
				};

static const uint32_t u32powersOf10[]=
				{
					UINT32_C(1), 	// 10^ 0
					UINT32_C(10), 	// 10^1
					UINT32_C(100), 	// 10^2
					UINT32_C(1000),	// 10^3
					UINT32_C(10000),// 10^4
					UINT32_C(100000),// 5
					UINT32_C(1000000),// 6
					UINT32_C(10000000),// 7
					UINT32_C(100000000),// 8
					UINT32_C(1000000000),// 9   [ largest possible 10^10 gives compiler error (overflow) ], 2^32=4,294,967,296
				};
/*
 *----------------------------------------------------------------------
 *
 * double fast_strtod(const char *s,char **endptr)
 *
 *	This procedure converts a floating-point number from an ASCII
 *	decimal representation to internal double-precision format.
 *  Also accepts "NaN", "Inf" and "Infinity" (any mix of case) which return NAN and INFINITY
 *  21-10-2025 -NAN is returned as -NAN.
 * Results:
 *	The return value is the floating-point equivalent of string.
 *	*endptr is set to the first character after the valid number 
 *
 * If endptr == NULL it is ignored.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
#define max_d_digits 19 // see above this is the largest possible in a uint64 

 
 double fast_strtod(const char *s,char **endptr) // if endptr != NULL returns 1st character thats not in the number
 {
  bool sign=false,expsign=false,got_number=false;
  uint64_t r=0; // mantissa
  int32_t exp=0,rexp=0;
  int32_t nos_mant_digits=0;
  const char *se=s; // string end - candidate for endptr
#ifdef DEBUG
  fprintf(stderr,"strtod(%s):\n",s);
#endif   
  if(s==NULL) return NAN; 
  while(isspace(*s)) ++s; // skip initial whitespace	
  // deal with leading sign
  if(*s=='+') ++s;
  else if(*s=='-')
  	{sign=true;
  	 ++s;
    }
  // this is the critical path - if the character is >'9' then it starts with a letter [ nan, inf ], note '.' is < '9'
  if(*s>'9') 
  	{// not a digit - work out what we have [off the critical path]
	  // NAN is a special case - NAN is  signed in the input and keeps this on the output
	  if(my_tolower(*s)=='n' && my_tolower(s[1])=='a' && my_tolower(s[2])=='n')
	  	{/* For more information on NAN's see my nan_type repository.
		  C99 allows nan to be followed by (...) , C17 7.22.1.3 says:
	  	The expected form of the subject sequence is an optional plus or minus sign, then one of the
		following:
		....
		— NAN or NAN(n-char-sequenceopt), ignoring case in the NAN part, where:
		n-char-sequence:
			digit
			nondigit
			n-char-sequence digit
			n-char-sequence nondigit
		The subject sequence is defined as the longest initial subsequence of the input string, starting with
		the first non-white-space character, that is of the expected form. The subject sequence contains no
		characters if the input string is not of the expected form.
	 	
	 	A character sequence NAN or NAN(n-char-sequenceopt) is interpreted as a quiet NaN, if
		supported in the return type, else like a subject sequence part that does not have the expected form;
		the meaning of the n-char sequence is implementation-defined.298) 
	 
	 	298)An implementation may use the n-char sequence to determine extra information to be represented in the NaN’s
		significand.
		
		That that "nondigit" is defined in 6.4.2.1 as
			nondigit: one of
				_ a b c d e f g h i j k l m
				n o p q r s t u v w x y z
				A B C D E F G H I J K L M
				N O P Q R S T U V W X Y Z
			digit: one of
				0 1 2 3 4 5 6 7 8 9	
		 On Windows (UCRT) for floats 0XFFC00000 (-NAN) gives "-nan(ind)" [ or -NAN(IND) ] , nanf("") => nan=>0X7FC00000
	  	*/
	  	 s+=3;// skip NAN [ which is valid by its self ]
	  	 const char *sn=s;// we now look forward to see if we have NAN(n-char-sequenceopt)
	  	 if(*sn=='(')
	  	 	{// ( optional n-char sequence then )  
	  	 	 sn++; // skip (
	  	 	 if(my_tolower(*sn)=='i' && my_tolower(sn[1])=='n' && my_tolower(sn[2])=='d' && sn[3]==')' )
	  	 	 	{s+=5; // "(ind)" => special case (+5 as we have already skipped the "(" )
	  	 	 	 if(endptr!=NULL) *endptr=(char *)s;
	  	 	 	 return -NAN;// this is always -ve
	  	 	 	}
	  	 	 // not nan(ind) - look for any other pattern allowed by C99
	  	 	 while(*sn && *sn!=')' ) ++sn;// for now follow C99 and just look for ), C17 would need a more complex check here
	  	 	 if(*sn==')') s=sn+1;// found (...) so we have a valid match, +1 to skip final ) [ otherwise just the "NAN" is valid so we don't change s ]
	  	 	}
		 if(endptr!=NULL) *endptr=(char *)s;
	  	 if(sign) return -nan("1");// -NAN would print back as  -nan(ind)
	  	 return NAN;
	  	}    
	  // INF or Infinity is a special case - and is signed
	  if(my_tolower(*s)=='i' && my_tolower(s[1])=='n' && my_tolower(s[2])=='f')
	  	{s+=3;// INF
	  	 if(my_tolower(*s)=='i' && my_tolower(s[1])=='n' && my_tolower(s[2])=='i' && my_tolower(s[3])=='t' && my_tolower(s[4])=='y' )
	  	  	s+=5; // "Infinity" is 5 more chars (inity) than "inf"
	  	 if(endptr!=NULL) *endptr=(char *)s;
	  	 if(sign) return -INFINITY;
	  	 return INFINITY;
	  	} 
	} // if(*s>'9') 
// at this point we know *s is a digit or a decimal point [ or rather *s is <='9' which includes 0 to 9 and decimal point ] 
#ifdef AFormatSupport 
	/* support hex floating point numbers of the format 0xh.hhhhp+/-d as generated by printf %a */
  if(*s=='0' && my_tolower(s[1])=='x' )
  	{ // got hex number
  	 double h;
  	 s+=2; // skip 0x
  	 // no need to skip leading zero's as we can just check is ms 4 bits of r are not zero
	 while(isxdigit(*s))
		{got_number=true; // have a valid number
	  	 if((r & UINT64_C(0xf000000000000000))==0)
	  		{ if(*s<='9')
			  	r=r*16+(*s-'0'); // 0..9
			  else
			  	r=r*16+(my_tolower(*s)-'a'+10); // a-f or A-F
			}
		else if(exp<2048)
		      exp+=4; // cannot actually capture digits beyond 16 but keep track of decimal point, trap  ensures we don't overflow exp when given a number with a silly number of digits (that would overflow a double)
		 ++s;
		}
  	 // now look for optional decimal point (and fractional bit of mantissa)
  	 if(*s=='.')
  		{ // got decimal point, skip and then look for fractional bit
  	 	 ++s;
		 while(isxdigit(*s))
			{got_number=true; // have a valid number
	  	 	 if((r & UINT64_C(0xf000000000000000))==0)
	  			{ if(*s<='9')
			  		r=r*16+(*s-'0'); // 0..9
			  	 else
			  		r=r*16+(my_tolower(*s)-'a'+10); // a-f or A-F
			  	 exp-=4;	
				}
			 // if we have too many digits after dp just ignore them
		 	 ++s;
		 	}
		}			  
  	 // got all of mantissa - see if its a valid number, if not we are done
  	 if(!got_number)
 		{if(endptr!=NULL) *endptr=(char *)se;
#ifdef DEBUG
 		 fprintf(stderr," strtod returns 0 (invalid hex number)\n"); 
#endif  	
 	 	 return 0;
 		}	
  	 se=s; // update to reflect end of a valid mantissa
  	 // now see if we have an  exponent
  	 if(my_tolower(*s)=='p')
  		{// have exponent, optional sign is 1st
  	 	 ++s ; // skip 'p'
  	 	 if(*s=='+') ++s;
  	 	 else if(*s=='-') 
  	 		{expsign=true;
  	 	 	 ++s;
  	 		}
  	 	while(my_isdigit(*s))
	   		{if(rexp<=2048)
		   		rexp=rexp*10+(*s - '0');  // if statement clips at a value that will result in +/-inf but will not overflow int
		 	 ++s;  
		 	 se=s; // update to reflect end of a valid exponent (p[+-]digit+)
			}
		}
 	 if(endptr!=NULL) *endptr=(char *)se; // we now know the end of the number - so save it now (means we can have multiple returns going forward without having to worry about this)	
 	 if(expsign) rexp=-rexp;	
 	 rexp+=exp; // add in correct to exponent from mantissa processing				
	 h=ldexp((double)r,rexp); // combine mantissa and exponent 
	 if(sign) h=-h;
#ifdef DEBUG
 	 fprintf(stderr," strtod (0x) returns %.18g [0x%.16A] (rexp=%d, exp=%d)\n",h,h,rexp,exp); 
#endif  	 
	 return h; // all done 	
	}
#endif	
  // skip leading zeros
  while(*s=='0')
  	{got_number=true; // have a number (0)
	 ++s;
	}
 if(my_isdigit(*s))
 	{// deal with 1st significant digit of mantissa. special case, moves got_number=true; out of the while loop below and saves a multiply in setting r
 	 got_number=true; // have a valid number
	 r=(*s-'0');
	 nos_mant_digits=1;
	 ++s;
	}
  // now read rest of the mantissa	
  while(my_isdigit(*s))
  	{ 
	  if(nos_mant_digits<max_d_digits)
	  	{ r=r*10+(*s-'0');
		  nos_mant_digits++;	
		}
	  else
	  	{ 
#if 1	  	
		  if(nos_mant_digits==max_d_digits)
	  	  		{if(*s >='5') r++; // use 1st "ignored digit" to round to nearest
	  	  		 nos_mant_digits++;
	  	  		}
#endif	  	  		
		  if(exp<2*maxExponent)
		      exp++; // cannot actually capture digits as more than 18 but keep track of decimal point, trap at 2*maxExponent ensures we don't overflow exp when given a number with a silly number of digits (that would overflow a double)
		}
	++s;
	}
  // now look for optional decimal point (and fractional bit of mantissa)
  if(*s=='.')
  	{ // got decimal point, skip and then look for fractional bit
  	 ++s;
  	 if(r==0)
  	 	{// number is zero at present, so deal with leading zeros in fractional bit of mantissa
  	 	 if(my_isdigit(*s)) got_number=true; // if r!=0 then we must already have had a digit so got_number has already been set to true
  	 	 while(*s=='0')
  	 	 	{
  	 	 	 ++s;
  	 	 	 if(exp > -2*maxExponent)
  	 	 	 	exp--; // test avoids issues with silly number of leading zeros
  	 	    }
  	 	}
  	 // now process the rest of the fractional bit of the mantissa, got_number is already true at this point in the code
	 while(my_isdigit(*s))
	 	{			
	 	 if(nos_mant_digits<max_d_digits)
	  			{ r=r*10+(*s-'0');
		  		  nos_mant_digits++;	
		  		  exp--;
				}
		 else
	  			{  // cannot actually capture digits as more than 19, so just ignore them (except for next digit which we use for rounding)
#if 1	  			
	  			  if(nos_mant_digits==max_d_digits ) 
	  			  		{if(*s >='5') r++; // use 1st "ignored digit" to round to nearest
	  			  		 nos_mant_digits++;
	  			  		}
#endif	  			  		
				}
		 ++s;
		}
 	}
  // got all of mantissa - see if its a valid number, if not we are done
  if(!got_number)
 	{if(endptr!=NULL) *endptr=(char *)se;
#ifdef DEBUG
 	fprintf(stderr," strtod returns 0 (invalid number)\n"); 
#endif  	
 	 return 0;
 	}	
  se=s; // update to reflect end of a valid mantissa
  // now see if we have an  exponent
  if(my_tolower(*s)=='e')
  	{// have exponent, optional sign is 1st
  	 ++s ; // skip 'e'
  	 if(*s=='+') ++s;
  	 else if(*s=='-') 
  	 	{expsign=true;
  	 	 ++s;
  	 	}
  	 while(my_isdigit(*s))
	   	{if(rexp<=2*maxExponent)
		   rexp=rexp*10+(*s - '0');  // if statement clips at a value that will result in +/-inf but will not overflow int
		 ++s;  
		 se=s; // update to reflect end of a valid exponent (e[+-]digit+)
		}
	}
 if(endptr!=NULL) *endptr=(char *)se; // we now know the end of the number - so save it now (means we can have multiple returns going forward without having to worry about this)	
 if(expsign) rexp=-rexp;	
 rexp+=exp; // add in correct to exponent from mantissa processing
 	
/* some optimisations for (common) special cases - there is no loss of accuracy with these optimisations */
 if(rexp>0 && rexp+nos_mant_digits<=max_d_digits)
 	{// optimisation: can do all calculations using uint64 which is fastish and exact
 	 r*=u64powersOf10[rexp];
 	 // just convert to double and return
 	 if(sign) return -(double)(r); 
	 else return (double)r;
 	}

 if(rexp<=22 && rexp >= -22 && r<=UINT64_C(9007199254740991) )
 	{// in this region we can use double  (another speed optimisation, but one than thats common and therefore worthwhile)
 	 // Double mantissa is 53 bits so the largest value it can exactly represent is 9,007,199,254,740,991
 	 // as powers of 2 are exact, 5^m is the limit for exact powers of 10, 5^22   = 2,384,185,791,015,625 which is well below 2^53-1
	 const double d_r=r;
 	 if(rexp<0)
 	 	{
	 	 if(sign) return -d_r/PosPowerOf10_hi[-rexp]; // negative exponent means we divide by powers of 10
	 	 else return d_r/PosPowerOf10_hi[-rexp];
	 	}
 	 if(rexp>0)
 	 	{
	 	 if(sign) return -d_r*PosPowerOf10_hi[rexp]; // positive exponent means we multiply by powers of 10
	 	 else return d_r*PosPowerOf10_hi[rexp];
	 	}
	 else // rexp==0
 	 	{
	 	 if(sign) return -d_r; 
	 	 else return d_r;
	 	}	 	 	
	}
#ifndef RYU_I_POWER10
 return ya_conv_mant_exp_to_double(sign,r,rexp);// new solution March 2026 multiply r*10^rexp and then add sign
#else
 // now we need to "normalise" the mantissa, if its an exact power of 10 (we compensate by adjusting the power of 10 exponent which is an exact process).
 // not doing this can give a 1 bit error as r may not convert exactly into a double and then we have the additional power10 error.
 // "optimisations" above deal with the case where r==0, so we can ignore that possibility here
 // This is necessary even when using "Ryu" for the powers of 10 conversion.
  if((r>UINT64_C(9007199254740991)  ) && (r&1)==0 && (uint64_t)(r * 0xcccccccccccccccdull) <= 0x3333333333333333ull) //  test for even and division by 5 (as 10=2*5) , see https://math.stackexchange.com/questions/1251327/is-there-a-fast-divisibility-check-for-a-fixed-divisor
 	{// we know r divides by 10 so we can use a do/while loop, 2^64/9007199254740991 = 2048 so at most we will divide by 10,000 here (i.e 4 times around the do/while loop)
	 do
	 	{r/=10;
	 	 rexp++;
	 	 nos_mant_digits--;
	 	} while((r>UINT64_C(9007199254740991) ) && (r&1)==0  && (uint64_t)(r * 0xcccccccccccccccdull) <= 0x3333333333333333ull);
	}	
 return ryu_conv_mant_exp_to_double(sign,r,rexp);// solution derived from Ryu,  multiply r*10^rexp and then add sign
#endif
}

 
/*
 *----------------------------------------------------------------------
 *
 * double fast_atof(const char *s,bool * not_number) :
 *
 *	This procedure converts a floating-point number from an ASCII
 *	decimal representation to internal double-precision format.
 *  Also accepts "NaN", "Inf" and "Infinity" (any mix of case) which return NAN and INFINITY
 * Results:
 *	The return value is the floating-point equivalent of string.
 *	If a terminating character is found before any floating-point
 *	digits, then zero is returned and *not_number is set to true (otherwise its set to false);
 *
 * If not_number == NULL it is ignored.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */ 
double fast_atof(const char *s,bool * not_number)
{char *se;
 double r;
 r=fast_strtod(s,&se);
 if(not_number != NULL) *not_number = (se==(char *)s); // if strtod found no number not_number is set to true
 return r;
}

double fast_atof_nan(const char *s)// like fast_atof, but returns NAN if the string does not start with a valid number (fast_atof returns 0)
{char *se;
 double r;
 r=fast_strtod(s,&se);
 if(se==s) return NAN; // if string does not contain a number return NAN
 return r;
}


/*
 *----------------------------------------------------------------------
 *
 * float fast_strtof(const char *s,char **endptr)
 *
 *	This procedure converts a floating-point number from an ASCII
 *	decimal representation to internal single-precision format.
 *  Also accepts "NaN", "Inf" and "Infinity" (any mix of case) which return NAN and INFINITY
 * 21-10-2025 now returnes -NAN when required.
 * Results:
 *	The return value is the floating-point equivalent of string.
 *	*endptr is set to the first character after the valid number 
 *
 * If endptr == NULL it is ignored.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* new version that just uses a single uint32_t for mantissa. At most 9 sig figs are needed for round loop exact with a float, and 32 bits allows max value of 4,294,967,295 (2^32 -1) */
#define maxfdigits 9


float fast_strtof(const char *s,char **endptr) // if endptr != NULL returns 1st character thats not in the number
 {
  double dr; // may need to use double precision to get an accurate float - we try hard below to avoid this either by uisng just a uint32, or just by using float's
  bool sign=false,expsign=false,got_number=false,round_digit=false;
  uint32_t r=0; // mantissa, uint32 can hold 9 digits which is enough for a float
  int exp=0,rexp=0;
  int nos_mant_digits=0;
  const char *se=s; // string end - candidate for endptr
#ifdef DEBUG
  fprintf(stderr,"strtof(%s):\n",s);
#endif   
  if(s==NULL) return NAN;
  while(isspace(*s)) ++s; // skip initial whitespace isspace() is specified by the C standard for this step	
  // deal with leading sign
  if(*s=='+') ++s;
  else if(*s=='-')
  	{sign=true;
  	 ++s;
    }

  // this is the critical path - if the character is >'9' then it starts with a letter [ nan, inf ], note '.' is < '9'
  if(*s>'9') 
  	{// not a digit - work out what we have [off the critical path]
	  // NAN is a special case - NAN is  signed in the input and keeps this on the output
	  if(my_tolower(*s)=='n' && my_tolower(s[1])=='a' && my_tolower(s[2])=='n')
	  	{/* For more information on NAN's see my nan_type repository.
		  C99 allows nan to be followed by (...) , C17 7.22.1.3 says:
	  	The expected form of the subject sequence is an optional plus or minus sign, then one of the
		following:
		....
		— NAN or NAN(n-char-sequenceopt), ignoring case in the NAN part, where:
		n-char-sequence:
			digit
			nondigit
			n-char-sequence digit
			n-char-sequence nondigit
		The subject sequence is defined as the longest initial subsequence of the input string, starting with
		the first non-white-space character, that is of the expected form. The subject sequence contains no
		characters if the input string is not of the expected form.
	 	
	 	A character sequence NAN or NAN(n-char-sequenceopt) is interpreted as a quiet NaN, if
		supported in the return type, else like a subject sequence part that does not have the expected form;
		the meaning of the n-char sequence is implementation-defined.298) 
	 
	 	298)An implementation may use the n-char sequence to determine extra information to be represented in the NaN’s
		significand.
		
		That that "nondigit" is defined in 6.4.2.1 as
			nondigit: one of
				_ a b c d e f g h i j k l m
				n o p q r s t u v w x y z
				A B C D E F G H I J K L M
				N O P Q R S T U V W X Y Z
			digit: one of
				0 1 2 3 4 5 6 7 8 9	
		 On Windows (UCRT) for floats 0XFFC00000 (-NAN) gives "-nan(ind)" [ or -NAN(IND) ] , nanf("") => nan=>0X7FC00000
	  	*/
	  	 s+=3;// skip NAN [ which is valid by its self ]
	  	 const char *sn=s;// we now look forward to see if we have NAN(n-char-sequenceopt)
	  	 if(*sn=='(')
	  	 	{// ( optional n-char sequence then )  
	  	 	 sn++; // skip (
	  	 	 if(my_tolower(*sn)=='i' && my_tolower(sn[1])=='n' && my_tolower(sn[2])=='d' && sn[3]==')' )
	  	 	 	{s+=5; // "(ind)" => special case (+5 as we have already skipped the "(" )
	  	 	 	 if(endptr!=NULL) *endptr=(char *)s;
	  	 	 	 return -NAN;// this is always -ve
	  	 	 	}
	  	 	 // not nan(ind) - look for any other pattern allowed by C99
	  	 	 while(*sn && *sn!=')' ) ++sn;// for now follow C99 and just look for ), C17 would need a more complex check here
	  	 	 if(*sn==')') s=sn+1;// found (...) so we have a valid match, +1 to skip final ) [ otherwise just the "NAN" is valid so we don't change s ]
	  	 	}
		 if(endptr!=NULL) *endptr=(char *)s;
	  	 if(sign) return -nanf("1");// -NAN would print back as  -nan(ind)
	  	 return NAN;
	  	}    
	  // INF or Infinity is a special case - and is signed
	  if(my_tolower(*s)=='i' && my_tolower(s[1])=='n' && my_tolower(s[2])=='f')
	  	{s+=3;// INF
	  	 if(my_tolower(*s)=='i' && my_tolower(s[1])=='n' && my_tolower(s[2])=='i' && my_tolower(s[3])=='t' && my_tolower(s[4])=='y' )
	  	  	s+=5; // "Infinity" is 5 more chars (inity) than "inf"
	  	 if(endptr!=NULL) *endptr=(char *)s;
	  	 if(sign) return -INFINITY;
	  	 return INFINITY;
	  	} 
	} // if(*s>'9') 
// at this point we know *s is a digit or a decimal point [ or rather *s is <='9' which includes 0 to 9 and decimal point ]
#ifdef AFormatSupport 
	/* support hex floating point numbers of the format 0xh.hhhhp+/-d as generated by printf %a - warning should use %.8a at most for a float */
  if(*s=='0' && my_tolower(s[1])=='x' )
  	{ // got hex number
  	 float h;
  	 s+=2; // skip 0x
  	 // no need to skip leading zero's as we can just check is ms 4 bits of r are not zero, we only expect 1 digit before decimal point, code here is more general
	 while(isxdigit(*s))
		{got_number=true; // have a valid number
	  	 if((r & UINT32_C(0xf0000000))==0)
	  		{ if(*s<='9')
			  	r=r*16+(*s-'0'); // 0..9
			  else
			  	r=r*16+(my_tolower(*s)-'a'+10); // a-f or A-F
			}
		else if(exp<2048)
		      exp+=4; // cannot actually capture digits beyond 8 but keep track of decimal point, trap  ensures we don't overflow exp when given a number with a silly number of digits (that would overflow a float)
		 ++s;
		}
  	 // now look for optional decimal point (and fractional bit of mantissa)
  	 if(*s=='.')
  		{ // got decimal point, skip and then look for fractional bit
  	 	 ++s;
		 while(isxdigit(*s))
			{got_number=true; // have a valid number
	  	 	 if((r & UINT32_C(0xf0000000))==0)
	  			{ if(*s<='9')
			  		r=r*16+(*s-'0'); // 0..9
			  	 else
			  		r=r*16+(my_tolower(*s)-'a'+10); // a-f or A-F
			  	 exp-=4;	
				}
			 else
				{// if we have too many digits after dp, round on next digit then just ignore them [ note extra digits are likely as printf() does not know about floats ] 
				 if(!round_digit)
					{if(*s=='8' || *s=='9' || isalpha(*s)) r++; // do rounding>=8 rounds up
					 if(r==0) {r=UINT32_C(0x10000000) ; exp+=4; } // r==0 means we overflowed, fix this
					 round_digit=true;// we can only round once
					}
				}
		 	 ++s;
		 	}
		}			  
  	 // got all of mantissa - see if its a valid number, if not we are done
  	 if(!got_number)
 		{if(endptr!=NULL) *endptr=(char *)se;
#ifdef DEBUG
 		 fprintf(stderr," strtof returns 0 (invalid hex number)\n"); 
#endif  	
 	 	 return 0;
 		}	
  	 se=s; // update to reflect end of a valid mantissa
  	 // now see if we have an  exponent
  	 if(my_tolower(*s)=='p')
  		{// have exponent, optional sign is 1st
  	 	 ++s ; // skip 'p'
  	 	 if(*s=='+') ++s;
  	 	 else if(*s=='-') 
  	 		{expsign=true;
  	 	 	 ++s;
  	 		}
  	 	while(my_isdigit(*s))
	   		{if(rexp<=2048)
		   		rexp=rexp*10+(*s - '0');  // if statement clips at a value that will result in +/-inf but will not overflow int
		 	 ++s;  
		 	 se=s; // update to reflect end of a valid exponent (p[+-]digit+)
			}
		}
 	 if(endptr!=NULL) *endptr=(char *)se; // we now know the end of the number - so save it now (means we can have multiple returns going forward without having to worry about this)	
 	 if(expsign) rexp=-rexp;	
 	 rexp+=exp; // add in correct to exponent from mantissa processing				
	 h=ldexpf((float)r,rexp); // combine mantissa and exponent 
	 if(sign) h=-h;
#ifdef DEBUG
 	 fprintf(stderr," strtof (0x) returns %.9g [0x%.8A] (rexp=%d, exp=%d)\n",h,h,rexp,exp); 
#endif  	 
	 return h; // all done 	
	}
#endif	
  // if we get here we are processing a decimal number 	    
  // skip leading zeros
  while(*s=='0')
  	{got_number=true; // have a number (0)
	 ++s;
	}
  // 1st "real" digit
  if(my_isdigit(*s))
  	{got_number=true; // have a valid number
  	 r=*s-'0';
  	 nos_mant_digits=1;
  	 ++s;
  	}
  // now read rest of the mantissa	
  while(my_isdigit(*s))
  	{ // no need to set got_number here as would have been set by if() above on 1st digit
	  if(nos_mant_digits<maxfdigits)
	  	{ r=r*10+(*s-'0');
		  nos_mant_digits++;	
		}
	  else
	  	{if(!round_digit)
	  		{if(*s>='5') r++; // do rounding, we know *s is a digit
	  		 round_digit=true;// we can only round once
	  		}
		 if(exp<2*maxfExponent)
		      exp++; // cannot actually capture digits as more than 9 but keep track of decimal point, trap at 2*maxExponent ensures we don't overflow exp when given a number with a silly number of digits (that would overflow a double)
		}
	++s;
	}
  // now look for optional decimal point (and fractional bit of mantissa)
  if(*s=='.')
  	{ // got decimal point, skip and then look for fractional bit
  	 ++s;
  	 if(r==0)
  	 	{// number is zero at present, so deal with leading zeros in fractional bit of mantissa
  	 	 if(my_isdigit(*s)) got_number=true; // if r!=0 then we must already have had a digit so got_number has already been set to true
  	 	 while(*s=='0')
  	 	 	{
  	 	 	 ++s;
  	 	 	 if(exp > -2*maxfExponent)
  	 	 	 	exp--; // test avoids issues with silly number of leading zeros
  	 	    }
  	 	}
  	 // now process the rest of the fractional bit of the mantissa, got_number is already true at this point in the code
	 while(my_isdigit(*s))
	 	{
	 	 if(nos_mant_digits<maxfdigits)
	  			{ r=r*10+(*s-'0');
		  		  nos_mant_digits++;	
		  		  exp--;
				}
		 else
	  			{  // cannot actually capture digits as more than 9, so just ignore them (but round on 1st over 9)
	  			 if(!round_digit)
			  		{if(*s>='5') r++; // do rounding, we know *s is a digit
			  		 round_digit=true;// we can only round once
			  		}
				}
		++s;
		}
 	}
  // got all of mantissa - see if its a valid number, if not we are done
  if(!got_number)
 	{if(endptr!=NULL) *endptr=(char *)se;
#ifdef DEBUG
 	fprintf(stderr," strtof returns 0 (invalid number)\n"); 
#endif  	
 	 return 0;
 	}	
  se=s; // update to reflect end of a valid mantissa
  // now see if we have an  exponent
  if(my_tolower(*s)=='e')
  	{// have exponent, optional sign is 1st
  	 ++s ; // skip 'e'
  	 if(*s=='+') ++s;
  	 else if(*s=='-') 
  	 	{expsign=true;
  	 	 ++s;
  	 	}
  	 while(my_isdigit(*s))
	   	{if(rexp<=2*maxfExponent)
		   rexp=rexp*10+(*s - '0');  // if statement clips at a value that will result in +/-inf but will not overflow int
		 ++s;  
		 se=s; // update to reflect end of a valid exponent (e[+-]digit+)
		}
	}
 if(endptr!=NULL) *endptr=(char *)se; // we now know the end of the number - so save it now (means we can have multiple returns going forward without having to worry about this)	
 if(expsign) rexp=-rexp;	
 rexp+=exp; // add in correct to exponent from mantissa processing
 if(rexp==0)
 	{// special case, rexp==0
	 // do not need to use double here, so we use float for speed.
 	 if(sign) return -((float)r); // r is unsigned so cannot do -r !
 	 else return (float) r;
	}
 // calculate dr=r*pow(10,rexp), but by using a lookup table of powers of 10 for speed and using doubles to ensure accuracy.
 if(rexp>0)
 	{
	 if(rexp+nos_mant_digits<=9)
	 	{// optimisation: can do all calculations using uint32 which is exact and fast	 
	 	 r*=u32powersOf10[rexp];
	 	 if(sign) return -((float)r); // negative exponent means we divide by powers of 10
	 	 else return (float)r;
	 	}
	 if(rexp<=10 && r<=16777215)
	 	{// in this region we can use float rather than double as 10^10 is exact as a float (another speed optimisation, but one than thats common and therefore worthwhile)
	 	 // float has 24 bit mantissa, so can store upto 16,777,215 exactly 
	 	 // As 10=2*5 and powers of 2 only effect the exponent, 5^10=9,765,625 which easily fits into a 24 bit mantissa
	 	 if(sign) return -((float)r*fltpowersOf10[rexp]); // positive exponent means we mutiply by powers of 10
	 	 else return (float)r*fltpowersOf10[rexp];	 
		}
	 if(rexp>maxfExponent)
		{// we have defininaly overflowed
		 if(sign) return -FLT_MAX;
 		 return FLT_MAX;
 		}
 	 dr=(double)r*PosPowerOf10_hi[rexp]; // as mantissa >= 1 this may overflow, but thats OK. Array is in table_bin_10.h
	}
 else if(rexp<0)
 	{// need to take care here as mantissa is > 1 so even dividing by 10^maxfExponent may not be enough, but here we use doubles which have a much wider exponent range so its not an issue
 	 rexp=-rexp;
 	 /* divide by powers of 10 (as doubles) => this gives 0 errors (I tried multiplying by 1/10^exp but this gave 2 round trip errors ) */
	 if(rexp<=10 && r<=16777215)
	 	{// in this region we can use float rather than double as 10^10 is exact as a float (another speed optimisation, but one than thats common and therefore worthwhile)
	 	 // float has 24 bit mantissa, so can store upto 16,777,215 exactly 
	 	 // As 10=2*5 and powers of 2 only effect the exponent, 5^10=9,765,625 which easily fits into a 24 bit mantissa
	 	 if(sign) return -((float)r/fltpowersOf10[rexp]); // negative exponent means we divide by powers of 10
	 	 else return (float)r/fltpowersOf10[rexp];	 
		} 	 
	 if(rexp>=nos_elements_in(PosPowerOf10_hi)) // Array is in table_bin_10.h
		dr=0.0; // underflow
	 else
		dr=(double)r/PosPowerOf10_hi[rexp]; // negative exponent means we divide by powers of 10	 	    
	}	
 if(isinf((float)dr)) dr=FLT_MAX;
 if(sign) dr= -dr;
#ifdef DEBUG
 // while this is the normal return there are several earlier return possibilities, which this will not print for (sorry).
 fprintf(stderr," strtof returns %.18g (rexp=%d, exp=%d)\n",(double)dr,rexp,exp); 
#endif 
 return (float)dr;
}



 /* this version is very similar to fast_strtof128 but for long doubles it uses the  u2_64 type for 128 bit integers
*/
#if defined(LDBL_MAX_10_EXP) && LDBL_MAX_10_EXP==4932 /* "true" long double */
#include "../u2_64-128bits-with-two-u64/u2_64.h"

/*
 *----------------------------------------------------------------------
 *
 * long double fast_strtold(const char *s,char **endptr)
 *
 *	This procedure converts a floating-point number from an ASCII
 *	decimal representation to internal f128_t format.
 *  Also accepts "NaN", "Inf" and "Infinity" (any mix of case) which return NAN and INFINITY
 *  21/10/2025 -NAN can now be returned.
 *  29/11/2025 - the only times NAN/INFINITY can be returned are when input is "nan" or "inf", otherwise we clip at 0/LDBL_MAX
 * Results:
 *	The return value is the floating-point equivalent of string.
 *	*endptr is set to the first character after the valid number 
 *
 * If endptr == NULL it is ignored.
 *
 * Side effects:
 *	None.
 *
 * This code uses the same algorithms as the double version above but uses a 128 unsigned int (actually a u2_64) to collect the mantissa (instead of a 64 bit one)
 *
 * Warning: while this should always work, the code assumes either LDBL_MAX_10_EXP==4932  ("true" long doubles sometimes called float80) or long double==double (as in the Windows API).
 *
 *----------------------------------------------------------------------
 */
 
long double fast_strtold(const char *s,char **endptr) // if endptr != NULL returns 1st character thats not in the number
 {
  long double dr;
  long double rh,rl; // double double result  
  bool sign=false,expsign=false,got_number=false; 
  u2_64 r={0,0}; // mantissa
  u2_64 mask_msb128={0,0x0f};
  mask_msb128=lshift_u2_64(mask_msb128,124);// mask for msb of 128 bits
  int exp=0,rexp=0;
  const char *se=s; // string end - candidate for endptr
#ifdef DEBUG
  fprintf(stderr,"fast_strtold(%s):\n",s);
#endif  
  if(s==NULL) return NAN;  
  while(isspace(*s)) ++s; // skip initial whitespace	
  // deal with leading sign
  if(*s=='+') ++s;
  else if(*s=='-')
  	{sign=true;
  	 ++s;
    }
  // this is the critical path - if the character is >'9' then it starts with a letter [ nan, inf ], note '.' is < '9'
  if(*s>'9') 
  	{// not a digit - work out what we have [off the critical path]
	  // NAN is a special case - NAN is  signed in the input and keeps this on the output
	  if(my_tolower(*s)=='n' && my_tolower(s[1])=='a' && my_tolower(s[2])=='n')
	  	{/* For more information on NAN's see my nan_type repository.
		  C99 allows nan to be followed by (...) , C17 7.22.1.3 says:
	  	The expected form of the subject sequence is an optional plus or minus sign, then one of the
		following:
		....
		— NAN or NAN(n-char-sequenceopt), ignoring case in the NAN part, where:
		n-char-sequence:
			digit
			nondigit
			n-char-sequence digit
			n-char-sequence nondigit
		The subject sequence is defined as the longest initial subsequence of the input string, starting with
		the first non-white-space character, that is of the expected form. The subject sequence contains no
		characters if the input string is not of the expected form.
	 	
	 	A character sequence NAN or NAN(n-char-sequenceopt) is interpreted as a quiet NaN, if
		supported in the return type, else like a subject sequence part that does not have the expected form;
		the meaning of the n-char sequence is implementation-defined.298) 
	 
	 	298)An implementation may use the n-char sequence to determine extra information to be represented in the NaN’s
		significand.
		
		That that "nondigit" is defined in 6.4.2.1 as
			nondigit: one of
				_ a b c d e f g h i j k l m
				n o p q r s t u v w x y z
				A B C D E F G H I J K L M
				N O P Q R S T U V W X Y Z
			digit: one of
				0 1 2 3 4 5 6 7 8 9	
		 On Windows (UCRT) for floats 0XFFC00000 (-NAN) gives "-nan(ind)" [ or -NAN(IND) ] , nanf("") => nan=>0X7FC00000
	  	*/
	  	 s+=3;// skip NAN [ which is valid by its self ]
	  	 const char *sn=s;// we now look forward to see if we have NAN(n-char-sequenceopt)
	  	 if(*sn=='(')
	  	 	{// ( optional n-char sequence then )  
	  	 	 sn++; // skip (
	  	 	 if(my_tolower(*sn)=='i' && my_tolower(sn[1])=='n' && my_tolower(sn[2])=='d' && sn[3]==')' )
	  	 	 	{s+=5; // "(ind)" => special case (+5 as we have already skipped the "(" )
	  	 	 	 if(endptr!=NULL) *endptr=(char *)s;
	  	 	 	 return -NAN;// this is always -ve
	  	 	 	}
	  	 	 // not nan(ind) - look for any other pattern allowed by C99
	  	 	 while(*sn && *sn!=')' ) ++sn;// for now follow C99 and just look for ), C17 would need a more complex check here
	  	 	 if(*sn==')') s=sn+1;// found (...) so we have a valid match, +1 to skip final ) [ otherwise just the "NAN" is valid so we don't change s ]
	  	 	}
		 if(endptr!=NULL) *endptr=(char *)s;
	  	 if(sign) return -nanl("1");// -NAN would print back as  -nan(ind)
	  	 return NAN;
	  	}    
	  // INF or Infinity is a special case - and is signed
	  if(my_tolower(*s)=='i' && my_tolower(s[1])=='n' && my_tolower(s[2])=='f')
	  	{s+=3;// INF
	  	 if(my_tolower(*s)=='i' && my_tolower(s[1])=='n' && my_tolower(s[2])=='i' && my_tolower(s[3])=='t' && my_tolower(s[4])=='y' )
	  	  	s+=5; // "Infinity" is 5 more chars (inity) than "inf"
	  	 if(endptr!=NULL) *endptr=(char *)s;
	  	 if(sign) return -INFINITY;
	  	 return INFINITY;
	  	} 
	} // if(*s>'9') 
// at this point we know *s is a digit or a decimal point [ or rather *s is <='9' which includes 0 to 9 and decimal point ]
#ifdef AFormatSupport 
	/* support hex floating point numbers of the format 0xh.hhhhp+/-d as generated by printf %a */
  if(*s=='0' && (s[1]=='x' || s[1] =='X'))
  	{ // got hex number
  	 s+=2; // skip 0x
  	 // no need to skip leading zero's as we can just check is ms 4 bits of r are not zero
	 while(isxdigit(*s))
		{got_number=true; // have a valid number
	  	 //if((r & mask_msb128 )==0)
	  	 if((and_u2_64(r,mask_msb128)).hi==0)
	  		{ if(*s<='9')
	  			{
			  	 //r=r*16+(*s-'0'); // 0..9
			  	 r=uadd_u2_64(lshift_u2_64(r,4),u64_to_u2_64(0,(*s-'0')));
			  	} 
			  else
			  	{
			  	 //r=r*16+(tolower(*s)-'a'+10); // a-f or A-F
			  	 r=uadd_u2_64(lshift_u2_64(r,4),u64_to_u2_64(0,(tolower(*s)-'a'+10)));
			  	}
			}
		else if(exp<LDBL_MAX_EXP)
		      exp+=4; // cannot actually capture digits beyond this but keep track of decimal point, trap  ensures we don't overflow exp when given a number with a silly number of digits 
		 ++s;
		}
  	 // now look for optional decimal point (and fractional bit of mantissa)
  	 if(*s=='.')
  		{ // got decimal point, skip and then look for fractional bit
  	 	 ++s;
		 while(isxdigit(*s))
			{got_number=true; // have a valid number
	  	 	 //if((r & mask_msb128)==0)
	  	 	 if((and_u2_64(r,mask_msb128)).hi==0)
	  			{ if(*s<='9')
			  		//r=r*16+(*s-'0'); // 0..9
			  		r=uadd_u2_64(lshift_u2_64(r,4),u64_to_u2_64(0,(*s-'0')));
			  	 else
			  		//r=r*16+(tolower(*s)-'a'+10); // a-f or A-F
			  		r=uadd_u2_64(lshift_u2_64(r,4),u64_to_u2_64(0,(tolower(*s)-'a'+10)));
			  	 exp-=4;	
				}
			 // if we have too many digits after dp just ignore them
		 	 ++s;
		 	}
		}			  
  	 // got all of mantissa - see if its a valid number, if not we are done
  	 if(!got_number)
 		{if(endptr!=NULL) *endptr=(char *)se;
#ifdef DEBUG
 		 fprintf(stderr," fast_strtold returns 0 (invalid hex number)\n"); 
#endif  	
 	 	 return 0;
 		}	
  	 se=s; // update to reflect end of a valid mantissa
  	 // now see if we have an  exponent
  	 if(*s=='p' || *s=='P')
  		{// have exponent, optional sign is 1st
  	 	 ++s ; // skip 'p'
  	 	 if(*s=='+') ++s;
  	 	 else if(*s=='-') 
  	 		{expsign=true;
  	 	 	 ++s;
  	 		}
  	 	while(my_isdigit(*s))
	   		{if(rexp<=LDBL_MAX_EXP)
		   		rexp=rexp*10+(*s - '0');  // if statement clips at a value that will result in +/-inf but will not overflow int
		 	 ++s;  
		 	 se=s; // update to reflect end of a valid exponent (p[+-]digit+)
			}
		}
 	 if(endptr!=NULL) *endptr=(char *)se; // we now know the end of the number - so save it now (means we can have multiple returns going forward without having to worry about this)	
 	 if(expsign) rexp=-rexp;	
 	 rexp+=exp; // add in correct to exponent from mantissa processing				
	 dr=ldexpl(u2_64_to_ld(r),rexp); // combine mantissa and exponent 
	 if(sign) dr=-dr;
#ifdef DEBUG
 	 fprintf(stderr," fast_strtold (0x) returns %.18g [0x%.16A] (rexp=%d, exp=%d)\n",(double)dr,(double)dr,rexp,exp); 
#endif  	 
	 return dr; // all done 	
	}
#endif	
  // Normal decimal number, first  skip leading zeros
  while(*s=='0')
  	{got_number=true; // have a number (0)
	 ++s;
	}
  // now read rest of the mantissa	
  while(my_isdigit(*s))
  	{ got_number=true; // have a valid number
	  //if((r & mask_msb128 )==0)
	  if((and_u2_64(r,mask_msb128)).hi==0)
	  	{ // r=r*10+(*s-'0');
	  	 r=uadd_u2_64(umul_u2_64_by_ten(r),u64_to_u2_64(0,*s-'0'));
		}
	  else
	  	{ 
		  if(exp<=2*LDBL_MAX_10_EXP)
		      exp++; // cannot actually capture more digits but keep track of decimal point, trap ensures we don't overflow exp when given a number with a silly number of digits (that would overflow a long double)
		}
	++s;
	}
  // now look for optional decimal point (and fractional bit of mantissa)
  if(*s=='.')
  	{ // got decimal point, skip and then look for fractional bit
  	 ++s;
  	 if((r.hi|r.lo)==0)
  	 	{// number is zero at present, so deal with leading zeros in fractional bit of mantissa
  	 	 while(*s=='0')
  	 	 	{got_number=true;
  	 	 	 ++s;
  	 	 	 if(exp > -2*LDBL_MAX_10_EXP)
  	 	 	 	exp--; // test avoids issues with silly number of leading zeros
  	 	    }
  	 	}
  	 // now process the rest of the fractional bit of the mantissa
	 while(my_isdigit(*s))
	 	{got_number=true;
#if 1	 	
  	 	// see if the whole remaining fractional bit is "0", if so can just skip. This speeds up some conversions (and slows others) but more importantly it ensures 1, 1.0, 1.00 & 1.15, 1.150, 1.1500 etc give exactly the same result
		 
	 	 if(*s=='0')
	 		{// got a zero, see if all remaining numbers in mantissa are zero
	 		 const char *s0=s;
	 	  	 while(*s0=='0') ++s0;
	 	  	 if(!my_isdigit(*s0))
	 			{// was all zero's, just skip them
		 	 	 s=s0;
		 	 	 break;
				}
			}		 	
#endif			
	 	 //if((r & mask_msb128 )==0)
	 	 //if((and_u2_64(r,mask_msb128)).hi==0 && exp>-23) /* LDBL_MAX = 1.18973149535723176502126385303097021e+4932L, issue is that if "printf" rounds up (e.g. 6385.... -> 634) then we return infinity. only 21 digits are needed for round trip exact so adding this limit is not an issue */ 
	 	 if((and_u2_64(r,mask_msb128)).hi==0 )
	  			{ //r=r*10+(*s-'0');	
	  			 r=uadd_u2_64(umul_u2_64_by_ten(r),u64_to_u2_64(0,*s-'0'));
		  		 exp--;
				}		
		 // else - cannot actually capture digits, so just ignore them 
		++s;
		}
 	}
  // got all of mantissa - see if its a valid number, if not we are done
  if(!got_number)
 	{if(endptr!=NULL) *endptr=(char *)se;
#ifdef DEBUG
 	fprintf(stderr," fast_strtold returns 0 (invalid number)\n"); 
#endif  	
 	 return 0;
 	}	
  se=s; // update to reflect end of a valid mantissa
  // now see if we have an  exponent
  if(*s=='e' || *s=='E')
  	{// have exponent, optional sign is 1st
  	 ++s ; // skip 'e'
  	 if(*s=='+') ++s;
  	 else if(*s=='-') 
  	 	{expsign=true;
  	 	 ++s;
  	 	}
  	 while(my_isdigit(*s))
	   	{if(rexp<=2*LDBL_MAX_10_EXP)
		   rexp=rexp*10+(*s - '0');  // if statement clips at a value that will result in +/-inf but will not overflow int
		 ++s;  
		 se=s; // update to reflect end of a valid exponent (e[+-]digit+)
		}
	}
 if(endptr!=NULL) *endptr=(char *)se; // we now know the end of the number - so save it now (means we can have multiple returns going forward without having to worry about this)	
 if(expsign) rexp=-rexp;	
 rexp+=exp; // add in correct to exponent from mantissa processing
 if(rexp>LDBL_MAX_10_EXP)
	{// we have defininaly overflowed - see comments on main return - this returns LDBL_MAX (only "inf" returns INFINITY
#ifdef DEBUG
 	 fprintf(stderr," fast_strtold returns LDBL_MAX\n"); 
#endif 	
	 if(sign) return -LDBL_MAX;
	 return LDBL_MAX;
	} 
 // now we need to convert r into a double-double long double and (eventually) multiply by 10^rexp
 // this needs to be done in blocks of 32 bits due to the limited size of the LD mantissa.
 // I have tried simpler solutions and they introduce errors!
 long double r1h,r1l,r2h,r2l;
 uint64_t l32,h32; // low and high 32 bits of  64 bits
 uint64_t c63=((uint64_t)1) << 63; // biggest shift possible with u64
 long double c64=c63;
 c64*=2; // 1<<64 - should be exact 
 if(r.hi==0)
 	{//  lower 64 bits => r1h/l (upper are 0) - this is the most likely case unless the user supplies a lot of digits in the mantissa (19 digits fit into r.lo).
	 l32=(r.lo) & 0xffffffffU;
	 h32=(r.lo)^l32;// XOR => zero low bits
	 if(h32==0)
		{
		 r1h=(long double)l32 ; 
		 r1l=0;
		} 
	 else /* split into two 32 bit numbers, combine them into a double-double -long double r1h/l */
	 	ld_twosum(&r1h,&r1l,  (long double)h32,(long double)l32); // ld_twosum(long double *xh, long double *xl,long double a,long double b)  // adds a and b to give double double "x". 	  	
 	}
 else
 	{// need to process all bits, do upper 64 bits (which is then subdivided into two sets of 32 bits) to r1h/l, then lower 64 bits to r2h/l, 
 	 // finally combine r1h/l & r2h/l to give a double-double long double in r1h/l
	 // do upper 64 bits first
	 l32=(r.hi) & 0xffffffffU;
	 h32=(r.hi)^l32;// XOR => zero low bits
	 if(h32==0)
		{
		 r1h=(long double)l32*c64 ; 
		 r1l=0;
		} 
	 else /* split into two 32 bit numbers */
	 	ld_twosum(&r1h,&r1l,  (long double)h32*c64,(long double)l32*c64);
	 // now repeat for lower 64 bits
	 l32=(r.lo) & 0xffffffffU;
	 h32=(r.lo)^l32;// XOR => zero low bits
	 if(h32==0)
		{// ld_twosum(long double *xh, long double *xl,long double a,long double b)  // adds a and b to give double double "x". 
		 r2h=(long double)l32 ; 
		 r2l=0;
		} 
	 else /* split into two 32 bit numbers */
	 	ld_twosum(&r2h,&r2l,  (long double)h32,(long double)l32); 	 
	 add_ldd_ldd(&r1h,&r1l,r1h,r1l,r2h,r2l); // merge together into a single double-double long double
 	}
 ldd_mult_power10(&rh,&rl, r1h,r1l, rexp ); // finally do power 10 	 	

 if(isinf(rh))
 	{// clip at LDBL_MAX (the only way we return INFINITY is if asked to convert "inf"
 	 // this also means we don't need to worry about a number very close to LDBL_MAX being rounded up when printed to a finite number of digits which could make it larger than LDBL_MAX.
 	 dr=LDBL_MAX;
 	}
 else
 	dr=rh+rl;
 if(sign) dr= -dr;
#if defined(DEBUG)
 // This is the normal return 
 if(!isfinite(dr))
     fprintf(stderr," fast_strtold returns %.18g (rexp=%d, exp=%d)\n",(double)dr,rexp,exp); 
#endif 

 return dr; 
}

#else // not true LD , ie double==long double
long double fast_strtold(const char *s,char **endptr)
{return fast_strtod(s,endptr);
}
#endif
 
 
 
#ifdef ATOF128 /* if defined add support for reading __float128 's */
/* following are predefined by gcc
__FLT128_MAX_10_EXP__ 4932
__FLT128_DENORM_MIN__ 6.47517511943802511092443895822764655e-4966F128
__FLT128_MIN_EXP__ (-16381)
__FLT128_MIN_10_EXP__ (-4931)
__FLT128_MANT_DIG__ 113
__FLT128_HAS_INFINITY__ 1
__FLT128_MAX_EXP__ 16384
__FLT128_HAS_DENORM__ 1
__FLT128_DIG__ 33
__FLT128_MAX__ 1.18973149535723176508575932662800702e+4932F128
__SIZEOF_FLOAT128__ 16
__FLT128_MIN__ 3.36210314311209350626267781732175260e-4932F128
__FLT128_HAS_QUIET_NAN__ 1
__FLT128_EPSILON__ 1.92592994438723585305597794258492732e-34F128
__FLT128_DECIMAL_DIG__ 36


__SIZEOF_INT128__ 16

These should be referenced as :

	FLT128_MAX: largest finite number
	FLT128_MIN: smallest positive number with full precision
	FLT128_EPSILON: difference between 1 and the next larger representable number 
	FLT128_DENORM_MIN: smallest positive denormalized number
	FLT128_MANT_DIG: number of digits in the mantissa (bit precision)
	FLT128_MIN_EXP: maximal negative exponent
	FLT128_MAX_EXP: maximal positive exponent
	FLT128_DIG: number of decimal digits in the mantissa
	FLT128_MIN_10_EXP: maximal negative decimal exponent
	FLT128_MAX_10_EXP: maximal positive decimal exponent
	
*/
/*
 *----------------------------------------------------------------------
 *
 * f128_t fast_strtof128(const char *s,char **endptr)
 *
 *	This procedure converts a floating-point number from an ASCII
 *	decimal representation to internal f128_t format.
 *  Also accepts "NaN", "Inf" and "Infinity" (any mix of case) which return NAN and INFINITY
 *  21/10/2025 -NAN can now be returned.
 * Results:
 *	The return value is the floating-point equivalent of string.
 *	*endptr is set to the first character after the valid number 
 *
 * If endptr == NULL it is ignored.
 *
 * Side effects:
 *	None.
 *
 * This code uses the same algorithms as the double version above but uses a 128 unsigned int (actually a u2_64) to collect mantissa (instead of a 64 bit one)
 *----------------------------------------------------------------------
 */
#if defined(__GNUC__) && defined(__SIZEOF_FLOAT128__)   && !defined(__BORLANDC__)      /* Builder C++ Community version 12.1 patch 1 defines __SIZEOF_FLOAT128__ but __float128's cannot be used in sensible programs due to compiler bugs */
  #include <quadmath.h> /* see https://gcc.gnu.org/onlinedocs/libquadmath/quadmath_005fsnprintf.html#quadmath_005fsnprintf - also needs quadmath library linking in - this is only available with gcc */
#endif

#include "../u2_64-128bits-with-two-u64/u2_64.h"

typedef __float128 f128_t;
 
__float128 fast_strtof128(const char *s,char **endptr) // if endptr != NULL returns 1st character thats not in the number
 {
  f128_t dr;
  f128_t rh,rl; // double double result  
  bool sign=false,expsign=false,got_number=false; 
  u2_64 r={0,0}; // mantissa
  u2_64 mask_msb128={0,0x0f};
  mask_msb128=lshift_u2_64(mask_msb128,124);// mask for msb of 128 bits
  int exp=0,rexp=0;
  const char *se=s; // string end - candidate for endptr
#ifdef DEBUG
  fprintf(stderr,"strtof128(%s):\n",s);
#endif    
  while(isspace(*s)) ++s; // skip initial whitespace	
  // deal with leading sign
  if(*s=='+') ++s;
  else if(*s=='-')
  	{sign=true;
  	 ++s;
    }
  // this is the critical path - if the character is >'9' then it starts with a letter [ nan, inf ], note '.' is < '9'
  if(*s>'9') 
  	{// not a digit - work out what we have [off the critical path]
	  // NAN is a special case - NAN is  signed in the input and keeps this on the output
	  if(my_tolower(*s)=='n' && my_tolower(s[1])=='a' && my_tolower(s[2])=='n')
	  	{/* For more information on NAN's see my nan_type repository.
		  C99 allows nan to be followed by (...) , C17 7.22.1.3 says:
	  	The expected form of the subject sequence is an optional plus or minus sign, then one of the
		following:
		....
		— NAN or NAN(n-char-sequenceopt), ignoring case in the NAN part, where:
		n-char-sequence:
			digit
			nondigit
			n-char-sequence digit
			n-char-sequence nondigit
		The subject sequence is defined as the longest initial subsequence of the input string, starting with
		the first non-white-space character, that is of the expected form. The subject sequence contains no
		characters if the input string is not of the expected form.
	 	
	 	A character sequence NAN or NAN(n-char-sequenceopt) is interpreted as a quiet NaN, if
		supported in the return type, else like a subject sequence part that does not have the expected form;
		the meaning of the n-char sequence is implementation-defined.298) 
	 
	 	298)An implementation may use the n-char sequence to determine extra information to be represented in the NaN’s
		significand.
		
		That that "nondigit" is defined in 6.4.2.1 as
			nondigit: one of
				_ a b c d e f g h i j k l m
				n o p q r s t u v w x y z
				A B C D E F G H I J K L M
				N O P Q R S T U V W X Y Z
			digit: one of
				0 1 2 3 4 5 6 7 8 9	
		 On Windows (UCRT) for floats 0XFFC00000 (-NAN) gives "-nan(ind)" [ or -NAN(IND) ] , nanf("") => nan=>0X7FC00000
	  	*/
	  	 s+=3;// skip NAN [ which is valid by its self ]
	  	 const char *sn=s;// we now look forward to see if we have NAN(n-char-sequenceopt)
	  	 if(*sn=='(')
	  	 	{// ( optional n-char sequence then )  
	  	 	 sn++; // skip (
	  	 	 if(my_tolower(*sn)=='i' && my_tolower(sn[1])=='n' && my_tolower(sn[2])=='d' && sn[3]==')' )
	  	 	 	{s+=5; // "(ind)" => special case (+5 as we have already skipped the "(" )
	  	 	 	 if(endptr!=NULL) *endptr=(char *)s;
	  	 	 	 return -NAN;// this is always -ve
	  	 	 	}
	  	 	 // not nan(ind) - look for any other pattern allowed by C99
	  	 	 while(*sn && *sn!=')' ) ++sn;// for now follow C99 and just look for ), C17 would need a more complex check here
	  	 	 if(*sn==')') s=sn+1;// found (...) so we have a valid match, +1 to skip final ) [ otherwise just the "NAN" is valid so we don't change s ]
	  	 	}
		 if(endptr!=NULL) *endptr=(char *)s;
	  	 if(sign) return -nan("1");// -NAN would print back as  -nan(ind)
	  	 return NAN;
	  	}    
	  // INF or Infinity is a special case - and is signed
	  if(my_tolower(*s)=='i' && my_tolower(s[1])=='n' && my_tolower(s[2])=='f')
	  	{s+=3;// INF
	  	 if(my_tolower(*s)=='i' && my_tolower(s[1])=='n' && my_tolower(s[2])=='i' && my_tolower(s[3])=='t' && my_tolower(s[4])=='y' )
	  	  	s+=5; // "Infinity" is 5 more chars (inity) than "inf"
	  	 if(endptr!=NULL) *endptr=(char *)s;
	  	 if(sign) return -INFINITY;
	  	 return INFINITY;
	  	} 
	} // if(*s>'9') 
// at this point we know *s is a digit or a decimal point [ or rather *s is <='9' which includes 0 to 9 and decimal point ]
#ifdef AFormatSupport 
	/* support hex floating point numbers of the format 0xh.hhhhp+/-d as generated by printf %a */
  if(*s=='0' && (s[1]=='x' || s[1] =='X'))
  	{ // got hex number
  	 s+=2; // skip 0x
  	 // no need to skip leading zero's as we can just check is ms 4 bits of r are not zero
	 while(isxdigit(*s))
		{got_number=true; // have a valid number
	  	 //if((r & mask_msb128 )==0)
	  	 if((and_u2_64(r,mask_msb128)).hi==0)
	  		{ if(*s<='9')
	  			{
			  	 //r=r*16+(*s-'0'); // 0..9
			  	 r=uadd_u2_64(lshift_u2_64(r,4),u64_to_u2_64(0,(*s-'0')));
			  	} 
			  else
			  	{
			  	 //r=r*16+(tolower(*s)-'a'+10); // a-f or A-F
			  	 r=uadd_u2_64(lshift_u2_64(r,4),u64_to_u2_64(0,(tolower(*s)-'a'+10)));
			  	}
			}
		else if(exp<FLT128_MAX_EXP)
		      exp+=4; // cannot actually capture digits beyond this but keep track of decimal point, trap  ensures we don't overflow exp when given a number with a silly number of digits 
		 ++s;
		}
  	 // now look for optional decimal point (and fractional bit of mantissa)
  	 if(*s=='.')
  		{ // got decimal point, skip and then look for fractional bit
  	 	 ++s;
		 while(isxdigit(*s))
			{got_number=true; // have a valid number
	  	 	 //if((r & mask_msb128)==0)
	  	 	 if((and_u2_64(r,mask_msb128)).hi==0)
	  			{ if(*s<='9')
			  		//r=r*16+(*s-'0'); // 0..9
			  		r=uadd_u2_64(lshift_u2_64(r,4),u64_to_u2_64(0,(*s-'0')));
			  	 else
			  		//r=r*16+(tolower(*s)-'a'+10); // a-f or A-F
			  		r=uadd_u2_64(lshift_u2_64(r,4),u64_to_u2_64(0,(tolower(*s)-'a'+10)));
			  	 exp-=4;	
				}
			 // if we have too many digits after dp just ignore them
		 	 ++s;
		 	}
		}			  
  	 // got all of mantissa - see if its a valid number, if not we are done
  	 if(!got_number)
 		{if(endptr!=NULL) *endptr=(char *)se;
#ifdef DEBUG
 		 fprintf(stderr," strtof128 returns 0 (invalid hex number)\n"); 
#endif  	
 	 	 return 0;
 		}	
  	 se=s; // update to reflect end of a valid mantissa
  	 // now see if we have an  exponent
  	 if(*s=='p' || *s=='P')
  		{// have exponent, optional sign is 1st
  	 	 ++s ; // skip 'p'
  	 	 if(*s=='+') ++s;
  	 	 else if(*s=='-') 
  	 		{expsign=true;
  	 	 	 ++s;
  	 		}
  	 	while(my_isdigit(*s))
	   		{if(rexp<=FLT128_MAX_EXP)
		   		rexp=rexp*10+(*s - '0');  // if statement clips at a value that will result in +/-inf but will not overflow int
		 	 ++s;  
		 	 se=s; // update to reflect end of a valid exponent (p[+-]digit+)
			}
		}
 	 if(endptr!=NULL) *endptr=(char *)se; // we now know the end of the number - so save it now (means we can have multiple returns going forward without having to worry about this)	
 	 if(expsign) rexp=-rexp;	
 	 rexp+=exp; // add in correct to exponent from mantissa processing				
	 dr=ldexpq(u2_64_to_flt128(r),rexp); // combine mantissa and exponent 
	 if(sign) dr=-dr;
#ifdef DEBUG
 	 fprintf(stderr," strtof128 (0x) returns %.18g [0x%.16A] (rexp=%d, exp=%d)\n",(double)dr,(double)dr,rexp,exp); 
#endif  	 
	 return dr; // all done 	
	}
#endif	
  // Normal decimal number, first  skip leading zeros
  while(*s=='0')
  	{got_number=true; // have a number (0)
	 ++s;
	}
  // now read rest of the mantissa	
  while(my_isdigit(*s))
  	{ got_number=true; // have a valid number
	  //if((r & mask_msb128 )==0)
	  if((and_u2_64(r,mask_msb128)).hi==0)
	  	{ // r=r*10+(*s-'0');
	  	 r=uadd_u2_64(umul_u2_64_by_ten(r),u64_to_u2_64(0,*s-'0'));
		}
	  else
	  	{ 
		  if(exp<=2*FLT128_MAX_10_EXP)
		      exp++; // cannot actually capture more digits but keep track of decimal point, trap ensures we don't overflow exp when given a number with a silly number of digits (that would overflow a flt128)
		}
	++s;
	}
  // now look for optional decimal point (and fractional bit of mantissa)
  if(*s=='.')
  	{ // got decimal point, skip and then look for fractional bit
  	 ++s;
  	 if((r.hi|r.lo)==0)
  	 	{// number is zero at present, so deal with leading zeros in fractional bit of mantissa
  	 	 while(*s=='0')
  	 	 	{got_number=true;
  	 	 	 ++s;
  	 	 	 if(exp > -2*FLT128_MAX_10_EXP)
  	 	 	 	exp--; // test avoids issues with silly number of leading zeros
  	 	    }
  	 	}
  	 // now process the rest of the fractional bit of the mantissa
	 while(my_isdigit(*s))
	 	{got_number=true;
#if 1	 	
  	 	// see if the whole remaining fractional bit is "0", if so can just skip. This speeds up some conversions (and slows others) but more importantly it ensures 1, 1.0, 1.00 & 1.15, 1.150, 1.1500 etc give exactly the same result
		 
	 	 if(*s=='0')
	 		{// got a zero, see if all remaining numbers in mantissa are zero
	 		 const char *s0=s;
	 	  	 while(*s0=='0') ++s0;
	 	  	 if(!my_isdigit(*s0))
	 			{// was all zero's, just skip them
		 	 	 s=s0;
		 	 	 break;
				}
			}		 	
#endif			
	 	 //if((r & mask_msb128 )==0)
	 	 if((and_u2_64(r,mask_msb128)).hi==0)
	  			{ //r=r*10+(*s-'0');	
	  			 r=uadd_u2_64(umul_u2_64_by_ten(r),u64_to_u2_64(0,*s-'0'));
		  		 exp--;
				}		
		 // else - cannot actually capture digits, so just ignore them 
		++s;
		}
 	}
  // got all of mantissa - see if its a valid number, if not we are done
  if(!got_number)
 	{if(endptr!=NULL) *endptr=(char *)se;
#ifdef DEBUG
 	fprintf(stderr," strtof128 returns 0 (invalid number)\n"); 
#endif  	
 	 return 0;
 	}	
  se=s; // update to reflect end of a valid mantissa
  // now see if we have an  exponent
  if(*s=='e' || *s=='E')
  	{// have exponent, optional sign is 1st
  	 ++s ; // skip 'e'
  	 if(*s=='+') ++s;
  	 else if(*s=='-') 
  	 	{expsign=true;
  	 	 ++s;
  	 	}
  	 while(my_isdigit(*s))
	   	{if(rexp<=2*FLT128_MAX_10_EXP)
		   rexp=rexp*10+(*s - '0');  // if statement clips at a value that will result in +/-inf but will not overflow int
		 ++s;  
		 se=s; // update to reflect end of a valid exponent (e[+-]digit+)
		}
	}
 if(endptr!=NULL) *endptr=(char *)se; // we now know the end of the number - so save it now (means we can have multiple returns going forward without having to worry about this)	
 if(expsign) rexp=-rexp;	
 rexp+=exp; // add in correct to exponent from mantissa processing
 /*  calculate dr=(f128_t)r*powl(10,rexp), using double double f128 maths for accuracy  */
 if(rexp>FLT128_MAX_10_EXP)
		{// we have defininaly overflowed
		 if(sign) return -FLT128_MAX;
 		 return FLT128_MAX;
 		}
 u2_64toDD_f128(r,&rh,&rl);	// convert r to dd
 // void f128_to_power10(__float128 *rh,__float128 *rl, __float128 rin_h, __float128 rin_l, int32_t rexp );  // rh/rl=rin_h/l*10^rexp  rexp can be +/- 
 f128_mult_power10(&dr,&rl,rh,rl,rexp); // result "hi" to dr
 if(isinfq(dr)) dr=FLT128_MAX;
 if(sign) dr= -dr;
#ifdef DEBUG
 // This is the normal return 
 fprintf(stderr," strtof128 returns %.18g (rexp=%d, exp=%d)\n",(double)dr,rexp,exp); 
#endif 

 return dr; 
}

#endif // if defined ATOF128

/* now restore gcc options to those set by the user */
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)) || defined(__clang__)
#pragma GCC pop_options
#endif
