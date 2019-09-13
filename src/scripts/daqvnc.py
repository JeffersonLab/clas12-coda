#!/usr/bin/env python
import os,sys,stat,json,socket,tempfile,argparse,subprocess,getpass

DEFAULTS=os.getenv('CLON_PARMS')+'/vnc/daqvnc.json'

class DaqVnc():

  def __init__(self,cfgfile):
    with open(cfgfile,'r') as xx:
      self.cfg=json.load(xx)
    self.pidfile='%s/.vnc/%s:%d.pid'%\
        (os.getenv('HOME'),self.cfg['host'],int(self.cfg['port']))
    self.passwdfile=None
    if getpass.getuser() == self.cfg['user']:
      self.passwdfile='%s/.vnc/passwd'%os.getenv('HOME')

  def startClient(self):
    cmd=['vncviewer']
    if self.passwdfile is not None:
      cmd.extend(['--passwordfile=%s'%self.passwdfile])
    cmd.extend(['-Shared',self.cfg['host']+':'+self.cfg['port']])
    subprocess.call(cmd)

  def killServer(self):
    subprocess.call(['vncserver','-kill',':'+self.cfg['port']])

  def startServer(self):
    if os.path.isfile(self.pidfile):
      sys.exit('\nERROR:  vncserver already running: '+self.pidfile+'\n')
    xx=tempfile.NamedTemporaryFile(mode='wr+b',delete=False)
    xx.write(self.cfg['xstartup'])
    xx.close()
    os.chmod(xx.name,stat.S_IWRITE|stat.S_IREAD|stat.S_IEXEC)
    cmd=['vncserver',\
        '-name',self.cfg['name'],\
        '-geometry',self.cfg['geometry'],\
        '-xstartup',xx.name,\
        ':'+self.cfg['port']]
    subprocess.call(cmd)

if __name__=='__main__':

  cli=argparse.ArgumentParser(description='Interface to Hall B DAQ VNC Session.')
  cli.add_argument('-config',help='path to JSON config (default=%(default)s)',metavar='PATH',default=DEFAULTS)
  cli.add_argument('command',choices=['stop','start','connect'])
  args=cli.parse_args(sys.argv[1:])

  dv=DaqVnc(args.config)
  if args.command=='connect':
    dv.startClient()
  else:
    if getpass.getuser() != dv.cfg['user']:
      sys.exit('\nERROR:  This requires user='+dv.cfg['user']+\
          ', but you are user='+getpass.getuser()+'\n')
    if dv.cfg['host'] != socket.gethostname():
      cmd=['ssh',dv.cfg['host'],'python',os.path.realpath(__file__)]
      cmd.extend(sys.argv[1:])
      subprocess.call(cmd)
      sys.exit()
    if args.command=='stop':
      dv.killServer()
    elif args.command=='start':
      dv.startServer()

