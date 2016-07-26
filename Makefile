#
#    touchwho is a program that monitor subfolder files in a folder
#    Copyright (C) 2016  Wei Shuai <cpuwolf@gmail.com>

#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.

#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.

#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.

all: touchwho

touchwho: touchwho.c
	gcc -Wall --static -o touchwho $<

clean: touchwho
	rm -rf $<
