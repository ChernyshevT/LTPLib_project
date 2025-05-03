/* Default (OpenMP) backaned
 * _default.cxx run_*.?xx *.py
 * 
 * Copyright 2024 Timofey Chernyshev
 * <thunarux@protonmail.com, chernyshev.tv@ihed.ras.ru>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
 
#include "api_backend.hxx"
#pragma message ("using " API_V)
 
extern "C" {
	LIB_EXPORT const char *descr{"OpenMP"};
	LIB_EXPORT const char *build{__DATE__ " " __TIME__};
	LIB_EXPORT const char *api_v{API_V};
}





