from flask import Flask, request, jsonify, json
from flask_mysqldb import MySQL
from flask_jwt_extended import (JWTManager, jwt_required, create_access_token, get_jwt_identity)
import docker
from threading import Thread
import logging
from datetime import datetime, timedelta
import time
import collections
import re

#from .classes import CreateSrtCommand, CreateSrtContainer, LaunchFfprobe, GetRxLogs, GetTxLogs

app = Flask (__name__)

##################
#### CONFIG ######
##################

# Don´t Sort aphabetically JSON response 
app.config['JSON_SORT_KEYS'] = False 

# Mysql connecttion
app.config['MYSQL_HOST'] = 'srt_ddbb'
app.config['MYSQL_USER'] = 'raul'
app.config['MYSQL_PASSWORD'] = 'rauldb'
app.config['MYSQL_ROOT_PASSWORD'] = 'mysql_p3dx'
app.config['MYSQL_DB'] = 'srt_db'
mysql = MySQL(app)

# jwt settings
app.config['JWT_SECRET_KEY'] = "SRTAPI?admin"
jwt = JWTManager(app)

# Logging
logging.basicConfig(level=logging.DEBUG, format='[%(levelname)s] (%(threadName)-9s) %(message)s',)

# Container versions
srtContainerVersion = "srt_alpine:v142" #cambiado a 1.4.2 https://github.com/Haivision/srt/releases/tag/v1.4.2
ffprobeContainerVersion = "ffprobe_alpine:v1"

# Create docker enviroment
client = docker.from_env()

##########################################
############ SECURITY STUFF ##############
##########################################

# Provide a method to create access tokens. The create_jwt()
# function is used to actually generate the token
@app.route('/v1/login', methods=['POST'])
def login():
    if not request.is_json:
        return jsonify({"Error": "Missing JSON in request"}), 400

    params = request.get_json()
    username = params.get('username', None)
    password = params.get('password', None)

    if not username:
        return jsonify({"Error": "Missing username parameter"}), 400
    if not password:
        return jsonify({"Error": "Missing password parameter"}), 400

    if username != 'admin' or password != 'password':
        return jsonify({"Error": "Bad username or password"}), 401

    #Hacemos que el token tenga una validez por X tiempo (ahora, una semana desde el momento en el que se crea)
    dt = datetime.now() + timedelta(days=7)
    # Identity can be any data that is json serializable
    ret = {'jwt': create_access_token(identity=username), 'exp':dt}
    return jsonify(ret), 200

# Protect a view with jwt_required, which requires a valid jwt
# to be present in the headers.
@app.route('/v1/protected', methods=['GET'])
@jwt_required
def protected():
    # Access the identity of the current user with get_jwt_identity
    return jsonify({'Granted Access': get_jwt_identity()}), 200


##########################################
############### TX ROUTES ################
##########################################

### GET TX ###

@app.route ('/v1/get_tx', methods =['GET'])
@jwt_required
def getTx():
    if request.method != 'GET':
        return
    cur = mysql.connection.cursor()
    query = """SELECT * FROM tx"""
    cur.execute(query)
    data = cur.fetchall()
    cur.close()
    txList = []
    for row in data:
        tx = collections.OrderedDict()
        tx['Id'] = row[0]
        tx['ServiceName'] = row[7]
        tx['Mode'] = row[1]
        tx['RxIp'] = row[2]
        tx['ListenPort'] = row[3]
        tx['UdpIn'] = row[4]
        tx['Latency'] = row[5]
        tx['SrtBw'] = row[6]
        tx['PassPhrase'] = row[26]
        tx['PbKeyLen'] = row [27]
        tx['GroupConnect'] = row [28]
        tx['InputBw'] = row [29]
        tx['OverHead'] = row [30]
        tx['FEC'] = row[31]
        tx['FecCols'] = str(row [32])
        tx['FecRows'] = str(row [33])
        tx['FecLayout'] = row [34]
        tx['FecArq'] = row [35]
        tx['Congestion'] = row [36]
        tx['TransType'] = row [37]
        tx['Status'] = row[9]
        tx['ContainerId'] = row[8]
        tx['LinkRtt'] = row[10]
        tx['LinkBandwidth'] = row[11]
        tx['LinkMaxBandwidth'] = row[25]
        tx['SendPackets'] = row[12]
        tx['SendPacketsUnique'] = row[13]
        tx['SendPacketsLost'] = row[14]
        tx['SendPacketsDropped'] = row[15]
        tx['SendPacketsRetransmitted'] = row[16]
        tx['SendPacketsFilterExtra'] = row[17]    
        tx['SendBytes'] = row[18]
        tx['SendBytesUnique'] = row[19]
        tx['SendBytesDropped'] = row[20]
        tx['SendMbitRate'] = row[21]
        tx['LineLoss'] = row[22]
        tx['Connected'] = row[23]
        tx['Reconnections'] = row[24]
        txList.append(tx)
    return (json.dumps(txList))

### GET ONE TX ###

@app.route ('/v1/get_tx/<string:id>', methods =['GET'])
@jwt_required
def getOneTx(id):
    if request.method != 'GET':
        return
    cur = mysql.connection.cursor()
    cur.execute('SELECT * FROM tx WHERE id = %s', [id])
    data = cur.fetchall()
    cur.close()
    if len(data) >= 1: #Check if ID already exists, if yes, extract data to show it.
        for row in data:
            tx_settings = collections.OrderedDict()
            tx_stats = collections.OrderedDict()
            tx_settings['Id'] = row[0]
            tx_settings['ServiceName'] = row[7]
            tx_settings['Mode'] = row[1]
            tx_settings['RxIp'] = row[2]
            tx_settings['ListenPort'] = row[3]
            tx_settings['UdpIn'] = row[4]
            tx_settings['Latency'] = row[5]
            tx_settings['SrtBw'] = row[6]
            tx_settings['PassPhrase'] = row[26]
            tx_settings['PbKeyLen'] = row [27]
            tx_settings['GroupConnect'] = row [28]
            tx_settings['InputBw'] = row [29]
            tx_settings['OverHead'] = row [30]
            tx_settings['FEC'] = row[31]
            tx_settings['FecCols'] = row [32]
            tx_settings['FecRows'] = row [33]
            tx_settings['FecLayout'] = row [34]
            tx_settings['FecArq'] = row [35]
            tx_settings['Congestion'] = row [36]
            tx_settings['TransType'] = row [37]
            tx_settings['Status'] = row[9]
            tx_settings['ContainerId'] = row[8]
            tx_stats['LinkRtt'] = row[10]
            tx_stats['LinkBandwidth'] = row[11]
            tx_stats['LinkMaxBandwidth'] = row[25]
            tx_stats['SendPackets'] = row[12]
            tx_stats['SendPacketsUnique'] = row[13]
            tx_stats['SendPacketsLost'] = row[14]
            tx_stats['SendPacketsDropped'] = row[15]
            tx_stats['SendPacketsRetransmitted'] = row[16]
            tx_stats['SendPacketsFilterExtra'] = row[17]    
            tx_stats['SendBytes'] = row[18]
            tx_stats['SendBytesUnique'] = row[19]
            tx_stats['SendBytesDropped'] = row[20]
            tx_stats['SendMbitRate'] = row[21]
            tx_stats['LineLoss'] = row[22]
            tx_stats['Connected'] = row[23]
            tx_stats['Reconnections'] = row[24]
        response = {"Settings": tx_settings, "Stats": tx_stats}
        return (json.dumps(response)) 
    else:
        if len(data) == 0:
            return jsonify({"TX": "TX with ID " +id+ " do not exist."}), 404

