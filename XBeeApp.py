import XBee_ext
import cherrypy
from cherrypy.lib import static
import threading
import json
import urllib
import urllib2
import time
import sys
import string
import base64
from threading import Thread
import os
import shutil
import zipfile

localDir = os.path.dirname(__file__)
absDir = os.path.join(os.getcwd(), localDir)
# Global Variables
USER = "khol"
PASSWORD = "action12"
#HOST = "http://142.103.25.37/"
HOST ="http://demo.sensetecnic.com/"
SERVER=HOST+'SenseTecnic/api/sensors/'
serialPort = "/dev/ttyS0"
baud = 115200 
timeout = 10000 #10 secs = 10000ms
BroadcastMSB =0x00000000
BroadcastLSB=0x0000FFFF

XBee=XBee_ext.XBeeSystem(baud, serialPort)
XBee.enableDataLogging("Sensor_Logs.txt")

XBeeLock = threading.RLock()

exitCode =0
def registerSensor(sensor, user, password):
    jsonSensor = json.dumps(sensor)
    base64string = base64.encodestring('%s:%s' % (user, password))[:-1]

    headers = {'User-Agent': 'httplib',
        'Content-Type': 'application/json',
        'Authorization': "Basic %s" % base64string
    }
    url = SERVER+sensor["name"]+"?_method=PUT"
                                       

    req = urllib2.Request(url, jsonSensor, headers)
    try:
        urllib2.urlopen(req)
    except urllib2.HTTPError, e:
        if(e.code !=204):
            print '%s while registering: %s' % (e,sensor["name"])
    except urllib2.URLError, e:
        raise SenseTecnicError('URLError while registering %s' % (sensor['name']),e)
    
    return 0



def encodeSensor(Nodes, index):
    global XBee
    global XBeeLock
    
    Node = Nodes.nodes[index]
    sensor = {
              'name':hex(Node.data.SN_High)[2:]+hex(Node.data.SN_Low)[2:],
              'longName':Node.data.Name,
              'description':'This is a  sensor created by a Python script using the REST API.',
              'latitude':49.19438,
              'longitude':-123.45678,
              'private':False,
              "schema":{"fields":
                        [{"name":"lat","type":"NUMBER",
                          "index":0,"required":True,"longName":"latitude",
                          "units":None},
                        {"name":"lng","type":"NUMBER",
                         "index":1,"required":True,
                         "longName":"longitude","units":None},
                         {"name":"DIO0","type":"NUMBER","index":2,
                          "required":True,
                          "longName":"data0","units":"none"},
                          {"name":"DIO1","type":"NUMBER","index":3,
                          "required":True,
                          "longName":"data1","units":"none"},
                          {"name":"DIO2","type":"NUMBER","index":4,
                          "required":True,
                          "longName":"data2","units":"none"},
                          {"name":"DIO3","type":"NUMBER","index":5,
                          "required":True,
                          "longName":"data3","units":"none"},
                          {"name":"DIO4","type":"NUMBER","index":6,
                          "required":True,
                          "longName":"data4","units":"none"},
                         {"name":"DIO5","type":"NUMBER","index":7,
                          "required":True,
                          "longName":"data5","units":"none"},
                         {"name":"DIO6","type":"NUMBER","index":8,
                          "required":True,
                          "longName":"data6","units":"none"},
                         {"name":"DIO7","type":"NUMBER","index":9,
                          "required":True,
                          "longName":"data7","units":"none"},
                          {"name":"DIO10","type":"NUMBER","index":10,
                          "required":True,
                          "longName":"data8","units":"none"},
                          {"name":"DIO11","type":"NUMBER","index":11,
                          "required":True,
                          "longName":"data9","units":"none"},
                         {"name":"DIO12","type":"NUMBER","index":12,
                          "required":True,
                          "longName":"data10","units":"none"},
                         {"name":"RXData1","type":"STRING","index":13,
                          "required":True,
                          "longName":"data11","units":"none"},
                         {"name":"RXData2","type":"STRING","index":14,
                          "required":True,
                          "longName":"data12","units":"none"},
                         {"name":"RXData3","type":"STRING","index":15,
                          "required":True,
                          "longName":"data13","units":"none"},
                          {"name":"RXData4","type":"STRING","index":16,
                          "required":True,
                          "longName":"data14","units":"none"}
                       
                         ]
                    }
              }
    return sensor # return the sensor object

def sendData(params, name, user, password):

    # send authorization headers premptively otherwise we get redirected to a login page
    base64string = base64.encodestring('%s:%s' % (user, password))[:-1]
        
    headers = {
        'User-Agent': 'httplib',
        'Content-Type': 'application/x-www-form-urlencoded',
        'Authorization': "Basic %s" % base64string
    }
    
    url = SERVER +name+"/data"
    req = urllib2.Request(url,params,headers)
    try:
        urllib2.urlopen(req)
    except urllib2.URLError, e:
        print 'error - sending event to sensor: %s' % (name)
        print "%s"% e
        #return -1
    return 0

def deleteSensor(sensorName, user, password):
    
    # send authorization headers premptively otherwise we get redirected to a login page
    base64string = base64.encodestring('%s:%s' % (user, password))[:-1]
        
    headers = {
        'User-Agent': 'httplib',
        'Authorization': "Basic %s" % base64string
    }
    
    url = SERVER+sensorName+"?_method=DELETE"
    #dummy data to force urllib2 to use a POST
    req = urllib2.Request(url,"dummy", headers)
    try:
        urllib2.urlopen(req)
    except urllib2.HTTPError, e:
        if (e.code != 204):
            print '%s while deleting sensor: %s' % (e,sensorName)
               
    except urllib2.URLError, e:
        print 'error - delete sensor: %s' % (sensorName)
    
    return 0


