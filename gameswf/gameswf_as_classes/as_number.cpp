// as_number.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "gameswf/gameswf_as_classes/as_number.h"
#include "gameswf/gameswf_as_classes/as_string.h"
#include "gameswf/gameswf_log.h"

namespace gameswf
{

	// static builtins methods of Number class

	static const char s_hex_digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVXYZW";

	void as_global_parse_float(const fn_call& fn)
	{
		if (fn.nargs > 0)  
		{
			double res;
			if (string_to_number(&res, fn.arg(0).asCString()))
			{
				fn.result->setNumber(res);
				return;
			}
		}
		fn.result->setNaN();
	}

	void as_global_parse_int(const fn_call& fn)
	{
		if (fn.nargs > 0)
		{
			int res;
			int base = fn.nargs > 1 ? fn.arg(1).asInt() : 10;
			if (string_to_number(&res, fn.arg(0).asCString(), base))
			{
				fn.result->setInt(res);
				return;
			}
		}
		fn.result->setNaN();
	}

	void as_global_isnan(const fn_call& fn)
	{
		if (fn.nargs == 1)  
		{
			if (isnan(fn.arg(0).asNumber()) == false)
			{
				fn.result->setBool(false);
				return;
			}
		}
		fn.result->setBool(true);
	}

	// Number(num:Object)
	void	as_global_number_ctor(const fn_call& fn)
	{
		if (fn.nargs == 1)
		{
			fn.result->setNumber(fn.arg(0).asNumber());
		}	
		else
		{
			fn.result->setNumber(0);
		}
	}

	void	as_number_to_string(const fn_call& fn)
	{
		double number = fn.this_value.asNumber();
		if (fn.nargs >= 1)
		{
			// radix:Number - Specifies the numeric base (from 2 to 36) to use for 
			// the number-to-string conversion. 
			// If you do not specify the radix parameter, the default value is 10.

			tu_string res;
			int val = (int) number;
			int radix = fn.arg(0).asInt();
			if (radix >= 2 && radix <= (int) strlen(s_hex_digits))
			{
				do
				{
					int k = val % radix;
					val = (int) (val / radix);
					tu_string digit;
					digit += s_hex_digits[k];
					res = digit + res;
				}
				while (val > 0);
			}
			fn.result->setString(res);
		}
		else
		{
			// @@ Moock says if value is a NAN, then result is "NaN"
			// INF goes to "Infinity"
			// -INF goes to "-Infinity"
			if (isnan(number))
			{
				fn.result->setString("NaN");
			} 
			else
			{
				char buffer[50];
				snprintf(buffer, 50, "%.14g", number);
				fn.result->setString(buffer);
			}
		}
	}

	void	as_number_valueof(const fn_call& fn)
	{
		assert(0);	//TODO
	}

};
