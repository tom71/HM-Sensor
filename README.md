# HM-Sensor
Nachbau des "Universalsensors"

- Ein bis vier Dallas DS18S20 Temperatur Sensor mit Data an Pin 5 und Power an Pin 9
- PullUp 4,7K zwischen Data und VCC 

Muss zusammen mit dem dev-Branch meines Forks der NewAskSin-Library compiliert und gelinkt werden.

### Entwicklungsumgebung
* Eclipse C/C++
* platformio 
<pre><code>platformio init --ide eclipse --board 328p8m 
platformio run</code></pre>