def XBeeClientFunction(b):
    global XBee
    global XBeeLock
    global exitCode
    

    while exitCode==0:
        
        Queue = XBee.get_regQ() # copy the req Q into this system thread
        PreviousNodes = XBee.getSensors()
        if Queue.size()>0:
            for i in range(0,Queue.size()):
            ## Register The New Sensors 
                sensor = encodeSensor(Queue,i)
                registerSensor(sensor,USER,PASSWORD)
                XBee.pop_regQ()
                time.sleep(2)

        Nodes = XBee.getSensors()# get the Sensors that are in the system
    # if there are any sensors, send the newest data to sense tecnic
        if Nodes.size()>0:
            for i in range(0,Nodes.size()):
                if i==PreviousNodes.size():
                    break
                if Nodes.nodes[i].data.lastSample!=PreviousNodes.nodes[i].data.lastSample:
                    params ="lat=49.22149&lng=-123.15856&"
                    params+=XBee.printAnalog(i,"&")
                    params+=XBee.printDigital(i,"&")
                    if Nodes.nodes[i].data.rxDataSize()>0:
                        rxString=""
                        for k in range(0,Nodes.nodes[i].data.rxDataSize()):
                            rxString+=chr(Nodes.nodes[i].data.rxData[k])
                        params+="RXData1="+rxString+"&"
                        print rxString
                    params+="timestamp="+str(int(Nodes.nodes[i].data.lastSample))
                    print params
                    name = hex(Nodes.nodes[i].data.SN_High)[2:]+hex(Nodes.nodes[i].data.SN_Low)[2:]
                    sendData(params,name,USER,PASSWORD)
                    time.sleep(10)
            PreviousNodes=Nodes
        if exitCode==1:
            break
    
    print "Exiting Client Thread"
    return 0

class Page:

    def header(self,title):
        colour1="CCCCCC"
        colour2="CCCCCC"
        colour3="CCCCCC"
        colour4="CCCCCC"
        coloura =""
        colourb=""
        colourc=""
        colourd=""
        if title=="Main-Page":
            colour1="993333"
            coloura="white"
        if title=="Config-Page":
            colour3="993333"
            colourc="white"
        if title=="View-Sensor-Page":
            colour2="993333"
            colourb="white"
        if title=="Log-Page":
            colour4="993333"
            colourd="white"
        
            
        return '''
        <!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
        <html>
        <head>
        <title>%s</title>
        <style type='text/css'>
        body {background-color: #FFFFFF; margin: 15px 15px 15px 15px; text-align:center}
        h1{font-family: arial, verdana, sans-serif;}
        h2 {font-family: arial,veranda, sans-serif;}
        a:hover{color:993333;text-decoration:none;}
        a:visited{color:993333;}
        
        </style>
        
        
        <script>
            function submitform()
            {
              document.getElementById("select").submit();
            }
        </script>

        <script type="text/javascript">
        <!--
        function getValue(){
        document.getElementById("send data").value = prompt("Enter your Message Here: ", "");
       
        }
        //-->
        </script>
        
       <script>
        function printMSG()
        {
         alert(document.getElementById("msg").value)
        }
        </script>

        <script type="text/javascript"><!--
            function getConfirmation()
            {
            var msg =document.getElementById("remove").value;
             var retVal = confirm("Are you sure want to " + msg+"?");
           if( retVal == true ){
           document.getElementById("removing").value=true;
           return true;
           }
           else{
      return false;
      }
       }
        //--></script>

        <script type="text/javascript"><!--
            function getServer()
            {
            var msg = document.getElementById("server").value;
            var x= "                                ";
            var value=prompt(x+"Enter Server URL"+x, msg);
            if(value!=null){
                document.getElementById("server").value =value;
                }
      }
        //--></script>
        
        <t style="float:right"> | </t><a style="margin-top:0;float:right;" href='http://www.magic.ubc.ca/wiki/pmwiki.php/ZigbeeSensorGateway/ZigbeeSensorGateway'>
        help</a><t style="float:right"> |</t>
        <table  cellspacing="0" cellpadding="4" bgcolor="#EEEEEE" border="0"><tr><td>
        <table  cellspacing="0" cellpadding="1" border="0" bgcolor="#ffffff">
        <tr><td><a style="margin-top:0;float:right;decoration:none" href='http://demo.sensetecnic.com/SenseTecnic/'>
        <img style  src="http://demo.sensetecnic.com/SenseTecnic/images/logo-med.png"
            alt='#########'></a></td></tr></td></tr></td></tr>
        </table>
        </td></tr></table>
        <div id="tab1" style="margin-top:-20px;background-color:#%s;min-height:20px;width:175px;position:absolute;left:200px;
        cell-padding:5px">
        <a style="margin:0 auto;color:%s" href='./'>Sensors Page</a>
        </div>
        <div id="tab2" style="margin-top:-20px;background-color:#%s;min-height:20px;width:175px;position:absolute;left:390px;
        cell-padding:5px">
        <a style="margin:0 auto;color:%s" href='./view'>Sensor Details</a>
        </div>
        <div id="tab3" style="margin-top:-20px;background-color:#%s;min-height:20px;width:175px;position:absolute;left:580px;
        cell-padding:5px">
        <a style="margin:0 auto;color:%s" href='./config'>Remote Config</a>
        </div>
        <div id="tab4" style="margin-top:-20px;background-color:#%s;min-height:20px;width:175px;position:absolute;
        left:770px;cell-padding:5px">
        <a style="margin:0 auto;color:%s" href='./logs'>Data Logs</a>
        </div>
        
    
       
    
        <div id="container" style="background-color:#EEEEEE;min-width:900px;float:center;margin-right:-10px;
        margin-left:-10px">
        <div id="header" style="background-color:#000000;min-width:900px;min-height:35px;float:center;">
        <t style="margin-top:10px;margin-left:20px;color:white;float:left">ZigBee Gateway
        </t></div>
        <div id="horizontal-line" style="margin-bottom:0;background-color:#FF9900;height:5px;min-width:900px;float:center;"></div>
        <div id="menu" style="background-color:#EEEEEE;width:195px;min-height:500px;float:left;">
        </head>
       '''%(title,colour1,coloura,colour2,colourb,colour3,colourc,colour4,colourd)


    def footer(self):
        return '''
            <div id="footer" style="background-color:#202020;clear:both;min-width=800px;text-align:center;float:center;
            margin-left:-10;margin-right:-10;">
                   <font color='FFFFFF'> WotKit.com 2012 </font></div></div></body>
                 </html>'''
        
        