### ADD TX###

@app.route ('/v1/add_tx', methods =['POST'])
@jwt_required
def addTx():  # sourcery skip
    if request.method != 'POST':
        return       
    #Check if the service_name already exists
    serviceName = request.json['ServiceName']
    cur = mysql.connection.cursor()
    cur.execute('SELECT id FROM tx WHERE service_name = %s', [serviceName])
    data = cur.fetchall()
    cur.close()
    if len(data) == 0: #Si no hay datos sobre es ID procedemos a darlo de Alta.
        mode = request.json['Mode']
        rxIp = request.json['RxIp']
        listenPort = request.json['ListenPort']
        udpIn = request.json['UdpIn'],
        latency = request.json['Latency']
        srtBw = request.json['SrtBw']
        passPhrase = request.json['PassPhrase']
        pbKeyLen = request.json['PbKeyLen']
        groupConnect = request.json['GroupConnect']
        inputBw = request.json['InputBw']
        overHead = request.json['OverHead']
        packetFilter = request.json['FEC']
        fecCols = request.json['FecCols']
        fecRows = request.json['FecRows']
        fecLayout = request.json['FecLayOut']
        fecArq = request.json['FecArq']
        congestion = request.json['Congestion']
        transType = request.json['TransType']
        #Ponemos a 0 l status y el containerID vacio
        status = 0
        containerId = ''
        cur = mysql.connection.cursor()
        cur.execute('INSERT INTO tx (service_name, mode, rx_ip, listen_port, udp_in, latency, srt_bw, passphrase, pbkeylen, groupconnect, input_bw, overhead, packetfilter, fec_cols, fec_rows, fec_layout, fec_arq, congestion, transtype, status, container_id) VALUES (%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)',
                    (serviceName, mode, rxIp, listenPort, udpIn, latency, srtBw, passPhrase, pbKeyLen, groupConnect, inputBw, overHead, packetFilter, fecCols, fecRows, fecLayout, fecArq, congestion, transType, status, containerId))
        mysql.connection.commit()
        cur.execute('SELECT id FROM tx WHERE service_name = %s', [serviceName])
        data = cur.fetchone()[0]
        cur.close()
        response = {"TX": serviceName +" Added Succesfully", "Id": data}
        return jsonify(response), 200
    else:       
        return jsonify({"TX": "ServiceName "+ serviceName + " already exist."}), 409

### UPDATE TX###

@app.route ('/v1/update_tx/<string:id>', methods = ['PUT'])
@jwt_required
def updateTx(id): 
    if request.method != 'PUT':
        return          
    #Check if ID already exists  
    cur = mysql.connection.cursor()
    cur.execute('SELECT id FROM tx WHERE id = %s', [id])
    data = cur.fetchall()
    cur.close()
    if len(data) >= 1: #Si existen datos sobre ese Id, hacemos update.
            serviceName = request.json['ServiceName']
            mode = request.json['Mode']
            rxIp = request.json['RxIp']
            listenPort = request.json['ListenPort']
            udpIn = request.json['UdpIn'],
            latency = request.json['Latency']
            srtBw = request.json['SrtBw'] 
            passPhrase = request.json['PassPhrase']
            pbKeyLen = request.json['PbKeyLen']
            groupConnect = request.json['GroupConnect']
            inputBw = request.json['InputBw']
            overHead = request.json['OverHead']
            packetFilter = request.json['FEC']
            fecCols = request.json['FecCols']
            fecRows = request.json['FecRows']
            fecLayout = request.json['FecLayOut']
            fecArq = request.json['FecArq']
            congestion = request.json['Congestion']
            transType = request.json['TransType']
            cur = mysql.connection.cursor()
            cur.execute('UPDATE tx SET service_name = %s, mode = %s, rx_ip = %s, listen_port = %s, udp_in = %s, latency = %s, srt_bw = %s, passphrase = %s, pbkeylen = %s, groupconnect = %s, input_bw = %s, overhead = %s, packetfilter = %s, fec_cols = %s, fec_rows = %s, fec_layout = %s, fec_arq = %s, congestion = %s, transtype  = %s WHERE id = %s', 
                        (serviceName, mode, rxIp, listenPort, udpIn, latency, srtBw, passPhrase, pbKeyLen, groupConnect, inputBw, overHead, packetFilter, fecCols, fecRows, fecLayout, fecArq, congestion, transType, id))
            mysql.connection.commit()
            cur.close()
            #return jsonify({"OK": "TX "+ serviceName +" with id "+ id +" Updated Succesfully"})
            response = {"TX": serviceName +" Updated Succesfully", "Id": id}
            return jsonify(response), 200
    else:           
        if len(data) == 0:
            return jsonify({"TX": "TX with ID " +id+ " do not exist."}), 404

### DELETE TX###

@app.route ('/v1/delete_tx/<string:id>', methods = ['DELETE'])
@jwt_required
def deleteTx(id):
    if request.method != 'DELETE':     
        return
     #Check if ID already exists
    cur = mysql.connection.cursor()
    cur.execute('SELECT id FROM tx WHERE id = %s', [id])
    data = cur.fetchall()
    cur.close()
    if len(data) >= 1: #Si existe el id y devuelve datos, lo borramos.
        cur = mysql.connection.cursor()
        cur.execute('DELETE FROM tx WHERE id = {0}' .format(id))
        mysql.connection.commit()
        cur.close()
        #return jsonify({"OK": "TX with id "+ id +" Deleted Succesfully"})
        response = {"TX": "Deleted Succesfully", "Id": id}
        return jsonify(response), 200
    else:
        if len(data) == 0:
            return jsonify({"TX": "TX with ID " +id+ " do not exist."}), 404

### START TX ###

