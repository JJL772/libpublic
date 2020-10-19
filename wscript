#! /usr/bin/env python
# encoding: utf-8
# mittorn, 2018

from waflib import Logs
import os

top = '.'

def options(opt):
	# stub
	return

def configure(conf):
	conf.env.append_unique('DEFINES', 'LIBPUBLIC=1')
	return

def build(bld):
	source = ['crtlib.cpp', 'crclib.cpp', 'appframework.cpp', 'threadtools.cpp', 'keyvalues.cpp', 'containers/string.cpp', 'xprof.cpp', 'platform.cpp',
			  'reflection.cpp', 'mem.cpp', 'logger.cpp', 'debug.cpp', 'cmdline.cpp']
	libs = []
	includes = list()
	includes.append(str(bld.env.COMMON))
	includes.append(str(bld.env.PUBLIC))
	includes.append(str(bld.env.ENGINE))
	includes.append(str(bld.env.ROOT))

	if bld.env.DEST_OS != 'win32':
		libs += ['RT', 'DL', 'PTHREAD']
	else:
		libs += ['USER32', 'SHELL32']
        
	bld(
		source   = source,
		target   = 'public',
		features = 'cxx cxxshlib',
		includes = includes,
		use	  = libs,
		subsystem = bld.env.MSVC_SUBSYSTEM
	)