class MainPage(Page):
    exposed=True

    def page(self):
        global XBee
        global XBeeLock
        
        #XBeeLock.acquire()
        Nodes=XBee.getSensors()
        coordName = XBee.getCoordName() # get the coordinators Node ID
        coordSN = hex(XBee.getCoordSN_MSB())[2:] +hex(XBee.getCoordSN_LSB())[2:]# get the Coord Serial Numbers and convert them to a hex string
        #XBeeLock.release()
        yield self.header("Main-Page")
        yield'''
        <body >
        <form id="select" action='./view' method="GET">
            <select onchange="submitform()" name="name" >'''
        for j in range(0,Nodes.nodes.size()):
            yield'''<option value="view%d">%s%s
                </option>'''%(j,hex(Nodes.nodes[j].data.SN_High)[2:],hex(Nodes.nodes[j].data.SN_Low)[2:])

        yield'''</select>
           
        <input id="select" type='submit' align="center" value='Select'></form>

        <br>
        <p id="config" style="margin-left:10px;text-align:left;font:15 arial;">
        <font color="#000000">
        <b><font color="#993333"><br>
        Sensors Registered</b></font><br>
        <h3 align='center' style="margin-left:-10px;text-align:center;font:15 arial;>
        <font color="#00000">%d</font></h3></p>
        <br>
        <p  style="margin-left:10px;text-align:left;font:15 arial;">
        <font color="#993333">
        <br><b>
        Upload Server</b></font>
        <form action="" method="POST">
        <input id="server" type='text' size='18' name="server" value="%s">
        <input id="getserver" type='submit' onclick="getServer();" value="Update">
        </form>
        <br>
        </div>
        <div id="table" style="background-color:#EEEEEE;min-height:500px;align:left;">
        <table id="outerTable" style ="border-left:5px solid;border-color:#FF9900;background-color:#EEEEEE;
        height:550px;cell-padding=0px;margin-top=-5px;margin-left=-5px;">
        <tr align=left><td valign=top style="min-width:625px;">
        <table id="innerTable" align="center" style ="cell-padding=0px;border:0px solid;border-color:#CCCCCC;
        background-color:#EEEEEE;margin-top:-3px;margin-left:-3px;margin-right:-3px;min-width:420px;float:center;">
        <tr style='background-color:#FFA500'>
        <th  width=''>Index</th>
        <th width='150'>Sensor</th>
        <th width='250'>Name</th>
        <th width='100'>Status</th>
        <th width="100">View</th>
        <th width=""></th>
        <th width=""></th>
        </tr>
        <tr align='center' style='background-color:#CCCCCC;'>
        <td>-</td>'''%(Nodes.nodes.size(),SERVER)
        yield'''
        <td>%s</td>
        <td>%s</td>
        <td>Active</td>
        <td>
        <form name='input0' action='view' method='GET'>
        <input type="hidden" name="name" value="viewlogs">
        <input type='submit' value='LOGS'></form>
        </td>
        <td><form name='input1' action='' method='POST'>
        '''%(coordSN, coordName)
        if XBee.checkLoggingStatus()==1:
            yield'''
            <input type="hidden" name="enableLogs" value="false">
            <input type='submit' value='DISABLE LOGS'></form></td>'''
        else:
            yield'''
        <input type="hidden" name="enableLogs" value="true">
        <input type='submit' value='ENABLE LOGS'></form></td>'''

        yield'''
        <td>
    <form name='input2' action='' method='POST'>
        <input type="hidden" name="discover" value="discover">
        <input type='submit' value='DISCOVER'></form></td>
        </tr>'''
    # get an updated list of the sensors each time the page is called
        #XBeeLock.acquire()
        Nodes = XBee.getSensors()
        #XBeeLock.release()
        ## for each sensor on the network, populate the rest of the table
        if Nodes.size()>0:
            for i in range (0, Nodes.size()):
                yield '''
                <tr align='center' style='background-color:#CCCCCC;'>
                <td>%d</td>
                <td>%s%s</td>
                <td>%s</td>'''%(i,hex(Nodes.nodes[i].data.SN_High)[2:],hex(Nodes.nodes[i].data.SN_Low)[2:],Nodes.nodes[i].data.Name)
        
                if Nodes.nodes[i].data.timeSinceLastSample>60000:
                    yield'''<td>Inactive</td>'''
                else:
                    yield '''<td>Active</td>'''         
                yield '''
                <td>
                <form name="input1" action='view' method='get'>
                <input type="hidden" name="name" value="view%d">
                <input type='submit' value='VIEW'></form>
                </td>
                <td>
                <form name="input2" action='config' method='get'>
                <input type="hidden" name="name" value="config%d">
                <input type='submit' value='CONFIG'></form>
                </td>
                <td>
                <form name="input3" action='' method='POST'>
                <input type='hidden' name='delete' value=%d>
                <input type='submit' value='DELETE'></form>
                </td>
                </tr>'''%(i,i,i)

        yield'''</table></tr></td></table></div></div>'''

        '''<div id="footer" style="background-color:#202020;clear:both;text-align:center;">
                   <font color='FFFFFF'> WotKit.com 2012 <font color='000000'></div></div>'''

            
        yield self.footer()

    def GET(self):
        return self.page()

    def POST(self,delete=None, i=None, discover=None,enableLogs=None, server=None):
        global XBee
        global XBeeLock
        global SERVER
        if server:
            if server != SERVER:
                SERVER=server
        if discover:
            XBee.doNodeDiscovery(1000);
        if enableLogs:
            if enableLogs=="true":
                XBee.enableDataLogging("Sensor_Logs.txt")
            if enableLogs=="false":
                XBee.disableDataLogging()
        
        if delete:
            #XBeeLock.acquire()
            Nodes=XBee.getSensors()
            Name=hex(Nodes.nodes[int(delete)].data.SN_High)[2:] +hex(Nodes.nodes[int(delete)].data.SN_Low)[2:]
            deleteSensor(Name, USER, PASSWORD)
            XBee.deleteSensor(int(delete))
            
            #XBeeLock.release()

        return self.page()
                
                
                 
                    