@app.route ('/v1/start_tx/<string:id>', methods = ['POST'])
@jwt_required
def startTx(id):  # sourcery skip
    if request.method != 'POST':
        return
    #Check if ID already exists  
    cur = mysql.connection.cursor()
    cur.execute('SELECT * FROM tx WHERE id = %s', [id])
    data = cur.fetchall()
    cur.close()
    if len(data) >= 1:
        #Si existe el id, Extraemos parametros de la bbdd. 
        for row in data:
            status = row[9]
            containerId= str(row[8])
            udpIn = str(row[4]) 
        if status == 0:  #Si el status es 0 pasamos a ver si existe ya un containerID, else está running o else existe containerID. 
            if len(containerId) == 0:
                mode = "tx"
                #Test Input con FFprobe
                ffprobeTest = LaunchFfprobe (udpIn, ffprobeContainerVersion, mode, id)
                ffprobeTest.start()
                ffprobeTest.join() #esperamos a que termine el comando de ffprobe.
                if ffprobeTest.error:
                    return jsonify({"Error": ""+ ffprobeTest.error +""}), 500
                
                #Create srt command
                srtTxCommand = CreateSrtCommand (data, id, mode)
                srtTxCommand.start()
                srtTxCommand.join()

                #Launch srt Container
                srtTxContainer = CreateSrtContainer (srtContainerVersion, srtTxCommand.command, srtTxCommand.name, mode, id)
                srtTxContainer.start()
                srtTxContainer.join()

                #Pillamos el containerID y si existe actualizamos
                container = srtTxContainer.cId
                if container:
                    cur = mysql.connection.cursor()
                    cur.execute('UPDATE tx SET container_id = %s, status = 1 WHERE id = %s',(container,id))
                    mysql.connection.commit()
                    cur.close()
                    txLogs = GetTxLogs (container, id) 
                    txLogs.start()
                    time.sleep(4)
                    #Check SRT connection Status (1 connected, 2 Connecting, 0 Disconnected)
                    if txLogs.connected == 1:
                        return jsonify({"TX": ""+ srtTxCommand.name +" Started Succesfully"}), 200 
                    else:
                        return jsonify({"TX": ""+ srtTxCommand.name +" Started but Waiting for Connection"}), 202
                else:
                    #return jsonify({"Error": ""+ apiError +""}), 500
                    return jsonify({"TX": "Container Initialization Error"}), 500
            else:
                return jsonify({"TX": "TX with ID "+ id +" Container Already Exist"}), 409
        else:       
            return jsonify({"TX": "TX with ID "+ id +" Already Started"}), 409
    else:      
        return jsonify({"TX": "TX with ID " +id+ " do not exist."}), 404

### STOP TX ###

@app.route ('/v1/stop_tx/<string:id>', methods = ['POST'])
@jwt_required
def stopTx(id):
    if request.method != 'POST':
        return
    #Check if ID already exists
    cur = mysql.connection.cursor()
    cur.execute('SELECT * FROM tx WHERE id = %s', [id])
    data = cur.fetchall()
    cur.close()
    if len(data) >= 1: #si devuelve datos paramos el container si es que está con status 1. Si no, ya estaba parado o no existe. Ponemos a 0 estadisticas y actualizamos status y containerID.
        for row in data:
            containerId = str(row[8])
            status = row[9]
        if status == 1:
            container = client.containers.get(containerId)
            container.stop()
            container.remove()
            time.sleep(3)
            cur = mysql.connection.cursor()
            cur.execute('UPDATE tx SET container_id = "", status = 0, link_rtt = "0", link_bandwidth = "0", link_maxbandwidth = "0", send_packets = "0", send_packetsUnique = "0", send_packetsLost = "0", send_packetsDropped = "0", send_packetsRetransmitted = "0", send_packetsFilterExtra = "0", send_bytes = "0", send_bytesUnique = "0", send_bytesDropped = "0" ,send_mbitRate = "0", line_loss = "0.0", connected = "0", reconnections= "0" WHERE id = %s',[id])
            mysql.connection.commit()
            cur.close()
            return jsonify({"TX": "TX with ID "+ id +" Stopped Succesfully"}), 200
        else:
            return jsonify({"TX": "TX with ID "+ id +" Already Stopped"}), 409
    else:
        if len(data) == 0:
            return jsonify({"TX": "TX with ID " +id+ " do not exist."}), 404

### RESTART TX ####

@app.route ('/v1/restart_tx/<string:id>', methods = ['POST'])
@jwt_required
def restartTx(id):  # sourcery skip
    if request.method != 'POST':
        return  
    #Check if ID already exists and get some parameters
    cur = mysql.connection.cursor()
    cur.execute('SELECT * FROM tx WHERE id = %s', [id])
    data = cur.fetchall()
    for row in data:
            containerId = row[8]
            status = row[9]  
    cur.close()
    if len(data) >= 1:   #Si devuelve datos la query y status == 1 reseteamos (ponemos a 0 estadisticas), lanzamos de nuevo GetLogs, y dependiendo del estado de conexión respondemos connected o connecting. 
        if status == 1:
            container = client.containers.get(containerId)
            container.restart()
            cur = mysql.connection.cursor()
            cur.execute('UPDATE tx SET link_rtt = "0", link_bandwidth = "0", link_maxbandwidth = "0", send_packets = "0", send_packetsUnique = "0", send_packetsLost = "0", send_packetsDropped = "0", send_packetsRetransmitted = "0", send_packetsFilterExtra = "0", send_bytes = "0", send_bytesUnique = "0", send_bytesDropped = "0" ,send_mbitRate = "0", line_loss = "0.0" WHERE id = %s',[id])
            mysql.connection.commit()
            cur.close()
            txLogs = GetTxLogs (container.short_id, id) 
            txLogs.start()
            time.sleep(4)
            if txLogs.connected == 1:
                return jsonify({"TX": "TX with ID "+ id +" Restarted Successfully"}), 200
            else:
                return jsonify({"TX": "TX_"+ id +" Restarted but Waiting for Connection"}), 202
        else:
            return jsonify({"TX": "TX with ID "+ id +" is Stopped"}), 409
    else:     
        if len(data) == 0:
            return jsonify({"TX": "TX with ID " +id+ " do not exist."}), 404

##########################################
############### RX ROUTES ################
##########################################

### GET RX ###

