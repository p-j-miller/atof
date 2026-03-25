# atof
Provides fast versions of the C functions atof(),strtod(),strtof(),strtold() and ,strtof128() which convert strings to floating point numbers.

Version 1.1 provides significantly faster string->double conversion and small speed improvements in the other conversions. All now support NAN(n-char-sequenceopt) - from C99 (and better defined in C17), but "n-char-sequenceopt" is just skipped in the input stream.

The following functions are provided:
~~~
	double fast_atof(const char *s,bool * not_number);
	double fast_atof_nan(const char *s);// like fast_atof, but returns NAN if whole string is not a valid number
	double fast_strtod(const char *s,char ** endptr);
	float fast_strtof(const char *s,char **endptr); // if endptr != NULL returns 1st character that's not in the number
	long double fast_strtold(const char *s,char ** endptr);
	__float128 fast_strtof128(const char *s,char **endptr); // if endptr != NULL returns 1st character that's not in the number
~~~
Timings:
~~~
 Timings (Winlibs gcc 15.2.0, w64, average from test program on i3 10100 ):
     MSVCRT strtod() - 2786ns/test
   	 UCRT strtod()   - 527ns/test (5,3* faster)
	 fast_strtod()	 - 187ns/test (14.9* faster MSVCRT, *2.8 faster than UCRT)
	 fast_strtod+ryu - 112ns/test (24.9* faster MSVCRT, *4.7 vs UCRT, *1.7 vs fast_strtod())
fast_strtod (1v1) 	 - 64ns/test  (43.5* faster MSVCRT, *8.2 vs UCRT, *2.9 vs fast_strtod() 1v0 , *1.8 vs fast_strtod+ryu 1v0)

 float conversions are significantly faster, while long double and f128 are slower.
~~~
For information on the Ryu algorithm see https://dl.acm.org/doi/10.1145/3360595 

Complementary functionality for printf,sprintf etc (which quickly convert floating point numbers to "strings") is provided by ya_sprint at https://github.com/p-j-miller/ya-sprintf , but the files here can be used without using ya_sprint if required.

Note that fast_strtof128() requires the compiler to support __float128's (it does not require __int128's).

# installation
It is recommended that these files are installed in a directory called atof-and-ftoa

 Note this code leverages (all available at https://github.com/p-j-miller ):
 ~~~
	ryu - from ya_sprintf (note this has a different license - see ya_sprintf for details) - this is optional and as of version 1v1 is not recommended as its slower than the version that does not use Ryu
	ya-dconvert from ya-sprintf
	double-double functions from double-double
	128 bit functionality using two uint64_t's from u2_64 
	power of 10 tables from power10
	my_printf for debugging from my_printf
~~~	
 A test program for these functions is included in ya_sprintf - to compile this (from directory containing ya_sprintf):
 
 Under Windows using winlibs gcc 15.2.0 -
 ~~~
C:\winlibs\winlibs-x86_64-posix-seh-gcc-15.2.0-mingw-w64ucrt-13.0.0-r2\mingw64\bin\gcc -Wall -m64 -fexcess-precision=standard -Ofast  -std=gnu99 -I. -I../ya-sprintf/ main.c ../atof-and-ftoa/atof.c ../double-double/double-double.c ../u2_64-128bits-with-two-u64/u2_64.c ../ya-sprintf/ya-dconvert.c ../my_printf/my_printf.c ../nan_type/nan_type.c ../ya-sprintf/ryu/d2fixed_ya_sprintf.c ../ya-sprintf/ryu/s2d_fast_atof.c ../hr_timer/hr_timer.c ../fma/fmaq.c -lquadmath -static -o test.exe
 ~~~ 
 Under Linux -
 ~~~
  gcc -m64 -Wall -Ofast -fexcess-precision=standard -I. -I../ya-sprintf/ -D_FORTIFY_SOURCE=1 main.c ../atof-and-ftoa/atof.c ../double-double/double-double.c ../u2_64-128bits-with-two-u64/u2_64.c ../ya-sprintf/ya-dconvert.c ../my_printf/my_printf.c ../nan_type/nan_type.c ../ya-sprintf/ryu/d2fixed_ya_sprintf.c ../ya-sprintf/ryu/s2d_fast_atof.c ../hr_timer/hr_timer.c ../fma/fmaq.c -lquadmath -lm -o test
~~~
For the expected output from the test program see main.c in ya_sprintf .
  
  
  
