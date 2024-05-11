<script type="text/javascript" 
  src="http://cdn.mathjax.org/mathjax/latest/MathJax.js?config=TeX-AMS-MML_HTMLorMML">
</script>
<script type="text/x-mathjax-config">
  MathJax.Hub.Config({ tex2jax: {inlineMath: [['$', '$']]}, messageStyle: "none" });
</script>

![alt text](img\track1.png)

### **Q1 ~ propose an overall design for the system, mainly focusing on the communication technology to be used. Motivate your choice** <br><br>


Measuring the states of each cellulose growing basins is a business-critical activity; guaranty performance, reliability error avoidance and recovery, must be our main goals. <br>
Consequently a battery-powered device is not advised to achieve sufficient reliability, even if it were possible to guarantee an autonomy of 14 days (or more) a non-swift battery replacement can delay required reactions to critical states causing the loss of 1 growing basin.<br>
Moreover, the direct power supply makes avoiding duty cycles (with deep sleep periods) possible, allowing researchers to require additional data asynchronously.
Since growing bacterial cellulose, as described, is a slow process, as default each basin will communicate its state once an hour, consequently allowing researchers to request additional data can be appropriate.<br>

**Peripheral network (very short range):**
![alt text](img\peripheral_network.png)<br>
The peripheral network is characterized by a very short distance between actors (sensor and cluster nodes), so it can be easily implemented through wired protocols. <br>
Each basin measure device comprises 3 independent sensor nodes (to enable error avoidance and simplify maintenance) connected to a cluster node through PoE (power over ethernet). The sensor nodes respectively measure the luminosity, sugar, and pH of the growing solution, they are programmed to perform 3 data measurements and send it to the cluster node as soon as they receive power.<br>
The cluster node act as a CoAP server, it's indipendently connected to each senor node throught ethernet connectivity (no possible collisions in this stage, ACK is not required) and it also provide power supply (turning on the device only when it require data). Its role is to aggregate senor data into a single message (each sensor perform multiple reading to mitigate measurements errors, and perform one CoAP post request for each reading) and send it to the coordinator.<br>
>**note:** this configuration allows sensor redundancy, if we attach multiple identical sensors to a cluster node, if (after raising power up) the main sensor does not respond (providing data) the cluster will ask data to the backup sensor.

>**note:** In this configuration the cluster node is a single point of failure, it must be highly reliable.

**Main network (short range):**
![alt text](img\main_network.png)<br>
The main network has to cover longer distances so a wired network is probably inconvinient; due to the small ammount of data excanged between cluster nodes and coordinator the wirless network can be based on comunication protocols designed with small data packets like ZigBee (frequency-band: 2.4GHz, bit rate: 250kb/s). <br>
To ensure reliability ZigBee network must run in Beacon Enabled mod to avoid collisions (with CAP time equal to zero), once an hour the coordinator will send a beacon and will receive updated data from each of the 20 cluster nodes; each cluster node will provide all the data in a single message with a fixed data length of 5 Bytes (the whole messages stays in a single packet with the standard size of 128 Bytes), the CFP time can be computed as follows:
$$ T_{CFP} = 20\cdot(\dfrac{128\cdot8}{250\cdot10^3}) \simeq 0,082s$$
ZigBee has a theoretical distance range of 100 m, so our implementation is required to use mesh topology to extend the maximum reachable distance (all the cluster nodes are full-function devices, compatible with AODV routing, and can contribute to the mesh network).<br>

>**note:** cluster Tree routing can also extend the maximum reachable distance, however, this routing algorithm is static and less reliable.

>**note:** cluster nodes aren't power constricted and can take the ZigBee antenna powered during sleeping phases to receive asynchronous data requests from the coordinator.

>**note:** the coordinator can run ZigBee2MQTT open protocol to achieve cloud independence, publishing each data coming from a cluster node to its specific MQTT topic.

### **Q2 ~ write the pseudocode of the firmware that should be run by the monitoring device installed on each basin**
Pseudocode for sensor nodes:
  ```
  void setup() {
    for(int i = 0; i<3; i++){
        sensorData = doSensorRead();
        CoAPPost(sensorDat);
    }
  }
  ```
Pseudocode for clusterNode nodes (with redundancy):
  ```
  void onBeaconRecv(beaconMsg){
    aggregatedData = Struct{
        float luminosity;
        float sugar;
        float ph;
    }

    for(sensor in luminositySensorList){
        try{
            aggregatedData.luminosity = average(sensor.getMeasure()); //provide power to the sensor and wait for data returned as a list, raise an exception if the sensor does not respond
            break;
        }catch{
            //communicate sensor failure
        }
    }

    //same for other sensor types

    sendZigBeeMSG(aggregatedData);
  }
  ```

<div style="page-break-after: always;"></div>