@app.route ('/v1/get_rx', methods =['GET'])
@jwt_required
def getRx():
    if request.method != 'GET':
        return
    cur = mysql.connection.cursor()
    query = """SELECT * FROM rx"""
    cur.execute(query)
    data = cur.fetchall()
    cur.close()
    rxList = []
    for row in data:
        rx = collections.OrderedDict()
        rx['Id'] = row[0]
        rx['ServiceName'] = row[7]
        rx['Mode'] = row[1]
        rx['TxIp'] = row[2]
        rx['ListenPort'] = row[3]
        rx['UdpOut'] = row[4]
        rx['Latency'] = row[5]
        rx['SrtBw'] = row[6]
        rx['PassPhrase'] = row[30]
        rx['PbKeyLen'] = row[31]
        rx['TlpktDrop'] = row [32]
        rx['TTL'] = row [33]
        rx['FEC'] = row[34]
        rx['FecCols'] = row [35]
        rx['FecRows'] = row [36]
        rx['FecLayout'] = row [37]
        rx['FecArq'] = row [38]
        rx['Congestion'] = row [39]
        rx['TransType'] = row [40]
        rx['GroupConnect'] = row [41]
        rx['GroupType'] = row [42]
        rx['GroupIp1'] = row [43]
        rx['GroupIp2'] = row [44]
        rx['Ip1Weight'] = row [45]
        rx['Ip2Weight'] = row [46]  
        rx['Status'] = row[9]
        rx['ContainerId'] = row[8]
        rx['LinkRtt'] = row[10]
        rx['LinkBandwidth'] = row[11]
        rx['LinkMaxBandwidth'] = row[29]
        rx['RecvPackets'] = row[12]
        rx['RecvPacketsUnique'] = row[13]
        rx['RecvPacketsLost'] = row[14]
        rx['RecvPacketsDropped'] = row[15]
        rx['RecvPacketsRetransmitted'] = row[16]
        rx['RecvPacketsBelated'] = row[17]
        rx['RecvPacketsFilterExtra'] = row[18]
        rx['RecvPacketsFilterSupply'] = row[19]
        rx['RecvPacketsFilterLoss'] = row[20]
        rx['RecvBytes'] = row[21]
        rx['RecvBytesUnique'] = row[22]
        rx['RecvBytesLost'] = row[23]
        rx['RecvBytesDropped'] = row[24]
        rx['RecvMbitRate'] = row[25]
        rx['LineLoss'] = row[26]
        rx['Connected'] = row[27]
        rx['Reconnections'] = row[28]
        rxList.append(rx)
    return (json.dumps(rxList))

### GET ONE RX ###

@app.route ('/v1/get_rx/<string:id>', methods =['GET'])
@jwt_required
def getOneRx(id):
    if request.method != 'GET':
        return
    #Check if ID already exists  
    cur = mysql.connection.cursor()
    cur.execute('SELECT * FROM rx WHERE id = %s', [id])
    data = cur.fetchall()
    cur.close()
    if len(data) >= 1: #Si se obtienen datos de la query, mostramos info, si no no existe.
        for row in data:
            rx_settings = collections.OrderedDict()
            rx_stats = collections.OrderedDict()
            rx_settings['Id'] = row[0]
            rx_settings['ServiceName'] = row[7]
            rx_settings['Mode'] = row[1]
            rx_settings['TxIp'] = row[2]
            rx_settings['ListenPort'] = row[3]
            rx_settings['UdpOut'] = row[4]
            rx_settings['Latency'] = row[5]
            rx_settings['SrtBw'] = row[6]
            rx_settings['PassPhrase'] = row[30]
            rx_settings['PbKeyLen'] = row[31]
            rx_settings['TlpktDrop'] = row [32]
            rx_settings['TTL'] = row [33]
            rx_settings['FEC'] = row[34]
            rx_settings['FecCols'] = row [35]
            rx_settings['FecRows'] = row [36]
            rx_settings['FecLayout'] = row [37]
            rx_settings['FecArq'] = row [38]
            rx_settings['Congestion'] = row [39]
            rx_settings['TransType'] = row [40]
            rx_settings['GroupConnect'] = row [41]
            rx_settings['GroupType'] = row [42]
            rx_settings['GroupIp1'] = row [43]
            rx_settings['GroupIp2'] = row [44]
            rx_settings['Ip1Weight'] = row [45]
            rx_settings['Ip2Weight'] = row [46]
            rx_settings['Status'] = row[9]
            rx_settings['ContainerId'] = row[8]
            rx_stats['LinkRtt'] = row[10]
            rx_stats['LinkBandwidth'] = row[11]
            rx_stats['LinkMaxBandwidth'] = row[29]
            rx_stats['RecvPackets'] = row[12]
            rx_stats['RecvPacketsUnique'] = row[13]
            rx_stats['RecvPacketsLost'] = row[14]
            rx_stats['RecvPacketsDropped'] = row[15]
            rx_stats['RecvPacketsRetransmitted'] = row[16]
            rx_stats['RecvPacketsBelated'] = row[17]
            rx_stats['RecvPacketsFilterExtra'] = row[18]
            rx_stats['RecvPacketsFilterSupply'] = row[19]
            rx_stats['RecvPacketsFilterLoss'] = row[20]
            rx_stats['RecvBytes'] = row[21]
            rx_stats['RecvBytesUnique'] = row[22]
            rx_stats['RecvBytesLost'] = row[23]
            rx_stats['RecvBytesDropped'] = row[24]
            rx_stats['RecvMbitRate'] = row[25]
            rx_stats['LineLoss'] = row[26]
            rx_stats['Connected'] = row[27]
            rx_stats['Reconnections'] = row[28]
        response = {"Settings": rx_settings, "Stats": rx_stats}
        return (json.dumps(response)), 200
    else:
        if len(data) == 0:
            return jsonify({"RX": "RX with ID " +id+ " do not exist."}), 404

### ADD RX ###

@app.route ('/v1/add_rx', methods =['POST'])
@jwt_required
def addRx():  # sourcery skip
    if request.method != 'POST':  
        return
    serviceName = request.json['ServiceName']
    #Check if the service_name already exists   
    cur = mysql.connection.cursor()
    cur.execute('SELECT service_name FROM rx WHERE service_name = %s', [serviceName])
    data = cur.fetchall()
    cur.close()
    if len(data) == 0: #Si no hay datos sobre es ID procedemos a darlo de Alta.
        mode = request.json['Mode']
        txIp = request.json['TxIp']
        listenPort = request.json['ListenPort']
        udpOut = request.json['UdpOut'],
        latency = request.json['Latency']
        srtBw = request.json['SrtBw']
        passPhrase = request.json['PassPhrase']
        pbKeyLen = request.json['PbKeyLen']
        tlpktDrop = request.json['TlpktDrop']
        ttl = request.json['TTL']
        packetFilter = request.json['FEC']
        fecCols = request.json['FecCols']
        fecRows = request.json['FecRows']
        fecLayout = request.json['FecLayOut']
        fecArq = request.json['FecArq']
        congestion = request.json['Congestion']
        transType = request.json['TransType']
        groupConnect = request.json['GroupConnect']
        groupType = request.json['GroupType']
        groupIp1 = request.json['GroupIp1']
        groupIp2 = request.json['GroupIp2']
        ip1Weight = request.json['Ip1Weight']
        ip2Weight = request.json['Ip2Weight']
        status = 0
        containerId = ''
        cur = mysql.connection.cursor()
        cur.execute('INSERT INTO rx (service_name, mode, tx_ip, listen_port, udp_out, latency, srt_bw, passphrase, pbkeylen, tlpktdrop, ttl, packetfilter, fec_cols, fec_rows, fec_layout, fec_arq, congestion, transtype, group_connect, group_type, group_ip1, group_ip2, ip1_weight, ip2_weight, status, container_id) VALUES (%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)',
                    (serviceName, mode, txIp, listenPort, udpOut, latency, srtBw, passPhrase, pbKeyLen, tlpktDrop, ttl, packetFilter, fecCols, fecRows, fecLayout, fecArq, congestion, transType, groupConnect, groupType, groupIp1, groupIp2, ip1Weight, ip2Weight, status, containerId))
        mysql.connection.commit()
        cur.execute('SELECT id FROM rx WHERE service_name = %s', [serviceName])
        data = cur.fetchone()[0]
        cur.close()
        response = {"RX": serviceName +" Added Succesfully", "Id": data}
        return jsonify(response), 200
    else:       
        return jsonify({"RX": "ServiceName "+ serviceName + " already exist."}), 409

