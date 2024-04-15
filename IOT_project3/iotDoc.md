<script type="text/javascript" 
  src="http://cdn.mathjax.org/mathjax/latest/MathJax.js?config=TeX-AMS-MML_HTMLorMML">
</script>
<script type="text/x-mathjax-config">
  MathJax.Hub.Config({ tex2jax: {inlineMath: [['$', '$']]}, messageStyle: "none" });
</script>

# IOT_project3
#### Briscini matteo [10709075] <br><br><br>
#### INDEX: <br>

<div style="page-break-after: always;"></div>

## **Requirements summary**
Use node-red to parse a sequence of MQTT messages (saved in [challenge3.csv](challenge3.csv)) and perform different actions based on the message's content.
In particular:
* if a message contains "Publish Message" the node will forward the message on the specified MQTT topic and the message payload is saved inside [filtered_pubs.csv]().
* if the message contains an MQTT ACK the node will increment a global ack counter, perform an HTTP request on [ThingSpeack](https://thingspeak.com/channels/2507855) update the field1 value with the ack number, the message content is saved inside [ack_log.csv]().
* other messages will be discarded.

## **Implementation**
We will split the whole implementation into 3 different phases: data generation, data receiving and message parsing, message reaction.

### **Data generation**
![alt text](img\nodeRedschema1.png)<br>
In this phase the flow will generate random data save it in a CSV file and send it locally through MQTT on the "challenge3/id_generator" topic with the following payload. If the inject named "timestamp" is enabled message will be sent with a rate of 1 message every 5 seconds.
```
 {"id": 7781, "timestamp":1710930219} //message payload example
```
"Is ready?" switch is used, combined with start and stop to emulate a physical switch on the device.

#### *Relevant JS function & blocks*
* **config (on start)**
 initializes all global variables to the desired values.
  ```
  // Code added here will be run once
  // whenever the node is started.
  global.set("mqttDefaulChannel", "challenge3/id_generator");
  global.set("ACKCounter", 0);
  global.set("receivedMessagesCounter", 0);
  global.set("tempCounter", 0);
  global.set("thingSpeakKey","VI5VOWUDI8Z5GCF1")
  ```
* **message1 setup (on message)**
  setup the received MQTT message.
  ```
  let mqtt_topic = String(global.get("mqttDefaulChannel"));  //get topic
  let jsonMessage = { "id": Math.random() * 5000, "timestamp": msg.payload};  //generate random paylod for the mqtt msg

  msg.topic = mqtt_topic;
  msg.payload = jsonMessage;

  return msg;
  ```

### **Data receiving and message parsing**
![alt text](img\nodeRedschema2.png)<br>
this block will receive up to 80 messages on "challenge3/id_generator" topic, and use the message id to get the correct row in the [challenge3.csv](challenge3.csv) file, based on the row content the flow will be reacting differently (as specified in requirement summary), in this phase the correct reaction is triggered. <br>
when the flow has already received 80 messages or occurs in error (in every stage), the stop function sets "is ready?" to false stopping messages publishing on the "challenge3/id_generator" topic.

#### *Relevant JS function & blocks*
* **mqtt input parser (on message)**
  parse the received MQTT message.
  ```
  global.set("receivedMessagesCounter", (global.get("receivedMessagesCounter")+1));  //increment the message counter
  msg.payload = parseInt(msg.payload.id % 7711);  //comput the row number
  return msg;
  ```
* **get required csv tupla (on message)**
  select the correct row from the CSV file using the received id previously saved as a flow variable.
* **reaction choice switch**
  based on the receive message trigger the correct reaction, if required.

<div style="page-break-after: always;"></div>

### **Message reaction**
![alt text](img\nodeRedschema3.png)<br>
the requirements ask to react to 2 different classes of messages: pubblish messages and ack messages.
### **publish reaction**

#### *Relevant JS function & blocks*

### **ack reaction**

#### *Relevant JS function & blocks*