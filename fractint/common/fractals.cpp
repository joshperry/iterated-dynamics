//
//	FRACTALS.C, FRACTALP.C and CALCFRAC.C actually calculate the fractal
//	images (well, SOMEBODY had to do it!).  The modules are set up so that
//	all logic that is independent of any fractal-specific code is in
//	CALCFRAC.C, the code that IS fractal-specific is in FRACTALS.C, and the
//	structure that ties (we hope!) everything together is in FRACTALP.C.
//	Original author Tim Wegner, but just about ALL the authors have
//	contributed SOME code to this routine at one time or another, or
//	contributed to one of the many massive restructurings.
//
//	The Fractal-specific routines are divided into three categories:
//
//	1. Routines that are called once-per-orbit to calculate the orbit
//		value. These have names like "XxxxFractal", and their function
//		pointers are stored in g_fractal_specific[g_fractal_type].orbitcalc. EVERY
//		new fractal type needs one of these. Return 0 to continue iterations,
//		1 if we're done. Results for integer fractals are left in 'g_new_z_l.real()' and
//		'g_new_z_l.imag()', for floating point fractals in 'new.real()' and 'new.imag()'.
//
//	2. Routines that are called once per pixel to set various variables
//		prior to the orbit calculation. These have names like xxx_per_pixel
//		and are fairly generic - chances are one is right for your new type.
//		They are stored in g_fractal_specific[g_fractal_type].per_pixel.
//
//	3. Routines that are called once per screen to set various variables.
//		These have names like XxxxSetup, and are stored in
//		g_fractal_specific[g_fractal_type].per_image.
//
//	4. The main fractal routine. Usually this will be standard_fractal(),
//		but if you have written a stand-alone fractal routine independent
//		of the standard_fractal mechanisms, your routine name goes here,
//		stored in g_fractal_specific[g_fractal_type].calculate_type.per_image.
//
//	Adding a new fractal type should be simply a matter of adding an item
//	to the 'g_fractal_specific' structure, writing (or re-using one of the existing)
//	an appropriate setup, per_image, per_pixel, and orbit routines.
//
#include <climits>
#include <string>

#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"
#include "externs.h"

#include "calcfrac.h"
#include "Externals.h"
#include "fpu.h"
#include "fracsubr.h"
#include "fractals.h"
#include "hcmplx.h"
#include "mpmath.h"
#include "EscapeTime.h"
#include "Formula.h"
#include "MathUtil.h"
#include "QuaternionEngine.h"

static double dx_pixel_calc();
static double dy_pixel_calc();
static long lx_pixel_calc();
static long ly_pixel_calc();

ComplexL g_coefficient_l = { 0, 0 };
ComplexL g_initial_z_l = { 0, 0 };
ComplexL g_old_z_l = { 0, 0 };
ComplexL g_new_z_l = { 0, 0 };
ComplexL g_temp_z_l = { 0, 0 };
ComplexL g_temp_sqr_l = { 0, 0 };
ComplexL g_parameter_l = { 0, 0 };
ComplexL g_parameter2_l = { 0, 0 };

StdComplexD g_coefficient(0.0, 0.0);
ComplexD g_temp_sqr = { 0.0, 0.0 };
ComplexD g_parameter = { 0, 0 };
ComplexD g_parameter2 = { 0, 0 };

int g_degree = 0;
double g_threshold = 0.0;
ComplexD g_power = { 0.0, 0.0};
int g_bit_shift_minus_1 = 0;
double g_two_pi = MathUtil::Pi*2.0;
int g_c_exp = 0;
ComplexD *g_float_parameter = 0;
ComplexL *g_long_parameter = 0; // used here and in jb.c
double g_cos_x = 0.0;
double g_sin_x = 0.0;
long g_one_fudge = 0;
long g_two_fudge = 0;

// pre-calculated values for fractal types Magnet2M & Magnet2J
static ComplexD s_3_c_minus_1 = { 0.0, 0.0 };		// 3*(g_float_parameter - 1)
static ComplexD s_3_c_minus_2 = { 0.0, 0.0 };        // 3*(g_float_parameter - 2)
static ComplexD s_c_minus_1_c_minus_2 = { 0.0, 0.0 }; // (g_float_parameter - 1)*(g_float_parameter - 2)
static ComplexD s_temp2 = { 0.0, 0.0 };
static double s_cos_y = 0.0;
static double s_sin_y = 0.0;
static double s_temp_exp = 0.0;
static double s_old_x_init_x_fp = 0.0;
static double s_old_y_init_y_fp = 0.0;
static double s_old_x_init_y_fp = 0.0;
static double s_old_y_init_x_fp = 0.0;
static long s_old_x_init_x = 0;
static long s_old_y_init_y = 0;
static long s_old_x_init_y = 0;
static long s_old_y_init_x = 0;
// temporary variables for trig use
static long s_cos_x_l = 0;
static long s_sin_x_l = 0;
static long s_cos_y_l = 0;
static long s_sin_y_l = 0;
static double s_xt;
static double s_yt;
static ComplexL s_temp_z2_l = { 0, 0 };

void magnet2_precalculate_fp() // precalculation for Magnet2 (M & J) for speed
{
	s_3_c_minus_1.real(g_float_parameter->real() - 1.0);
	s_3_c_minus_1.imag(g_float_parameter->imag());
	s_3_c_minus_2.real(g_float_parameter->real() - 2.0);
	s_3_c_minus_2.imag(g_float_parameter->imag());
	s_c_minus_1_c_minus_2.real(s_3_c_minus_1.real()*s_3_c_minus_2.real() - s_3_c_minus_1.imag()*s_3_c_minus_2.imag());
	s_c_minus_1_c_minus_2.imag(s_3_c_minus_1.real()*s_3_c_minus_2.imag() + s_3_c_minus_1.imag()*s_3_c_minus_2.real());
	s_3_c_minus_1.real(s_3_c_minus_1.real() + s_3_c_minus_1.real() + s_3_c_minus_1.real());
	s_3_c_minus_1.imag(s_3_c_minus_1.imag() + s_3_c_minus_1.imag() + s_3_c_minus_1.imag());
	s_3_c_minus_2.real(s_3_c_minus_2.real() + s_3_c_minus_2.real() + s_3_c_minus_2.real());
	s_3_c_minus_2.imag(s_3_c_minus_2.imag() + s_3_c_minus_2.imag() + s_3_c_minus_2.imag());
}