### UPDATE RX ###

@app.route ('/v1/update_rx/<string:id>', methods = ['PUT'])
@jwt_required
def updateRx(id):
    if request.method != 'PUT':   
        return
    #Check if ID already exists 
    cur = mysql.connection.cursor()
    cur.execute('SELECT id FROM rx WHERE id = %s', [id])
    data = cur.fetchall()
    cur.close()
    if len(data) >= 1: #Si existen datos sobre ese Id, hacemos update.
        cur = mysql.connection.cursor()
        serviceName = request.json['ServiceName']
        mode = request.json['Mode']
        txIp = request.json['TxIp']
        listenPort = request.json['ListenPort']
        udpOut = request.json['UdpOut'],
        latency = request.json['Latency']
        srtBw = request.json['SrtBw']
        passPhrase = request.json['PassPhrase']
        pbKeyLen = request.json['PbKeyLen']
        tlpktDrop = request.json['TlpktDrop']
        ttl = request.json['TTL']
        packetFilter = request.json['FEC']
        fecCols = request.json['FecCols']
        fecRows = request.json['FecRows']
        fecLayout = request.json['FecLayOut']
        fecArq = request.json['FecArq']
        congestion = request.json['Congestion']
        transType = request.json['TransType']
        groupConnect = request.json['GroupConnect']
        groupType = request.json['GroupType']
        groupIp1 = request.json['GroupIp1']
        groupIp2 = request.json['GroupIp2']
        ip1Weight = request.json['Ip1Weight']
        ip2Weight = request.json['Ip2Weight']
        cur.execute('UPDATE rx SET  service_name = %s, mode = %s, tx_ip = %s, listen_port = %s, udp_out = %s, latency = %s, srt_bw = %s, passphrase = %s, pbkeylen = %s, tlpktdrop = %s, ttl = %s, packetfilter = %s, fec_cols = %s, fec_rows = %s, fec_layout = %s, fec_arq = %s, congestion = %s, transtype = %s, group_connect = %s, group_type = %s, group_ip1 = %s, group_ip2 = %s, ip1_weight = %s, ip2_weight = %s WHERE id = %s', 
                    (serviceName, mode, txIp, listenPort, udpOut, latency, srtBw, passPhrase, pbKeyLen, tlpktDrop, ttl, packetFilter, fecCols, fecRows, fecLayout, fecArq, congestion, transType, groupConnect, groupType, groupIp1, groupIp2, ip1Weight, ip2Weight, id))
        mysql.connection.commit()
        cur.close()
        #return jsonify({"OK": "RX "+ serviceName +" with id "+ id +" Updated Succesfully"})
        response = {"RX": serviceName +" Updated Succesfully", "Id": id}
        return jsonify(response), 200
    else:       
        if len(data) == 0:
            return jsonify({"RX": "RX with ID " +id+ " do not exist."}), 404

### DELETE RX ###

@app.route ('/v1/delete_rx/<string:id>', methods = ['DELETE'])
@jwt_required
def deleteRx(id):
    if request.method != 'DELETE':
        return 
    #Check if ID already exists
    cur = mysql.connection.cursor()
    cur.execute('SELECT id FROM rx WHERE id = %s', [id])
    data = cur.fetchall()
    cur.close()
    if len(data) >= 1: #Si existe el id y devuelve datos, lo borramos.
        cur = mysql.connection.cursor()
        cur.execute('DELETE FROM rx WHERE id = {0}' .format(id))
        mysql.connection.commit()
        cur.close()
        #return jsonify({"OK": "RX with id "+ id +" Deleted Succesfully"})
        response = {"RX": "Deleted Succesfully", "Id": id}
        return jsonify(response), 200
    else:     
        if len(data) == 0:
            return jsonify({"RX": "RX with ID " +id+ " do not exist."}), 404

### START RX ###

@app.route ('/v1/start_rx/<string:id>', methods = ['POST'])
@jwt_required
def startRx(id):  # sourcery skip
    if request.method != 'POST':   
        return
    #Check if ID already exists
    cur = mysql.connection.cursor()
    cur.execute('SELECT * FROM rx WHERE id = %s', [id])
    data = cur.fetchall()
    cur.close()
    if len(data) >= 1: #Si existe el id, Extraemos parametros de la bbdd.
        for row in data:
            status = row[9]
            containerId= str(row[8])
        if status == 0:  
            if len(containerId) == 0:
                mode = "rx"
                #Create srt command
                srtRxCommand = CreateSrtCommand (data, id, mode)
                srtRxCommand.start()
                srtRxCommand.join()

                #Create SRT RX Container                
                srtRxContainer = CreateSrtContainer (srtContainerVersion, srtRxCommand.command, srtRxCommand.name, mode, id)
                srtRxContainer.start()
                srtRxContainer.join()
                
                #Pillamos el containerID y si existe actualizamos
                container = srtRxContainer.cId 
                if container:
                    cur = mysql.connection.cursor()
                    cur.execute('UPDATE rx SET container_id = %s, status = 1 WHERE id = %s',(container,id)) 
                    mysql.connection.commit()
                    cur.close()
                    RxLogs = GetRxLogs (container, id) #container.short_id
                    RxLogs.start()
                    time.sleep(4)
                    if RxLogs.connected == 1:  #Chequeamos el estado de la conexión de SRT para ver si esta connected o connecting.
                        return jsonify({"RX": ""+ srtRxCommand.name +" Connected and Started Successfully"}), 200   
                    else:
                        return jsonify({"RX": ""+ srtRxCommand.name +" Started but Waiting for Connection"}), 202             
                else:
                    #return jsonify({"Error": ""+ error +""}), 500
                    return jsonify({"RX": "Container Initialization Error"}), 500
            else:
                return jsonify({"RX": "RX with ID "+ id +" Already Exists"}), 409
        else:   
            return jsonify({"RX": "RX with ID "+ id +" Already Started"}), 409
    else:     
        return jsonify({"RX": "RX with ID " +id+ " do not exist."}), 404

### STOP RX ###

