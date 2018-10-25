#!/usr/bin/env python
import sys,socket,argparse,time
DRYRUN=False

######################################################
# Peak PCAN-Ethernet Gateway Protocol
######################################################
# (Network Byte Order, except for CAN data)
# bytes name
# 2     length (fixed, 0x24, includes length field)
# 2     message type (fixed, 0x80)
# 8     tag (unused)
# 4     timestamp low (unused)
# 4     timestamp high (unused)
# 1     channel (unused)
# 1     DLC (data length count in bytes)
# 2     flags (unused)
# 4     CAN ID (0:28=ID, 29=0, 30=RTR, 31=1/0=Ext/Std)
# 8     CAN DATA
######################################################

class PCANEthField():
  def __init__(self,name,nbytes,value):
    self.name=name
    self.nbytes=nbytes
    self.value=value

class WienerCanId():
  def __init__(self,nodeID,subObjectID,rtr):
    self.nodeID=nodeID
    self.subObjectID=subObjectID
    self.rtr=rtr
  def getId(self):
    data = 0
    data |= self.nodeID
    data |= self.subObjectID << 7
    data |= self.rtr<<30
    return data
  def show(self):
    print 'WCANID: %d %d 0x%x'%(self.subObjectID,self.nodeID,self.getId())

class PCANEthFrame():
  def __init__(self):
    self.fields=[]
    self.fields.append(PCANEthField('LENGTH', 2,[0x24]))
    self.fields.append(PCANEthField('TYPE',   2,[0x80]))
    self.fields.append(PCANEthField('TAG',    8,[0]))
    self.fields.append(PCANEthField('TIMELO', 4,[0]))
    self.fields.append(PCANEthField('TIMEHI', 4,[0]))
    self.fields.append(PCANEthField('CHAN',   1,[0]))
    self.fields.append(PCANEthField('DLC',    1,[0]))
    self.fields.append(PCANEthField('FLAG',   2,[0]))
    self.fields.append(PCANEthField('CANID',  4,[0]))
    self.fields.append(PCANEthField('CANDATA',8,[0]))
  def getField(self,name):
    for field in self.fields:
      if field.name==name:
        return field
    return None
  def setCanId(self,value):
    self.getField('CANID').value[0]=value
  def setCanData(self,candata,nbytes):
    self.getField('CANDATA').value[0]=candata[0]
    self.getField('CANDATA').length=nbytes
  def setCanDataLength(self,dlc):
    self.getField('DLC').value[0]=dlc
  def getMessage(self):
    data=[]
    for field in self.fields:
      if field.name=='CANID':
        for ibyte in reversed(range(field.nbytes)):
          data.append('%.2x'%((field.value[0] >> ibyte*8)&0xFF))
      elif field.name=='CANDATA':
        data.append('%.2x'%field.value[0])
        for ibyte in range(1,field.nbytes): data.append('%.2x'%0)
      else:
        # non CAN data is big-endian
        for ibyte in range(1,field.nbytes): data.append('%.2x'%0)
        data.append('%.2x'%field.value[0])
    return data
  def show(self):
    for xx in self.getMessage():
      print '0x%s'%xx,
    print

class PCANWienerMessage():
  def __init__(self,nodeId,subObjectId,rtr,data):
    self.frame = PCANEthFrame()
    self.wienerCanid = WienerCanId(nodeId,subObjectId,rtr)
    self.frame.setCanId(self.wienerCanid.getId())
    self.frame.setCanData(data,8)
    self.frame.setCanDataLength(len(data))
  def show(self):
    print
    self.wienerCanid.show()
    self.frame.show()
  def sendMessage(self,sock):
    if DRYRUN:
      self.show()
    else:
      nsent=0
      for datum in self.frame.getMessage():
        nsent += sock.send(datum.decode('hex'))
      if nsent!=36: print 'Error, #bytes sent =',nsent

def switchOff(sock,canNode):
  rtr,subObjectId,data=0,1,[0x1]
  PCANWienerMessage(canNode,subObjectId,rtr,data).sendMessage(sock)

def switchOn(sock,canNode):
  rtr,subObjectId,data=0,1,[0x3]
  PCANWienerMessage(canNode,subObjectId,rtr,data).sendMessage(sock)

def main():
  global DRYRUN
  parser = argparse.ArgumentParser(description='Power Cycle Wiener Fantray via CANBUS and Peak PCAN-Eth Gateway.')
  parser.add_argument('-d',dest='dryRun',  action='store_true',      default=False, help='dry run (print bytes, do not send)')
  parser.add_argument('-p',dest='port',    action='store', type=int, default=50000, help='Gateway port number')
  parser.add_argument('-n',dest='canNode', action='store', type=int, default=2,     help='CANBUS node id')
  parser.add_argument('--on', dest='switchOff',action='store_false', default=True,  help='Switch ON (default=cycle)')
  parser.add_argument('--off',dest='switchOn', action='store_false', default=True,  help='Switch OFF (default=cycle)')
  parser.add_argument('hostname', help='Gateway hostname')
  args = parser.parse_args()
  DRYRUN = args.dryRun

  try:
    ipaddr = socket.gethostbyname(args.hostname)
  except socket.gaierror:
    print '\nUnknown hostname: '+args.hostname+'\n'
    sys.exit(parser.print_usage())

  sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

  try:
    sock.connect((ipaddr,args.port))
  except socket.error:
    print '\nConnection Error, wrong hostname or port?\n'
    sys.exit(parser.print_usage())

  if args.switchOff:
    switchOff(sock,args.canNode)

  if args.switchOn:
    if not DRYRUN and args.switchOff: time.sleep(5)
    switchOn(sock,args.canNode)

  sock.close()

if __name__=='__main__':
  main()