// --------------------------------------------------------------------
// Bailout Routines Macros
// --------------------------------------------------------------------
int bail_out_mod_fp()
{
	g_temp_sqr.real(sqr(g_new_z.real()));
	g_temp_sqr.imag(sqr(g_new_z.imag()));
	g_magnitude = g_temp_sqr.real() + g_temp_sqr.imag();
	if (g_magnitude >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int bail_out_real_fp()
{
	g_temp_sqr.real(sqr(g_new_z.real()));
	g_temp_sqr.imag(sqr(g_new_z.imag()));
	g_magnitude = g_temp_sqr.real() + g_temp_sqr.imag();
	if (g_temp_sqr.real() >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int bail_out_imag_fp()
{
	g_temp_sqr.real(sqr(g_new_z.real()));
	g_temp_sqr.imag(sqr(g_new_z.imag()));
	g_magnitude = g_temp_sqr.real() + g_temp_sqr.imag();
	if (g_temp_sqr.imag() >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int bail_out_or_fp()
{
	g_temp_sqr.real(sqr(g_new_z.real()));
	g_temp_sqr.imag(sqr(g_new_z.imag()));
	g_magnitude = g_temp_sqr.real() + g_temp_sqr.imag();
	if (g_temp_sqr.real() >= g_rq_limit || g_temp_sqr.imag() >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int bail_out_and_fp()
{
	g_temp_sqr.real(sqr(g_new_z.real()));
	g_temp_sqr.imag(sqr(g_new_z.imag()));
	g_magnitude = g_temp_sqr.real() + g_temp_sqr.imag();
	if (g_temp_sqr.real() >= g_rq_limit && g_temp_sqr.imag() >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int bail_out_manhattan_fp()
{
	double manhmag;
	g_temp_sqr.real(sqr(g_new_z.real()));
	g_temp_sqr.imag(sqr(g_new_z.imag()));
	g_magnitude = g_temp_sqr.real() + g_temp_sqr.imag();
	manhmag = fabs(g_new_z.real()) + fabs(g_new_z.imag());
	if ((manhmag*manhmag) >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int bail_out_manhattan_r_fp()
{
	double manrmag;
	g_temp_sqr.real(sqr(g_new_z.real()));
	g_temp_sqr.imag(sqr(g_new_z.imag()));
	g_magnitude = g_temp_sqr.real() + g_temp_sqr.imag();
	manrmag = g_new_z.real() + g_new_z.imag(); // don't need abs() since we square it next
	if ((manrmag*manrmag) >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

static long const TRIG_LIMIT_16 = (8L << 16);		// domain limit of fast trig functions

inline long TRIG_ARG_L(long x)
{
	if (labs(x) > TRIG_LIMIT_16)
	{
		return DoubleToFudge(fmod(FudgeToDouble(x), g_two_pi));
	}
	return x;
}

// --------------------------------------------------------------------
// Fractal (once per iteration) routines
// --------------------------------------------------------------------

// Raise complex number (base) to the (exp) power, storing the result
// in complex (result).
//
void complex_power(ComplexD const *base, int exp, ComplexD *result)
{
	if (exp < 0)
	{
		complex_power(base, -exp, result);
		CMPLXrecip(*result, *result);
		return;
	}

	s_xt = base->real();
	s_yt = base->imag();

	if (exp & 1)
	{
		result->real(s_xt);
		result->imag(s_yt);
	}
	else
	{
		result->real(1.0);
		result->imag(0.0);
	}

	exp >>= 1;
	while (exp)
	{
		double temp = s_xt*s_xt - s_yt*s_yt;
		s_yt = 2*s_xt*s_yt;
		s_xt = temp;

		if (exp & 1)
		{
				temp = s_xt*result->real() - s_yt*result->imag();
				result->imag(result->imag()*s_xt + s_yt*result->real());
				result->real(temp);
		}
		exp >>= 1;
	}
}

#if !defined(NO_FIXED_POINT_MATH)
// long version
static long lxt, lyt, lt2;
int complex_power_l(ComplexL *base, int exp, ComplexL *result, int bit_shift)
{
	long maxarg = 64L << bit_shift;

	if (exp < 0)
	{
		g_overflow = (complex_power_l(base, -exp, result, bit_shift) != 0);
		LCMPLXrecip(*result, *result);
		return g_overflow;
	}

	g_overflow = false;
	lxt = base->real();
	lyt = base->imag();

	if (exp & 1)
	{
		result->real(lxt);
		result->imag(lyt);
	}
	else
	{
		result->real(1L << bit_shift);
		result->imag(0L);
	}

	exp >>= 1;
	while (exp)
	{
		//
		// if (labs(lxt) >= maxarg || labs(lyt) >= maxarg)
		//	return -1;
		//
		lt2 = multiply(lxt, lxt, bit_shift) - multiply(lyt, lyt, bit_shift);
		lyt = multiply(lxt, lyt, g_bit_shift_minus_1);
		if (g_overflow)
		{
			return g_overflow;
		}
		lxt = lt2;

		if (exp & 1)
		{
				lt2 = multiply(lxt, result->real(), bit_shift) - multiply(lyt, result->imag(), bit_shift);
				result->imag(multiply(result->imag(), lxt, bit_shift) + multiply(lyt, result->real(), bit_shift));
				result->real(lt2);
		}
		exp >>= 1;
	}
	if (result->real() == 0 && result->imag() == 0)
	{
		g_overflow = true;
	}
	return g_overflow;
}
#endif

int barnsley1_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// Barnsley's Mandelbrot type M1 from "Fractals
	// Everywhere" by Michael Barnsley, p. 322

	// calculate intermediate products
	s_old_x_init_x = multiply(g_old_z_l.real(), g_long_parameter->real(), g_bit_shift);
	s_old_y_init_y = multiply(g_old_z_l.imag(), g_long_parameter->imag(), g_bit_shift);
	s_old_x_init_y = multiply(g_old_z_l.real(), g_long_parameter->imag(), g_bit_shift);
	s_old_y_init_x = multiply(g_old_z_l.imag(), g_long_parameter->real(), g_bit_shift);
	// orbit calculation
	if (g_old_z_l.real() >= 0)
	{
		g_new_z_l.real((s_old_x_init_x - g_long_parameter->real() - s_old_y_init_y));
		g_new_z_l.imag((s_old_y_init_x - g_long_parameter->imag() + s_old_x_init_y));
	}
	else
	{
		g_new_z_l.real((s_old_x_init_x + g_long_parameter->real() - s_old_y_init_y));
		g_new_z_l.imag((s_old_y_init_x + g_long_parameter->imag() + s_old_x_init_y));
	}
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int barnsley1_orbit_fp()
{
	// Barnsley's Mandelbrot type M1 from "Fractals
	// Everywhere" by Michael Barnsley, p. 322
	// note that fast >= 287 equiv in fracsuba.asm must be kept in step

	// calculate intermediate products
	s_old_x_init_x_fp = g_old_z.real()*g_float_parameter->real();
	s_old_y_init_y_fp = g_old_z.imag()*g_float_parameter->imag();
	s_old_x_init_y_fp = g_old_z.real()*g_float_parameter->imag();
	s_old_y_init_x_fp = g_old_z.imag()*g_float_parameter->real();
	// orbit calculation
	if (g_old_z.real() >= 0)
	{
		g_new_z.real((s_old_x_init_x_fp - g_float_parameter->real() - s_old_y_init_y_fp));
		g_new_z.imag((s_old_y_init_x_fp - g_float_parameter->imag() + s_old_x_init_y_fp));
	}
	else
	{
		g_new_z.real((s_old_x_init_x_fp + g_float_parameter->real() - s_old_y_init_y_fp));
		g_new_z.imag((s_old_y_init_x_fp + g_float_parameter->imag() + s_old_x_init_y_fp));
	}
	return g_externs.BailOutFp();
}

int barnsley2_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// An unnamed Mandelbrot/Julia function from "Fractals
	// Everywhere" by Michael Barnsley, p. 331, example 4.2
	// note that fast >= 287 equiv in fracsuba.asm must be kept in step

	// calculate intermediate products
	s_old_x_init_x = multiply(g_old_z_l.real(), g_long_parameter->real(), g_bit_shift);
	s_old_y_init_y = multiply(g_old_z_l.imag(), g_long_parameter->imag(), g_bit_shift);
	s_old_x_init_y = multiply(g_old_z_l.real(), g_long_parameter->imag(), g_bit_shift);
	s_old_y_init_x = multiply(g_old_z_l.imag(), g_long_parameter->real(), g_bit_shift);

	// orbit calculation
	if (s_old_x_init_y + s_old_y_init_x >= 0)
	{
		g_new_z_l.real(s_old_x_init_x - g_long_parameter->real() - s_old_y_init_y);
		g_new_z_l.imag(s_old_y_init_x - g_long_parameter->imag() + s_old_x_init_y);
	}
	else
	{
		g_new_z_l.real(s_old_x_init_x + g_long_parameter->real() - s_old_y_init_y);
		g_new_z_l.imag(s_old_y_init_x + g_long_parameter->imag() + s_old_x_init_y);
	}
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int barnsley2_orbit_fp()
{
	// An unnamed Mandelbrot/Julia function from "Fractals
	// Everywhere" by Michael Barnsley, p. 331, example 4.2

	// calculate intermediate products
	s_old_x_init_x_fp = g_old_z.real()*g_float_parameter->real();
	s_old_y_init_y_fp = g_old_z.imag()*g_float_parameter->imag();
	s_old_x_init_y_fp = g_old_z.real()*g_float_parameter->imag();
	s_old_y_init_x_fp = g_old_z.imag()*g_float_parameter->real();

	// orbit calculation
	if (s_old_x_init_y_fp + s_old_y_init_x_fp >= 0)
	{
		g_new_z.real(s_old_x_init_x_fp - g_float_parameter->real() - s_old_y_init_y_fp);
		g_new_z.imag(s_old_y_init_x_fp - g_float_parameter->imag() + s_old_x_init_y_fp);
	}
	else
	{
		g_new_z.real(s_old_x_init_x_fp + g_float_parameter->real() - s_old_y_init_y_fp);
		g_new_z.imag(s_old_y_init_x_fp + g_float_parameter->imag() + s_old_x_init_y_fp);
	}
	return g_externs.BailOutFp();
}

int julia_orbit()
{
	// used for C prototype of fast integer math routines for classic
	// Mandelbrot and Julia
	g_new_z_l.real(g_temp_sqr_l.real() - g_temp_sqr_l.imag() + g_long_parameter->real());
	g_new_z_l.imag(multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift_minus_1) + g_long_parameter->imag());
	return g_externs.BailOutL();
}

int julia_orbit_fp()
{
	// floating point version of classical Mandelbrot/Julia
	// note that fast >= 287 equiv in fracsuba.asm must be kept in step
	g_new_z.real(g_temp_sqr.real() - g_temp_sqr.imag() + g_float_parameter->real());
	g_new_z.imag(2.0*g_old_z.real()*g_old_z.imag() + g_float_parameter->imag());
	return g_externs.BailOutFp();
}

int lambda_orbit_fp()
{
	// variation of classical Mandelbrot/Julia
	// note that fast >= 287 equiv in fracsuba.asm must be kept in step

	g_temp_sqr.real(g_old_z.real() - g_temp_sqr.real() + g_temp_sqr.imag());
	g_temp_sqr.imag(-g_old_z.imag()*g_old_z.real());
	g_temp_sqr.imag(g_temp_sqr.imag() + g_temp_sqr.imag() + g_old_z.imag());

	g_new_z.real(g_float_parameter->real()*g_temp_sqr.real() - g_float_parameter->imag()*g_temp_sqr.imag());
	g_new_z.imag(g_float_parameter->real()*g_temp_sqr.imag() + g_float_parameter->imag()*g_temp_sqr.real());
	return g_externs.BailOutFp();
}

int lambda_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// variation of classical Mandelbrot/Julia

	// in complex math) temp = Z*(1-Z)
	g_temp_sqr_l.real(g_old_z_l.real() - g_temp_sqr_l.real() + g_temp_sqr_l.imag());
	g_temp_sqr_l.imag(g_old_z_l.imag() - multiply(g_old_z_l.imag(), g_old_z_l.real(), g_bit_shift_minus_1));
	// (in complex math) Z = Lambda*Z
	g_new_z_l.real(multiply(g_long_parameter->real(), g_temp_sqr_l.real(), g_bit_shift)
		- multiply(g_long_parameter->imag(), g_temp_sqr_l.imag(), g_bit_shift));
	g_new_z_l.imag(multiply(g_long_parameter->real(), g_temp_sqr_l.imag(), g_bit_shift)
		+ multiply(g_long_parameter->imag(), g_temp_sqr_l.real(), g_bit_shift));
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int sierpinski_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// following code translated from basic - see "Fractals
	// Everywhere" by Michael Barnsley, p. 251, Program 7.1.1
	g_new_z_l.real((g_old_z_l.real() << 1));				// new.real() = 2*old.real()
	g_new_z_l.imag((g_old_z_l.imag() << 1));				// new.imag() = 2*old.imag()
	if (g_old_z_l.imag() > g_temp_z_l.imag())				// if old.imag() > .5
	{
		g_new_z_l.imag(g_new_z_l.imag() - g_temp_z_l.real()); // new.imag() = 2*old.imag() - 1
	}
	else if (g_old_z_l.real() > g_temp_z_l.imag())			// if old.real() > .5
	{
		g_new_z_l.real(g_new_z_l.real() - g_temp_z_l.real()); // new.real() = 2*old.real() - 1
	}
	// end barnsley code
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int sierpinski_orbit_fp()
{
	// following code translated from basic - see "Fractals
	// Everywhere" by Michael Barnsley, p. 251, Program 7.1.1

	g_new_z.real(g_old_z.real() + g_old_z.real());
	g_new_z.imag(g_old_z.imag() + g_old_z.imag());
	if (g_old_z.imag() > .5)
	{
		g_new_z.imag(g_new_z.imag() - 1);
	}
	else if (g_old_z.real() > .5)
	{
		g_new_z.real(g_new_z.real() - 1);
	}

	// end barnsley code
	return g_externs.BailOutFp();
}

int lambda_exponent_orbit_fp()
{
	// found this in  "Science of Fractal Images"
	if ((fabs(g_old_z.imag()) >= 1.0e3)
		|| (fabs(g_old_z.real()) >= 8))
	{
		return 1;
	}
	FPUsincos(g_old_z.imag(), &s_sin_y, &s_cos_y);

	if (g_old_z.real() >= g_rq_limit && s_cos_y >= 0.0)
	{
		return 1;
	}
	s_temp_exp = exp(g_old_z.real());
	g_temp_z.real(s_temp_exp*s_cos_y);
	g_temp_z.imag(s_temp_exp*s_sin_y);

	// multiply by lamda
	g_new_z.real(g_float_parameter->real()*g_temp_z.real() - g_float_parameter->imag()*g_temp_z.imag());
	g_new_z.imag(g_float_parameter->imag()*g_temp_z.real() + g_float_parameter->real()*g_temp_z.imag());
	g_old_z = g_new_z;
	return 0;
}

int lambda_exponent_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	long tmp;
	// found this in  "Science of Fractal Images"
	assert((1000L << g_bit_shift) == DoubleToFudge(1000.0));
	assert((8L << g_bit_shift) == DoubleToFudge(8.0));
	if ((labs(g_old_z_l.imag()) >= (1000L << g_bit_shift))
		|| (labs(g_old_z_l.real()) >= (8L << g_bit_shift)))
	{
		return 1;
	}

	SinCos086(g_old_z_l.imag(), &s_sin_y_l,  &s_cos_y_l);

	if (g_old_z_l.real() >= g_rq_limit_l && s_cos_y_l >= 0L)
	{
		return 1;
	}
	tmp = Exp086(g_old_z_l.real());

	g_temp_z_l.real(multiply(tmp,      s_cos_y_l,   g_bit_shift));
	g_temp_z_l.imag(multiply(tmp,      s_sin_y_l,   g_bit_shift));

	g_new_z_l.real(multiply(g_long_parameter->real(), g_temp_z_l.real(), g_bit_shift)
			- multiply(g_long_parameter->imag(), g_temp_z_l.imag(), g_bit_shift));
	g_new_z_l.imag(multiply(g_long_parameter->real(), g_temp_z_l.imag(), g_bit_shift)
			+ multiply(g_long_parameter->imag(), g_temp_z_l.real(), g_bit_shift));
	g_old_z_l = g_new_z_l;
	return 0;
#else
	return 0;
#endif
}

int trig_plus_exponent_orbit_fp()
{
	// another Scientific American biomorph type
	// z(n + 1) = e**z(n) + trig(z(n)) + C

	if (fabs(g_old_z.real()) >= 6.4e2) // DOMAIN errors
	{
		return 1;
	}
	s_temp_exp = exp(g_old_z.real());
	FPUsincos(g_old_z.imag(), &s_sin_y, &s_cos_y);
	CMPLXtrig0(g_old_z, g_new_z);

	// new =   trig(old) + e**old + C
	g_new_z.real(g_new_z.real() + s_temp_exp*s_cos_y + g_float_parameter->real());
	g_new_z.imag(g_new_z.imag() + s_temp_exp*s_sin_y + g_float_parameter->imag());
	return g_externs.BailOutFp();
}

int trig_plus_exponent_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// calculate exp(z)
	long tmp;

	// domain check for fast transcendental functions
	if ((labs(g_old_z_l.real()) > TRIG_LIMIT_16)
		|| (labs(g_old_z_l.imag()) > TRIG_LIMIT_16))
	{
		return 1;
	}

	tmp = Exp086(g_old_z_l.real());
	SinCos086  (g_old_z_l.imag(), &s_sin_y_l,  &s_cos_y_l);
	LCMPLXtrig0(g_old_z_l, g_new_z_l);
	g_new_z_l.real(g_new_z_l.real() + multiply(tmp, s_cos_y_l, g_bit_shift) + g_long_parameter->real());
	g_new_z_l.imag(g_new_z_l.imag() + multiply(tmp, s_sin_y_l, g_bit_shift) + g_long_parameter->imag());
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int marks_lambda_orbit()
{
	// Mark Peterson's variation of "lambda" function

	// Z1 = (C^(exp-1)*Z**2) + C
#if !defined(NO_FIXED_POINT_MATH)
	g_temp_z_l.real(g_temp_sqr_l.real() - g_temp_sqr_l.imag());
	g_temp_z_l.imag(multiply(g_old_z_l.real() , g_old_z_l.imag() , g_bit_shift_minus_1));

	g_new_z_l.real(multiply(g_coefficient_l.real(), g_temp_z_l.real(), g_bit_shift)
		- multiply(g_coefficient_l.imag(), g_temp_z_l.imag(), g_bit_shift) + g_long_parameter->real());
	g_new_z_l.imag(multiply(g_coefficient_l.real(), g_temp_z_l.imag(), g_bit_shift)
		+ multiply(g_coefficient_l.imag(), g_temp_z_l.real(), g_bit_shift) + g_long_parameter->imag());

	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int marks_lambda_orbit_fp()
{
	// Mark Peterson's variation of "lambda" function

	// Z1 = (C^(exp-1)*Z**2) + C
	g_temp_z.real(g_temp_sqr.real() - g_temp_sqr.imag());
	g_temp_z.imag(g_old_z.real()*g_old_z.imag()*2);

	g_new_z.real(g_coefficient.real()*g_temp_z.real() - g_coefficient.imag()*g_temp_z.imag() + g_float_parameter->real());
	g_new_z.imag(g_coefficient.real()*g_temp_z.imag() + g_coefficient.imag()*g_temp_z.real() + g_float_parameter->imag());

	return g_externs.BailOutFp();
}

int unity_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// brought to you by Mark Peterson - you won't find this in any fractal
	// books unless they saw it here first - Mark invented it!
	long xx_one = multiply(g_old_z_l.real(), g_old_z_l.real(), g_bit_shift) + multiply(g_old_z_l.imag(), g_old_z_l.imag(), g_bit_shift);
	if ((xx_one > g_two_fudge) || (labs(xx_one - g_one_fudge) < g_delta_min))
	{
		return 1;
	}
	g_old_z_l.imag(multiply(g_two_fudge - xx_one, g_old_z_l.real(), g_bit_shift));
	g_old_z_l.real(multiply(g_two_fudge - xx_one, g_old_z_l.imag(), g_bit_shift));
	g_new_z_l = g_old_z_l;  // TW added this line
	return 0;
#else
	return 0;
#endif
}

int unity_orbit_fp()
{
	// brought to you by Mark Peterson - you won't find this in any fractal
	// books unless they saw it here first - Mark invented it!
	double xx_one = sqr(g_old_z.real()) + sqr(g_old_z.imag());
	if ((xx_one > 2.0) || (fabs(xx_one - 1.0) < g_delta_min_fp))
	{
		return 1;
	}
	g_old_z.imag((2.0 - xx_one)* g_old_z.real());
	g_old_z.real((2.0 - xx_one)* g_old_z.imag());
	g_new_z = g_old_z;  // TW added this line
	return 0;
}

int mandel4_orbit()
{
	// By writing this code, Bert has left behind the excuse "don't
	// know what a fractal is, just know how to make'em go fast".
	// Bert is hereby declared a bonafide fractal expert! Supposedly
	// this routine calculates the Mandelbrot/Julia set based on the
	// polynomial z**4 + lambda, but I wouldn't know -- can't follow
	// all that integer math speedup stuff - Tim

	// first, compute (x + iy)**2
#if !defined(NO_FIXED_POINT_MATH)
	g_new_z_l.real(g_temp_sqr_l.real() - g_temp_sqr_l.imag());
	g_new_z_l.imag(multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift_minus_1));
	if (g_externs.BailOutL())
	{
		return 1;
	}

	// then, compute ((x + iy)**2)**2 + lambda
	g_new_z_l.real(g_temp_sqr_l.real() - g_temp_sqr_l.imag() + g_long_parameter->real());
	g_new_z_l.imag(multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift_minus_1) + g_long_parameter->imag());
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int mandel4_orbit_fp()
{
	// first, compute (x + iy)**2
	g_new_z.real(g_temp_sqr.real() - g_temp_sqr.imag());
	g_new_z.imag(g_old_z.real()*g_old_z.imag()*2);
	if (g_externs.BailOutFp())
	{
		return 1;
	}

	// then, compute ((x + iy)**2)**2 + lambda
	g_new_z.real(g_temp_sqr.real() - g_temp_sqr.imag() + g_float_parameter->real());
	g_new_z.imag( g_old_z.real()*g_old_z.imag()*2 + g_float_parameter->imag());
	return g_externs.BailOutFp();
}

int z_to_z_plus_z_orbit_fp()
{
	complex_power(&g_old_z, int(g_parameters[P2_REAL]), &g_new_z);
	g_old_z = ComplexPower(g_old_z, g_old_z);
	g_new_z.real(g_new_z.real() + g_old_z.real() +g_float_parameter->real());
	g_new_z.imag(g_new_z.imag() + g_old_z.imag() +g_float_parameter->imag());
	return g_externs.BailOutFp();
}

int z_power_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	if (complex_power_l(&g_old_z_l, g_c_exp, &g_new_z_l, g_bit_shift))
	{
		g_new_z_l.real(g_new_z_l.imag(8L << g_bit_shift));
	}
	g_new_z_l.real(g_new_z_l.real() + g_long_parameter->real());
	g_new_z_l.imag(g_new_z_l.imag() + g_long_parameter->imag());
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int complex_z_power_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	ComplexD x = ComplexFudgeToDouble(g_old_z_l);
	ComplexD y = ComplexFudgeToDouble(g_parameter2_l);
	x = ComplexPower(x, y);
	if (fabs(x.real()) < g_fudge_limit && fabs(x.imag()) < g_fudge_limit)
	{
		g_new_z_l = ComplexDoubleToFudge(x);
	}
	else
	{
		g_overflow = true;
	}
	g_new_z_l.real(g_new_z_l.real() + g_long_parameter->real());
	g_new_z_l.imag(g_new_z_l.imag() + g_long_parameter->imag());
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int z_power_orbit_fp()
{
	complex_power(&g_old_z, g_c_exp, &g_new_z);
	g_new_z.real(g_new_z.real() + g_float_parameter->real());
	g_new_z.imag(g_new_z.imag() + g_float_parameter->imag());
	return g_externs.BailOutFp();
}

int complex_z_power_orbit_fp()
{
	g_new_z = ComplexPower(g_old_z, g_parameter2);
	g_new_z.real(g_new_z.real() + g_float_parameter->real());
	g_new_z.imag(g_new_z.imag() + g_float_parameter->imag());
	return g_externs.BailOutFp();
}

int barnsley3_orbit()
{
	// An unnamed Mandelbrot/Julia function from "Fractals
	// Everywhere" by Michael Barnsley, p. 292, example 4.1

	// calculate intermediate products
#if !defined(NO_FIXED_POINT_MATH)
	s_old_x_init_x = multiply(g_old_z_l.real(), g_old_z_l.real(), g_bit_shift);
	s_old_y_init_y = multiply(g_old_z_l.imag(), g_old_z_l.imag(), g_bit_shift);
	s_old_x_init_y = multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift);

	// orbit calculation
	if (g_old_z_l.real() > 0)
	{
		g_new_z_l.real(s_old_x_init_x   - s_old_y_init_y - g_externs.Fudge());
		g_new_z_l.imag(s_old_x_init_y << 1);
	}
	else
	{
		g_new_z_l.real(s_old_x_init_x - s_old_y_init_y - g_externs.Fudge()
			+ multiply(g_long_parameter->real(), g_old_z_l.real(), g_bit_shift));
		g_new_z_l.imag(s_old_x_init_y <<1);

		// This term added by Tim Wegner to make dependent on the
		// imaginary part of the parameter. (Otherwise Mandelbrot
		// is uninteresting.
		g_new_z_l.imag(g_new_z_l.imag() + multiply(g_long_parameter->imag(), g_old_z_l.real(), g_bit_shift));
	}
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

//
//	An unnamed Mandelbrot/Julia function from "Fractals
//	Everywhere" by Michael Barnsley, p. 292, example 4.1
//
int barnsley3_orbit_fp()
{
	// calculate intermediate products
	s_old_x_init_x_fp = g_old_z.real()*g_old_z.real();
	s_old_y_init_y_fp = g_old_z.imag()*g_old_z.imag();
	s_old_x_init_y_fp = g_old_z.real()*g_old_z.imag();

	// orbit calculation
	if (g_old_z.real() > 0)
	{
		g_new_z.real(s_old_x_init_x_fp - s_old_y_init_y_fp - 1.0);
		g_new_z.imag(s_old_x_init_y_fp*2);
	}
	else
	{
		g_new_z.real(s_old_x_init_x_fp - s_old_y_init_y_fp -1.0 + g_float_parameter->real()*g_old_z.real());
		g_new_z.imag(s_old_x_init_y_fp*2);

		// This term added by Tim Wegner to make dependent on the
		// imaginary part of the parameter. (Otherwise Mandelbrot
		// is uninteresting.
		g_new_z.imag(g_new_z.imag() + g_float_parameter->imag()*g_old_z.real());
	}
	return g_externs.BailOutFp();
}

// From Scientific American, July 1989
// A Biomorph
// z(n + 1) = trig(z(n)) + z(n)**2 + C
int trig_plus_z_squared_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	LCMPLXtrig0(g_old_z_l, g_new_z_l);
	g_new_z_l.real(g_new_z_l.real() + g_temp_sqr_l.real() - g_temp_sqr_l.imag() + g_long_parameter->real());
	g_new_z_l.imag(g_new_z_l.imag() + multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift_minus_1) + g_long_parameter->imag());
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

// From Scientific American, July 1989
// A Biomorph
// z(n + 1) = trig(z(n)) + z(n)**2 + C
int trig_plus_z_squared_orbit_fp()
{
	CMPLXtrig0(g_old_z, g_new_z);
	g_new_z.real(g_new_z.real() + g_temp_sqr.real() - g_temp_sqr.imag() + g_float_parameter->real());
	g_new_z.imag(g_new_z.imag() + 2.0*g_old_z.real()*g_old_z.imag() + g_float_parameter->imag());
	return g_externs.BailOutFp();
}

// Richard8 {c = z = pixel: z = sin(z) + sin(pixel), |z| <= 50}
int richard8_orbit_fp()
{
	CMPLXtrig0(g_old_z, g_new_z);
	// CMPLXtrig1(*g_float_parameter, g_temp_z);
	g_new_z.real(g_new_z.real() + g_temp_z.real());
	g_new_z.imag(g_new_z.imag() + g_temp_z.imag());
	return g_externs.BailOutFp();
}

// Richard8 {c = z = pixel: z = sin(z) + sin(pixel), |z| <= 50}
int richard8_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	LCMPLXtrig0(g_old_z_l, g_new_z_l);
	// LCMPLXtrig1(*g_long_parameter, g_temp_z_l);
	g_new_z_l.real(g_new_z_l.real() + g_temp_z_l.real());
	g_new_z_l.imag(g_new_z_l.imag() + g_temp_z_l.imag());
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int popcorn_old_orbit_fp()
{
	g_temp_z = g_old_z;
	g_temp_z.real(g_temp_z.real()*3.0);
	g_temp_z.imag(g_temp_z.imag()*3.0);
	FPUsincos(g_temp_z.real(), &g_sin_x, &g_cos_x);
	FPUsincos(g_temp_z.imag(), &s_sin_y, &s_cos_y);
	g_temp_z.real(g_sin_x/g_cos_x + g_old_z.real());
	g_temp_z.imag(s_sin_y/s_cos_y + g_old_z.imag());
	FPUsincos(g_temp_z.real(), &g_sin_x, &g_cos_x);
	FPUsincos(g_temp_z.imag(), &s_sin_y, &s_cos_y);
	g_new_z.real(g_old_z.real() - g_parameter.real()*s_sin_y);
	g_new_z.imag(g_old_z.imag() - g_parameter.real()*g_sin_x);
	if (g_plot_color == plot_color_none)
	{
		plot_orbit(g_new_z.real(), g_new_z.imag(), 1 + g_row % g_colors);
		g_old_z = g_new_z;
	}
	else
	{
		// FLOATBAILOUT();
		// PB The above line was weird, not what it seems to be!  But, bracketing
		// it or always doing it (either of which seem more likely to be what
		// was intended) changes the image for the worse, so I'm not touching it.
		// Same applies to int form in next routine.
		// PB later: recoded inline, still leaving it weird
		g_temp_sqr.real(sqr(g_new_z.real()));
	}
	g_temp_sqr.imag(sqr(g_new_z.imag()));
	g_magnitude = g_temp_sqr.real() + g_temp_sqr.imag();
	if (g_magnitude >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int popcorn_orbit_fp()
{
	g_temp_z = g_old_z;
	g_temp_z.real(g_temp_z.real()*3.0);
	g_temp_z.imag(g_temp_z.imag()*3.0);
	FPUsincos(g_temp_z.real(), &g_sin_x, &g_cos_x);
	FPUsincos(g_temp_z.imag(), &s_sin_y, &s_cos_y);
	g_temp_z.real(g_sin_x/g_cos_x + g_old_z.real());
	g_temp_z.imag(s_sin_y/s_cos_y + g_old_z.imag());
	FPUsincos(g_temp_z.real(), &g_sin_x, &g_cos_x);
	FPUsincos(g_temp_z.imag(), &s_sin_y, &s_cos_y);
	g_new_z.real(g_old_z.real() - g_parameter.real()*s_sin_y);
	g_new_z.imag(g_old_z.imag() - g_parameter.real()*g_sin_x);
	//
	// g_new_z.real(g_old_z.real() - g_parameter.real()*sin(g_old_z.imag() + tan(3*g_old_z.imag())));
	// g_new_z.imag(g_old_z.imag() - g_parameter.real()*sin(g_old_z.real() + tan(3*g_old_z.real())));
	//
	if (g_plot_color == plot_color_none)
	{
		plot_orbit(g_new_z.real(), g_new_z.imag(), 1 + g_row % g_colors);
		g_old_z = g_new_z;
	}
	// else
	// FLOATBAILOUT();
	// PB The above line was weird, not what it seems to be!  But, bracketing
	//		it or always doing it (either of which seem more likely to be what
	//		was intended) changes the image for the worse, so I'm not touching it.
	//		Same applies to int form in next routine.
	// PB later: recoded inline, still leaving it weird
	// JCO: sqr's should always be done, else magnitude could be wrong
	g_temp_sqr.real(sqr(g_new_z.real()));
	g_temp_sqr.imag(sqr(g_new_z.imag()));
	g_magnitude = g_temp_sqr.real() + g_temp_sqr.imag();
	if (g_magnitude >= g_rq_limit || fabs(g_new_z.real()) > g_rq_limit2 || fabs(g_new_z.imag()) > g_rq_limit2)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int popcorn_old_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	g_temp_z_l = g_old_z_l;
	g_temp_z_l.real(g_temp_z_l.real()*3L);
	g_temp_z_l.imag(g_temp_z_l.imag()*3L);
	g_temp_z_l.real(TRIG_ARG_L(g_temp_z_l.real()));
	g_temp_z_l.imag(TRIG_ARG_L(g_temp_z_l.imag()));
	SinCos086(g_temp_z_l.real(), &s_sin_x_l, &s_cos_x_l);
	SinCos086(g_temp_z_l.imag(), &s_sin_y_l, &s_cos_y_l);
	g_temp_z_l.real(divide(s_sin_x_l, s_cos_x_l, g_bit_shift) + g_old_z_l.real());
	g_temp_z_l.imag(divide(s_sin_y_l, s_cos_y_l, g_bit_shift) + g_old_z_l.imag());
	g_temp_z_l.real(TRIG_ARG_L(g_temp_z_l.real()));
	g_temp_z_l.imag(TRIG_ARG_L(g_temp_z_l.imag()));
	SinCos086(g_temp_z_l.real(), &s_sin_x_l, &s_cos_x_l);
	SinCos086(g_temp_z_l.imag(), &s_sin_y_l, &s_cos_y_l);
	g_new_z_l.real(g_old_z_l.real() - multiply(g_parameter_l.real(), s_sin_y_l, g_bit_shift));
	g_new_z_l.imag(g_old_z_l.imag() - multiply(g_parameter_l.real(), s_sin_x_l, g_bit_shift));
	if (g_plot_color == plot_color_none)
	{
		plot_orbit_i(g_new_z_l.real(), g_new_z_l.imag(), 1 + g_row % g_colors);
		g_old_z_l = g_new_z_l;
	}
	else
	{
		// LONGBAILOUT();
		// PB above still the old way, is weird, see notes in FP popcorn case
		g_temp_sqr_l.real(lsqr(g_new_z_l.real()));
		g_temp_sqr_l.imag(lsqr(g_new_z_l.imag()));
	}
	g_magnitude_l = g_temp_sqr_l.real() + g_temp_sqr_l.imag();
	if (g_magnitude_l >= g_rq_limit_l || g_magnitude_l < 0 || labs(g_new_z_l.real()) > g_rq_limit2_l
			|| labs(g_new_z_l.imag()) > g_rq_limit2_l)
					return 1;
	g_old_z_l = g_new_z_l;
	return 0;
#else
	return 0;
#endif
}

int popcorn_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	g_temp_z_l = g_old_z_l;
	g_temp_z_l.real(g_temp_z_l.real()*3L);
	g_temp_z_l.imag(g_temp_z_l.imag()*3L);
	g_temp_z_l.real(TRIG_ARG_L(g_temp_z_l.real()));
	g_temp_z_l.imag(TRIG_ARG_L(g_temp_z_l.imag()));
	SinCos086(g_temp_z_l.real(), &s_sin_x_l, &s_cos_x_l);
	SinCos086(g_temp_z_l.imag(), &s_sin_y_l, &s_cos_y_l);
	g_temp_z_l.real(divide(s_sin_x_l, s_cos_x_l, g_bit_shift) + g_old_z_l.real());
	g_temp_z_l.imag(divide(s_sin_y_l, s_cos_y_l, g_bit_shift) + g_old_z_l.imag());
	g_temp_z_l.real(TRIG_ARG_L(g_temp_z_l.real()));
	g_temp_z_l.imag(TRIG_ARG_L(g_temp_z_l.imag()));
	SinCos086(g_temp_z_l.real(), &s_sin_x_l, &s_cos_x_l);
	SinCos086(g_temp_z_l.imag(), &s_sin_y_l, &s_cos_y_l);
	g_new_z_l.real(g_old_z_l.real() - multiply(g_parameter_l.real(), s_sin_y_l, g_bit_shift));
	g_new_z_l.imag(g_old_z_l.imag() - multiply(g_parameter_l.real(), s_sin_x_l, g_bit_shift));
	if (g_plot_color == plot_color_none)
	{
		plot_orbit_i(g_new_z_l.real(), g_new_z_l.imag(), 1 + g_row % g_colors);
		g_old_z_l = g_new_z_l;
	}
	// else
	// JCO: sqr's should always be done, else magnitude could be wrong
	g_temp_sqr_l.real(lsqr(g_new_z_l.real()));
	g_temp_sqr_l.imag(lsqr(g_new_z_l.imag()));
	g_magnitude_l = g_temp_sqr_l.real() + g_temp_sqr_l.imag();
	if (g_magnitude_l >= g_rq_limit_l
		|| g_magnitude_l < 0
		|| labs(g_new_z_l.real()) > g_rq_limit2_l
		|| labs(g_new_z_l.imag()) > g_rq_limit2_l)
	{
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
#else
	return 0;
#endif
}

// Popcorn generalization proposed by HB

int popcorn_fn_orbit_fp()
{
	ComplexD tmpx;
	ComplexD tmpy;

	// tmpx contains the generalized value of the old real "x" equation
	CMPLXtimesreal(g_parameter2, g_old_z.imag(), g_temp_z);  // tmp = (C*old.imag())
	CMPLXtrig1(g_temp_z, tmpx);             // tmpx = trig1(tmp)
	tmpx.real(tmpx.real() + g_old_z.imag());                  // tmpx = old.imag() + trig1(tmp)
	CMPLXtrig0(tmpx, g_temp_z);             // tmp = trig0(tmpx)
	CMPLXmult(g_temp_z, g_parameter, tmpx);         // tmpx = tmp*h

	// tmpy contains the generalized value of the old real "y" equation
	CMPLXtimesreal(g_parameter2, g_old_z.real(), g_temp_z);  // tmp = (C*old.real())
	CMPLXtrig3(g_temp_z, tmpy);             // tmpy = trig3(tmp)
	tmpy.real(tmpy.real() + g_old_z.real());                  // tmpy = old.real() + trig1(tmp)
	CMPLXtrig2(tmpy, g_temp_z);             // tmp = trig2(tmpy)

	CMPLXmult(g_temp_z, g_parameter, tmpy);         // tmpy = tmp*h

	g_new_z.real(g_old_z.real() - tmpx.real() - tmpy.imag());
	g_new_z.imag(g_old_z.imag() - tmpy.real() - tmpx.imag());

	if (g_plot_color == plot_color_none)
	{
		plot_orbit(g_new_z.real(), g_new_z.imag(), 1 + g_row % g_colors);
		g_old_z = g_new_z;
	}

	g_temp_sqr.real(sqr(g_new_z.real()));
	g_temp_sqr.imag(sqr(g_new_z.imag()));
	g_magnitude = g_temp_sqr.real() + g_temp_sqr.imag();
	if (g_magnitude >= g_rq_limit
		|| fabs(g_new_z.real()) > g_rq_limit2 || fabs(g_new_z.imag()) > g_rq_limit2)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

inline void fix_overflow(ComplexL &arg)
{
	if (g_overflow)
	{
		arg.real(g_externs.Fudge());
		arg.imag(0);
		g_overflow = false;
	}
}

int popcorn_fn_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	ComplexL ltmpx, ltmpy;

	g_overflow = false;

	// ltmpx contains the generalized value of the old real "x" equation
	LCMPLXtimesreal(g_parameter2_l, g_old_z_l.imag(), g_temp_z_l); // tmp = (C*old.imag())
	LCMPLXtrig1(g_temp_z_l, ltmpx);             // tmpx = trig1(tmp)
	fix_overflow(ltmpx);
	ltmpx.real(ltmpx.real() + g_old_z_l.imag());                   // tmpx = old.imag() + trig1(tmp)
	LCMPLXtrig0(ltmpx, g_temp_z_l);             // tmp = trig0(tmpx)
	fix_overflow(g_temp_z_l);
	LCMPLXmult(g_temp_z_l, g_parameter_l, ltmpx);        // tmpx = tmp*h

	// ltmpy contains the generalized value of the old real "y" equation
	LCMPLXtimesreal(g_parameter2_l, g_old_z_l.real(), g_temp_z_l); // tmp = (C*old.real())
	LCMPLXtrig3(g_temp_z_l, ltmpy);             // tmpy = trig3(tmp)
	fix_overflow(ltmpy);
	ltmpy.real(ltmpy.real() + g_old_z_l.real());                   // tmpy = old.real() + trig1(tmp)
	LCMPLXtrig2(ltmpy, g_temp_z_l);             // tmp = trig2(tmpy)
	fix_overflow(g_temp_z_l);
	LCMPLXmult(g_temp_z_l, g_parameter_l, ltmpy);        // tmpy = tmp*h

	g_new_z_l.real(g_old_z_l.real() - ltmpx.real() - ltmpy.imag());
	g_new_z_l.imag(g_old_z_l.imag() - ltmpy.real() - ltmpx.imag());

	if (g_plot_color == plot_color_none)
	{
		plot_orbit_i(g_new_z_l.real(), g_new_z_l.imag(), 1 + g_row % g_colors);
		g_old_z_l = g_new_z_l;
	}
	g_temp_sqr_l.real(lsqr(g_new_z_l.real()));
	g_temp_sqr_l.imag(lsqr(g_new_z_l.imag()));
	g_magnitude_l = g_temp_sqr_l.real() + g_temp_sqr_l.imag();
	if (g_magnitude_l >= g_rq_limit_l || g_magnitude_l < 0
		|| labs(g_new_z_l.real()) > g_rq_limit2_l
		|| labs(g_new_z_l.imag()) > g_rq_limit2_l)
	{
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
#else
	return 0;
#endif
}

inline ComplexD operator*(ComplexD const &left, StdComplexD const &right)
{
	ComplexD temp;
	temp.real(right.real());
	temp.imag(right.imag());
	ComplexD result;
	FPUcplxmul(&left, &temp, &result);
	return result;
}

int marks_complex_mandelbrot_orbit()
{
	g_temp_z.real(g_temp_sqr.real() - g_temp_sqr.imag());
	g_temp_z.imag(2*g_old_z.real()*g_old_z.imag());
	g_new_z = g_temp_z*g_coefficient;
	g_new_z.real(g_new_z.real() + g_float_parameter->real());
	g_new_z.imag(g_new_z.imag() + g_float_parameter->imag());
	return g_externs.BailOutFp();
}

int spider_orbit_fp()
{
	// Spider(XAXIS) { c = z=pixel: z = z*z + c; c = c/2 + z, |z| <= 4 }
	g_new_z.real(g_temp_sqr.real() - g_temp_sqr.imag() + g_temp_z.real());
	g_new_z.imag(2*g_old_z.real()*g_old_z.imag() + g_temp_z.imag());
	g_temp_z.real(g_temp_z.real()/2 + g_new_z.real());
	g_temp_z.imag(g_temp_z.imag()/2 + g_new_z.imag());
	return g_externs.BailOutFp();
}

int spider_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// Spider(XAXIS) { c = z=pixel: z = z*z + c; c = c/2 + z, |z| <= 4 }
	g_new_z_l.real(g_temp_sqr_l.real() - g_temp_sqr_l.imag() + g_temp_z_l.real());
	g_new_z_l.imag(multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift_minus_1) + g_temp_z_l.imag());
	g_temp_z_l.real((g_temp_z_l.real() >> 1) + g_new_z_l.real());
	g_temp_z_l.imag((g_temp_z_l.imag() >> 1) + g_new_z_l.imag());
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int tetrate_orbit_fp()
{
	// Tetrate(XAXIS) { c = z=pixel: z = c^z, |z| <= (P1 + 3) }
	g_new_z = ComplexPower(*g_float_parameter, g_old_z);
	return g_externs.BailOutFp();
}

int z_trig_z_plus_z_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z = (p1*z*trig(z)) + p2*z
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);					// g_temp_z_l = trig(old)
	LCMPLXmult(g_parameter_l, g_temp_z_l, g_temp_z_l);	// g_temp_z_l  = p1*trig(old)
	ComplexL temp;
	LCMPLXmult(g_old_z_l, g_temp_z_l, temp);			// temp = p1*old*trig(old)
	LCMPLXmult(g_parameter2_l, g_old_z_l, g_temp_z_l);	// g_temp_z_l  = p2*old
	LCMPLXadd(temp, g_temp_z_l, g_new_z_l);				// g_new_z_l  = temp + p2*old
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int scott_z_trig_z_plus_z_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z = (z*trig(z)) + z
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);          // g_temp_z_l  = trig(old)
	LCMPLXmult(g_old_z_l, g_temp_z_l, g_new_z_l);       // g_new_z_l  = old*trig(old)
	LCMPLXadd(g_new_z_l, g_old_z_l, g_new_z_l);        // g_new_z_l  = trig(old) + old
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int skinner_z_trig_z_minus_z_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z = (z*trig(z))-z
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);          // g_temp_z_l  = trig(old)
	LCMPLXmult(g_old_z_l, g_temp_z_l, g_new_z_l);       // g_new_z_l  = old*trig(old)
	LCMPLXsub(g_new_z_l, g_old_z_l, g_new_z_l);        // g_new_z_l  = trig(old) - old
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int z_trig_z_plus_z_orbit_fp()
{
	// z = (p1*z*trig(z)) + p2*z
	CMPLXtrig0(g_old_z, g_temp_z);          // tmp  = trig(old)
	CMPLXmult(g_parameter, g_temp_z, g_temp_z);      // tmp  = p1*trig(old)
	CMPLXmult(g_old_z, g_temp_z, s_temp2);      // s_temp2 = p1*old*trig(old)
	CMPLXmult(g_parameter2, g_old_z, g_temp_z);     // tmp  = p2*old
	CMPLXadd(s_temp2, g_temp_z, g_new_z);       // new  = p1*trig(old) + p2*old
	return g_externs.BailOutFp();
}

int scott_z_trig_z_plus_z_orbit_fp()
{
	// z = (z*trig(z)) + z
	CMPLXtrig0(g_old_z, g_temp_z);         // tmp  = trig(old)
	CMPLXmult(g_old_z, g_temp_z, g_new_z);       // new  = old*trig(old)
	CMPLXadd(g_new_z, g_old_z, g_new_z);        // new  = trig(old) + old
	return g_externs.BailOutFp();
}

int skinner_z_trig_z_minus_z_orbit_fp()
{
	// z = (z*trig(z))-z
	CMPLXtrig0(g_old_z, g_temp_z);         // tmp  = trig(old)
	CMPLXmult(g_old_z, g_temp_z, g_new_z);       // new  = old*trig(old)
	CMPLXsub(g_new_z, g_old_z, g_new_z);        // new  = trig(old) - old
	return g_externs.BailOutFp();
}

int sqr_1_over_trig_z_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z = sqr(1/trig(z))
	LCMPLXtrig0(g_old_z_l, g_old_z_l);
	LCMPLXrecip(g_old_z_l, g_old_z_l);
	LCMPLXsqr(g_old_z_l, g_new_z_l);
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int sqr_1_over_trig_z_orbit_fp()
{
	// z = sqr(1/trig(z))
	CMPLXtrig0(g_old_z, g_old_z);
	CMPLXrecip(g_old_z, g_old_z);
	CMPLXsqr(g_old_z, g_new_z);
	return g_externs.BailOutFp();
}

int trig_plus_trig_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z = trig(0, z)*p1 + trig1(z)*p2
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);
	LCMPLXmult(g_parameter_l, g_temp_z_l, g_temp_z_l);
	ComplexL temp;
	LCMPLXtrig1(g_old_z_l, temp);
	LCMPLXmult(g_parameter2_l, temp, g_old_z_l);
	LCMPLXadd(g_temp_z_l, g_old_z_l, g_new_z_l);
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int trig_plus_trig_orbit_fp()
{
	// z = trig0(z)*p1 + trig1(z)*p2
	CMPLXtrig0(g_old_z, g_temp_z);
	CMPLXmult(g_parameter, g_temp_z, g_temp_z);
	CMPLXtrig1(g_old_z, g_old_z);
	CMPLXmult(g_parameter2, g_old_z, g_old_z);
	CMPLXadd(g_temp_z, g_old_z, g_new_z);
	return g_externs.BailOutFp();
}

// The following four fractals are based on the idea of parallel
//	or alternate calculations.  The shift is made when the mod
//	reaches a given value.

int lambda_trig_or_trig_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z = trig0(z)*p1 if mod(old) < p2.real() and
	// trig1(z)*p1 if mod(old) >= p2.real()
	if ((LCMPLXmod(g_old_z_l)) < g_parameter2_l.real())
	{
		LCMPLXtrig0(g_old_z_l, g_temp_z_l);
		LCMPLXmult(*g_long_parameter, g_temp_z_l, g_new_z_l);
	}
	else
	{
		LCMPLXtrig1(g_old_z_l, g_temp_z_l);
		LCMPLXmult(*g_long_parameter, g_temp_z_l, g_new_z_l);
	}
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int lambda_trig_or_trig_orbit_fp()
{
	// z = trig0(z)*p1 if mod(old) < p2.real() and
	//		trig1(z)*p1 if mod(old) >= p2.real()
	if (CMPLXmod(g_old_z) < g_parameter2.real())
	{
		CMPLXtrig0(g_old_z, g_old_z);
		FPUcplxmul(g_float_parameter, &g_old_z, &g_new_z);
	}
	else
	{
		CMPLXtrig1(g_old_z, g_old_z);
		FPUcplxmul(g_float_parameter, &g_old_z, &g_new_z);
	}
	return g_externs.BailOutFp();
}

int julia_trig_or_trig_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z = trig0(z) + p1 if mod(old) < p2.real() and
	//		trig1(z) + p1 if mod(old) >= p2.real()
	if (LCMPLXmod(g_old_z_l) < g_parameter2_l.real())
	{
		LCMPLXtrig0(g_old_z_l, g_temp_z_l);
		LCMPLXadd(*g_long_parameter, g_temp_z_l, g_new_z_l);
	}
	else
	{
		LCMPLXtrig1(g_old_z_l, g_temp_z_l);
		LCMPLXadd(*g_long_parameter, g_temp_z_l, g_new_z_l);
	}
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int julia_trig_or_trig_orbit_fp()
{
	// z = trig0(z) + p1 if mod(old) < p2.real() and
	//		trig1(z) + p1 if mod(old) >= p2.real()
	if (CMPLXmod(g_old_z) < g_parameter2.real())
	{
		CMPLXtrig0(g_old_z, g_old_z);
		CMPLXadd(*g_float_parameter, g_old_z, g_new_z);
	}
	else
	{
		CMPLXtrig1(g_old_z, g_old_z);
		CMPLXadd(*g_float_parameter, g_old_z, g_new_z);
	}
	return g_externs.BailOutFp();
}

int phoenix_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z(n + 1) = z(n)^2 + p + qy(n),  y(n + 1) = z(n)
	g_temp_z_l.real(multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift));
	g_new_z_l.real(g_temp_sqr_l.real() - g_temp_sqr_l.imag() + g_long_parameter->real()
		+ multiply(g_long_parameter->imag(), s_temp_z2_l.real(), g_bit_shift));
	g_new_z_l.imag(g_temp_z_l.real() + g_temp_z_l.real()
		+ multiply(g_long_parameter->imag(), s_temp_z2_l.imag(), g_bit_shift));
	s_temp_z2_l = g_old_z_l; // set s_temp_z2_l to Y value
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int phoenix_orbit_fp()
{
	// z(n + 1) = z(n)^2 + p + qy(n),  y(n + 1) = z(n)
	g_temp_z.real(g_old_z.real()*g_old_z.imag());
	g_new_z.real(g_temp_sqr.real() - g_temp_sqr.imag() + g_float_parameter->real() + (g_float_parameter->imag()*s_temp2.real()));
	g_new_z.imag((g_temp_z.real() + g_temp_z.real()) + (g_float_parameter->imag()*s_temp2.imag()));
	s_temp2 = g_old_z; // set s_temp2 to Y value
	return g_externs.BailOutFp();
}

int phoenix_complex_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z(n + 1) = z(n)^2 + p + qy(n),  y(n + 1) = z(n)
	g_temp_z_l.real(multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift));
	g_new_z_l.real(g_temp_sqr_l.real() - g_temp_sqr_l.imag() + g_long_parameter->real()
		+ multiply(g_parameter2_l.real(), s_temp_z2_l.real(), g_bit_shift)
		- multiply(g_parameter2_l.imag(), s_temp_z2_l.imag(), g_bit_shift));
	g_new_z_l.imag(g_temp_z_l.real() + g_temp_z_l.real() + g_long_parameter->imag()
		+ multiply(g_parameter2_l.real(), s_temp_z2_l.imag(), g_bit_shift)
		+ multiply(g_parameter2_l.imag(), s_temp_z2_l.real(), g_bit_shift));
	s_temp_z2_l = g_old_z_l; // set s_temp_z2_l to Y value
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int phoenix_complex_orbit_fp()
{
	// z(n + 1) = z(n)^2 + p1 + p2*y(n),  y(n + 1) = z(n)
	g_temp_z.real(g_old_z.real()*g_old_z.imag());
	g_new_z.real(g_temp_sqr.real() - g_temp_sqr.imag() + g_float_parameter->real() + (g_parameter2.real()*s_temp2.real()) - (g_parameter2.imag()*s_temp2.imag()));
	g_new_z.imag((g_temp_z.real() + g_temp_z.real()) + g_float_parameter->imag() + (g_parameter2.real()*s_temp2.imag()) + (g_parameter2.imag()*s_temp2.real()));
	s_temp2 = g_old_z; // set s_temp2 to Y value
	return g_externs.BailOutFp();
}

int phoenix_plus_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z(n + 1) = z(n)^(degree-1)*(z(n) + p) + qy(n),  y(n + 1) = z(n)
	int i;
	ComplexL loldplus;
	loldplus = g_old_z_l;
	g_temp_z_l = g_old_z_l;
	for (i = 1; i < g_degree; i++)  // degree >= 2, degree = degree-1 in setup
	{
		LCMPLXmult(g_old_z_l, g_temp_z_l, g_temp_z_l); // = old^(degree-1)
	}
	loldplus.real(loldplus.real() + g_long_parameter->real());
	ComplexL lnewminus;
	LCMPLXmult(g_temp_z_l, loldplus, lnewminus);
	g_new_z_l.real(lnewminus.real() + multiply(g_long_parameter->imag(), s_temp_z2_l.real(), g_bit_shift));
	g_new_z_l.imag(lnewminus.imag() + multiply(g_long_parameter->imag(), s_temp_z2_l.imag(), g_bit_shift));
	s_temp_z2_l = g_old_z_l; // set s_temp_z2_l to Y value
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int phoenix_plus_orbit_fp()
{
	// z(n + 1) = z(n)^(degree-1)*(z(n) + p) + qy(n),  y(n + 1) = z(n)
	int i;
	ComplexD oldplus;
	ComplexD newminus;
	oldplus = g_old_z;
	g_temp_z = g_old_z;
	for (i = 1; i < g_degree; i++)  // degree >= 2, degree = degree-1 in setup
	{
		FPUcplxmul(&g_old_z, &g_temp_z, &g_temp_z); // = old^(degree-1)
	}
	oldplus.real(oldplus.real() + g_float_parameter->real());
	FPUcplxmul(&g_temp_z, &oldplus, &newminus);
	g_new_z.real(newminus.real() + (g_float_parameter->imag()*s_temp2.real()));
	g_new_z.imag(newminus.imag() + (g_float_parameter->imag()*s_temp2.imag()));
	s_temp2 = g_old_z; // set s_temp2 to Y value
	return g_externs.BailOutFp();
}

int phoenix_minus_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z(n + 1) = z(n)^(degree-2)*(z(n)^2 + p) + qy(n),  y(n + 1) = z(n)
	int i;
	ComplexL loldsqr;
	LCMPLXmult(g_old_z_l, g_old_z_l, loldsqr);
	g_temp_z_l = g_old_z_l;
	for (i = 1; i < g_degree; i++)  // degree >= 3, degree = degree-2 in setup
	{
		LCMPLXmult(g_old_z_l, g_temp_z_l, g_temp_z_l); // = old^(degree-2)
	}
	loldsqr.real(loldsqr.real() + g_long_parameter->real());
	ComplexL lnewminus;
	LCMPLXmult(g_temp_z_l, loldsqr, lnewminus);
	g_new_z_l.real(lnewminus.real() + multiply(g_long_parameter->imag(), s_temp_z2_l.real(), g_bit_shift));
	g_new_z_l.imag(lnewminus.imag() + multiply(g_long_parameter->imag(), s_temp_z2_l.imag(), g_bit_shift));
	s_temp_z2_l = g_old_z_l; // set s_temp_z2_l to Y value
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int phoenix_minus_orbit_fp()
{
	// z(n + 1) = z(n)^(degree-2)*(z(n)^2 + p) + qy(n),  y(n + 1) = z(n)
	int i;
	ComplexD oldsqr;
	ComplexD newminus;
	FPUcplxmul(&g_old_z, &g_old_z, &oldsqr);
	g_temp_z = g_old_z;
	for (i = 1; i < g_degree; i++)  // degree >= 3, degree = degree-2 in setup
	{
		FPUcplxmul(&g_old_z, &g_temp_z, &g_temp_z); // = old^(degree-2)
	}
	oldsqr.real(oldsqr.real() + g_float_parameter->real());
	FPUcplxmul(&g_temp_z, &oldsqr, &newminus);
	g_new_z.real(newminus.real() + (g_float_parameter->imag()*s_temp2.real()));
	g_new_z.imag(newminus.imag() + (g_float_parameter->imag()*s_temp2.imag()));
	s_temp2 = g_old_z; // set s_temp2 to Y value
	return g_externs.BailOutFp();
}

int phoenix_complex_plus_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z(n + 1) = z(n)^(degree-1)*(z(n) + p) + qy(n),  y(n + 1) = z(n)
	int i;
	ComplexL loldplus;
	loldplus = g_old_z_l;
	g_temp_z_l = g_old_z_l;
	for (i = 1; i < g_degree; i++)  // degree >= 2, degree = degree-1 in setup
	{
		LCMPLXmult(g_old_z_l, g_temp_z_l, g_temp_z_l); // = old^(degree-1)
	}
	loldplus.real(loldplus.real() + g_long_parameter->real());
	loldplus.imag(loldplus.imag() + g_long_parameter->imag());
	ComplexL lnewminus;
	LCMPLXmult(g_temp_z_l, loldplus, lnewminus);
	LCMPLXmult(g_parameter2_l, s_temp_z2_l, g_temp_z_l);
	g_new_z_l.real(lnewminus.real() + g_temp_z_l.real());
	g_new_z_l.imag(lnewminus.imag() + g_temp_z_l.imag());
	s_temp_z2_l = g_old_z_l; // set s_temp_z2_l to Y value
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int phoenix_complex_plus_orbit_fp()
{
	// z(n + 1) = z(n)^(degree-1)*(z(n) + p) + qy(n),  y(n + 1) = z(n)
	int i;
	ComplexD oldplus;
	ComplexD newminus;
	oldplus = g_old_z;
	g_temp_z = g_old_z;
	for (i = 1; i < g_degree; i++)  // degree >= 2, degree = degree-1 in setup
	{
		FPUcplxmul(&g_old_z, &g_temp_z, &g_temp_z); // = old^(degree-1)
	}
	oldplus.real(oldplus.real() + g_float_parameter->real());
	oldplus.imag(oldplus.imag() + g_float_parameter->imag());
	FPUcplxmul(&g_temp_z, &oldplus, &newminus);
	FPUcplxmul(&g_parameter2, &s_temp2, &g_temp_z);
	g_new_z.real(newminus.real() + g_temp_z.real());
	g_new_z.imag(newminus.imag() + g_temp_z.imag());
	s_temp2 = g_old_z; // set s_temp2 to Y value
	return g_externs.BailOutFp();
}

int phoenix_complex_minus_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z(n + 1) = z(n)^(degree-2)*(z(n)^2 + p) + qy(n),  y(n + 1) = z(n)
	int i;
	ComplexL loldsqr;
	LCMPLXmult(g_old_z_l, g_old_z_l, loldsqr);
	g_temp_z_l = g_old_z_l;
	for (i = 1; i < g_degree; i++)  // degree >= 3, degree = degree-2 in setup
	{
		LCMPLXmult(g_old_z_l, g_temp_z_l, g_temp_z_l); // = old^(degree-2)
	}
	loldsqr.real(loldsqr.real() + g_long_parameter->real());
	loldsqr.imag(loldsqr.imag() + g_long_parameter->imag());
	ComplexL lnewminus;
	LCMPLXmult(g_temp_z_l, loldsqr, lnewminus);
	LCMPLXmult(g_parameter2_l, s_temp_z2_l, g_temp_z_l);
	g_new_z_l.real(lnewminus.real() + g_temp_z_l.real());
	g_new_z_l.imag(lnewminus.imag() + g_temp_z_l.imag());
	s_temp_z2_l = g_old_z_l; // set s_temp_z2_l to Y value
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int phoenix_complex_minus_orbit_fp()
{
	// z(n + 1) = z(n)^(degree-2)*(z(n)^2 + p) + qy(n),  y(n + 1) = z(n)
	int i;
	ComplexD oldsqr;
	ComplexD newminus;
	FPUcplxmul(&g_old_z, &g_old_z, &oldsqr);
	g_temp_z = g_old_z;
	for (i = 1; i < g_degree; i++)  // degree >= 3, degree = degree-2 in setup
	{
		FPUcplxmul(&g_old_z, &g_temp_z, &g_temp_z); // = old^(degree-2)
	}
	oldsqr.real(oldsqr.real() + g_float_parameter->real());
	oldsqr.imag(oldsqr.imag() + g_float_parameter->imag());
	FPUcplxmul(&g_temp_z, &oldsqr, &newminus);
	FPUcplxmul(&g_parameter2, &s_temp2, &g_temp_z);
	g_new_z.real(newminus.real() + g_temp_z.real());
	g_new_z.imag(newminus.imag() + g_temp_z.imag());
	s_temp2 = g_old_z; // set s_temp2 to Y value
	return g_externs.BailOutFp();
}

int scott_trig_plus_trig_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z = trig0(z) + trig1(z)
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);
	LCMPLXtrig1(g_old_z_l, g_old_z_l);
	LCMPLXadd(g_temp_z_l, g_old_z_l, g_new_z_l);
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int scott_trig_plus_trig_orbit_fp()
{
	// z = trig0(z) + trig1(z)
	CMPLXtrig0(g_old_z, g_temp_z);
	CMPLXtrig1(g_old_z, s_temp2);
	CMPLXadd(g_temp_z, s_temp2, g_new_z);
	return g_externs.BailOutFp();
}

int skinner_trig_sub_trig_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z = trig(0, z)-trig1(z)
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);
	LCMPLXtrig1(g_old_z_l, s_temp_z2_l);
	LCMPLXsub(g_temp_z_l, s_temp_z2_l, g_new_z_l);
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int skinner_trig_sub_trig_orbit_fp()
{
	// z = trig0(z)-trig1(z)
	CMPLXtrig0(g_old_z, g_temp_z);
	CMPLXtrig1(g_old_z, s_temp2);
	CMPLXsub(g_temp_z, s_temp2, g_new_z);
	return g_externs.BailOutFp();
}

int trig_trig_orbit_fp()
{
	// z = trig0(z)*trig1(z)
	CMPLXtrig0(g_old_z, g_temp_z);
	CMPLXtrig1(g_old_z, g_old_z);
	CMPLXmult(g_temp_z, g_old_z, g_new_z);
	return g_externs.BailOutFp();
}

#if !defined(NO_FIXED_POINT_MATH)
// call float version of fractal if integer math overflow
int try_float_fractal(int (*fpFractal)())
{
	g_overflow = false;
	// g_old_z_l had better not be changed!
	g_old_z = ComplexFudgeToDouble(g_old_z_l);
	g_temp_sqr.real(sqr(g_old_z.real()));
	g_temp_sqr.imag(sqr(g_old_z.imag()));
	fpFractal();
	g_new_z_l = ComplexDoubleToFudge(g_new_z);
	return 0;
}
#endif

int trig_trig_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	ComplexL temp;
	// z = trig0(z)*trig1(z)
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);
	LCMPLXtrig1(g_old_z_l, temp);
	LCMPLXmult(g_temp_z_l, temp, g_new_z_l);
	if (g_overflow)
	{
		try_float_fractal(trig_trig_orbit_fp);
	}
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

//
// Next six orbit functions are one type - extra functions are
// special cases written for speed.
//
int trig_plus_sqr_orbit() // generalization of Scott and Skinner types
{
#if !defined(NO_FIXED_POINT_MATH)
	// { z = pixel: z = (p1, p2)*trig(z) + (p3, p4)*sqr(z), |z|<BAILOUT }
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);     // g_temp_z_l = trig(g_old_z_l)
	LCMPLXmult(g_parameter_l, g_temp_z_l, g_new_z_l); // g_new_z_l = g_parameter_l*trig(g_old_z_l)
	LCMPLXsqr_old(g_temp_z_l);         // g_temp_z_l = sqr(g_old_z_l)
	LCMPLXmult(g_parameter2_l, g_temp_z_l, g_temp_z_l); // g_temp_z_l = g_parameter2_l*sqr(g_old_z_l)
	LCMPLXadd(g_new_z_l, g_temp_z_l, g_new_z_l);   // g_new_z_l = g_parameter_l*trig(g_old_z_l) + g_parameter2_l*sqr(g_old_z_l)
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int trig_plus_sqr_orbit_fp() // generalization of Scott and Skinner types
{
	// { z = pixel: z = (p1, p2)*trig(z) + (p3, p4)*sqr(z), |z|<BAILOUT }
	CMPLXtrig0(g_old_z, g_temp_z);     // tmp = trig(old)
	CMPLXmult(g_parameter, g_temp_z, g_new_z); // new = g_parameter*trig(old)

	CMPLXsqr_old(g_temp_z);        // tmp = sqr(old)
	CMPLXmult(g_parameter2, g_temp_z, s_temp2); // tmp = g_parameter2*sqr(old)
	CMPLXadd(g_new_z, s_temp2, g_new_z);    // new = g_parameter*trig(old) + g_parameter2*sqr(old)
	return g_externs.BailOutFp();
}

int scott_trig_plus_sqr_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// { z = pixel: z = trig(z) + sqr(z), |z|<BAILOUT }
	LCMPLXtrig0(g_old_z_l, g_new_z_l);    // g_new_z_l = trig(g_old_z_l)
	LCMPLXsqr_old(g_temp_z_l);        // g_old_z_l = sqr(g_old_z_l)
	LCMPLXadd(g_temp_z_l, g_new_z_l, g_new_z_l);  // g_new_z_l = trig(g_old_z_l) + sqr(g_old_z_l)
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int scott_trig_plus_sqr_orbit_fp() // float version
{
	// { z = pixel: z = sin(z) + sqr(z), |z|<BAILOUT }
	CMPLXtrig0(g_old_z, g_new_z);       // new = trig(old)
	CMPLXsqr_old(g_temp_z);          // tmp = sqr(old)
	CMPLXadd(g_new_z, g_temp_z, g_new_z);      // new = trig(old) + sqr(old)
	return g_externs.BailOutFp();
}

int skinner_trig_sub_sqr_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// { z = pixel: z = sin(z)-sqr(z), |z|<BAILOUT }
	LCMPLXtrig0(g_old_z_l, g_new_z_l);    // g_new_z_l = trig(g_old_z_l)
	LCMPLXsqr_old(g_temp_z_l);        // g_old_z_l = sqr(g_old_z_l)
	LCMPLXsub(g_new_z_l, g_temp_z_l, g_new_z_l);  // g_new_z_l = trig(g_old_z_l)-sqr(g_old_z_l)
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int skinner_trig_sub_sqr_orbit_fp()
{
	// { z = pixel: z = sin(z)-sqr(z), |z|<BAILOUT }
	CMPLXtrig0(g_old_z, g_new_z);       // new = trig(old)
	CMPLXsqr_old(g_temp_z);          // old = sqr(old)
	CMPLXsub(g_new_z, g_temp_z, g_new_z);      // new = trig(old)-sqr(old)
	return g_externs.BailOutFp();
}

int trig_z_squared_orbit_fp()
{
	// { z = pixel: z = trig(z*z), |z|<TEST }
	CMPLXsqr_old(g_temp_z);
	CMPLXtrig0(g_temp_z, g_new_z);
	return g_externs.BailOutFp();
}

int trig_z_squared_orbit() // this doesn't work very well
{
#if !defined(NO_FIXED_POINT_MATH)
	// { z = pixel: z = trig(z*z), |z|<TEST }
	long l16triglim_2 = 8L << 15;
	LCMPLXsqr_old(g_temp_z_l);
	if (labs(g_temp_z_l.real()) > l16triglim_2 || labs(g_temp_z_l.imag()) > l16triglim_2)
	{
		g_overflow = true;
	}

	if (g_overflow)
	{
		try_float_fractal(trig_z_squared_orbit_fp);
	}
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int sqr_trig_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// { z = pixel: z = sqr(trig(z)), |z|<TEST}
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);
	LCMPLXsqr(g_temp_z_l, g_new_z_l);
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int sqr_trig_orbit_fp()
{
	// SZSB(XYAXIS) { z = pixel, TEST = (p1 + 3): z = sin(z)*sin(z), |z|<TEST}
	CMPLXtrig0(g_old_z, g_temp_z);
	CMPLXsqr(g_temp_z, g_new_z);
	return g_externs.BailOutFp();
}

int magnet1_orbit_fp()    // Z = ((Z**2 + C - 1)/(2Z + C - 2))**2
{                   // In "Beauty of Fractals", code by Kev Allen.
	ComplexD top;
	ComplexD bot;
	ComplexD tmp;
	double div;

	top.real(g_temp_sqr.real() - g_temp_sqr.imag() + g_float_parameter->real() - 1); // top = Z**2 + C-1
	top.imag(g_old_z.real()*g_old_z.imag());
	top.imag(top.imag() + top.imag() + g_float_parameter->imag());

	bot.real(g_old_z.real() + g_old_z.real() + g_float_parameter->real() - 2);       // bot = 2*Z + C-2
	bot.imag(g_old_z.imag() + g_old_z.imag() + g_float_parameter->imag());

	div = bot.real()*bot.real() + bot.imag()*bot.imag();                // tmp = top/bot
	if (div < FLT_MIN)
	{
		return 1;
	}
	tmp.real((top.real()*bot.real() + top.imag()*bot.imag())/div);
	tmp.imag((top.imag()*bot.real() - top.real()*bot.imag())/div);

	g_new_z.real((tmp.real() + tmp.imag())*(tmp.real() - tmp.imag()));      // Z = tmp**2
	g_new_z.imag(tmp.real()*tmp.imag());
	g_new_z.imag(g_new_z.imag() + g_new_z.imag());

	return g_externs.BailOutFp();
}

// Z = ((Z**3 + 3(C-1)Z + (C-1)(C-2))/
// (3Z**2 + 3(C-2)Z + (C-1)(C-2) + 1))**2
int magnet2_orbit_fp()
{
	// In "Beauty of Fractals", code by Kev Allen.
	ComplexD top;
	ComplexD bot;
	ComplexD tmp;
	double div;

	top.real(g_old_z.real()*(g_temp_sqr.real()-g_temp_sqr.imag()-g_temp_sqr.imag()-g_temp_sqr.imag() + s_3_c_minus_1.real())
			- g_old_z.imag()*s_3_c_minus_1.imag() + s_c_minus_1_c_minus_2.real());
	top.imag(g_old_z.imag()*(g_temp_sqr.real() + g_temp_sqr.real() + g_temp_sqr.real()-g_temp_sqr.imag() + s_3_c_minus_1.real())
			+ g_old_z.real()*s_3_c_minus_1.imag() + s_c_minus_1_c_minus_2.imag());

	bot.real(g_temp_sqr.real() - g_temp_sqr.imag());
	bot.real(bot.real() + bot.real() + bot.real()
			+ g_old_z.real()*s_3_c_minus_2.real() - g_old_z.imag()*s_3_c_minus_2.imag()
			+ s_c_minus_1_c_minus_2.real() + 1.0);
	bot.imag(g_old_z.real()*g_old_z.imag());
	bot.imag(bot.imag() + bot.imag());
	bot.imag(bot.imag() + bot.imag() + bot.imag()
			+ g_old_z.real()*s_3_c_minus_2.imag() + g_old_z.imag()*s_3_c_minus_2.real()
			+ s_c_minus_1_c_minus_2.imag());

	div = bot.real()*bot.real() + bot.imag()*bot.imag();                // tmp = top/bot
	if (div < FLT_MIN)
	{
		return 1;
	}
	tmp.real((top.real()*bot.real() + top.imag()*bot.imag())/div);
	tmp.imag((top.imag()*bot.real() - top.real()*bot.imag())/div);

	g_new_z.real((tmp.real() + tmp.imag())*(tmp.real() - tmp.imag()));      // Z = tmp**2
	g_new_z.imag(tmp.real()*tmp.imag());
	g_new_z.imag(g_new_z.imag() + g_new_z.imag());

	return g_externs.BailOutFp();
}

int lambda_trig_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	if (labs(g_old_z_l.real()) >= g_rq_limit2_l
		|| labs(g_old_z_l.imag()) >= g_rq_limit2_l)
	{
		return 1;
	}
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);           // g_temp_z_l = trig(g_old_z_l)
	LCMPLXmult(*g_long_parameter, g_temp_z_l, g_new_z_l);   // g_new_z_l = g_long_parameter*trig(g_old_z_l)
	g_old_z_l = g_new_z_l;
	return 0;
#else
	return 0;
#endif
}

int lambda_trig_orbit_fp()
{
	if (fabs(g_old_z.real()) >= g_rq_limit2
		|| fabs(g_old_z.imag()) >= g_rq_limit2)
	{
		return 1;
	}
	CMPLXtrig0(g_old_z, g_temp_z);              // tmp = trig(old)
	CMPLXmult(*g_float_parameter, g_temp_z, g_new_z);   // new = g_long_parameter*trig(old)
	g_old_z = g_new_z;
	return 0;
}

// bailouts are different for different trig functions
int lambda_trig1_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	if (labs(g_old_z_l.imag()) >= g_rq_limit2_l)
	{
		return 1;
	}
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);           // g_temp_z_l = trig(g_old_z_l)
	LCMPLXmult(*g_long_parameter, g_temp_z_l, g_new_z_l);   // g_new_z_l = g_long_parameter*trig(g_old_z_l)
	g_old_z_l = g_new_z_l;
	return 0;