class ConfigPage(Page):
    global XBee
    global XBeeLock

    ##def index(self):
      ##  return
   # index.exposed=True
    exposed = True


    
    def page(self, i=0, msg=None):

        global XBee
        global XBeeLock
        
        #XBeeLock.acquire()
        localNodes = XBee.getSensors()
        
        #XBeeLock.release()
        ##(Nodes.nodes[i].data.sn_high,Nodes.nodes[i].data.sn_low)
       #Nodes.nodes[i].data.sn_high
        yield self.header("Config-Page")
        if msg:
            yield '''
            <input id="msg" type="hidden" value="%s"></input><body onload="printMSG()">'''%msg
        yield'''
            <form id="select" action='' method="GET">
            <select onchange="submitform()" name="name" >'''
        for j in range(0,localNodes.nodes.size()):
            if i==j:
                yield '''submitform()<option value="config%d" selected="selected">%s%s
            </option>'''%(i,hex(localNodes.nodes[i].data.SN_High)[2:],hex(localNodes.nodes[i].data.SN_Low)[2:])
            else:
                yield'''<option value="config%d">%s%s
                </option>'''%(j,hex(localNodes.nodes[j].data.SN_High)[2:],hex(localNodes.nodes[j].data.SN_Low)[2:])

        yield'''</select>
           
            <input id="select" type='submit' align="center" value='Select'></form>
            <form name='input1' action='view' method='GET' id="input1"><br>
        <input type ='hidden' name ="name" value="view%d">
            <input type='submit' align="center" value='View Sensor'></form>'''%(i)

        yield '''
            <form name="input3" action='' method="POST">
            <input type ='hidden' name ="write" value="write%d">
            <input style="position:absolute; top:725px; left:40px" type='submit'  value='Write Changes'></form>
            '''%(i)
        analog = XBee.printAnalog(i,":")
        digital =XBee.printDigital(i,":")
        start=0
        yield '''
            <p id="config" style="margin-left:40px;text-align:left;font:15 arial;">
            <font color="#000000">'''
        while True:
            start = analog.find("DIO",start)
            if start ==-1:
                break
            start = start+len("DIO")
            end =analog.find("=",start)
            pin = int(analog[start:end])
            start = end +len("=")
            # next we can get the value 
            end = analog.find(":", start)
            value = analog[start:end]
            start = end
            yield'''
                <b><font color="#993333">DIO%d: </b></font><br>
                        ADC
                    %s
                    <br>
                    '''%(pin,value)

        start =0
        while True:
            start = digital.find("DIO",start)
            if start ==-1:
                 break
            start = start+len("DIO")
            end = digital.find("=",start)
            pin = int(digital[start:end])
            start =end+len("=")
            end = digital.find(":", start)
            value = int(digital[start:end])
            start =end
            if value == 0:
                value = "Low"
            elif value ==1:
                value="High"
            yield '''
                     <b><font color="#993333">DIO%d:</b></font><br>
                    Digital
                    %s
                    <br>
                    '''%(pin,value)
        yield'''
            </p>
            <form input=input2 action='' method='POST' id=input2>
            <input align='center' type='submit' value=
            'Update Fields'>
            <br>
            <input align='center' type='reset' value=
            'Reset Fields'></div>'''
        yield '''
            <div id="table" style="background-color:#EEEEEE;align:left;">'''
        yield '''
            <table  style ="align-center;border-left:5px solid;border-color:#FF9900;background-color:#EEEEEE;
            margin-right:0px">
            <tr style='background-color:#FFA500'>
            <th width="200">Item</th>
            <th width="320">Value</th>
            <th width="">Details</th>
            </tr>
            <tr align='center' style='background-color:#CCCCCC;'>
            <th>NodeID</th>
            '''
        yield'''
            <td height="35px"><input type='text' name='nodeID' value=%s></td>
            <td height="35px"><font size='2'>Changes the Node Identifier String (max 20
            chars)</font></td> 
            </tr>'''% (localNodes.nodes[i].data.Name)
        yield '''
            <tr align='center' style='background-color:#CCCCCC;'>
            <th>IO Sample Rate</th>
            <td height="35px"><input type='text' name='sampleRate' value=%d></td>
            <td height="35px"><font size='2'>Enter IO sample rate in msec (max
            65000 msec)</font></td>
            </tr>'''%(localNodes.nodes[i].data.sampleRate)
        if (localNodes.nodes[i].data.deviceType)==2:
            yield'''
            <tr align='center' style='background-color:#CCCCCC;'>
            <th>Sleep Enabled</th>
            <td height="35px"> <input type='radio' name='sleep' value='1'> Disable</td>
            <td height="35px"><font size='1' Disables Sleep Mode <br> Warning! Response time may vary <font size='2'>
            </font></td>
            </tr>'''
            yield'''
            <tr align='center' style='background-color:#CCCCCC;'>
            <th height="35px">Sleep Rate</th>
            <td height="35px"><input type='text' maxlength='8' size='8' name='sleepRate'
            value=%d></td>
            <td height="35px"><font size='2'>Adjusts time the device will sleep periodically 
             (times 10 msec)</font></td>
            </tr>'''%(localNodes.nodes[i].data.sleepRate)
            yield'''
            <tr align='center' style='background-color:#CCCCCC;'>
            <th >Time Until Sleep</th>
            <td height="35px"><input type='text' maxlength='8' size='8' name='timeUntilSleep'
            value=%d></td>
            <td height="35px"><font size='2'>Adjust the length of time a device will
            remain awake until next power down (msec)(</font></td>
            </tr>''' % (localNodes.nodes[i].data.timeUntilSleep)
        else:
            yield'''
            <tr align='center' style='background-color:#CCCCCC;'>
            <th>Sleep Disbled</th>
            <td height="35px"><input type='radio' name='sleep' value='2'> Enable</td>
            <td height="35px"><font size='2'>Enables Sleep Mode <br>
            Warning! Enabling sleep may Orphan nodes connected to this device</font></td>
            </tr>'''
        yield '''
            <tr align='center' style='background-color:#CCCCCC;'>
            <th>DIO0</th>
            <td height="35px"><font size='2'>
            <input type='radio' name='DIO0' value="1">CB
            <input type='radio' name='DIO0' value="02">
            ADC<input type='radio' name='DIO0' value="05"> DH<input type=
            'radio' name='DIO0' value="04"> DL <input type='radio' name='DIO0'
            value="03"> DI <input type='radio' name='DIO0' value="0">
            OFF</font></td>
            <td><font size='2'>Configures DIO0 of Remote XBee<br>CB=Commissioning Button</font></td>
            </tr>'''
        yield '''
            <tr align='center' style='background-color:#CCCCCC;'>
            <th>DIO1</th>
            <td height="35px"><font size='2'><input type='radio' name='DIO1' value="02">
            ADC<input type='radio' name='DIO1' value="0005"> DH<input type=
            'radio' name='DIO1' value="0004"> DL <input type='radio' name='DIO1'
            value="0003"> DI <input type='radio' name='DIO1' value="0000">
            OFF</font></td>
            <td><font size='2'>Configures DIO1 of Remote XBee</font></td>
            </tr>'''
     
        yield'''
            <tr align='center' style='background-color:#CCCCCC;'>
            <th>DIO2</th>
            <td height="35px"><font size='2'><input type='radio' name='DIO2' value="0002">
            ADC<input type='radio' name='DIO2' value="05"> DH<input type=
            'radio' name='DIO2' value="04"> DL <input type='radio' name='DIO2'
            value="03"> DI <input type='radio' name='DIO2' value="0000">
            OFF</font></td>
            <td height="35px"><font size='2'>Configures DIO2 of Remote XBee</font></td>
            </tr>'''
        yield'''
            <tr align='center' style='background-color:#CCCCCC;'>
            <th>DIO3</th>
            <td height="35px"><font size='2'><input type='radio' name='DIO3' value="02">
            ADC<input type='radio' name='DIO3' value="05"> DH<input type=
            'radio' name='DIO3' value="04">DL<input type='radio' name='DIO3'
            value="03"> DI <input type='radio' name='DIO3' value="00">
            OFF</font></td>
            <td height="35px"><font size='2'>Configures DIO3 of Remote XBee</font></td>
            </tr>'''
        yield'''
            <tr align='center' style='background-color:#CCCCCC;'>
            <th>DIO4</th>
            <td height="35px"><font size='2'><input type='radio' name='DIO4' value="05">
            DH<input type='radio' name='DIO4' value="04"> DL <input type=
            'radio' name='DIO4' value="03"> DI <input type='radio' name='DIO4'
            value="00"> OFF</font></td>
            <td height="40px"><font size='2'>Configures DIO4 of Remote XBee</font></td>
            </tr>'''
        yield'''
            <tr align='center' style='background-color:#CCCCCC;'>
            <th>DIO5</th>
            <td height="35px"><font size='2'>
            <input type='radio' name='DIO5' value="01">AI
            <input type='radio' name='DIO5' value="05">
            DH<input type='radio' name='DIO5' value="04"> DL <input type=
            'radio' name='DIO5' value="03"> DI <input type='radio' name='DIO5'
            value="00"> OFF</font></td>
            <td height="40px"><font size='2'>Configures DIO5 of Remote XBee<br>AI=Association Indicator</font></td>
            </tr>'''
        yield'''
            <tr align='center' style='background-color:#CCCCCC;'>
            <th>DIO6</th>
            <td height="35px"><font size='2'>
            <input type='radio' name='DIO6' value="05">
            DH<input type='radio' name='DIO6' value="04"> DL <input type=
            'radio' name='DIO6' value="03"> DI <input type='radio' name='DIO6'
            value="00"> OFF</font></td>
            <td height="40px"><font size='2'>Configures DIO6 of Remote XBee</font></td>
            </tr>'''
        yield'''
            <tr align='center' style='background-color:#CCCCCC;'>
            <th>DIO7</th>
            <td height="35px"><font size='2'><input type='radio' name='DIO7' value="05">
            DH<input type='radio' name='DIO7' value="04"> DL <input type=
            'radio' name='DIO7' value="03"> DI <input type='radio' name='DIO7'
            value="00"> OFF</font></td>
            <td height="40px"><font size='2'>Configures DIO7 of Remote XBee</font></td>
            </tr>'''
        yield'''
            <tr align='center' style='background-color:#CCCCCC;'>
            <th>DIO10</th>
            <td height="35px"><font size='2'><input type='radio' name='DIO10' value="01">
            PWM<input type='radio' name='DIO10' value="05"> DH<input type=
            'radio' name='DIO10' value="04"> DL <input type='radio' name=
            'DIO10' value="03"> DI <input type='radio' name='DIO10' value="00">
            OFF</font></td>
            <td height="40px"><font size='2'>Configures DIO10 of Remote XBee <br> PWM=RSSI/PWM on Pin 10</font></td>
            </tr>'''

        yield'''
            <tr align='center' style='background-color:#CCCCCC;'>
            <th>DIO11</th>
            <td height="35px"><font size='2'><input type='radio' name='DIO11' value="05">
            DH<input type='radio' name='DIO11' value="04"> DL <input type=
            'radio' name='DIO11' value="03"> DI <input type='radio' name=
            'DIO11' value="00"> OFF</font></td>
            <td><font size='2'>Configures DIO11 of Remote XBee</font></td>
            </tr>'''
        yield'''
            <tr align='center' style='background-color:#CCCCCC;'>
            <th>DIO12</th>
            <td height="35px"><font size='2'><input type='radio' name='DIO12' value="05">
            DH<input type='radio' name='DIO12' value="04"> DL <input type=
            'radio' name='DIO12' value="03"> DI <input type='radio' name=
            'DIO12' value="00"> OFF</font></td>
            <td height="35px"><font size='2'>Configures DIO12 of Remote XBee</font></td>
            </tr>
            <tr align='center' style='background-color:#CCCCCC;'>

            <th>Custom Command</th>
            <td height="35px">Command =<input type='text' name='command' value='' maxlength='2' size='2'>
            <br>Value = <input type='text' name='commandValue' value='' maxlength='8' size='8'></td>
            <td height="35px"><font size='2'>Send a Custom Command to Remote XBee</font></td> 
            </form>
            </table></div></div>'''
       
        
        yield self.footer()


    
    def GET(self,name=None):
        i =0
        Nodes = XBee.getSensors()
        if Nodes.nodes.size()<=0:
            return '''<body onload="printMSG()">%s
            </div>
            <div id="veritcal-line" style="margin-bottom:0;background-color:#FF9900;width:5px;min-height:500px;float:left;">
            </div>
            <div id="middle" style="margin-bottom:0;background-color:#EEEEEE;min-height:500px;float:center;">
            <br><br><h1>No Sensors to Configure</h1><a href="./">return to main</a></div></div>
            %s'''%(self.header("Config-Page"),self.footer())
        if name:
            if name[0:6] =="config":
                i = int(name[6:len(name)])
                return self.page(i)
       
        return self.page(i)
            

    def POST(self,name=None,nodeID=None, DIO0=None,
         DIO1=None,DIO2=None,DIO3=None,DIO4=None,DIO5=None,DIO6=None,
             DIO7=None,DIO10=None,DIO11=None,DIO12=None,command=None, commandValue=None,
              sampleRate=None, sleep=None, sleepRate =None, timeUntilSleep=None,
             write=None):


        global XBee
        global XBeeLock
        i=0
        msg=""
        if name:
            if name[0:6] =="config":
                i = int(name[6:len(name)])
        #XBeeLock.acquire()
        if DIO0:
            if(int(DIO0)==1):
                XBee.changePinMode(i,"D0", 0,5000); # need to disable pin first before assinging CB
            if XBee.changePinMode(i,"D0",int(DIO0),5000)==1:
                    msg+="successfully set DIOO"
        if DIO1:
            success = XBee.changePinMode(i,"D1",int(DIO1),5000);
            if success==1:
                msg+= "successfully set DIO1 "
        if DIO2:
            success =XBee.changePinMode(i,"D2",int(DIO2),5000);
            if success ==1:
                msg+= "successfully set DIO2 "
        if DIO3:
            success = XBee.changePinMode(i,"D3",int(DIO3),5000);
            if success ==1:
                msg+= "successfully set DIO3 "
        if DIO4:
            success=XBee.changePinMode(i,"D4",int(DIO4),5000);
            if success ==1:
                msg+= "successfully set DIO4 "
        if DIO5:
            success=XBee.changePinMode(i,"D5",int(DIO5),5000);
            if success ==1:
                msg+= "successfully set DIO5 "
        if DIO6:
            success=XBee.changePinMode(i,"D6",int(DIO6),5000);
            if success ==1:
                msg+= "successfully set DIO6 "
        if DIO7:
            success=XBee.changePinMode(i,"D7",int(DIO7),5000);
            if success ==1:
                msg+= "successfully set DIO7 "
        if DIO10:
            success=XBee.changePinMode(i,"P0",int(DIO10),5000);
            if success ==1:
                msg+= "successfully set DIO10 "
        if DIO11:
            success=XBee.changePinMode(i,"P1",int(DIO11),5000);
            if success ==1:
                msg+= "successfully set DIO11"
        if DIO12:
            success=XBee.changePinMode(i,"P2",int(DIO12),5000);
            if success ==1:
                msg+= "successfully set DIO12 "
        if nodeID:
            if XBee.changeName(i,str(nodeID))==1:
                msg+="successfully set NodeID "
        if sampleRate:
            print sampleRate
            success = XBee.changeSampleRate(i,int(sampleRate),5000)
            if success==1:
                msg+="successfully set sampleRate "
        if sleep:
            print sleep
            success = XBee.changeSleepMode(i,int(sleep),5000)
            print '%d'%success
            if success==1:
                msg+="Successfully changed SleepMode "
        if sleepRate:
            success = XBee.changeSleepRate(i,int(sleepRate))
            if success==1:
                msg+="successfully changed sleep rate "
        if timeUntilSleep:
            success=XBee.changeTimeUntilSleep(i,int(timeUntilSleep))
            if success==1:
                msg+="successfully changed time until sleep "
        if command:
            if commandValue:
                if XBee.RemoteATSet(i,str(command),int(commandValue),5000)==1:
                    msg+="successfully send command %s with value %s"%(command,commandValue)
        if write:
            if XBee.Write(i)==1:
                msg+="succesfully wrote values to non-volatile memory"
        XBee.forceSample(i,5000)
        #XBeeLock.release()
        return self.page(int(i),msg)
        
    #processPost.exposed=True