@app.route ('/v1/stop_rx/<string:id>', methods = ['POST'])
@jwt_required
def stopRx(id):
    if request.method != 'POST':
        return  
    #Check if ID already exists
    cur = mysql.connection.cursor()
    cur.execute('SELECT * FROM rx WHERE id = %s', [id])
    data = cur.fetchall()
    cur.close()
    if len(data) >= 1: #Si existen datos sobre ese Id, hacemos stop si status es 1.
        for row in data:
            containerId = str(row[8])
            status = row[9]
        if status == 1:
            container = client.containers.get(containerId)
            container.stop()
            container.remove()
            time.sleep(3)
            cur = mysql.connection.cursor()
            cur.execute('UPDATE rx SET container_id = "", status = 0, link_rtt = "0", link_bandwidth = "0", link_maxbandwidth = "0", recv_packets = "0", recv_packetsUnique = "0", recv_packetsLost = "0", recv_packetsDropped = "0", recv_packetsRetransmitted = "0", recv_packetsBelated = "0", recv_packetsFilterExtra = "0", recv_packetsFilterSupply = "0", recv_packetsFilterLoss = "0", recv_bytes = "0", recv_bytesUnique = "0", recv_bytesLost = "0", recv_bytesDropped = "0", recv_mbitRate = "0", line_loss = "0.0", connected= "0", reconnections= "0"  WHERE id = %s',[id])
            mysql.connection.commit()
            cur.close() 
            return jsonify({"RX": "RX with ID "+ id +" Stopped Successfully"}), 200
        else:
            return jsonify({"RX": "RX with ID "+ id +" Already Stopped"}), 409
    else:     
        if len(data) == 0:
            return jsonify({"RX": "RX with ID " +id+ " do not exist."}), 404

### RESTART RX ####

@app.route ('/v1/restart_rx/<string:id>', methods = ['POST'])
@jwt_required
def restartRx(id):  # sourcery skip
    if request.method != 'POST':
        return  
    #Check if ID already exists and get some parameters
    cur = mysql.connection.cursor()
    cur.execute('SELECT * FROM rx WHERE id = %s', [id])
    data = cur.fetchall()
    for row in data:
            containerId = str(row[8])
            status = row[9]   
    cur.close()
    if len(data) >= 1: #Si devuelve datos la query y status == 1 reseteamos (ponemos a 0 estadisticas), lanzamos de nuevo GetLogs, y dependiendo del estado de conexión respondemos connected o connecting.
        if status == 1:
            container = client.containers.get(containerId)
            container.restart()
            cur = mysql.connection.cursor()
            cur.execute('UPDATE rx SET link_rtt = "0", link_bandwidth = "0", link_maxbandwidth = "0", recv_packets = "0", recv_packetsUnique = "0", recv_packetsLost = "0", recv_packetsDropped = "0", recv_packetsRetransmitted = "0", recv_packetsBelated = "0", recv_packetsFilterExtra = "0", recv_packetsFilterSupply = "0", recv_packetsFilterLoss = "0", recv_bytes = "0", recv_bytesUnique = "0", recv_bytesLost = "0", recv_bytesDropped = "0", recv_mbitRate = "0", line_loss = "0.0" WHERE id = %s',[id])
            mysql.connection.commit()
            cur.close()
            rxLogs = GetRxLogs (container.short_id, id) 
            rxLogs.start()
            time.sleep(4)
            if rxLogs.connected == 1:
                return jsonify({"RX": "RX with ID "+ id +" Restarted Successfully"}), 200
            else:
                return jsonify({"RX": "RX_"+ id +" Restarted but Waiting for Connection"}), 202
        else:
            return jsonify({"RX": "RX with ID "+ id +" is Stopped"}), 409
    else:     
        if len(data) == 0:
            return jsonify({"RX": "RX with ID " +id+ " do not exist."}), 404

########################
###### CLASSES #########
########################
 
#Clase para generar el comando SRT apropiado para TX o RX.          
class CreateSrtCommand(Thread):
    def __init__(self, data, id, mode):
        self.id = id
        self.data = data
        self.mode = mode
        super(CreateSrtCommand, self).__init__()
        self.command = None
        self.name = None 
    
    def run(self):
        if self.mode == "rx": #Si es para RX, conseguimos todos los parametros, hacemos calculos y generamos el comando según lo obtenido.
            for row in self.data:
                mode = row[1]
                txIp = row[2]
                listenPort = str(row[3])
                udpOut = row[4]
                latency = str(row[5])
                srtBw = row[6]
                serviceName = row[7]
                passPhrase = row[30]
                pbKeyLen = str(row[31])
                tlpktDrop = str(row [32])
                ttl = str(row [33])
                packetFilter = row[34]
                fecCols = str(row [35])
                fecRows = str(row [36])
                fecLayout = row [37]
                fecArq = row [38]
                congestion = row [39]
                transType = row [40]
                groupConnect = row [41]
                groupType = row [42]
                groupIp1 = row [43]
                groupIp2 = row [44]
                ip1Weight = row [45]
                ip2Weight = row [46]  

            bw = int((srtBw * 1000)/8)  #Pasamos srt_bw de Kbps a Bps
            header = "-s:1000 -pf:json -f -t:-1"
            in1 = "-i 'srt://"+ txIp +":"+ listenPort +"?mode="+ mode +""
            parameters = "&latency="+ latency +"&maxbw=" + str(bw) +"&congestion="+ congestion +"&transtype="+ transType +"&tlpktdrop="+ tlpktDrop +""
            end = "'"
            output = "-o 'udp://"+ udpOut +"?ttl="+ ttl +"'"   
            if groupConnect == 1:
                header = "-s:1000 -pf:json -f -t:-1 -g"
                in1 = "-i 'srt://*:?type="+ groupType +""
                ip1 = "'"+ groupIp1 +"?weight="+ ip1Weight +"'"
                ip2 = "'"+ groupIp2 +"?weight="+ ip2Weight +"'"
                out1 = "-o 'udp://"+ udpOut +"?ttl="+ ttl +"'"
                output = " ".join((ip1, ip2, out1))
            if len(passPhrase) >= 8:
                    passPhrase = "&passphrase="+ passPhrase +"&pbkeylen="+ pbKeyLen +""
                    parameters = "".join((parameters, passPhrase))
            if packetFilter == 1:
                fec = "&packetfilter=fec,cols:"+ fecCols +",rows:"+ fecRows +""
                parameters = "".join((parameters, fec))
                if len(fecLayout) >= 1:
                    layout = ",layout:"+ fecLayout +""
                    parameters = "".join((parameters, layout))
                if len(fecArq) >= 1:
                    arq = ",arq:"+ fecArq +""
                    parameters = "".join((parameters, arq))
            input = "".join((in1, parameters, end))
            self.command = " ".join((header, input, output))
            self.name = "RX_"+self.id+"_"+serviceName+""
            
        elif self.mode == "tx": #si es un TX, conseguimos todos los parametros, hacemos calculos y generamos el comando según lo obtenido.
            for row in self.data:
                mode = row[1]
                rxIp = row[2]
                listenPort = str(row[3])
                udpIn = row[4]
                latency = str(row[5])
                srtBw = row[6]
                serviceName = row[7]
                passPhrase = row[26]
                pbKeyLen = str(row [27])
                groupConnect = row [28]
                inputBw = row [29]
                overHead = str(row [30])
                packetFilter = row[31]
                fecCols = str(row [32])
                fecRows = str(row [33])
                fecLayout = row [34]
                fecArq = row [35]
                congestion = row [36]
                transType = row [37]

            bw = int((srtBw * 1000)/8)  #Pasamos srtBw de Kbps a Bps
            iBw = int((inputBw * 1000)/8)  #Pasamos inputBw de Kbps a Bps
            rcvBuf = iBw * 2 #Ponemos que el buffer de recepción sea el doble de la inputbw
            header = "-s:1000 -pf:json -f -t:-1"
            input = "-i 'udp://"+ udpIn +"?rcvbuf="+ str(rcvBuf) +"'"
            parameters = "&latency=" +latency+"&maxbw="+ str(bw) +"&inputbw="+ str(iBw) +"&oheadbw="+ overHead +"&congestion="+ congestion +"&transtype="+ transType +""
            end = "'"
            out1 = "-o 'srt://"+ rxIp +":"+ listenPort +"?mode="+ mode +""
            if groupConnect == 1:
                out1 = "-o 'srt://"+ rxIp +":"+ listenPort +"?groupconnect=1"
            if len(passPhrase) >= 8:
                passPhrase = "&passphrase="+ passPhrase +"&pbkeylen="+ pbKeyLen +""
                parameters = "".join((passPhrase, parameters))
            if packetFilter == 1:
                fec = "&packetfilter=fec,cols:"+ fecCols +",rows:"+ fecRows +""
                parameters = "".join((parameters, fec))
                if len(fecLayout) >= 1:
                    layout = ",layout:"+ fecLayout +""
                    parameters = "".join((parameters, layout))
                if len(fecArq) >= 1:
                    arq = "&arq:"+ fecArq +""
                    parameters = "".join((parameters, arq))
                   
            output = "".join((out1, parameters, end))
            self.command = " ".join((header, input, output))
            self.name = "TX_"+self.id+"_"+serviceName+""