#else
	return 0;
#endif
}

int lambda_trig1_orbit_fp()
{
	if (fabs(g_old_z.imag()) >= g_rq_limit2)
	{
		return 1;
	}
	CMPLXtrig0(g_old_z, g_temp_z);              // tmp = trig(old)
	CMPLXmult(*g_float_parameter, g_temp_z, g_new_z);   // new = g_long_parameter*trig(old)
	g_old_z = g_new_z;
	return 0;
}

int lambda_trig2_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	if (labs(g_old_z_l.real()) >= g_rq_limit2_l)
	{
		return 1;
	}
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);           // g_temp_z_l = trig(g_old_z_l)
	LCMPLXmult(*g_long_parameter, g_temp_z_l, g_new_z_l);   // g_new_z_l = g_long_parameter*trig(g_old_z_l)
	g_old_z_l = g_new_z_l;
	return 0;
#else
	return 0;
#endif
}

int lambda_trig2_orbit_fp()
{
#if !defined(NO_FIXED_POINT_MATH)
	if (fabs(g_old_z.real()) >= g_rq_limit2)
	{
		return 1;
	}
	CMPLXtrig0(g_old_z, g_temp_z);              // tmp = trig(old)
	CMPLXmult(*g_float_parameter, g_temp_z, g_new_z);   // new = g_long_parameter*trig(old)
	g_old_z = g_new_z;
	return 0;
#else
	return 0;
#endif
}

