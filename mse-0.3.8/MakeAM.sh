rm Makefile.am;
echo "
#+----------------------------------------------------------------------------+
#| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
#| Copyright:    (C) 2001 - 2008 Twan van Laarhoven and \"coppro\"            |
#| License:      GNU General Public License 2 or later (see file COPYING)     |
#+----------------------------------------------------------------------------+

# This file is generated by MakeAM.sh. DO NOT EDIT!

# This flag allows us to use subdirectories:
AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = magicseteditor
AM_CXXFLAGS = @WX_CXXFLAGS@ \$(BOOST_CXXFLAGS) -DUNICODE -I . -Wall
AM_LDFLAGS  = @WX_LIBS@ \$(BOOST_LDFLAGS)

magicseteditor_LDADD = \$(BOOST_REGEX_LIB)

magicseteditor_SOURCES =

# The script used to generate is MakeAM.sh " > Makefile.am;

find . -name *.cpp | sed "s/\./magicseteditor_SOURCES += ./" >> Makefile.am;