class LogPage(Page):
    exposed =True
    def page(self,i):
        global XBee

        Nodes=XBee.getSensors()
        yield self.header("Log-Page")

        yield '''<body><form id="select" action="./logs" method="GET">
            <select onchange="submitform()" name="name" >
            <option value="%s">%s'''%("","All")
        for j in range(0,Nodes.nodes.size()):
            if(i==j):
                yield'''<option value="view%d" selected="selected">%s%s
                </option>'''%(j,hex(Nodes.nodes[j].data.SN_High)[2:],hex(Nodes.nodes[j].data.SN_Low)[2:])
            else:
                yield'''<option value="%d">%s%s
                </option>'''%(j,hex(Nodes.nodes[j].data.SN_High)[2:],hex(Nodes.nodes[j].data.SN_Low)[2:])
        yield'''</select>
        <input id="select" type='submit' align="center" value='Select'></form><br><br>
        <br>
        <form id="archive" action="" method="POST">
        <input type="hidden" name="archive" value="true" >
        <input type ="submit" align="center" value="Archive"></form>'''
        if XBee.checkLoggingStatus()==1:
            yield '''
        <br><form id="disable" action="" method="POST">
        <input type="hidden" name="disable" value="true">
        <input type ="submit" align="center" value="Disable Logging">
        </form>
        <br>'''
        else:
            yield '''
        <br><form id="enable" action="" method="POST">
        <input type="hidden" name="enable" value="true">
        <input type ="submit" align="center" value="Enable Logging">
        </form>
        <br>'''
        yield'''
        <br><br>
        <a style="text-decoration: none" href= './logs/download'>
        <input type ="Button" align="center" value="Download Logs"></a>
        <br>
        <br>
        <br>
        <form id="clear" action="" method="POST">
        <input type="hidden" name="clear" value="true">
        <input type ="submit" align="center" value="Clear Logs">
        </form>
        <br>
        </div>
        <div id="vertical-line" style="background-color:#FF9900;width:5px;min-height:550px;float:left;"></div>
        <div id="logs" style="background-color:#EEEEEE;height:550px;align:left;overflow:scroll;float:center">'''
        file=open("Sensor_Logs.txt")
        #print file.read()
        for line in file:
            if(i>-1):
                t=line
                start =t.find("13a200;",40)
                start = start+len("13a200;")
                end = t.find(';',start)
                id = (t[start:end])
                if id==hex(Nodes.nodes[i].data.SN_Low)[2:]:
                    yield '''<p id="logdata" style="text-align:left;font-size:11px;font:courier light;">%s<br></p>'''%line
            else:
                 yield'''<p id="logdata" style="text-align:left;font-size:11px;font:courier light;">%s<br></p>'''%line

                
        yield'''</div>'''
        yield self.footer();


    def GET(self ,name=None):
        i=-1
        if name:
            i = int(name)
        return self.page(i);
    def POST(self, name=None, clear=None, archive=None, disable=None, enable=None, download=None):
        i=-1
        if name:
            i = int(name)
        if archive:
            archiveName="Sensor_Logs_%s"%(time.ctime(time.time()))+".txt"
            for filename in os.listdir("."):
               if filename =="Sensor_Logs.txt":
                   shutil.copyfile(filename,archiveName)
                   
            import zipfile
            try:
                import zlib
                compression = zipfile.ZIP_DEFLATED
            except:
                compression = zipfile.ZIP_STORED


            zf = zipfile.ZipFile('Sensor_Logs.zip', mode='a')
            try:
                zf.write(archiveName,compress_type=compression)
            finally:
                zf.close()
                os.remove(archiveName)

            
            #for filename in os.listdir("."):
               #if filename =="Sensor_Logs.txt":
                   #shutil.copyfile(filename, "Logs_%s"%(time.ctime(time.time()))+".txt")

        if disable:
            XBee.disableDataLogging()
        if enable:
            XBee.enableDataLogging("Sensor_Logs.txt");
        if clear:
            shutil.copyfile("Sensor_Logs.txt", "Logs_%s"%(time.ctime(time.time()))+".txt")
            open("Sensor_Logs.txt","w")
           
        return self.page(i);

    