int man_o_war_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// From Art Matrix via Lee Skinner
	g_new_z_l.real(g_temp_sqr_l.real() - g_temp_sqr_l.imag() + g_temp_z_l.real() + g_long_parameter->real());
	g_new_z_l.imag(multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift_minus_1) + g_temp_z_l.imag() + g_long_parameter->imag());
	g_temp_z_l = g_old_z_l;
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int man_o_war_orbit_fp()
{
	// From Art Matrix via Lee Skinner
	// note that fast >= 287 equiv in fracsuba.asm must be kept in step
	g_new_z.real(g_temp_sqr.real() - g_temp_sqr.imag() + g_temp_z.real() + g_float_parameter->real());
	g_new_z.imag(2.0*g_old_z.real()*g_old_z.imag() + g_temp_z.imag() + g_float_parameter->imag());
	g_temp_z = g_old_z;
	return g_externs.BailOutFp();
}

//	MarksMandelPwr (XAXIS)
//	{
//		z = pixel, c = z ^ (z - 1):
//			z = c*sqr(z) + pixel,
//		|z| <= 4
//	}
//
int marks_mandel_power_orbit_fp()
{
	CMPLXtrig0(g_old_z, g_new_z);
	CMPLXmult(g_temp_z, g_new_z, g_new_z);
	g_new_z.real(g_new_z.real() + g_float_parameter->real());
	g_new_z.imag(g_new_z.imag() + g_float_parameter->imag());
	return g_externs.BailOutFp();
}