### **Q3 ~ as an add-on, you are required to install a VGA camera (640x480 pixels, 8 bits per pixel) to monitor the status of the growing process. Is the solution proposed at the previous points still valid? If not, propose an alternative solution.**
Adding a VGA camera to the infrastructure significantly increases the amount of data exchanged on the 2 networks. <br>
The peripheral network is already suitable for big amounts of data and can be maintained identically by simply adding the VGA camera as a new sensor node. However ZigBee is not designed for high traffic and continuous communication, so the main network requires additional attention. <br>
2 solutions are possible, the main network can be completely replaced with a communication technology capable of handling larger data packets, and one option can be WiFi, which can ensure high reliability and greater transmission capacity. <br>
Alternatively, we can keep the previous network unchanged keeping ZigBee connectivity between cluster nodes and the coordinator, but providing an additional network (wired as ethernet, or wireless as WiFi) with its dedicated access point, in this configuration VGA camera will not be plugged to the cluster node, but it will communicate directly with the coordinator. A dedicated network more suitable for such different traffic topologies can justify the additional complexity of this second configuration, moreover if the VGA camera is added in a later time, with the previous network already running, this option allows a transparent update.
>**note:** in the second case, particular attention is required to avoid interference, WiFi and ZigBee work on the same frequency-band (2.4 Ghz), channels must be seated properly.

<div style="page-break-after: always;"></div>

![alt text](img\track2.png)

### **Q4 ~ What is the beacon interval (BI) in ms?**

$$ BI = \dfrac{L}{r} = \dfrac{128\cdot8}{10\cdot10^3} = 0,1024s = 102,4ms$$

### **Q5 ~ What is the slot time (Ts) in ms?**

$$T_{slot}= \dfrac{L}{R} = \dfrac{128\cdot*8}{250\cdot10^3}=4,096\cdot10^{-3}s = 4,096ms$$

### **Q6 ~ Assuming the maximum duty cycle allowed is 30%, what is the active part of the superframe (Tactive) in ms?**

$$ T_{active} = T_{CFP} = 0,3\cdot BI = 0,03072s = 30,72ms $$

### **Q7 ~ How many active slots are there in the CFP?**

$$ N_{slots} = \left(\dfrac{T_{CFP}}{T_{slot}}-1\right) = 6,5 \simeq 6 slots\space(lower-approximation)$$
> **note:** 7 if we also consider the slot used for the beacon 
### **Q8 ~ How many inactive slots are there in the CFP?**

$$ N_{active-slots} = \dfrac{BI-T_{CFP}}{T_{slot}} = 17,5 \simeq 18slots$$

### **Q9 ~ How many motes can join the network?**
considering the worst case (all the device transmit simultaneusly with r=20kb/s):

$$ N_{devices} = N_{slots}\cdot\dfrac{min(r)}{max(r)}=6\cdot\dfrac{10}{20}=3devices $$



<div style="page-break-after: always;"></div>

![alt text](img\track3.png)

### **Q10 ~ What are the main factors you would look at to make your final choice?**
Various environmental and economic factors can condition the choosing between LoRa and NB-Iot, however there are some key aspects to consider when choosing between LoRa and NB-IoT:
* **Costs:** NB-IoT leverages licensed cellular spectrum owned by mobile operators, which may involve recurring costs based on data usage or subscription plans. On the other hand, LoRa requires a specific gateway and might involve upfront costs for hardware.
* **Transmission range and interference avoidance:** Both LoRa and NB-IoT should have sufficient range.  However, LoRa relies on unlicensed bands and this can cause interference problems this makes it susceptible to interference, in particular from other devices using the same bands.
Furthermore, LoRa is known for its long-range capabilities, especially in open areas; but NB-IoT offers good penetration through buildings and obstacles, which could be ideal for urban and indoor environments.
* **Power Consumption:** Both LoRa and NB-IoT are designed to achieve low power consumption, but typically NB-IoT consumes more power than LoRa devices due to the cellular technology involved. 
> **Note:** this last point is relevant only if hour weather monitoring station is battery powered.
* **Security:** LoRa uses AES 128-bit encryption for data security, as opposed to a more robust 256-bit 3GPP encryption standard used by NB-IoT.
> **Note:** also, data Rate and transmission latency can be relevant factors in the choice between LoRa and NB-IoT in some specific applications, however for weather monitoring applications data updates may not be frequent or large, so we will not consider those aspects in our choice.

Finally, NB-IoT is better suited for dense urban environments (like Milan) where reliable data transmission and robust security are crucial and can pay off additional periodical costs. <br>
However LoRa remains preferible in rural eviroments where power consumption and long-range coverage are critical factors; so, despite in our case NB-IoT being more suitable, this is not a fixed choice and it must be taken only after the correct evaluations, not only on the specific application, but also considering the operating environment.

### **Q11 ~ You opt to use LoRa, using an open-source gateway close by (e.g., provided by the Things Network). However, your transmission are not successfull. What are the possible causes, and what kind of solutions could be adopted?**
Assuming the device is correctly installed and its antenna is precisely placed, after verifying that all the configuration parameters are well set and the transmission frequency is in the range accepted from the LoRa gateway. We have to remember that LoRa relies on unlicensed bands and interferences from other wireless transmissions are possible. Generally signal collision happens when 2 different signals overlap in the reception if and only if those 2 signals have the same spreading factors and the same frequency carries them. If collisions happen changing those 2 parameters can be enough to avoid them. Alternatively, if supported by the gateway, LoRa allows the implementation of frequency hopping to improve resistance to external interference.<br>
Also, the transmission power must be set correctly to overcome the receiver sensitivity, considering the distance from the LoRa gateway and possible signal obstruction by buildings, trees, or other obstacles.