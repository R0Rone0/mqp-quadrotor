#include <iostream>
#include <stdarg.h>
#include <stdio.h>

//////////////////////////////////////////////////////////////////////////////
// Log it
void doLog(const char * const n_fmt, ...)
{
static int	nlines	= 0;

	va_list	ap;
	va_start(ap, n_fmt);
	if (n_fmt != 0) {
		char	n_work[8192];
		::vsnprintf(n_work, sizeof(n_work)-1, n_fmt, ap);
		std::cout << n_work;

		nlines++;
		if (nlines > 100) {
			std::cout << std::endl;
			nlines	= 0;
		}
		else
			std::cout << "\n";
	}
	va_end(ap);

	return;
}	// doLog()