int marks_mandel_power_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	LCMPLXtrig0(g_old_z_l, g_new_z_l);
	LCMPLXmult(g_temp_z_l, g_new_z_l, g_new_z_l);
	g_new_z_l.real(g_new_z_l.real() + g_long_parameter->real());
	g_new_z_l.imag(g_new_z_l.imag() + g_long_parameter->imag());
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

//
//	I was coding Marksmandelpower and failed to use some temporary
//	variables. The result was nice, and since my name is not on any fractal,
//	I thought I would immortalize myself with this error!
//	Tim Wegner
//
int tims_error_orbit_fp()
{
	CMPLXtrig0(g_old_z, g_new_z);
	g_new_z.real(g_new_z.real()*g_temp_z.real() - g_new_z.imag()*g_temp_z.imag());
	g_new_z.imag(g_new_z.real()*g_temp_z.imag() - g_new_z.imag()*g_temp_z.real());
	g_new_z.real(g_new_z.real() + g_float_parameter->real());
	g_new_z.imag(g_new_z.imag() + g_float_parameter->imag());
	return g_externs.BailOutFp();
}

int tims_error_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	LCMPLXtrig0(g_old_z_l, g_new_z_l);
	g_new_z_l.real(multiply(g_new_z_l.real(), g_temp_z_l.real(), g_bit_shift)-multiply(g_new_z_l.imag(), g_temp_z_l.imag(), g_bit_shift));
	g_new_z_l.imag(multiply(g_new_z_l.real(), g_temp_z_l.imag(), g_bit_shift)-multiply(g_new_z_l.imag(), g_temp_z_l.real(), g_bit_shift));
	g_new_z_l.real(g_new_z_l.real() + g_long_parameter->real());
	g_new_z_l.imag(g_new_z_l.imag() + g_long_parameter->imag());
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int circle_orbit_fp()
{
	long i;
	i = long(g_parameters[P1_REAL]*(g_temp_sqr.real() + g_temp_sqr.imag()));
	g_color_iter = i % g_colors;
	return 1;
}