class SensorViewPage(Page):
    exposed = True
    def page(self, i=0, edit=-1):
        global XBeeLock
        global XBee
        inactiveTime=60000
        #XBeeLock.acquire()
        Nodes =XBee.getSensors()
        #XBeeLock.release()
        
        yield self.header("View-Sensor-Page")
        yield'''<body>
        <form id="select" action='./view' method="GET">
            <select onchange="submitform()" name="name" >'''
        for j in range(0,Nodes.nodes.size()):
            if(i==j):
                yield'''<option value="view%d" selected="selecte">%s%s
                </option>'''%(j,hex(Nodes.nodes[j].data.SN_High)[2:],hex(Nodes.nodes[j].data.SN_Low)[2:])
            else:
                yield'''<option value="view%d">%s%s
                </option>'''%(j,hex(Nodes.nodes[j].data.SN_High)[2:],hex(Nodes.nodes[j].data.SN_Low)[2:])

        yield'''</select>
        <input id="select" type='submit' align="center" value='Select'></form>
            <p id="view" style="margin-left:5px;text-align:left;font:15 arial;"<br><b><font color="#993333">Serial Number:</font></b><br>%X%X<br>
            <b><font color="#993333">Node ID:</b></font><br>%s<br>
            <b><font color="#993333">Network Address:</font></b><br>%X<br>
            <b><font color="#993333">Joined:</font></b><br>%s<br>'''%(
                Nodes.nodes[i].data.SN_High,Nodes.nodes[i].data.SN_Low,
                                 Nodes.nodes[i].data.Name,
                                 Nodes.nodes[i].data.Address16,
                                    time.ctime(int(Nodes.nodes[i].data.joined)))
        if Nodes.nodes[i].data.deviceType==1:
            yield '''<b><font color="#993333">Device Type:</b></font><br>Router<br>'''
        else:
            yield '''<b><font color="#993333">Device Type:</b></font><br>End Device<br>'''
        yield '''<b><font color="#993333">Parent Address:</b></font><br>%X<br>
                 <b><font color="#993333">Last Sample:</b></font><br>%s <br>
                 <b><font color="#993333">Time between Samples:</font></b><br>%d ms<br>
                 '''%(Nodes.nodes[i].data.parentAddress,
                                 time.ctime(int(Nodes.nodes[i].data.lastSample/1000)),
                                 Nodes.nodes[i].data.timeSinceLastSample)
        if(Nodes.nodes[i].data.timeSinceLastSample>inactiveTime):
            yield '''<b><font color="#993333">Status:</b></font><br>Inactive'''
        else:
            yield '''<b><font color="#993333">Status:</b></font><br> Active'''
        yield'''</p><form name='input4' action='config' method='get' id="input4">
            <input type='hidden' name = 'name' value ='config%d'>
            <input type='submit' value='CONFIGURE'></form>
            <form action="" method="POST">
            <input id="send data" type="hidden" name="send" value="">
            <input type="submit"  onclick="getValue();" value="Send Message"/></form>
            <form  name="input5" action='' method='POST'>
            <input type="hidden" id="removing" name="decommission" value=""/>
            <input id="remove" type='submit' onclick="getConfirmation();" value='REMOVE SENSOR'>
            </form></div>'''%(i)
    
        yield'''
        <div id="table" style="background-color:#EEEEEE;min-height:500px;align:left;">
        <table id="outerTable" style ="border-left:5px solid;border-color:#FF9900;background-color:#EEEEEE;
        height:550px;cell-padding=0px;margin-top=-5px;margin-left=-5px;">
        <tr align=left><td valign=top style="min-width:650px;">
        <table id="innerTable" align="center" style ="cell-padding=0px;border:0px solid;border-color:#EEEEEE;background-color:#EEEEEE;
            margin-top:-3px;margin-left:-3px;margin-right:-3px;min-width:450px;float:center;">
            <tr style='background-color:#FFA500;float:center;'>
            <th width="170">Field Name</th>
            <th width="100">LongName</th>
            <th  width="100">Type</th>
            <th width="100">Value</th>
            <th width="500" float="center">Enable Data Mapping</th>
            </tr>'''
        #XBeeLock.acquire()
        analog = XBee.printAnalog(i,":")
        digital =XBee.printDigital(i,":")
        #XBeeLock.release()

        
        # this method gets the pin number and the value from the string
        # first get the pin number
        start =0
        while True:
            start = analog.find("DIO",start)
            if start ==-1:
                break
            start = start+len("DIO")
            end =analog.find("=",start)
            pin = int(analog[start:end])
            start = end +len("=")
            # next we can get the value 
            end = analog.find(":", start)
            value = analog[start:end]
            print value
            start = end
            yield '''
                    <tr align='center' style='min-height:35px;background-color:#CCCCCC;'>
                    <td height=40px>DIO%d</td>
                    <td >ADC%d</td>
                    <td>ANALOG</td>
                    <td>%s</td>
                    <td>'''%(pin,pin, value)
            if edit==pin:
                yield '''
                <form action ='' method = 'POST'>
                min =<input type=text maxlength = '8' size ='8' name='_min' value=%d>
                max =<input type=text maxlength = '8' size ='8' name='_max' value=%d>
                <input type='hidden' name='channel' value ='%d'>
                <input type='submit' name='submit' value='SUBMIT' /></form>'''%(Nodes.nodes[i].data.minimum[pin],
                                                                  Nodes.nodes[i].data.maximum[pin], pin)
            else:
                yield'''
                    <form action='' method='post'>min = %d max = %d
                    <input type="hidden" name='edit' value='%d'>
                    <input type='submit' value='EDIT'></form>
                    </td>
                    </tr>'''%(Nodes.nodes[i].data.minimum[pin],Nodes.nodes[i].data.maximum[pin],pin)
            # repeat for digital Channels
        start =0
        while True:
            start = digital.find("DIO",start)
            if start ==-1:
                 break
            start = start+len("DIO")
            end = digital.find("=",start)
            pin = int(digital[start:end])
            start =end+len("=")
            end = digital.find(":", start)
            value = int(digital[start:end])
            start =end
            if value == 0:
                value = "LOW"
            elif value ==1:
                value="HIGH"
                
            yield '''
                <tr align='center' style='height:35px;background-color:#CCCCCC;'>
                <td height=40px; >DIO%d</td>
                <td>DI0%d</td>
                <td>DIGITAL</td>
                
                <td>%s</td>
                <td>N/A</td>
                </tr>'''%(pin, pin, value)
        if Nodes.nodes[i].data.rxDataSize()>0:
            rxDataString=""
            start=0 
            for j in range(0,Nodes.nodes[i].data.rxDataSize()):
                rxDataString+=chr(Nodes.nodes[i].data.rxData[j])
            print rxDataString
            k=0
            while True:
                end=rxDataString.find(";",start)
                if start==-1 or end==-1:
                    break
                rxDataName=rxDataString[start:end]
                start=end+len(";")
                end=rxDataString.find(";",start)
                rxData=rxDataString[start:end]
                start=end+len(";")
                k+=1
                yield'''
                <tr align='center' style='height:35px;background-color:#CCCCCC;'>
                <td height=40px;>RX DATA%d</td>
                <td>%s</td>
                <td>RX</td>
                <td>%s</td>
                <td>N/A</td>'''%(k,rxDataName,rxData)
            
        
        yield '''
            </table></td></tr>
            </table></div>'''

        yield self.footer()


            


    def GET(self, name=None, edit=None):
        i=0
        Nodes=XBee.getSensors()
        if not(Nodes.nodes.size()>0):
            return '''<body>%s
            </div>
            <div id="veritcal-line" style="margin-bottom:0;background-color:#FF9900;width:5px;min-height:500px;float:left;">
            </div>
            <div id="middle" style="margin-bottom:0;background-color:#EEEEEE;min-height:500px;float:center;">
            <br><br><h1>No Sensors to View</h1><a href="./">return to main</a></div></div>
            %s'''%(self.header("View-Sensor-Page"),self.footer())
        if name:
            if name[0:4]=="view":
                i = int(name[4:len(name)])
        return self.page(i)
            
            

    def POST(self, name=None, i =0, submit =None, edit=None,_min=None, _max =None, channel=None,
             recommission=None, decommission=None, send=None):
        global XBee
        global XBeeLock
        if name:
            if name[0:4] =="view":
                i = int(name[4:len(name)])
        if submit and _max and _min and channel:
            channel= int(channel)
            XBee.changeAnalog(i, channel,int(_min),int(_max))
            XBee.forceSample(i,5000)
        if decommission=="true":
            XBee.softCommissionButtonPress(i,4)
            XBee.deleteSensor(int(i))
        Nodes=XBee.getSensors()
        if send:
            from XBee_ext import RxData
            print send
            print send[0]
            rxData=RxData(len(send))
            for j in range(0,len(rxData)):
                rxData[j]= ord(send[j])
            XBee.sendRxData(0,rxData,5000)
            
        if not(Nodes.nodes.size()>0):
            return '''<body>%s
            </div>
            <div id="veritcal-line" style="margin-bottom:0;background-color:#FF9900;width:5px;min-height:500px;float:left;">
            </div>
            <div id="middle" style="margin-bottom:0;background-color:#EEEEEE;min-height:500px;float:center;">
            <br><br><h1>No Sensors to View</h1><a href="./">return to main</a></div></div>
            %s'''%(self.header("View-Sensor-Page"),self.footer())
        
        else:
            if edit:
                channel= int(edit)
                return self.page(i,channel)
            else:
                return self.page(i)

