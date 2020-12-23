from building import *

cwd     = GetCurrentDir()
CPPPATH = [cwd]
src     = Glob('*.c')

group = DefineGroup('ulog_tcp', src, depend = [''], CPPPATH = CPPPATH)

Return('group')