/*
int circle_orbit()
{
	long i;
	i = multiply(g_parameter_l.real(), (g_temp_sqr_l.real() + g_temp_sqr_l.imag()), g_bit_shift);
	i >>= g_bit_shift;
	g_color_iter = i % g_colors);
	return 1;
}
*/

// --------------------------------------------------------------------
// Initialization (once per pixel) routines
// --------------------------------------------------------------------

// transform points with reciprocal function
void invert_z(ComplexD *z)
{
	*z = g_externs.DPixel();
	z->real(z->real() - g_f_x_center);
	z->imag(z->imag() - g_f_y_center);  // Normalize values to center of circle

	g_temp_sqr.real(sqr(z->real()) + sqr(z->imag()));  // Get old radius
	g_temp_sqr.real((fabs(g_temp_sqr.real()) > FLT_MIN) ? (g_f_radius/g_temp_sqr.real()) : FLT_MAX);
	z->real(z->real()*g_temp_sqr.real());
	z->imag(z->imag()*g_temp_sqr.real());      // Perform inversion
	z->real(z->real() + g_f_x_center);
	z->imag(z->imag() + g_f_y_center); // Renormalize
}

int julia_per_pixel_l()
{
#if !defined(NO_FIXED_POINT_MATH)
	// integer julia types
	// lambda
	// barnsleyj1
	// barnsleyj2
	// sierpinski
	if (g_invert)
	{
		// invert
		invert_z(&g_old_z);

		// watch out for overflow
		if (sqr(g_old_z.real()) + sqr(g_old_z.imag()) >= 127)
		{
			g_old_z.real(8);  // value to bail out in one iteration
			g_old_z.imag(8);
		}

		// convert to fudged longs
		g_old_z_l = ComplexDoubleToFudge(g_old_z);
	}
	else
	{
		g_old_z_l = g_externs.LPixel();
	}
	return 0;
#else
	return 0;
#endif
}