class downloadArchive():
    exposed = True
    def GET(self):
        archiveName="Sensor_Logs_%s"%(time.ctime(time.time()))+".txt"
        for filename in os.listdir("."):
            if filename =="Sensor_Logs.txt":
                shutil.copyfile(filename,archiveName)
        try:
            compression = zipfile.ZIP_DEFLATED
        except:
            compression = zipfile.ZIP_STORED


        zf = zipfile.ZipFile('Sensor_Logs.zip', mode='a')
        try:
            zf.write(archiveName,compress_type=compression)
        finally:
            zf.close()
            os.remove(archiveName)
            path = os.path.join(absDir,"Sensor_Logs.zip")
        return static.serve_file(path, "application/x-download",
                                 "attachment", os.path.basename(path))               






       
root = MainPage()
root.config = ConfigPage()
root.view = SensorViewPage()
root.logs=LogPage()
root.logs.download=downloadArchive()
cherrypy.config.update({'server.socket_port': 8080,'server.socket_host' : "0.0.0.0",'server.thread_pool' : 1,})
def autoArchiveThread(b):
    TimeSinceLastUpdate =0
    global exitCode
    try:
        while exitCode!=1:
            if exitCode ==1:
                break
            time.sleep(1)
            if (time.time()-TimeSinceLastUpdate)>(24*3600):
                archiveName="Sensor_Logs_%s"%(time.ctime(time.time()))+".txt"
                for filename in os.listdir("."):
                   if filename =="Sensor_Logs.txt":
                       shutil.copyfile(filename,archiveName)
                try:
                    compression = zipfile.ZIP_DEFLATED
                except:
                    compression = zipfile.ZIP_STORED


                zf = zipfile.ZipFile('Sensor_Logs.zip', mode='a')
                try:
                    zf.write(archiveName,compress_type=compression)
                finally:
                    zf.close()
                    os.remove(archiveName)
                    TimeSinceLastUpdate=time.time()
    except:
        return 0
    finally:
        print "Exiting Auto-Archive Thread"
        return 0
                
    return 0