#Clase para crear el contenedor de SRT en base al comando generado en CreateSrtCommand                
class CreateSrtContainer(Thread):
    def __init__(self, version, command, serviceName, mode, id):
        self.version = version
        self.command = command
        self.serviceName = serviceName
        self.mode = mode
        self.id = id
        super(CreateSrtContainer, self).__init__()
        self.status = None
        self.apiError = None
        self.cId = None
    
    def run(self):
        container = client.containers.run(self.version, 
                command= self.command,
                name= self.serviceName,
                remove=False,
                restart_policy= {"Name": "always"},
                network_mode="host",
                volumes_from="srt_api",
                volumes= {'/var/run/docker.sock':{'bind': '/var/run/docker.sock', 'mode': 'rw'}},
                privileged= False,
                stdin_open= False,
                tty= True,
                detach= True,
                stdout= True)

        #me gustaría comprobar mejor es status, pero no funciona bien container.status == "running"
        if container.id:
            #Update container ID and status
            self.status = "1"
            self.cId = container.short_id
        else:
            container.stop()
            container.remove()
            #Hay que ver como devolver y usar docker.APIError
            self.apiError = str(container.APIError)
            self.status = "0"
 
#Clase para comprobar que la udp de entrada en los TX existe/está activa, si no está no se lanza el TX.         
class LaunchFfprobe(Thread):
    
    def __init__(self, input, version, mode, id):
        self.input = input
        self.version = version
        self.mode = mode
        self.id = id
        super(LaunchFfprobe, self).__init__()
        self.bitrate = None
        self.error = None
        self.name = None
        
    def run(self):
        if self.mode == "tx":
            self.input = "udp://"+self.input+""
            self.name = "FFprobe_TX_"+self.id+""
        elif self.mode == "rx":
            self.input = "srt://"+self.input+""
            self.name = "FFprobe_RX_"+self.id+""
        container = client.containers.run(self.version, 
                command= "-v quiet -timeout 500000 -print_format json -show_format -show_error -i "+self.input+"",
                remove=True,
                name= self.name,
                network_mode="host",
                volumes= {'/var/run/docker.sock':{'bind': '/var/run/docker.sock', 'mode': 'rw'}},
                privileged= False,
                stdin_open= False,
                tty= True,
                detach= True,
                stdout= True)
        time.sleep(1)
        for log in container.attach(stream=True): #Chequeamos si en el log que sale de FFProbe existe la Línea que contenga Input/output error, si existe es que no hay udp de entrada.
            print(log)
            log = log.decode('utf-8')
            if re.search('Input/output error', log):
                self.error = "Input/output error. Maybe there is no active UDP source."

