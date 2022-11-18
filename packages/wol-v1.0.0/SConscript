from building import *

cwd     = GetCurrentDir()
CPPPATH = [
    cwd
]
src     = Glob('*.c')


group = DefineGroup('wol', src, depend = ['PKG_USING_WOL'], CPPPATH = CPPPATH)

Return('group')