if __name__ =="__main__":
    
# Start the main function
    #XBee.setSerial("/dev/ttyUSB0")
	XBee.setSerial("/dev/ttyS0")
    open("Sensor_Logs.txt", "w") # resets the Logfile each time app is started
    XBee.setDefaultSampleRate(1000)
    XBee.setDiscoveryTimeout(500)
    XBee.initializeNetwork() # initialize the network
    autoArchiveTimer = Thread(target=autoArchiveThread, args=(0,))
    autoArchiveTimer.stop=threading.Event()

    client = Thread(target=XBeeClientFunction,args=(0,))
    client.stop=threading.Event()
    try:
            cherrypy.engine.unsubscribe('start',autoArchiveTimer.start())
            cherrypy.engine.unsubscribe('start',client.start())
            cherrypy.tree.mount(root,config={'/': {'request.dispatch': cherrypy.dispatch.MethodDispatcher()}})
            cherrypy.server.start()
            cherrypy.engine.start()
            while 1:
                time.sleep(1)
    except KeyboardInterrupt:
        exitCode=1
    finally:
        exitCode=1
        cherrypy.engine.exit()
        client.join()
        autoArchiveTimer.join()
    
    print "Terminating..."
    
            

    


    







#cherrypy.server.start()
#cherrypy.quickstart(XBeePage(), 'tutorial.conf')
#cherrypy.quickstart(XBeePage())
