API REST en Python/Flask + gunicorn + NGINX para manejar contenedores de SRT (https://github.com/Haivision/srt)

- Building the app:

For building the app you only need to run "docker-compose up --build"


Chequear Issues en srt (por si hay algún problema que se pueda parecer a algo que nos pase -> https://github.com/Haivision/srt/issues)

- Endpoints reciben y envían JSON para la configuración o las respuestas de la API: 
    
    @ [POST] login (http://10.40.80.166:4000/v1/login) para conseguir el token de acceso, “username”: “admin”, “password”: “password”

        ```
        {
	    "username": "admin",
	    "password": "password"
        }
        ```

    - Devuelve:

            ```
            {
            "jwt": "eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJleHAiOjE2MDY5MjIwMTcsImlhdCI6MTYwNjkxODQxNywibmJmIjoxNjA2OTE4NDE3LCJzdWIiOiJyYWRpb3MifQ.R7Kh9noCunu8AImWpy3Uuiy-kqIKiYKdNKZ5NnYNIjI",
            "exp": "Wed, 09 Dec 2020 14:13:37 GMT"
            }
            ```

        Hay que incluír éste Token en las peticiones que queramos hacer para que funcione. Se haría en la petición Auth -> BearerToken -> campo Token

    @ [GET] protected (http://10.40.80.166:4000/v1/protected) para testear que la validación del token ha funcionado.

    - Devuelve: 

            ```
            {
            "Granted Access": "titania"
            }
            ```

    - ENDPOINTS RX (Si OK devuelven 200OK y un texto con nombre del servicio e ID, si van mal devuelven error 500 con alguna explicación. Las respuestas en JSON, excepto algunas de codigo o docker que pueden devolver en html con otros codigos.)

        @ [POST] add_rx (http://10.40.80.166:4000/v1/add_rx)
        ```
        {
            "ServiceName": "Test_3",
            "Mode": "caller",
            "TxIp": "80.84.129.34",
            "ListenPort": 7005,
            "UdpOut": "224.10.10.149:5009",
            "Latency": 700,
            "SrtBw": 12000,
            "PassPhrase": "mansillasoto",
            "PbKeyLen": 16,
            "TlpktDrop": 0,
            "TTL": 24,
            "FEC": 0,
            "FecCols": 10,
            "FecRows": 5,
            "FecLayOut": "staircase",
            "FecArq": "always",
            "Congestion": "live",
            "TransType": "live",
            "GroupConnect": 0,
            "GroupType": "backup",
            "GroupIp1": "",
            "GroupIp2": "",
            "Ip1Weight": 60,
            "Ip2Weight": 40
        }
        ```

        @ [PUT] update_rx (http://10.40.80.166:4000/v1/update_rx/id)
        ```
        {
            "ServiceName": "Test_3",
            "Mode": "caller",
            "TxIp": "80.84.129.34",
            "ListenPort": 7005,
            "UdpOut": "224.10.10.149:5009",
            "Latency": 700,
            "SrtBw": 12000,
            "PassPhrase": "mansillasoto",
            "PbKeyLen": 16,
            "TlpktDrop": 0,
            "TTL": 24,
            "FEC": 0,
            "FecCols": 10,
            "FecRows": 5,
            "FecLayOut": "staircase",
            "FecArq": "always",
            "Congestion": "live",
            "TransType": "live",
            "GroupConnect": 0,
            "GroupType": "backup",
            "GroupIp1": "",
            "GroupIp2": "",
            "Ip1Weight": 60,
            "Ip2Weight": 40
        }
        ```

        @ [DELETE] delete_rx (http://10.40.80.166:4000/v1/delete_rx/id)

        @ [GET] get_rx (http://10.40.80.166:4000/v1/get_rx)
        ```
        [
            {
                "Id": 5,
                "ServiceName": "Test_3",
                "Mode": "caller",
                "TxIp": "80.84.129.34",
                "ListenPort": 7005,
                "UdpOut": "224.10.10.149:5009",
                "Latency": 700,
                "SrtBw": 12000,
                "PassPhrase": "mansillasoto",
                "PbKeyLen": 16,
                "TlpktDrop": 0,
                "TTL": 24,
                "FEC": 0,
                "FecCols": 10,
                "FecRows": 5,
                "FecLayout": "staircase",
                "FecArq": "always",
                "Congestion": "live",
                "TransType": "live",
                "GroupConnect": 0,
                "GroupType": "backup",
                "GroupIp1": "",
                "GroupIp2": "",
                "Ip1Weight": 60,
                "Ip2Weight": 40,
                "Status": 1,
                "ContainerId": "93f1946d9e",
                "LinkRtt": 33.423,
                "LinkBandwidth": 102.78,
                "LinkMaxBandwidth": 12.0,
                "RecvPackets": 273412,
                "RecvPacketsUnique": 273412,
                "RecvPacketsLost": 0,
                "RecvPacketsDropped": 0,
                "RecvPacketsRetransmitted": 0,
                "RecvPacketsBelated": 0,
                "RecvPacketsFilterExtra": 0,
                "RecvPacketsFilterSupply": 0,
                "RecvPacketsFilterLoss": 0,
                "RecvBytes": 371840320,
                "RecvBytesUnique": 371840320,
                "RecvBytesLost": 0,
                "RecvBytesDropped": 0,
                "RecvMbitRate": 6.40594,
                "LineLoss": 0.0,
                "Connected": 1,
                "Reconnections": 0
            }
        ]
        ```

        @ [GET] get_one_rx (http://10.40.80.166:4000/v1/get_rx/id)
        ```
        {
            "Id": 5,
            "ServiceName": "Test_3",
            "Mode": "caller",
            "TxIp": "80.84.129.34",
            "ListenPort": 7005,
            "UdpOut": "224.10.10.149:5009",
            "Latency": 700,
            "SrtBw": 12000,
            "PassPhrase": "mansillasoto",
            "PbKeyLen": 16,
            "TlpktDrop": 0,
            "TTL": 24,
            "FEC": 0,
            "FecCols": 10,
            "FecRows": 5,
            "FecLayout": "staircase",
            "FecArq": "always",
            "Congestion": "live",
            "TransType": "live",
            "GroupConnect": 0,
            "GroupType": "backup",
            "GroupIp1": "",
            "GroupIp2": "",
            "Ip1Weight": 60,
            "Ip2Weight": 40,
            "Status": 1,
            "ContainerId": "5c083b179c",
            "LinkRtt": 35.497,
            "LinkBandwidth": 109.656,
            "LinkMaxBandwidth": 12.0,
            "RecvPackets": 8320,
            "RecvPacketsUnique": 8320,
            "RecvPacketsLost": 0,
            "RecvPacketsDropped": 0,
            "RecvPacketsRetransmitted": 0,
            "RecvPacketsBelated": 0,
            "RecvPacketsFilterExtra": 0,
            "RecvPacketsFilterSupply": 0,
            "RecvPacketsFilterLoss": 0,
            "RecvBytes": 11315200,
            "RecvBytesUnique": 11315200,
            "RecvBytesLost": 0,
            "RecvBytesDropped": 0,
            "RecvMbitRate": 5.56379,
            "LineLoss": 0.0,
            "Connected": 1,
            "Reconnections": 1
        }
        ```

        @ [POST] start_rx (http://10.40.80.166:4000/v1/start_rx/id)

        @ [POST] stop_rx (http://10.40.80.166:4000/v1/stop_rx/id)

        @ [POST] restart_rx (http://10.40.80.166:4000/v1/restart_rx/id)


    - ENDPOINTS TX (Si OK devuelven 200OK y un texto con nombre del servicio e ID, si van mal devuelven error del rango 400 con alguna explicación. Las respuestas en JSON, excepto algunas de codigo o docker que pueden devolver en html con otros codigos.)

        @ [POST] add_tx (http://10.40.80.166:4000/v1/add_tx)
        ```
        {
            "ServiceName": "TX_2",
            "Mode": "listener",
            "RxIp": "",
            "ListenPort": "9001",
            "UdpIn": "224.10.10.149:5009",
            "Latency": "700",
            "SrtBw": "12000",
            "PassPhrase": "",
            "PbKeyLen": 16,
            "GroupConnect": 0,
            "InputBw": 0,
            "OverHead": 25,
            "FEC": 0,
            "FecCols": 0,
            "FecRows": 0,
            "FecLayOut": "staircase",
            "FecArq": "always",
            "Congestion": "live",
            "TransType": "live"
        }
        ```

        @ [PUT] update_tx (http://10.40.80.166:4000/v1/update_tx/id)
        ```
        {
            "ServiceName": "TX_2",
            "Mode": "listener",
            "RxIp": "",
            "ListenPort": "9001",
            "UdpIn": "224.10.10.149:5009",
            "Latency": "700",
            "SrtBw": "12000",
            "PassPhrase": "",
            "PbKeyLen": 16,
            "GroupConnect": 0,
            "InputBw": 0,
            "OverHead": 25,
            "FEC": 0,
            "FecCols": 0,
            "FecRows": 0,
            "FecLayOut": "staircase",
            "FecArq": "always",
            "Congestion": "live",
            "TransType": "live"
        }
        ```

        @ [DELETE] delete_tx (http://10.40.80.166:4000/v1/delete_tx/id)

        @ [GET] get_tx (http://10.40.80.166:4000/v1/get_tx)
        ```
        [
            {
                "Id": 6,
                "ServiceName": "TX_2",
                "Mode": "listener",
                "RxIp": "",
                "ListenPort": 9001,
                "UdpIn": "224.10.10.149:5009",
                "Latency": 2000,
                "SrtBw": 12000,
                "PassPhrase": "",
                "PbKeyLen": 16,
                "GroupConnect": 0,
                "InputBw": 6000,
                "OverHead": 25,
                "PacketFilter": 0,
                "FecCols": 0,
                "FecRows": 0,
                "FecLayout": "staircase",
                "FecArq": "always",
                "Congestion": "live",
                "TransType": "live",
                "Status": 1,
                "ContainerId": "14ef4d1a59",
                "LinkRtt": 40.367,
                "LinkBandwidth": 2094.08,
                "LinkMaxBandwidth": 12.0,
                "SendPackets": 4095,
                "SendPacketsUnique": 3990,
                "SendPacketsLost": 105,
                "SendPacketsDropped": 0,
                "SendPacketsRetransmitted": 105,
                "SendPacketsFilterExtra": 0,
                "SendBytes": 5569200,
                "SendBytesUnique": 5426400,
                "SendBytesDropped": 0,
                "SendMbitRate": 6.56343,
                "LineLoss": 2.5641,
                "Connected": 1,
                "Reconnections": 1
            }
        ]
        ```

        @ [GET] get_one_tx (http://10.40.80.166:4000/v1/get_tx/id)
        ```
        {
            "Id": 6,
            "ServiceName": "TX_2",
            "Mode": "listener",
            "TxIp": "",
            "ListenPort": 9001,
            "UdpOut": "224.10.10.149:5009",
            "Latency": 2000,
            "SrtBw": 12000,
            "PassPhrase": "",
            "PbKeyLen": 16,
            "GroupConnect": 0,
            "InputBw": 6000,
            "OverHead": 25,
            "PacketFilter": 0,
            "FecCols": 0,
            "FecRows": 0,
            "FecLayout": "staircase",
            "FecArq": "always",
            "Congestion": "live",
            "TransType": "live",
            "Status": 0,
            "ContainerId": "",
            "LinkRtt": 0.0,
            "LinkBandwidth": 0.0,
            "LinkMaxBandwidth": 0.0,
            "SendPackets": 0,
            "SendPacketsUnique": 0,
            "SendPacketsLost": 0,
            "SendPacketsDropped": 0,
            "SendPacketsRetransmitted": 0,
            "SendPacketsFilterExtra": 0,
            "SendBytes": 0,
            "SendBytesUnique": 0,
            "SendBytesDropped": 0,
            "SendMbitRate": 0.0,
            "LineLoss": 0.0,
            "Connected": 0,
            "Reconnections": 0
        }
        ```

        @ [POST] start_tx (http://10.40.80.166:4000/v1/start_tx/id)

        @ [POST] stop_tx (http://10.40.80.166:4000/v1/stop_tx/id)

        @ [POST] restart_tx (http://10.40.80.166:4000/v1/restart_tx/id)


Parámetros SRT:
    
    - ServiceName: Nombre de la sesión de TX o RX

    - Mode: Caller, Listener o Rendezvous (En Rendezvous, tanto RX como TX tienen que poner el mismo ListenPort)

    - RxIp: IP de destino al que manda un TX. Si es un TX Listener no hay que poner IP. Solo necesario para TX REndezvous y TX Caller.

    - TxIp: Ip de origen a la que conecta un RX ya sea como Caller o en Rendezvous. Si es RX Listener no se usaría.

    - ListenPort: Puesto de escucha en el que se publica el TX o al que se conecta el RX.

    - UdpIn: Udp de entrada para un TX.

    - UdpOut: Udp de salida para un RX.

    - TTL: para la udp out del RX.

    - Latency: Latencia para la sesión SRT. Normalmente 4 x RTT (ping). En ms.

    - SrtBw: Ancho de banda máximo a utilizar en la sesión SRT. (Kbps) 12000 = 12Mbps. Tendría que ser al menos la suma del InputBw + OverHead

    - InputBw: Ancho de banda de la multicast de entrada en TX. (Kbps) 12000 = 12Mbps

    - OverHead: Porcentaje de la conexión para recuperar errores SRT. Por defecto 25% sobre el InputBw.

    - Connected: 0 (disconnected), 1 (connected), 2 (waiting for connection en TX, Connecting en RX)

    - Status: 1 (running) o 0 (stopped)

    - Reconnections: Numero de reconexiones que ha habido durante una sesión SRT entre TX y RX.

    - Congestion: Live por defecto si no hay otro parámetro, también podría ser file, pero no lo usamos.

    - Transtype: Live o File, siempre hacemos live por defecto.

    - TlpktDrop: Sólo en RX. 0 o 1 (TooLatePacketDrop) descartar paquetes que llegan muy tarde.

    - PassPhrase: Clave para encriptar una Transmisión SRT. Al menos debe tener 8 caracteres y debe ser la misma en ambos extremos.
    
    - PbKeyLen: Bits para el cifrado de la clave. Default 16. (16,24,32) Necesario solo en TX.

    Experimentales:

        - FEC: Para añadir un extra de FEC a la transmisión. En ambos extremos deberemos configurar lo mismo.

            - FEC: 0 o 1 (desactivado, activado)

            - FecCols = INT > 0 por defecto lo tengo en 10

            - FecRows = INT > 0 por defecto lo tengo en 5

            - FecLayOut = even / staircase (por defecto lo pongo en staircase)

            - FecArq = always / onreq / never (Automatic Repeat Request (ARQ) protocol) por defecto lo tengo en always. Puede ser On Request o Never.

        - Bonding: Para poder salir por diferentes IPs de una máquina y recibir por diferentes caminos en otra.

            Para TX y RX:
            
                - GroupConnect = Para usar Bonding. 0 o 1.

            Para RX:

                - GroupType = De momento sólo podemos usar backup. Se podrán configurar dos IPs del TX para recibir de ellas y que SRT elija la mejor en cada momento. Hay que esperar para meter Broadcast y Balancing.

                - GroupIp1 = Ip Primaria de RX

                - GroupIp2 = Ip Secundaria de RX

                - Ip1Weight = Peso para la RX de Ip Primaria

                - Ip2Weight = Peso para la RX de Ip Secundaria

- Estadisticas SRT:

    RX: 

        ```
        "LinkRtt": 35.497,              -> PINg entre TX y RX
        "LinkBandwidth": 109.656,       -> Ancho de banda estimado de la conexión (lo calcula SRT)
        "LinkMaxBandwidth": 12.0,       -> Ancho de banda máximo a usar en la conexión, establecido por el usuario.
        "RecvPackets": 8320,            -> Paquetes recibidos en el intervalo X -> ahora está configurado a 1000 paquetes por intervalo.
        "RecvPacketsUnique": 8320,      -> Paquetes únicos recibidos en el intervalo X
        "RecvPacketsLost": 0,           -> Paquetes perdidos en el intervalo X
        "RecvPacketsDropped": 0,        -> Paquetes dropeados en el intervalo X
        "RecvPacketsRetransmitted": 0,  -> Paquetes retransmitidos en el intervalo X
        "RecvPacketsBelated": 0,        -> Paquetes que han llegado más tarde de lo esperado en el intervalo X
        "RecvPacketsFilterExtra": 0,    -> The total number of packet filter control packets received by the packet filter
        "RecvPacketsFilterSupply": 0,   -> The total number of lost DATA packets recovered by the packet filter at the receiver side
        "RecvPacketsFilterLoss": 0,     -> The total number of lost DATA packets not recovered by the packet filter at the receiver side
        "RecvBytes": 11315200,          -> Bytes recibidos en el intervalo X
        "RecvBytesUnique": 11315200,    -> Bytes únicos recibidos en el intervalo X
        "RecvBytesLost": 0,             -> Bytes perdidos en el intervalo X
        "RecvBytesDropped": 0,          -> Bytes dropeados en el intervalo X
        "RecvMbitRate": 5.56379,        -> Bitrate recibido en el intervalo X
        "LineLoss": 0.0,                -> Tasa Media de pérdida en % de la conexión entre RX y TX
        ```

    TX:

        ```
        "LinkRtt": 0.0,                 -> PINg entre TX y RX
        "LinkBandwidth": 0.0,           -> Ancho de banda estimado de la conexión (lo calcula SRT)
        "LinkMaxBandwidth": 0.0,        -> Ancho de banda máximo a usar en la conexión, establecido por el usuario.
        "SendPackets": 0,               -> Paquetes enviados en el intervalo X -> ahora está configurado a 1000 paquetes por intervalo.
        "SendPacketsUnique": 0,         -> Paquetes únicos enviados en el intervalo X
        "SendPacketsLost": 0,           -> Paquetes perdidos en el intervalo X
        "SendPacketsDropped": 0,        -> Paquetes dropeados en el intervalo X
        "SendPacketsRetransmitted": 0,  -> Paquetes retransmitidos en el intervalo X
        "SendPacketsFilterExtra": 0,    -> The total number of packet filter control packets generated by the packet filter
        "SendBytes": 0,                 -> Bytes enviados en el intervalo X
        "SendBytesUnique": 0,           -> Bytes únicos enviados en el intervalo X
        "SendBytesDropped": 0,          -> Bytes dropeados en el intervalo X
        "SendMbitRate": 0.0,            -> Bitrate usado en la TX en el intervalo X
        "LineLoss": 0.0,                -> Tasa Media de pérdida en % de la conexión entre TX y RX
        ```
    
    Las estadísticas que pone Bytes incluyen payload y todos los headers (20 bytes IPv4 + 8 bytes UDP + 16 bytes SRT)


- Contenedores -> (Dockerfile.app) & (Dockerfile.bbdd) & (Dockerfile.nginx)

    - API: contenedor de Python para ejecutar el código de la API. Usa el puerto TCP 4000 para recibir peticiones.
    - BBDD: base de datos MariaDB (SQL) para guardar los parámetros de cada TX/RX. Puerto TCP 3306
    - NGINX: Servidor web para enrutar peticiones a la api pasando por gunicorn (servidor de aplicaciones)

- Gunicorn: recibe las peticiones del cliente, a través del servidor web, y las transforma para que se ejecute el código correspondiente de la aplicación. Ejecutaremos gunicorn con X workers, que serán los encargados de recibir peticiones en paralelo para gestionar la carga. Ahora está en 3 workers, se configura en el apartado CDM del Dockerfile.app

    - Ejecutar un grupo de procesos/subprocesos de trabajo
    - Traducir las solicitudes procedentes de Nginx (u otro servidor web) para que sean compatibles con WSGI
    - Llamar al código de Python cuando llega una solicitud
    - Traducir las respuestas WSGI de su aplicación en respuestas http adecuadas

- Estructura:
```
    srt_api_
        _api_
            _app.py (codigo de la aplicación)
            _wsgi.py (iniciamos la app)
        _k8s_
            _testing with kubernetes files
        _mariadb_
            _my.cnf (config file for mariadb)
            _srt_db.sql (srt empty database file)
        _nginx_
            _nginx.conf (config file for nginx)
        .env (No se usa todavía. archivo para guardar las variables "confidenciales")
        .gitignore
        docker.compose.yml (phpmyadmin can be deleted, its only for ddbb debugging)
        Dockerfile.app (for buiding api container)
        Dockerfile.bbdd (for buiding bbdd container)
        Dockerfile.nginx (for buiding nginx container)
        Jenkinsfile (job for https://jenkins.overon.es:90/view/Innovaci%C3%B3n/job/build_srt_api/)
        README.md (API Documentation)
        requirements.txt (requisitos para el codigo de la api)
        _venv_ (entorno virtual para desarrollo en local de la aplicación)
```

- ToDo:
 
    - Probar FEC, los comandos no se rompen pero no se si funciona correctamente.
    - Probar Bonding.
    - Probar bien con Titania.
