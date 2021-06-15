#!/usr/bin/env python
import os,sys,glob,stat,json,socket,tempfile,argparse,subprocess,getpass

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

  configurations = None

  if os.getenv('CLON_PARMS') is None:
    print('Warning:  CLON_PARMS undefined, so full path must be specified for -config.')
  elif not os.path.isdir(os.getenv('CLON_PARMS')+'/vnc'):
    print('Warning:  CLON_PARMS/vnc does not exist, so full path must be specified for -config.')
  else:
    configurations = [os.path.basename(x)[0:-5] for x in glob.glob(os.getenv('CLON_PARMS')+'/vnc/*.json')]

  cli=argparse.ArgumentParser(description='Interface to Hall B DAQ VNC Session.')
  cli.add_argument('-config',help='short name or path to configuration file',metavar='PATH',default='daqvnc')
  cli.add_argument('command',choices=['stop','start','connect','list'])
  args=cli.parse_args(sys.argv[1:])

  if args.command == 'list':
    print('Available configurations:')
    print('\n'.join(configurations))

  else:

    config = args.config
    if not os.path.isfile(config):
      if config in configurations:
        config = '%s/vnc/%s.json'%(os.getenv('CLON_PARMS'),config)
      else:
        cli.error('Configuration not found:  '+args.config)

    dv=DaqVnc(config)

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