int richard8_per_pixel()
{
#if !defined(NO_FIXED_POINT_MATH)
	mandelbrot_per_pixel_l();
	LCMPLXtrig1(*g_long_parameter, g_temp_z_l);
	LCMPLXmult(g_temp_z_l, g_parameter2_l, g_temp_z_l);
	return 1;
#else
	return 0;
#endif
}

int mandelbrot_per_pixel_l()
{
#if !defined(NO_FIXED_POINT_MATH)
	// integer mandel types
	// barnsleym1
	// barnsleym2
	g_initial_z_l = g_externs.LPixel();

	if (g_invert)
	{
		// invert
		invert_z(&g_initial_z);

		// watch out for overflow
		if (sqr(g_initial_z.real()) + sqr(g_initial_z.imag()) >= 127)
		{
			g_initial_z.real(8);  // value to bail out in one iteration
			g_initial_z.imag(8);
		}

		// convert to fudged longs
		g_initial_z_l = ComplexDoubleToFudge(g_initial_z);
	}

	g_old_z_l = (g_externs.UseInitialOrbitZ() == INITIALZ_ORBIT) ? g_initial_orbit_z_l : g_initial_z_l;

	g_old_z_l.real(g_old_z_l.real() + g_parameter_l.real());    // initial pertubation of parameters set
	g_old_z_l.imag(g_old_z_l.imag() + g_parameter_l.imag());
	return 1; // 1st iteration has been done
#else
	return 0;
#endif
}

int julia_per_pixel()
{
	// julia

	if (g_invert)
	{
		// invert
		invert_z(&g_old_z);

		// watch out for overflow
		if (g_bit_shift <= 24)
		{
			if (sqr(g_old_z.real()) + sqr(g_old_z.imag()) >= 127)
			{
				g_old_z.real(8);  // value to bail out in one iteration
				g_old_z.imag(8);
			}
		}
		else if (sqr(g_old_z.real()) + sqr(g_old_z.imag()) >= 4.0)
		{
			g_old_z.real(2);  // value to bail out in one iteration
			g_old_z.imag(2);
		}

		// convert to fudged longs
		g_old_z_l = ComplexDoubleToFudge(g_old_z);
	}
	else
	{
		g_old_z_l = g_externs.LPixel();
	}

	g_temp_sqr_l.real(multiply(g_old_z_l.real(), g_old_z_l.real(), g_bit_shift));
	g_temp_sqr_l.imag(multiply(g_old_z_l.imag(), g_old_z_l.imag(), g_bit_shift));
	g_temp_z_l = g_old_z_l;
	return 0;
}

int marks_mandelbrot_power_per_pixel()
{
#if !defined(NO_FIXED_POINT_MATH)
	mandelbrot_per_pixel();
	g_temp_z_l = g_old_z_l;
	g_temp_z_l.real(g_temp_z_l.real() - g_externs.Fudge());
	LCMPLXpwr(g_old_z_l, g_temp_z_l, g_temp_z_l);
	return 1;
#else
	return 0;
#endif
}

int mandelbrot_per_pixel()
{
	if (g_invert)
	{
		invert_z(&g_initial_z);

		// watch out for overflow
		if (g_bit_shift <= 24)
		{
			if (sqr(g_initial_z.real()) + sqr(g_initial_z.imag()) >= 127)
			{
				g_initial_z.real(8);  // value to bail out in one iteration
				g_initial_z.imag(8);
			}
		}
		else if (sqr(g_initial_z.real()) + sqr(g_initial_z.imag()) >= 4)
		{
			g_initial_z.real(2);  // value to bail out in one iteration
			g_initial_z.imag(2);
		}

		// convert to fudged longs
		g_initial_z_l = ComplexDoubleToFudge(g_initial_z);
	}
	else
	{
		g_initial_z_l = g_externs.LPixel();
	}
	switch (g_fractal_type)
	{
	case FRACTYPE_MANDELBROT_LAMBDA:              // Critical Value 0.5 + 0.0i
		g_old_z_l.real(g_externs.Fudge() >> 1);
		g_old_z_l.imag(0);
		break;
	default:
		g_old_z_l = g_initial_z_l;
		break;
	}

	// alter g_initial_z value
	if (g_externs.UseInitialOrbitZ() == INITIALZ_ORBIT)
	{
		g_old_z_l = g_initial_orbit_z_l;
	}
	else if (g_externs.UseInitialOrbitZ() == INITIALZ_PIXEL)
	{
		g_old_z_l = g_initial_z_l;
	}

	if (inside_coloring_beauty_of_fractals_allowed())
	{
		// kludge to match "Beauty of Fractals" picture since we start
		// Mandelbrot iteration with g_initial_z rather than 0
		g_old_z_l = g_parameter_l;				// initial pertubation of parameters set
		g_color_iter = -1;
	}
	else
	{
		g_old_z_l.real(g_old_z_l.real() + g_parameter_l.real()); // initial pertubation of parameters set
		g_old_z_l.imag(g_old_z_l.imag() + g_parameter_l.imag());
	}
	g_temp_z_l = g_initial_z_l; // for spider
	g_temp_sqr_l.real(multiply(g_old_z_l.real(), g_old_z_l.real(), g_bit_shift));
	g_temp_sqr_l.imag(multiply(g_old_z_l.imag(), g_old_z_l.imag(), g_bit_shift));
	return 1; // 1st iteration has been done
}

int marks_mandelbrot_per_pixel()
{
#if !defined(NO_FIXED_POINT_MATH)
	// marksmandel
	if (g_invert)
	{
		invert_z(&g_initial_z);

		// watch out for overflow
		if (sqr(g_initial_z.real()) + sqr(g_initial_z.imag()) >= 127)
		{
			g_initial_z.real(8);  // value to bail out in one iteration
			g_initial_z.imag(8);
		}

		// convert to fudged longs
		g_initial_z_l = ComplexDoubleToFudge(g_initial_z);
	}
	else
	{
		g_initial_z_l = g_externs.LPixel();
	}

	g_old_z_l = (g_externs.UseInitialOrbitZ() == INITIALZ_ORBIT) ? g_initial_orbit_z_l : g_initial_z_l;

	g_old_z_l.real(g_old_z_l.real() + g_parameter_l.real());    // initial pertubation of parameters set
	g_old_z_l.imag(g_old_z_l.imag() + g_parameter_l.imag());

	if (g_c_exp > 3)
	{
		complex_power_l(&g_old_z_l, g_c_exp-1, &g_coefficient_l, g_bit_shift);
	}
	else if (g_c_exp == 3)
	{
		g_coefficient_l.real(multiply(g_old_z_l.real(), g_old_z_l.real(), g_bit_shift)
			- multiply(g_old_z_l.imag(), g_old_z_l.imag(), g_bit_shift));
		g_coefficient_l.imag(multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift_minus_1));
	}
	else if (g_c_exp == 2)
	{
		g_coefficient_l = g_old_z_l;
	}
	else if (g_c_exp < 2)
	{
		g_coefficient_l.real(1L << g_bit_shift);
		g_coefficient_l.imag(0L);
	}

	g_temp_sqr_l.real(multiply(g_old_z_l.real(), g_old_z_l.real(), g_bit_shift));
	g_temp_sqr_l.imag(multiply(g_old_z_l.imag(), g_old_z_l.imag(), g_bit_shift));
#endif
	return 1; // 1st iteration has been done
}

int marks_mandelbrot_per_pixel_fp()
{
	// marksmandel

	if (g_invert)
	{
		invert_z(&g_initial_z);
	}
	else
	{
		g_initial_z = g_externs.DPixel();
	}

	g_old_z = (g_externs.UseInitialOrbitZ() == INITIALZ_ORBIT) ? g_initial_orbit_z : g_initial_z;

	g_old_z.real(g_old_z.real() + g_parameter.real());      // initial pertubation of parameters set
	g_old_z.imag(g_old_z.imag() + g_parameter.imag());

	g_temp_sqr.real(sqr(g_old_z.real()));
	g_temp_sqr.imag(sqr(g_old_z.imag()));

	if (g_c_exp > 3)
	{
		complex_power(g_old_z, g_c_exp-1, g_coefficient);
	}
	else if (g_c_exp == 3)
	{
		g_coefficient.real(g_temp_sqr.real() - g_temp_sqr.imag());
		g_coefficient.imag(g_old_z.real()*g_old_z.imag()*2);
	}
	else if (g_c_exp == 2)
	{
		Assign(g_coefficient, g_old_z);
	}
	else if (g_c_exp < 2)
	{
		g_coefficient = StdComplexD(1.0, 0.0);
	}

	return 1; // 1st iteration has been done
}

int marks_mandelbrot_power_per_pixel_fp()
{
	mandelbrot_per_pixel_fp();
	g_temp_z = g_old_z;
	g_temp_z.real(g_temp_z.real() - 1.0);
	CMPLXpwr(g_old_z, g_temp_z, g_temp_z);
	return 1;
}

int mandelbrot_per_pixel_fp()
{
	// floating point mandelbrot
	// mandelfp

	if (g_invert)
	{
		invert_z(&g_initial_z);
	}
	else
	{
		g_initial_z = g_externs.DPixel();
	}
	switch (g_fractal_type)
	{
	case FRACTYPE_MAGNET_2M:
		magnet2_precalculate_fp();
	case FRACTYPE_MAGNET_1M:           // Crit Val Zero both, but neither
		g_old_z.real(g_old_z.imag(0.0)); // is of the form f(Z, C) = Z*g(Z) + C
		break;
	case FRACTYPE_MANDELBROT_LAMBDA_FP:            // Critical Value 0.5 + 0.0i
		g_old_z.real(0.5);
		g_old_z.imag(0.0);
		break;
	default:
		g_old_z = g_initial_z;
		break;
	}

	// alter g_initial_z value
	if (g_externs.UseInitialOrbitZ() == INITIALZ_ORBIT)
	{
		g_old_z = g_initial_orbit_z;
	}
	else if (g_externs.UseInitialOrbitZ() == INITIALZ_PIXEL)
	{
		g_old_z = g_initial_z;
	}

	if (inside_coloring_beauty_of_fractals_allowed())
	{
		// kludge to match "Beauty of Fractals" picture since we start
		// Mandelbrot iteration with g_initial_z rather than 0
		g_old_z = g_parameter;				// initial pertubation of parameters set
		g_color_iter = -1;
	}
	else
	{
		g_old_z.real(g_old_z.real() + g_parameter.real());
		g_old_z.imag(g_old_z.imag() + g_parameter.imag());
	}
	g_temp_z = g_initial_z; // for spider
	g_temp_sqr.real(sqr(g_old_z.real()));  // precalculated value for regular Mandelbrot
	g_temp_sqr.imag(sqr(g_old_z.imag()));
	return 1; // 1st iteration has been done
}

int julia_per_pixel_fp()
{
	// floating point julia
	// juliafp
	if (g_invert)
	{
		invert_z(&g_old_z);
	}
	else
	{
		g_old_z = g_externs.DPixel();
	}
	g_temp_sqr.real(sqr(g_old_z.real()));  // precalculated value for regular Julia
	g_temp_sqr.imag(sqr(g_old_z.imag()));
	g_temp_z = g_old_z;
	return 0;
}

int julia_per_pixel_mpc()
{
	return 0;
}

int other_richard8_per_pixel_fp()
{
	other_mandelbrot_per_pixel_fp();
	CMPLXtrig1(*g_float_parameter, g_temp_z);
	CMPLXmult(g_temp_z, g_parameter2, g_temp_z);
	return 1;
}

int other_mandelbrot_per_pixel_fp()
{
	if (g_invert)
	{
		invert_z(&g_initial_z);
	}
	else
	{
		g_initial_z = g_externs.DPixel();
	}

	g_old_z = (g_externs.UseInitialOrbitZ() == INITIALZ_ORBIT) ? g_initial_orbit_z : g_initial_z;

	g_old_z.real(g_old_z.real() + g_parameter.real());      // initial pertubation of parameters set
	g_old_z.imag(g_old_z.imag() + g_parameter.imag());

	return 1; // 1st iteration has been done
}

int other_julia_per_pixel_fp()
{
	if (g_invert)
	{
		invert_z(&g_old_z);
	}
	else
	{
		g_old_z = g_externs.DPixel();
	}
	return 0;
}

int marks_complex_mandelbrot_per_pixel()
{
	if (g_invert)
	{
		invert_z(&g_initial_z);
	}
	else
	{
		g_initial_z = g_externs.DPixel();
	}
	g_old_z.real(g_initial_z.real() + g_parameter.real()); // initial pertubation of parameters set
	g_old_z.imag(g_initial_z.imag() + g_parameter.imag());
	g_temp_sqr.real(sqr(g_old_z.real()));  // precalculated value
	g_temp_sqr.imag(sqr(g_old_z.imag()));
	Assign(g_coefficient, ComplexPower(g_initial_z, g_power));
	return 1;
}

int phoenix_per_pixel()
{
#if !defined(NO_FIXED_POINT_MATH)
	if (g_invert)
	{
		// invert
		invert_z(&g_old_z);

		// watch out for overflow
		if (sqr(g_old_z.real()) + sqr(g_old_z.imag()) >= 127)
		{
			g_old_z.real(8);  // value to bail out in one iteration
			g_old_z.imag(8);
		}

		// convert to fudged longs
		g_old_z_l = ComplexDoubleToFudge(g_old_z);
	}
	else
	{
		g_old_z_l = g_externs.LPixel();
	}
	g_temp_sqr_l.real(multiply(g_old_z_l.real(), g_old_z_l.real(), g_bit_shift));
	g_temp_sqr_l.imag(multiply(g_old_z_l.imag(), g_old_z_l.imag(), g_bit_shift));
	s_temp_z2_l.real(0L); // use s_temp_z2_l as the complex Y value
	s_temp_z2_l.imag(0L);
	return 0;
#else
	return 0;
#endif
}