#New RX class to get RxLogs. Now it include other classes that were out before (better performance)
class GetRxLogs(Thread):

    def __init__(self, containerId, id):
        self.id = id
        self.cId = containerId
        self.connected = 0
        super(GetRxLogs, self).__init__()

    def run(self):
        container = client.containers.get(self.cId)
        reconnections = -1 #Seteamos reconnections a -1 ya que cuando entren en status == 1 y encontremos SRT source connected se habrá conectado por primera vez y reconnections valdrá 0.
        for log in container.attach(stream=True): #Chequeamos los logs del container, sacamos algunos parametros de la bbdd y buscamos coincidencias con algunas frases para ver el estado de la conexion y/o sacar estadísticas.
            with app.app_context():
                cur = mysql.connection.cursor()
                query = """SELECT * FROM rx WHERE id = %s"""
                cur.execute(query, (self.id,))
                data = cur.fetchall()
                cur.close()
            for row in data:
                serviceName = row[7]
                status = row[9]
                #connected = row[27]
            if status == 1:
                log = log.decode('utf-8')
                if re.search('bytes received',log): #Si recibimos esta linea en el log, continuamos. A veces esto rompía el proceso de lectura del log.
                        continue
                elif re.search('SRT source connected',log): #Se ha conectado al TX, actualizamos campos de la bbdd. Cada vez que detecte esta línea, reconnections +1 y self.connected = 1
                    reconnections += 1
                    with app.app_context():
                        cur = mysql.connection.cursor()
                        cur.execute('UPDATE rx SET connected = "1", reconnections = %s WHERE id = %s',(reconnections, self.id,))
                        mysql.connection.commit()
                        cur.close()
                    self.connected = 1
                elif re.search('SRT source disconnected',log): #El TX se ha desconectado, actualizamos varios parametos de la bbdd y ponemos a 0 algunas de las estadísticas.
                    with app.app_context():
                        cur = mysql.connection.cursor()
                        cur.execute('UPDATE rx SET connected = "2", link_rtt = "0", link_bandwidth = "0", link_maxbandwidth = "0", recv_mbitRate = "0" WHERE id = %s',[self.id])
                        mysql.connection.commit()
                        cur.close()
                    self.connected = 2
                else:
                    if self.connected == 1: #Si hemos entrado en SRT source connected procedemos a cargar el JSON de stats para leerlas y meterlas en bbdd.
                        try: #A veces fallaba aleatoriamente la lectura del json de salida del container para leer stats, con este try/except lo evitamos.
                            line = json.loads(log)
                        except Exception as e: 
                            # Logs the error appropriately.
                            print (""+ serviceName +" Error:", e)
                            continue
                        try: #A veces fallaba aleatoriamente la lectura del json de salida del container para leer stats, con este try/except lo evitamos.

                            with app.app_context():
                                cur = mysql.connection.cursor()
                                cur.execute('UPDATE rx SET link_rtt = %s, link_bandwidth = %s, link_maxbandwidth = %s, recv_packets = %s, recv_packetsUnique = %s, recv_packetsLost = %s, recv_packetsDropped = %s, recv_packetsRetransmitted = %s, recv_packetsBelated = %s, recv_packetsFilterExtra = %s, recv_packetsFilterSupply = %s, recv_packetsFilterLoss = %s, recv_bytes = %s, recv_bytesUnique = %s, recv_bytesLost = %s, recv_bytesDropped = %s, recv_mbitRate = %s, line_loss = %s WHERE id = %s', 
                                        (line['link']['rtt'], 
                                        line['link']['bandwidth'],
                                        line['link']['maxBandwidth'], 
                                        line['recv']['packets'], 
                                        line['recv']['packetsUnique'], 
                                        line['recv']['packetsLost'], 
                                        line['recv']['packetsDropped'], 
                                        line['recv']['packetsRetransmitted'], 
                                        line['recv']['packetsBelated'], 
                                        line['recv']['packetsFilterExtra'], 
                                        line['recv']['packetsFilterSupply'], 
                                        line['recv']['packetsFilterLoss'], 
                                        line['recv']['bytes'], 
                                        line['recv']['bytesUnique'], 
                                        line['recv']['bytesLost'], 
                                        line['recv']['bytesDropped'], 
                                        line['recv']['mbitRate'], 
                                        ((int(line['recv']['packetsLost'])/int(line['recv']['packets']))*100),
                                        self.id))
                                mysql.connection.commit()
                                cur.close()      
                        except Exception as e: 
                            # Logs the error appropriately.
                            print (""+ serviceName +" Error:", e)
                            continue
                        
#New TX class to get RxLogs. Now it include other classes that were out before (better performance)
class GetTxLogs(Thread):

    def __init__(self, containerId, id):
        self.id = id
        self.cId = containerId
        self.connected = 0
        super(GetTxLogs, self).__init__()

    def run(self):
        container = client.containers.get(self.cId)
        reconnections = -1 #Seteamos reconnections a -1 ya que cuando entren en status == 1 y encontremos SRT source connected se habrá conectado por primera vez y reconnections valdrá 0.
        for log in container.attach(stream=True): #Chequeamos los logs del container, sacamos algunos parametros de la bbdd y buscamos coincidencias con algunas frases para ver el estado de la conexion y/o sacar estadísticas.
            with app.app_context():
                cur = mysql.connection.cursor()
                query = """SELECT * FROM tx WHERE id = %s"""
                cur.execute(query, (self.id,))
                data = cur.fetchall()
                cur.close()
            for row in data:
                serviceName = row[7]
                status = row[9]
                #connected = row[23]
            if status == 1:
                log = log.decode('utf-8')
                if re.search('bytes received',log): #Si recibimos esta linea en el log, continuamos. A veces esto rompía el proceso de lectura del log.
                    continue       
                elif re.search('Accepted SRT target connection',log): #El RX se ha conectado, actualizamos campos de la bbdd. Cada vez que detecte esta línea, reconnections +1 y self.connected = 1
                    reconnections += 1
                    with app.app_context():
                        cur = mysql.connection.cursor()
                        cur.execute('UPDATE tx SET connected = "1", reconnections = %s WHERE id = %s',(reconnections, self.id,))
                        mysql.connection.commit()
                        cur.close()
                    self.connected = 1
                elif re.search('SRT target disconnected',log):  #El RX se ha desconectado, actualizamos varios parametos de la bbdd y ponemos a 0 algunas de las estadísticas.
                    with app.app_context():
                        cur = mysql.connection.cursor()
                        cur.execute('UPDATE tx SET connected = "2", link_rtt = "0", link_bandwidth = "0", link_maxbandwidth = "0",send_mbitRate = "0" WHERE id = %s',[self.id])
                        mysql.connection.commit()
                        cur.close()
                    self.connected = 2 
                else:
                    if self.connected == 1: #Si hemos entrado en SRT source connected procedemos a cargar el JSON de stats para leerlas y meterlas en bbdd.
                        try: #A veces fallaba aleatoriamente la lectura del json de salida del container para leer stats, con este try/except lo evitamos.
                            line = json.loads(log)
                        except Exception as e:
                            print (""+ serviceName +" Error:", e)
                            continue
                        try: #A veces fallaba aleatoriamente la lectura del json de salida del container para leer stats, con este try/except lo evitamos.
                            with app.app_context():
                                cur = mysql.connection.cursor()
                                cur.execute('UPDATE tx SET link_rtt = %s, link_bandwidth = %s, link_maxbandwidth = %s, send_packets = %s, send_packetsUnique = %s, send_packetsLost = %s, send_packetsDropped = %s, send_packetsRetransmitted = %s, send_packetsFilterExtra = %s, send_bytes = %s, send_bytesUnique = %s, send_bytesDropped = %s ,send_mbitRate = %s, line_loss = %s WHERE id = %s', 
                                        (line['link']['rtt'], 
                                        line['link']['bandwidth'],
                                        line['link']['maxBandwidth'], 
                                        line['send']['packets'], 
                                        line['send']['packetsUnique'], 
                                        line['send']['packetsLost'], 
                                        line['send']['packetsDropped'], 
                                        line['send']['packetsRetransmitted'], 
                                        line['send']['packetsFilterExtra'], 
                                        line['send']['bytes'], 
                                        line['send']['bytesUnique'], 
                                        line['send']['bytesDropped'], 
                                        line['send']['mbitRate'],
                                        ((int(line['send']['packetsLost'])/int(line['send']['packets']))*100), 
                                        self.id))
                                mysql.connection.commit()
                                cur.close()
                        except Exception as e:
                            print (""+ serviceName +" Error:", e)
                            continue
