
from building import *
import rtconfig

cwd     = GetCurrentDir()
src  = Glob('src/*.c')
src  += Glob('port/*.c')
CPPPATH = [cwd,cwd+'/port']
LOCAL_CCFLAGS = ''

group = DefineGroup('wolfMQTT', src, depend = [''], CPPPATH = CPPPATH, LOCAL_CCFLAGS = LOCAL_CCFLAGS)

Return('group')