int phoenix_per_pixel_fp()
{
	if (g_invert)
	{
		invert_z(&g_old_z);
	}
	else
	{
		g_old_z = g_externs.DPixel();
	}
	g_temp_sqr.real(sqr(g_old_z.real()));  // precalculated value
	g_temp_sqr.imag(sqr(g_old_z.imag()));
	s_temp2.real(0.0); // use s_temp2 as the complex Y value
	s_temp2.imag(0.0);
	return 0;
}

int mandelbrot_phoenix_per_pixel()
{
#if !defined(NO_FIXED_POINT_MATH)
	g_initial_z_l = g_externs.LPixel();

	if (g_invert)
	{
		// invert
		invert_z(&g_initial_z);

		// watch out for overflow
		if (sqr(g_initial_z.real()) + sqr(g_initial_z.imag()) >= 127)
		{
			g_initial_z.real(8);  // value to bail out in one iteration
			g_initial_z.imag(8);
		}

		// convert to fudged longs
		g_initial_z_l = ComplexDoubleToFudge(g_initial_z);
	}

	g_old_z_l = (g_externs.UseInitialOrbitZ() == INITIALZ_ORBIT) ? g_initial_orbit_z_l : g_initial_z_l;

	g_old_z_l.real(g_old_z_l.real() + g_parameter_l.real());    // initial pertubation of parameters set
	g_old_z_l.imag(g_old_z_l.imag() + g_parameter_l.imag());
	g_temp_sqr_l.real(multiply(g_old_z_l.real(), g_old_z_l.real(), g_bit_shift));
	g_temp_sqr_l.imag(multiply(g_old_z_l.imag(), g_old_z_l.imag(), g_bit_shift));
	s_temp_z2_l.real(0L);
	s_temp_z2_l.imag(0L);
	return 1; // 1st iteration has been done
#else
	return 0;
#endif
}

int mandelbrot_phoenix_per_pixel_fp()
{
	if (g_invert)
	{
		invert_z(&g_initial_z);
	}
	else
	{
		g_initial_z = g_externs.DPixel();
	}

	g_old_z = (g_externs.UseInitialOrbitZ() == INITIALZ_ORBIT) ? g_initial_orbit_z : g_initial_z;

	g_old_z.real(g_old_z.real() + g_parameter.real());      // initial pertubation of parameters set
	g_old_z.imag(g_old_z.imag() + g_parameter.imag());
	g_temp_sqr.real(sqr(g_old_z.real()));  // precalculated value
	g_temp_sqr.imag(sqr(g_old_z.imag()));
	s_temp2.real(0.0);
	s_temp2.imag(0.0);
	return 1; // 1st iteration has been done
}

int hyper_complex_orbit_fp()
{
	HyperComplexD hold;
	hold.real(g_old_z.real());
	hold.imag(g_old_z.imag());
	hold.z(g_float_parameter->real());
	hold.t(g_float_parameter->imag());

	HyperComplexD hnew;
	HComplexTrig0(&hold, &hnew);

	hnew.real(hnew.real() + g_c_quaternion.real());
	hnew.imag(hnew.imag() + g_c_quaternion.R_component_2());
	hnew.z(hnew.z() + g_c_quaternion.R_component_3());
	hnew.t(hnew.t() + g_c_quaternion.R_component_4());

	g_old_z.real(hnew.real());
	g_old_z.imag(hnew.imag());
	g_new_z = g_old_z;
	g_float_parameter->real(hnew.z());
	g_float_parameter->imag(hnew.t());

	// Check bailout
	g_magnitude = sqr(g_old_z.real()) + sqr(g_old_z.imag())
		+ sqr(g_float_parameter->real()) + sqr(g_float_parameter->imag());
	if (g_magnitude > g_rq_limit)
	{
		return 1;
	}
	return 0;
}

// Beauty of Fractals pp. 125 - 127
int volterra_lotka_orbit_fp()
{
	double half = g_parameters[P1_REAL]/2.0;
	double xy = g_old_z.real()*g_old_z.imag();
	double u = g_old_z.real() - xy;
	double w = -g_old_z.imag() + xy;
	double a = g_old_z.real() + g_parameters[P1_IMAG]*u;
	double b = g_old_z.imag() + g_parameters[P1_IMAG]*w;
	double ab = a*b;
	g_new_z.real(g_old_z.real() + half*(u + (a - ab)));
	g_new_z.imag(g_old_z.imag() + half*(w + (-b + ab)));
	return g_externs.BailOutFp();
}

// Science of Fractal Images pp. 185, 187
int escher_orbit_fp()
{
	g_new_z.real(g_temp_sqr.real() - g_temp_sqr.imag()); // standard Julia with C == (0.0, 0.0i)
	g_new_z.imag(2.0*g_old_z.real()*g_old_z.imag());
	ComplexD oldtest;
	oldtest.real(g_new_z.real()*15.0);    // scale it
	oldtest.imag(g_new_z.imag()*15.0);
	ComplexD testsqr;
	testsqr.real(sqr(oldtest.real()));  // set up to test with user-specified ...
	testsqr.imag(sqr(oldtest.imag()));  // ... Julia as the target set
	double testsize = 0.0;
	long testiter = 0;
	while (testsize <= g_rq_limit && testiter < g_max_iteration) // nested Julia loop
	{
		ComplexD newtest;
		newtest.real(testsqr.real() - testsqr.imag() + g_parameters[P1_REAL]);
		newtest.imag(2.0*oldtest.real()*oldtest.imag() + g_parameters[P1_IMAG]);
		testsqr.real(sqr(newtest.real()));
		testsqr.imag(sqr(newtest.imag()));
		testsize = testsqr.real() + testsqr.imag();
		oldtest = newtest;
		testiter++;
	}
	if (testsize > g_rq_limit) // point not in target set
	{
		return g_externs.BailOutFp();
	}
	else // make distinct level sets if point stayed in target set
	{
		g_color_iter = ((3L*g_color_iter) % 255L) + 1L;
		return 1;
	}
}

static ComplexD s_mandelmix4_a;
static ComplexD s_mandelmix4_b;
static ComplexD s_mandelmix4_c;
static ComplexD s_mandelmix4_d;
static ComplexD s_mandelmix4_f;
static ComplexD s_mandelmix4_g;
static ComplexD s_mandelmix4_h;
static ComplexD s_mandelmix4_j;
static ComplexD s_mandelmix4_k;
static ComplexD s_mandelmix4_l;
static ComplexD s_mandelmix4_z;

bool mandelbrot_mix4_setup()
{
	int sign_array = 0;
	s_mandelmix4_a.real(g_parameters[P1_REAL]);
	s_mandelmix4_a.imag(0.0);						// a = real(p1),
	s_mandelmix4_b.real(g_parameters[P1_IMAG]);
	s_mandelmix4_b.imag(0.0);						// b = imag(p1),
	s_mandelmix4_d.real(g_parameters[P2_REAL]);
	s_mandelmix4_d.imag(0.0);						// d = real(p2),
	s_mandelmix4_f.real(g_parameters[P2_IMAG]);
	s_mandelmix4_f.imag(0.0);						// f = imag(p2),
	s_mandelmix4_k.real(g_parameters[P3_REAL] + 1.0);
	s_mandelmix4_k.imag(0.0);						// k = real(p3) + 1,
	s_mandelmix4_l.real(g_parameters[P3_IMAG] + 100.0);
	s_mandelmix4_l.imag(0.0);						// l = imag(p3) + 100,
	CMPLXrecip(s_mandelmix4_f, s_mandelmix4_g);				// g = 1/f,
	CMPLXrecip(s_mandelmix4_d, s_mandelmix4_h);				// h = 1/d,
	CMPLXsub(s_mandelmix4_f, s_mandelmix4_b, g_temp_z);			// tmp = f-b
	CMPLXrecip(g_temp_z, s_mandelmix4_j);				// j = 1/(f-b)
	CMPLXneg(s_mandelmix4_a, g_temp_z);
	CMPLXmult(g_temp_z, s_mandelmix4_b, g_temp_z);			// z = (-a*b*g*h)^j,
	CMPLXmult(g_temp_z, s_mandelmix4_g, g_temp_z);
	CMPLXmult(g_temp_z, s_mandelmix4_h, g_temp_z);

	//
	//	This code kludge attempts to duplicate the behavior
	//	of the parser in determining the sign of zero of the
	//	imaginary part of the argument of the power function. The
	//	reason this is important is that the complex arctangent
	//	returns PI in one case and -PI in the other, depending
	//	on the sign bit of zero, and we wish the results to be
	//	compatible with Jim Muth's mix4 formula using the parser.
	//
	//	First create a number encoding the signs of a, b, g , h. Our
	//	kludge assumes that those signs determine the behavior.
	//
	if (s_mandelmix4_a.real() < 0.0)
	{
		sign_array += 8;
	}
	if (s_mandelmix4_b.real() < 0.0)
	{
		sign_array += 4;
	}
	if (s_mandelmix4_g.real() < 0.0)
	{
		sign_array += 2;
	}
	if (s_mandelmix4_h.real() < 0.0)
	{
		sign_array += 1;
	}
	// TODO: does this really do anything? 0.0 == -0.0
	if (g_temp_z.imag() == 0.0) // we know tmp.imag() IS zero but ...
	{
		switch (sign_array)
		{
		//
		//	Add to this list the magic numbers of any cases
		//	in which the fractal does not match the formula version
		//
		case 15: // 1111
		case 10: // 1010
		case  6: // 0110
		case  5: // 0101
		case  3: // 0011
		case  0: // 0000
			g_temp_z.imag(-g_temp_z.imag()); // swap sign bit
		default: // do nothing - remaining cases already OK
			break;
		}
		// in case our kludge failed, let the user fix it
		if (DEBUGMODE_SWAP_SIGN == g_debug_mode)
		{
			g_temp_z.imag(-g_temp_z.imag());
		}
	}

	CMPLXpwr(g_temp_z, s_mandelmix4_j, g_temp_z);   // note: z is old
	// in case our kludge failed, let the user fix it
	if (g_parameters[P4_REAL] < 0.0)
	{
		g_temp_z.imag(-g_temp_z.imag());
	}

	if (g_externs.BailOut() == 0)
	{
		g_rq_limit = s_mandelmix4_l.real();
		g_rq_limit2 = g_rq_limit*g_rq_limit;
	}
	return true;
}

int mandelbrot_mix4_per_pixel_fp()
{
	if (g_invert)
	{
		invert_z(&g_initial_z);
	}
	else
	{
		g_initial_z = g_externs.DPixel();
	}
	g_old_z = g_temp_z;
	CMPLXtrig0(g_initial_z, s_mandelmix4_c);        // c = fn1(pixel):
	return 0; // 1st iteration has been NOT been done
}

int mandelbrot_mix4_orbit_fp() // from formula by Jim Muth
{
	// z = k*((a*(z^b)) + (d*(z^f))) + c,
	ComplexD z_b;
	ComplexD z_f;
	CMPLXpwr(g_old_z, s_mandelmix4_b, z_b);     // (z^b)
	CMPLXpwr(g_old_z, s_mandelmix4_f, z_f);     // (z^f)
	g_new_z.real(s_mandelmix4_k.real()*s_mandelmix4_a.real()*z_b.real()
		+ s_mandelmix4_k.real()*s_mandelmix4_d.real()*z_f.real() + s_mandelmix4_c.real());
	g_new_z.imag(s_mandelmix4_k.real()*s_mandelmix4_a.real()*z_b.imag()
		+ s_mandelmix4_k.real()*s_mandelmix4_d.real()*z_f.imag() + s_mandelmix4_c.imag());
	return g_externs.BailOutFp();
}

//
// The following functions calculate the real and imaginary complex
// coordinates of the point in the complex plane corresponding to
// the screen coordinates (col, row) at the current zoom corners
// settings. The functions come in two flavors. One looks up the pixel
// values using the precalculated grid arrays g_x0, g_x1, g_y0, and g_y1,
// which has a speed advantage but is limited to MAX_PIXELS image
// dimensions. The other calculates the complex coordinates at a
// cost of two additions and two multiplications for each component,
// but works at any resolution.
//

// Real component, grid lookup version - requires g_x0/g_x1 arrays
static double dx_pixel_grid()
{
	return g_escape_time_state.m_grid_fp.x_pixel_grid(g_col, g_row);
}

// Real component, calculation version - does not require arrays
double dx_pixel_calc()
{
	return double(g_escape_time_state.m_grid_fp.x_min()
		+ g_col*g_escape_time_state.m_grid_fp.delta_x()
		+ g_row*g_escape_time_state.m_grid_fp.delta_x2());
}

// Imaginary component, grid lookup version - requires g_y0/g_y1 arrays
static double dy_pixel_grid()
{
	return g_escape_time_state.m_grid_fp.y_pixel_grid(g_col, g_row);
}

// Imaginary component, calculation version - does not require arrays
static double dy_pixel_calc()
{
	return double(g_escape_time_state.m_grid_fp.y_max() - g_row*g_escape_time_state.m_grid_fp.delta_y() - g_col*g_escape_time_state.m_grid_fp.delta_y2());
}

// Real component, grid lookup version - requires g_x0_l/g_x1_l arrays
static long lx_pixel_grid()
{
	return g_escape_time_state.m_grid_l.x_pixel_grid(g_col, g_row);
}

// Real component, calculation version - does not require arrays
static long lx_pixel_calc()
{
	return g_escape_time_state.m_grid_l.x_min() + g_col*g_escape_time_state.m_grid_l.delta_x() + g_row*g_escape_time_state.m_grid_l.delta_x2();
}

// Imaginary component, grid lookup version - requires g_y0_l/g_y1_l arrays
static long ly_pixel_grid()
{
	return g_escape_time_state.m_grid_l.y_pixel_grid(g_col, g_row);
}

// Imaginary component, calculation version - does not require arrays
static long ly_pixel_calc()
{
	return g_escape_time_state.m_grid_l.y_max() - g_row*g_escape_time_state.m_grid_l.delta_y() - g_col*g_escape_time_state.m_grid_l.delta_y2();
}

void set_pixel_calc_functions()
{
	if (g_escape_time_state.m_use_grid)
	{
		g_externs.SetDxPixel(dx_pixel_grid);
		g_externs.SetDyPixel(dy_pixel_grid);
		g_externs.SetLxPixel(lx_pixel_grid);
		g_externs.SetLyPixel(ly_pixel_grid);
	}
	else
	{
		g_externs.SetDxPixel(dx_pixel_calc);
		g_externs.SetDyPixel(dy_pixel_calc);
		g_externs.SetLxPixel(lx_pixel_calc);
		g_externs.SetLyPixel(ly_pixel_calc);
	}
}

void initialize_pixel_calc_functions(Externals &externs)
{
	externs.SetDxPixel(dx_pixel_grid);
	externs.SetDyPixel(dy_pixel_grid);
	externs.SetLxPixel(lx_pixel_grid);
	externs.SetLyPixel(ly_pixel_grid);
}
