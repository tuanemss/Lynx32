/*
 *  Lynx32 - version
 *
 *  Copyright (c) 2026 tuanem
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "plib.h"
#include "version.h"

/*
 * something like iboot DEBUG
 *
 * \x1b[m      : color reset
 * \x1b[1;5;31m : bold + blink + red color
*/

int cmd_version(int argc, CmdArg *argv)
{
	(void)argc;
	(void)argv;
	printf_("\x1b[1;5;31mLynx32.RELEASE-2026.1.1\x1b[m\n");

	return 0;
}